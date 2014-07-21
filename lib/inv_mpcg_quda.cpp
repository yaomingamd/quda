#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <quda_internal.h>
#include <color_spinor_field.h>
#include <blas_quda.h>
#include <dslash_quda.h>
#include <invert_quda.h>
#include <util_quda.h>
#include <sys/time.h>

#include <face_quda.h>

#include <iostream>

namespace quda {


  template<typename T>
    static void applyT(T d_out[], const T d_in[], const T gamma[], const T rho[], int N)
    { 
      if(N <= 0) return;
      for(int i=0; i<N; ++i){
        d_out[i] = d_in[i]/gamma[i];
      }

      for(int i=0; i<N-1; ++i){
        d_out[i] += (d_in[i+1]*(1-rho[i+1])/(gamma[i+1]*rho[i+1]));
      }

      for(int i=1; i<N; ++i){
        d_out[i] -= d_in[i-1]/(rho[i-1]*gamma[i-1]);
      }

      return;
    }

  template<typename T>
    static void applyB(T d_out[], const T d_in[], int N)
    {
      d_out[0] = static_cast<T>(0);
      for(int i=1; i<N; ++i) d_out[i] = d_in[i-1]; 
      return;
    }

  void print(const double d[], int n){
    for(int i=0; i<n; ++i){
      std::cout << d[i] << " ";
    }
    std::cout << std::endl;
  }

  template<typename T>
    static void zero(T d[], int N){
      for(int i=0; i<N; ++i) d[i] = static_cast<T>(0);
    }

  template<typename T>
    static void applyThirdTerm(T d_out[], const T d_in[], int k, int j, int s, const T gamma[], const T rho[], const T gamma_kprev[], const T rho_kprev[])
    {
      // s is the number of steps
      // The input and output vectors are of dimension 2*s + 1
      const int dim = 2*s + 1;    

      zero(d_out, dim);
      if(k) applyT(d_out, d_in, gamma_kprev, rho_kprev, s); // compute the upper half of the vector


      applyB(d_out+s, d_in+s, s+1); // update the lower half

      // This has to come after applyB
      if(k) d_out[s] -= d_in[s-1]/(rho_kprev[s-1]*gamma_kprev[s-1]);

      // Finally scale everything
      for(int i=0; i<dim; ++i) d_out[i] *= -rho[j]*gamma[j];

      return;
    }

  template<typename T>
    static void computeCoeffs(T d_out[], const T d_p1[], const T d_p2[], int k, int j, int s, const T gamma[], const T rho[], const T gamma_kprev[], const T rho_kprev[])
    {
      applyThirdTerm(d_out, d_p1, k, j-1, s, gamma, rho, gamma_kprev, rho_kprev);


      for(int i=0; i<(2*s+1); ++i){
        d_out[i] += rho[j-1]*d_p1[i] + (1 - rho[j-1])*d_p2[i];
      }
      return;
    }




  MPCG::MPCG(DiracMatrix &mat, SolverParam &param, TimeProfile &profile) :
    Solver(param, profile), mat(mat)
  {

  }

  MPCG::~MPCG() {

  }

  void MPCG::computeMatrixPowers(cudaColorSpinorField out[], cudaColorSpinorField &in, int nvec)
  {
    cudaColorSpinorField temp(in);
    out[0] = in;
    for(int i=1; i<nvec; ++i){
      mat(out[i], out[i-1], temp);
    }
    return;
  }

  void MPCG::computeMatrixPowers(std::vector<cudaColorSpinorField>& out, std::vector<cudaColorSpinorField>& in, int nsteps)
  {
    cudaColorSpinorField temp(in[0]);

    for(int i=0; i<=nsteps; ++i) out[i] = in[i];

    for(int i=(nsteps+1); i<=(2*nsteps); ++i){
      mat(out[i], out[i-1], temp);
    }
  }



  void MPCG::operator()(cudaColorSpinorField &x, cudaColorSpinorField &b) 
  {

    // Check to see that we're not trying to invert on a zero-field source    
    const double b2 = norm2(b);
    if(b2 == 0){
      profile.Stop(QUDA_PROFILE_INIT);
      printfQuda("Warning: inverting on zero-field source\n");
      x=b;
      param.true_res = 0.0;
      param.true_res_hq = 0.0;
      return;
    }


    cudaColorSpinorField temp(b); // temporary field

    // Use ColorSpinorParam to create zerod fields
    ColorSpinorParam csParam(x);
    csParam.create = QUDA_ZERO_FIELD_CREATE;
    cudaColorSpinorField x_prev(x,csParam);

    const int s = 4;

    printf("Nstep = %d\n", s);

    // create the residual array and the matrix powers array
    std::vector<cudaColorSpinorField> R(s+1,cudaColorSpinorField(b,csParam));
    std::vector<cudaColorSpinorField> V(2*s+1,cudaColorSpinorField(b,csParam));

    // Set up the first residual
    for(int i=0; i<s; ++i) zeroCuda(R[i]);


    mat(R[s], x, temp);
    double r2 = xmyNormCuda(b,R[s]);

    double stop = stopping(param.tol, b2, param.residual_type);


    int it = 0;
    double* d    = new double[2*s+1];
    double* d_p1 = new double[2*s+1];
    double* d_p2 = new double[2*s+1];

    // Matrix powers kernel
    // The first s vectors hold the previous s residuals 
    // v[s] holds current residual
    // v[s+1] holds A r
    // v[s+2] holds A^(2)r
    // v[2*s] holds A^(s)r
    cudaColorSpinorField w(b);

    double rAr;

    //   double gamma_prev = 0.0;
    //   double mu_prev = 0.0;
    //    double rho_prev = 0.0;
    double rho[s];
    double mu[s];
    double gamma[s];   
    double rho_kprev[s];
    double gamma_kprev[s];


    int k = 0;
    while(!convergence(r2,0.0,stop,0.0) && it < param.maxiter){
      // compute the matrix powers kernel - need to set r[s] above
      //computeMatrixPowers(V, R, s); 
      computeMatrixPowers(V, R, s); 

      R[0] = R[s];

      for(int j=0; j<s; ++j){ 
        const int prev_idx = j ? j-1 : s-1;
        cudaColorSpinorField& R_prev = R[prev_idx];
        double& mu_prev    = mu[prev_idx];
        double& rho_prev   = rho[prev_idx];
        double& gamma_prev = gamma[prev_idx];



        if(j == 0){ 
          zero(d, 2*s+1); d[s+1] = 1.0;
          w = V[s+1];
        }else{
          if(j==1){ 
            zero(d_p2, 2*s+1);
            if(k > 0){
              d_p2[s-2] = (1 - rho_kprev[s-1])/(rho_kprev[s-1]*gamma_kprev[s-1]);
              d_p2[s-1] = (1./gamma_kprev[s-1]);
              d_p2[s]   = (-1./(gamma_kprev[s-1]*rho_kprev[s-1]));
            }
          }
          computeCoeffs(d, d_p1, d_p2, k, j, s, gamma, rho, gamma_kprev, rho_kprev);
          zeroCuda(w); 
          for(int i=0; i<(2*s+1); ++i){
            if(d[i] != 0.) axpyCuda(d[i], V[i], w);
          }
        }


        r2 = norm2(R[j]); // v[s] = r[s*k]
        rAr = reDotProductCuda(w,R[j]);
        mu[j] = r2;
        gamma[j] = r2/rAr;


        if(it==0){
          rho[j] = 1.0;
        }else{
          rho[j] = 1.0/(1.0 - (gamma[j]/gamma_prev)*(mu[j]/mu_prev)*(1.0/rho_prev));  
        }

        R[j+1] = R_prev;
        axCuda((1.0 - rho[j]), R[j+1]);
        axpyCuda(rho[j], R[j], R[j+1]);
        axpyCuda(-rho[j]*gamma[j], w, R[j+1]);

        // copy d to d_p1
        if(j>0) for(int i=0; i<(2*s+1); ++i) d_p2[i] = d_p1[i];
        for(int i=0; i<(2*s+1); ++i) d_p1[i] = d[i];

        PrintStats("MPCG", it, r2, b2, 0.0);
        it++; 
      } // loop over j

      for(int i=0; i<s; ++i){
        rho_kprev[i] = rho[i];
        gamma_kprev[i] = gamma[i];
      }
      k++;
    }

    PrintSummary("MPCG", it, r2, b2);

    delete[] d;
    delete[] d_p1;
    delete[] d_p2;

    return;
  }

} // namespace quda
