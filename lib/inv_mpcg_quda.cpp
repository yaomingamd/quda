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

  // out[i] = D^i in
  void MPCG::computeMatrixPowers(cudaColorSpinorField out[], cudaColorSpinorField &in, int nvec)
  {
    cudaColorSpinorField temp(in);
    out[0] = in;
    for(int i=1; i<nvec; ++i){
      mat(out[i], out[i-1], temp);
    }
    return;
  }


  // MW: out[i]= in[i] for i=0..nsteps, out[i]= D out[i-1]
  void MPCG::computeMatrixPowers(std::vector<cudaColorSpinorField>& out, std::vector<cudaColorSpinorField>& in, int nsteps)
  {
    cudaColorSpinorField temp(in[0]);

    for(int i=0; i<=nsteps; ++i) out[i] = in[i];

    for(int i=(nsteps+1); i<=(2*nsteps); ++i){
      mat(out[i], out[i-1], temp);
    }
    return;
  }

#ifdef SSTEP
  static void computeGramMatrix(double** G, std::vector<cudaColorSpinorField>& v, double* mu){

    const int dim = v.size();
    const int nsteps = (dim-1)/2;

    {
      std::vector<cudaColorSpinorField*> vp1; vp1.reserve((nsteps+1)*nsteps);
      std::vector<cudaColorSpinorField*> vp2; vp2.reserve((nsteps+1)*nsteps);
      double g[(nsteps+1)*nsteps];
    
      for(int i=0; i<nsteps; ++i){
        for(int j=nsteps; j<dim; j++){
          vp1.push_back(&v[i]);
          vp2.push_back(&v[j]);
        }
      }
      reDotProductCuda(g, vp1, vp2);
      int k=0;
      for(int i=0; i<nsteps; ++i){
        for(int j=nsteps; j<dim; j++){
          G[i][j] = g[k++];
        }
      }
   }


    const int num = dim-nsteps;
    const int offset = nsteps;
    double d[2*nsteps+1];
    std::vector<cudaColorSpinorField*> vp1; vp1.reserve(2*nsteps+1);
    std::vector<cudaColorSpinorField*> vp2; vp2.reserve(2*nsteps+1);
    for(int i=0; i<=nsteps; ++i){
      vp1.push_back(&v[0+offset]);
      vp2.push_back(&v[i+offset]);
    }
    for(int i=1; i<=nsteps; ++i){
      vp1.push_back(&v[i+offset]);
      vp2.push_back(&v[nsteps+offset]);
    }


    reDotProductCuda(d,vp1,vp2); 

    for(int i=0; i<num; ++i){
      for(int j=0; j<=i; ++j){
        G[j+offset][i+offset] = G[i+offset][j+offset] = d[i+j];
      }
    }
    for(int i=0; i<nsteps; ++i){
      G[i][i] = mu[i];
    }
  } 

          
  static void computeMuNu(double& result, const double* u, double** G, const double* v, int dim){

    result = 0.0;
    const int nsteps = (dim-1)/2;
    
    for(int i=nsteps; i<dim; ++i){
      for(int j=nsteps; j<dim; ++j){
        result += u[i]*v[j]*G[i][j];
      }
    }

    for(int i=0; i<nsteps; ++i){
      for(int j=nsteps; j<dim; ++j){
        result += (u[i]*v[j] + u[j]*v[i])*G[i][j];
      }
      result += u[i]*v[i]*G[i][i];
    }


    return;
  }
#endif // SSTEP

  void MPCG::operator()(cudaColorSpinorField &x, cudaColorSpinorField &b) 
  {
#ifndef SSTEP
    errorQuda("S-step solvers not built\n");
#else
    // Check to see that we're not trying to invert on a zero-field source    
    const double b2 = norm2(b);
    if(b2 == 0){
      profile.TPSTOP(QUDA_PROFILE_INIT);
      printfQuda("Warning: inverting on zero-field source\n");
      x=b;
      param.true_res = 0.0;
      param.true_res_hq = 0.0;
      return;
    }

//    const double x2 = norm2(x);
//    printfQuda("Inital guess norm: %e \n",x2);
    cudaColorSpinorField r(b);

    // Use ColorSpinorParam to create zerod fields
    ColorSpinorParam csParam(x);
    csParam.create = QUDA_ZERO_FIELD_CREATE;
//    cudaColorSpinorField y(b, csParam);
//
//    mat(r, x, y);
//
//    double tr2 = xmyNormCuda(b, r);
//    PrintStats("MPCG", 0, tr2, b2, 0.0);


    cudaColorSpinorField temp(b,csParam); // temporary field
    cudaColorSpinorField x_prev(x,csParam);
    cudaColorSpinorField x_new(x,csParam);

    // currently use fixed s=2
    const int s = 2;

    // create the residual array and the matrix powers array
    std::vector<cudaColorSpinorField> R(s+2,cudaColorSpinorField(b));
    std::vector<cudaColorSpinorField> V(2*s+1,cudaColorSpinorField(b));

    // Set up the first residual
    for(int i=0; i<s+2; ++i) zeroCuda(R[i]);

//
    mat(R[s], x, temp);
    double r2 = xmyNormCuda(b,R[s]);
//    double br2 = norm2(R[s]);
//    printfQuda("exi it %i j+1 %i j %i prev %i r2 %e cr2 %e\n",0, 0, 0, 0, r2,br2);
    double stop = stopping(param.tol, b2, param.residual_type);


    int it = 0; // s*k+j
    double* d    = new double[2*s+1];
    zero(d,2*s+1);
    double* d_p1 = new double[2*s+1];
    zero(d_p1,2*s+1);
    double* d_p2 = new double[2*s+1];
    zero(d_p2,2*s+1);
    double* g    = new double[2*s+1];
    zero(g,2*s+1);
    double* g_p1 = new double[2*s+1];
    zero(g_p1,2*s+1);
    double* g_p2 = new double[2*s+1];
    zero(g_p2,2*s+1);
    double** G  = new double*[2*s+1];
    for(int i=0; i<(2*s+1); ++i){
      G[i] = new double[2*s+1];
    } 


    // Matrix powers kernel
    // The first s vectors hold the previous s residuals 
    // v[s] holds current residual
    // v[s+1] holds A r
    // v[s+2] holds A^(2)r
    // v[2*s] holds A^(s)r
    cudaColorSpinorField w(b);
//    cudaColorSpinorField myw(b);


    double rAr=0.;

//    printfQuda("rar init %f \n",rAr);
    //   double gamma_prev = 0.0;
    //   double mu_prev = 0.0;
    //    double rho_prev = 0.0;
    double rho[s];
    double mu[s];
    double gamma[s];   
    double rho_kprev[s];
    double gamma_kprev[s];

    zero(mu,s);
    zero(rho,s);
    zero(gamma,s);
    zero(rho_kprev,s);
    zero(gamma_kprev,s);

    for(int i=0; i<(2*s+1); ++i){
      zero(G[i], (2*s+1)); 
    }


    int k = 0;
    // outer k loop
    PrintStats("MPCG", it, r2, b2, 0.0);
    while(!convergence(r2,0.0,stop,0.0) && it < param.maxiter){

      // compute the matrix powers kernel - need to set r[s] above

      computeMatrixPowers(V, R, s);
      R[0] = R[s];
//      printfQuda("MP %i \t",k);
//      for(int i=0;i<2*s+1;i++){
//        printfQuda("%i: %e \t",i, norm2(V[i]));
//      }
//      printfQuda("\n");
      computeGramMatrix(G,V, mu);
    
     //
//      double crAr;
//      double cr2;

      int j = 0;
      // inner j loop
      while(!convergence(r2,0.0,stop,0.0) && j<s){ 
//        printfQuda("mpcg loop k j %i %i\n",k,j);
        // if j=0 previous iteration is s-1
        const int prev_idx = j ? j-1 : s-1;
//printfQuda("it %i k %i j %i prev_idx %i\n",it,k, j, prev_idx);
        // References to results from previous iteration
        cudaColorSpinorField& R_prev = R[j ? j-1: s+1]; // MW above we set R[0]=R[s] ...
        double mu_prev    = mu[prev_idx];
        double rho_prev   = rho[prev_idx];
        double gamma_prev = gamma[prev_idx];


        // setup (j=0 here is j=1 in Eq 5.10, 5.15here)
        if(j == 0){

          // d coefficients for j=0
          zero(d, 2*s+1); d[s+1] = 1.0; // MW is this right? EQ 510 says 0_{2s+1,1} for k=0, j=0 (note we start at j=0, this corresponds to j=1)
          // g coefficients for j=0
          zero(g, 2*s+1); g[s] = 1.0;

          w = V[s+1];
//          for(int i=0; i<(2*s+1); ++i){
//          printfQuda("g[%i] %e\t",i,g[i]);
//          }
//          printfQuda("\n");
//          for(int i=0; i<(2*s+1); ++i){
//          printfQuda("d[%i] %e\t",i,d[i]);
//          }
//          printfQuda("\n");
        }else{
//           j > 0
          if(j==1){
            zero(d_p2, 2*s+1);
            if(k > 0){
              d_p2[s-2] = (1 - rho_kprev[s-1])/(rho_kprev[s-1]*gamma_kprev[s-1]);
              d_p2[s-1] = (1./gamma_kprev[s-1]);
              d_p2[s]   = (-1./(gamma_kprev[s-1]*rho_kprev[s-1]));
            }

            zero(g_p2, 2*s+1);
            if(k > 0) g_p2[s-1] = 1.0;
          }

          // compute g coeffs
          for(int i=0; i<(2*s+1); ++i){
            g[i] = rho[j-1]*g_p1[i] - rho[j-1]*gamma[j-1]*d_p1[i] 
                 + (1 - rho[j-1])*g_p2[i];
//          printfQuda("g[%i] %e\t",i,g[i]);
          }
//       printfQuda("\n");
          // compute d coeffs 
          computeCoeffs(d, d_p1, d_p2, k, j, s, gamma, rho, gamma_kprev, rho_kprev);
//          for(int i=0; i<(2*s+1); ++i){
//          printfQuda("d[%i] %e\t",i,d[i]);
//          }
//          printfQuda("\n");
          zeroCuda(w); 
          for(int i=0; i<(2*s+1); ++i){
            if(d[i] != 0.) axpyCuda(d[i], V[i], w);
          }
        } // end j>0
//        printfQuda(" %i rAr %e \t r2 %f\n",it,rAr,r2);


;
        computeMuNu(r2, g, G, g, 2*s+1);
        computeMuNu(rAr, g, G, d, 2*s+1);
//        printfQuda("ca %i rAr %e \t r2 %e\n",it,rAr,r2);
//        if (it==0){
//        crAr=reDotProductCuda(R[j],w);
//        cr2=norm2(R[j]);
//        printfQuda("ex %i rAr %e \t r2 %e\n",it,crAr,cr2);
//        r2=cr2;
//        rAr=crAr;
//        }
//        else{

        // explicit w
//        mat(myw, R[j], temp);
//          crAr=reDotProductCuda(w,R[j]);
//        double mycrAr=reDotProductCuda(myw,R[j]);
//        double delw = xmyNormCuda(w,myw);
//        double dpw = reDotProductCuda(myw,w);
//
//
//          cr2=norm2(R[j]);
//          printfQuda("ex %i rAr %e (%e delta %e <w,myw> %e) \t r2 %e\n",it,crAr,mycrAr,delw,dpw,cr2);
          //r2=cr2;
          //rAr=crAr;
//        }




//        printfQuda("Coeffprev0 mu[j] %e gammma %e rho %e \n",mu_prev,gamma_prev,rho_prev);
        mu[j] = r2;
        gamma[j] = r2/rAr;
//        printfQuda("Coeffprev0 mu[j] %e gammma %e rho %e \n",mu_prev,gamma_prev,rho_prev);
        rho[j] = (it==0) ? 1.0 : 1.0/(1.0 - (gamma[j]/gamma_prev)*(mu[j]/mu_prev)*(1.0/rho_prev));  
//        printfQuda("Coeff mu[j] %e gammma %e rho %e \n",mu[j],gamma[j],rho[j]);


//        printfQuda("Norm it %i j %i R_prev %e R[j] %e  \n",it,j,norm2(R_prev),norm2(R[j]));
        R[j+1] = R_prev;
        axCuda((1.0 - rho[j]), R[j+1]);
        axpyCuda(rho[j], R[j], R[j+1]);
        axpyCuda(-rho[j]*gamma[j], w, R[j+1]);


//        printfQuda("exi it %i j+1 %i j %i prev %i r2 %e cr2 %e\n",it, j+1, j, prev_idx, norm2(R[j+1]),cr2);


        x_new = x_prev;
        axCuda((1.0 - rho[j]), x_new);
        axpyCuda(rho[j], x, x_new);
        axpyCuda(gamma[j]*rho[j], R[j], x_new);



        // copy d to d_p1
        if(j>0){ 
          for(int i=0; i<(2*s+1); ++i) d_p2[i] = d_p1[i];
          for(int i=0; i<(2*s+1); ++i) g_p2[i] = g_p1[i];
        }
        for(int i=0; i<(2*s+1); ++i) d_p1[i] = d[i];
        for(int i=0; i<(2*s+1); ++i) g_p1[i] = g[i];
    

        PrintStats("MPCG", it, r2, b2, 0.0);
        it++;


        x_prev = x;
        x = x_new;
        ++j;
      } // loop over j



      for(int i=0; i<s; ++i){
        rho_kprev[i] = rho[i];
        gamma_kprev[i] = gamma[i];
      }
      R[s+1] = R[s-1]; //R[3] = R[1]
//      R[0] = R[s]; // R[0] == R[2]
      k++;
    }


    mat(R[0], x_prev, temp);
    param.true_res = sqrt(xmyNormCuda(b, R[0]) / b2);


    PrintSummary("MPCG", it, r2, b2);

    delete[] d;
    delete[] d_p1;
    delete[] d_p2;

    delete[] g;
    delete[] g_p1;
    delete[] g_p2;

    for(int i=0; i<(2*s+1); ++i){
      delete[] G[i];
    }
    delete[] G;
#endif // sstep
    return;
  }

} // namespace quda
