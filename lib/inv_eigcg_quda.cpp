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
#include <string.h>

#include <face_quda.h>

#include <iostream>

#include <blas_magma.h>

#define DEBUG_MODE

#define MAX_EIGENVEC_WINDOW 16

/*
Based on  eigCG(nev, m) algorithm:
A. Stathopolous and K. Orginos, arXiv:0707.0131
*/

namespace quda {

   static DeflationParam *defl_param = 0;

   template<typename Float, typename CudaComplex>
   class EigCGArgs{
     
      private:
      BlasMagmaArgs *eigcg_magma_args;
    
      //host Lanczos matrice, and its eigenvalue/vector arrays:
      std::complex<Float> *hTm;//VH A V
      Float  *hTvalm;   //eigenvalues of both T[m,  m  ] and T[m-1, m-1] (re-used)

      //device Lanczos matrix, and its eigenvalue/vector arrays:
      CudaComplex *dTm;     //VH A V
      CudaComplex *dTvecm; //eigenvectors of T[m,  m  ]
      CudaComplex *dTvecm1; //eigenvectors of T[m-1,m-1]

      int m;
      int nev;
      int ldm;
      
      public:

      EigCGArgs(int m, int nev);
      ~EigCGArgs();
      
      //methods for constructing Lanczos matrix:
      void LoadLanczosDiag(int idx, double alpha, double alpha0, double beta0);
      void LoadLanczosOffDiag(int idx, double alpha0, double beta0);
      
      //methods for Rayleigh Ritz procedure: 
      int RestartVm(void* vm, const int cld, const int clen, const int vprec);//complex length

      //methods 
      void FillLanczosDiag(const int _2nev);
      void FillLanczosOffDiag(const int _2nev, cudaColorSpinorField *v, cudaColorSpinorField *u, double inv_sqrt_r2);
   };

   template<typename Float, typename CudaComplex>
   EigCGArgs<Float, CudaComplex>::EigCGArgs(int m, int nev): m(m), nev(nev){
    //include pad?
    ldm    = ((m+15)/16)*16;//too naive
       
    //magma initialization:
    const int prec = sizeof(Float);
    eigcg_magma_args = new BlasMagmaArgs(m, nev, ldm, prec);

    hTm     = new std::complex<Float>[ldm*m];//VH A V
    hTvalm  = (Float*)malloc(m*sizeof(Float));   //eigenvalues of both T[m,  m  ] and T[m-1, m-1] (re-used)

    //allocate dTm etc. buffers on GPU:
    cudaMalloc(&dTm, ldm*m*sizeof(CudaComplex));//  
    cudaMalloc(&dTvecm, ldm*m*sizeof(CudaComplex));  
    cudaMalloc(&dTvecm1, ldm*m*sizeof(CudaComplex));  

    //set everything to zero:
    cudaMemset(dTm, 0, ldm*m*sizeof(CudaComplex));//?
    cudaMemset(dTvecm, 0, ldm*m*sizeof(CudaComplex));
    cudaMemset(dTvecm1, 0, ldm*m*sizeof(CudaComplex));

    //Error check...
    checkCudaError();

    return;
  }

  template<typename Float, typename CudaComplex>
  EigCGArgs<Float, CudaComplex>::~EigCGArgs() {
    delete[] hTm;

    free(hTvalm);

    cudaFree(dTm);
    cudaFree(dTvecm);
    cudaFree(dTvecm1);

    delete eigcg_magma_args;

    return;
  }

  template<typename Float, typename CudaComplex>
  void EigCGArgs<Float, CudaComplex>::LoadLanczosDiag(int idx, double alpha, double alpha0, double beta0)
  {
    hTm[idx*ldm+idx] = std::complex<Float>((Float)(1.0/alpha + beta0/alpha0), 0.0);
    return;
  } 

  template<typename Float, typename CudaComplex>
  void EigCGArgs<Float, CudaComplex>::LoadLanczosOffDiag(int idx, double alpha, double beta)
  {
    hTm[(idx+1)*ldm+idx] = std::complex<Float>((Float)(-sqrt(beta)/alpha), 0.0f);//'U' 
    hTm[idx*ldm+(idx+1)] = hTm[(idx+1)*ldm+idx];//'L'
    return;
  }

  template<typename Float, typename CudaComplex>
  int EigCGArgs<Float, CudaComplex>::RestartVm(void* v, const int cld, const int clen, const int vprec) 
  {
    //Create device version of the Lanczos matrix:
    cudaMemcpy(dTm, hTm, ldm*m*sizeof(CudaComplex), cudaMemcpyDefault);//!

    //Solve m-dimensional eigenproblem:
    cudaMemcpy(dTvecm, dTm,   ldm*m*sizeof(CudaComplex), cudaMemcpyDefault);
    eigcg_magma_args->MagmaHEEVD((void*)dTvecm, (void*)hTvalm, m);

    //Solve (m-1)-dimensional eigenproblem:
    cudaMemcpy(dTvecm1, dTm,   ldm*m*sizeof(CudaComplex), cudaMemcpyDefault);
    eigcg_magma_args->MagmaHEEVD((void*)dTvecm1, (void*)hTvalm, m-1);

    //Zero the last row (coloumn-major format of the matrix re-interpreted as 2D row-major formated):
    cudaMemset2D(&dTvecm1[(m-1)], ldm*sizeof(CudaComplex), 0, sizeof(CudaComplex),  (m-1));

    //Attach nev old vectors to nev new vectors (note 2*nev << m):
    cudaMemcpy(&dTvecm[ldm*nev], dTvecm1, ldm*nev*sizeof(CudaComplex), cudaMemcpyDefault);

    //Perform QR-factorization and compute QH*Tm*Q:
    int i = eigcg_magma_args->MagmaORTH_2nev((void*)dTvecm, (void*)dTm);

    //Solve 2nev-dimensional eigenproblem:
    eigcg_magma_args->MagmaHEEVD((void*)dTm, (void*)hTvalm, i);

    //solve zero unused part of the eigenvectors in dTm:
    cudaMemset2D(&(dTm[i]), ldm*sizeof(CudaComplex), 0, (m-i)*sizeof(CudaComplex), i);//check..

    //Restart V:
    eigcg_magma_args->RestartV(v, cld, clen, vprec, (void*)dTvecm, (void*)dTm);

    return i;
  }


  template<typename Float, typename CudaComplex>
  void EigCGArgs<Float, CudaComplex>::FillLanczosDiag(const int _2nev)
 {
    memset(hTm, 0, ldm*m*sizeof(std::complex<Float>));
    for (int i = 0; i < _2nev; i++) hTm[i*ldm+i]= hTvalm[i];//fill-up diagonal

    return;
 }

  template<typename Float, typename CudaComplex>
  void EigCGArgs<Float, CudaComplex>::FillLanczosOffDiag(const int _2nev, cudaColorSpinorField *v, cudaColorSpinorField *u, double inv_sqrt_r2)
  {
    if(v->Precision() != u->Precision()) errorQuda("\nIncorrect precision...\n");
    for (int i = 0; i < _2nev; i++){
       std::complex<double> s = cDotProductCuda(*v, u->Eigenvec(i));
       s *= inv_sqrt_r2;
       hTm[_2nev*ldm+i] = std::complex<Float>((Float)s.real(), (Float)s.imag());
       hTm[i*ldm+_2nev] = conj(hTm[_2nev*ldm+i]);
    }
  }

  // set the required parameters for the initCG solver
  void fillInitCGSolveParam(SolverParam &initCGparam) {
    initCGparam.iter   = 0;
    initCGparam.gflops = 0;
    initCGparam.secs   = 0;

    initCGparam.inv_type        = QUDA_CG_INVERTER;       // use CG solver
    initCGparam.use_init_guess  = QUDA_USE_INIT_GUESS_YES;// use deflated initial guess...
  }

  IncEigCG::IncEigCG(DiracMatrix &mat, DiracMatrix &matSloppy, DiracMatrix &matCGSloppy, DiracMatrix &matDefl, SolverParam &param, TimeProfile &profile) :
    DeflatedSolver(param, profile), mat(mat), matSloppy(matSloppy), matCGSloppy(matCGSloppy), matDefl(matDefl), search_space_prec(QUDA_INVALID_PRECISION), 
    Vm(0), initCGparam(param), profile(profile), eigcg_alloc(false)
  {
    if((param.rhs_idx < param.deflation_grid) || (param.inv_type == QUDA_EIGCG_INVERTER))
    {
       if(param.nev > MAX_EIGENVEC_WINDOW )
       { 
          warningQuda("\nWarning: the eigenvector window is too big, using default value %d.\n", MAX_EIGENVEC_WINDOW);
          param.nev = MAX_EIGENVEC_WINDOW;
       }

       search_space_prec = param.precision_ritz;
       //
       use_eigcg = true;
       //
       printfQuda("\nIncEigCG will deploy eigCG(m=%d, nev=%d) solver.\n", param.m, param.nev);
    }
    else
    {
       fillInitCGSolveParam(initCGparam);
       //
       use_eigcg = false;
       //
       printfQuda("\nIncEigCG will deploy initCG solver.\n");
    }

    return;
  }

  IncEigCG::~IncEigCG() {

    if(eigcg_alloc)   delete Vm;

  }

  void IncEigCG::EigCG(cudaColorSpinorField &x, cudaColorSpinorField &b) 
  {

    if (eigcg_precision != x.Precision()) errorQuda("\nInput/output field precision is incorrect (solver precision: %u spinor precision: %u).\n", eigcg_precision, x.Precision());

    profile.Start(QUDA_PROFILE_INIT);

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

    cudaColorSpinorField r(b);

    ColorSpinorParam csParam(x);
    csParam.create = QUDA_ZERO_FIELD_CREATE;
    cudaColorSpinorField y(b, csParam);

    //mat(r, x, y);
    //double r2 = xmyNormCuda(b, r);//compute residual
  
    csParam.setPrecision(eigcg_precision);

    cudaColorSpinorField Ap(x, csParam);

    cudaColorSpinorField tmp(x, csParam);

    cudaColorSpinorField *tmp2_p = &tmp;

    //matSloppy(r, x, tmp, tmp2);
    //double r2 = xmyNormCuda(b, r);//compute residual

    // tmp only needed for multi-gpu Wilson-like kernels
    if (mat.Type() != typeid(DiracStaggeredPC).name() && 
	mat.Type() != typeid(DiracStaggered).name()) {
      tmp2_p = new cudaColorSpinorField(x, csParam);
    }
    cudaColorSpinorField &tmp2 = *tmp2_p;

    matSloppy(r, x, tmp, tmp2);
    double r2 = xmyNormCuda(b, r);//compute residual

    cudaColorSpinorField p(r);

    zeroCuda(y);
    
    const bool use_heavy_quark_res = 
      (param.residual_type & QUDA_HEAVY_QUARK_RESIDUAL) ? true : false;
    
    profile.Stop(QUDA_PROFILE_INIT);
    profile.Start(QUDA_PROFILE_PREAMBLE);

    double r2_old;
    double stop = b2*param.tol*param.tol; // stopping condition of solver

    double heavy_quark_res = 0.0; // heavy quark residual
    if(use_heavy_quark_res) heavy_quark_res = sqrt(HeavyQuarkResidualNormCuda(x,r).z);
    int heavy_quark_check = 10; // how often to check the heavy quark residual

    double alpha=1.0, beta=0.0;
 
    double pAp;

    int eigvRestart = 0;

    profile.Stop(QUDA_PROFILE_PREAMBLE);
    profile.Start(QUDA_PROFILE_COMPUTE);
    blas_flops = 0;

//eigCG specific code:
    if(eigcg_alloc == false){

       printfQuda("\nAllocating resources for the EigCG solver...\n");

       //Create an eigenvector set:
       csParam.create   = QUDA_ZERO_FIELD_CREATE;
       csParam.setPrecision(search_space_prec);//eigCG internal search space precision: must be adjustable.
       csParam.eigv_dim = param.m;

       Vm = new cudaColorSpinorField(csParam); //search space for Ritz vectors

       checkCudaError();
       printfQuda("\n..done.\n");
       
       eigcg_alloc = true;
    }

    ColorSpinorParam eigParam(Vm->Eigenvec(0));
    eigParam.create = QUDA_ZERO_FIELD_CREATE;
    cudaColorSpinorField  *v0   = new cudaColorSpinorField(Vm->Eigenvec(0), eigParam); //temporary field. 

    cudaColorSpinorField Ap0(Ap);

    //create EigCG objects:
    EigCGArgs<double, cuDoubleComplex> *eigcg_args = new EigCGArgs<double, cuDoubleComplex>(param.m, param.nev); //must be adjustable..
    
    //EigCG additional parameters:
    double alpha0 = 1.0, beta0 = 0.0;

//Begin CG iterations:
    int k=0, l=0;
    
    PrintStats("EigCG", k, r2, b2, heavy_quark_res);

    double sigma = 0.0;

    while ( !convergence(r2, heavy_quark_res, stop, param.tol_hq) && k < param.maxiter) {

      if(k > 0)
      {
        beta0 = beta;

        beta = sigma / r2_old;
        axpyZpbxCuda(alpha, p, x, r, beta);

        if (use_heavy_quark_res && k%heavy_quark_check==0) { 
	     heavy_quark_res = sqrt(xpyHeavyQuarkResidualNormCuda(x, y, r).z);//note:y is a zero array here.
        }
      }

      //save previous mat-vec result 
      if (l == param.m) copyCuda(Ap0, Ap);

      //mat(Ap, p, tmp, tmp2); // tmp as tmp
      matSloppy(Ap, p, tmp, tmp2);  

      //construct the Lanczos matrix:
      if(l > 0){
        eigcg_args->LoadLanczosDiag(l-1, alpha, alpha0, beta0);
      }

      //Begin Rayleigh-Ritz procedure:
      if (l == param.m){

         //Restart search space : 
         int cldn = Vm->EigvTotalLength() >> 1; //complex leading dimension
         int clen = Vm->EigvLength()      >> 1; //complex vector length
         //
         int _2nev = eigcg_args->RestartVm(Vm->V(), cldn, clen, Vm->Precision());           

         //Fill-up diagonal elements of the matrix T
         eigcg_args->FillLanczosDiag(_2nev);

         //Compute Ap0 = Ap - beta*Ap0:
         xpayCuda(Ap, -beta, Ap0);//mind precision...
           
         copyCuda(*v0, Ap0);//convert arrays here:
         eigcg_args->FillLanczosOffDiag(_2nev, v0, Vm, 1.0 / sqrt(r2));

         eigvRestart++;
         l = _2nev;

      } else{ //no-RR branch:

         if(l > 0){
            eigcg_args->LoadLanczosOffDiag(l-1, alpha, beta);
         }
      }

      //construct Lanczos basis:
      copyCuda(Vm->Eigenvec(l), r);//convert arrays

      //rescale the vector
      axCuda(1.0 / sqrt(r2), Vm->Eigenvec(l));

      //update search space index
      l += 1;

      //end of RR-procedure
      alpha0 = alpha;


      pAp    = reDotProductCuda(p, Ap);
      alpha  = r2 / pAp; 

      // here we are deploying the alternative beta computation 

      r2_old = r2;
      Complex cg_norm = axpyCGNormCuda(-alpha, Ap, r);
      r2 = real(cg_norm); // (r_new, r_new)
      sigma = imag(cg_norm) >= 0.0 ? imag(cg_norm) : r2; // use r2 if (r_k+1, r_k+1-r_k) breaks

      k++;

      PrintStats("EigCG", k, r2, b2, heavy_quark_res);
    }

//Free eigcg resources:
    delete eigcg_args;

    profile.Stop(QUDA_PROFILE_COMPUTE);
    profile.Start(QUDA_PROFILE_EPILOGUE);

    param.secs = profile.Last(QUDA_PROFILE_COMPUTE);
    double gflops = (quda::blas_flops + mat.flops())*1e-9;
    reduceDouble(gflops);
    param.gflops = gflops;
    param.iter += k;

    if (k==param.maxiter) 
      warningQuda("Exceeded maximum iterations %d", param.maxiter);

    if (getVerbosity() >= QUDA_VERBOSE){
      printfQuda("EigCG: Eigenspace restarts = %d\n", eigvRestart);
    }

    // compute the true residuals
    //mat(r, x, y);
    matSloppy(r, x, tmp, tmp2);

    param.true_res = sqrt(xmyNormCuda(b, r) / b2);
#if (__COMPUTE_CAPABILITY__ >= 200)
    param.true_res_hq = sqrt(HeavyQuarkResidualNormCuda(x,r).z);
#else
    param.true_res_hq = 0.0;
#endif      
    PrintSummary("EigCG", k, r2, b2);

    // reset the flops counters
    quda::blas_flops = 0;
    mat.flops();

    profile.Stop(QUDA_PROFILE_EPILOGUE);
    profile.Start(QUDA_PROFILE_FREE);

    if (&tmp2 != &tmp) delete tmp2_p;

//Clean EigCG resources:
    delete v0;

    profile.Stop(QUDA_PROFILE_FREE);

    return;
  }

//END of eigcg solver.

//Deflation space management:
  void IncEigCG::CreateDeflationSpace(cudaColorSpinorField &eigcgSpinor, DeflationParam *&dpar)
  {
    printfQuda("\nCreate deflation space...\n");

    if(eigcgSpinor.SiteSubset() != QUDA_PARITY_SITE_SUBSET) errorQuda("\nRitz spinors must be parity spinors\n");//or adjust it

    ColorSpinorParam cudaEigvParam(eigcgSpinor);

    dpar = new DeflationParam(cudaEigvParam, param);

    printfQuda("\n...done.\n");

    //dpar->PrintInfo();

    return;
  }

  void IncEigCG::DeleteDeflationSpace(DeflationParam *&dpar)
  {
    if(dpar != 0) 
    {
      delete dpar;
      dpar = 0;
    }

    return;
  }


  void IncEigCG::ExpandDeflationSpace(DeflationParam *dpar, const int newnevs)
  {
     if(!use_eigcg || (newnevs == 0)) return; //nothing to do

     if(!eigcg_alloc) errorQuda("\nError: cannot expand deflation spase (eigcg ritz vectors were cleaned).\n"); 
     
     printfQuda("\nConstruct projection matrix..\n");

     int addednev = 0;

     if((newnevs + dpar->cur_dim) > dpar->ld) errorQuda("\nIncorrect deflation space...\n"); //nothing to do...

     //GS orthogonalization

     Complex alpha;

     for(int i = dpar->cur_dim; i < (dpar->cur_dim + newnevs); i++)
     {
       for(int j = 0; j < i; j++)
       {
         alpha = cDotProductCuda(dpar->cudaRitzVectors->Eigenvec(j), dpar->cudaRitzVectors->Eigenvec(i));//<j,i>
         Complex scale = Complex(-alpha.real(), -alpha.imag());
         caxpyCuda(scale, dpar->cudaRitzVectors->Eigenvec(j), dpar->cudaRitzVectors->Eigenvec(i)); //i-<j,i>j
       }
         
       alpha = norm2(dpar->cudaRitzVectors->Eigenvec(i));
       if(alpha.real() > 1e-16)
       {
          axCuda(1.0 /sqrt(alpha.real()), dpar->cudaRitzVectors->Eigenvec(i));  
          addednev += 1;
       }
       else
       {
          errorQuda("\nCannot orthogonalize %dth vector\n", i);
       }
     }

     ColorSpinorParam csParam(dpar->cudaRitzVectors->Eigenvec(0));
     csParam.create = QUDA_ZERO_FIELD_CREATE;

     csParam.eigv_dim  = 0;
     csParam.eigv_id   = -1;

     cudaColorSpinorField *W   = new cudaColorSpinorField(csParam); 
     cudaColorSpinorField *W2  = new cudaColorSpinorField(csParam);

     cudaColorSpinorField tmp (*W, csParam);

     for (int j = dpar->cur_dim; j < (dpar->cur_dim+addednev); j++)//
     {
       matDefl(*W, dpar->cudaRitzVectors->Eigenvec(j), tmp);

       //off-diagonal:
       for (int i = 0; i < j; i++)//row id
       {
          alpha  =  cDotProductCuda(dpar->cudaRitzVectors->Eigenvec(i), *W);
          //
          dpar->proj_matrix[j*dpar->ld+i] = alpha;
          dpar->proj_matrix[i*dpar->ld+j] = conj(alpha);//conj
       }

       //diagonal:
       alpha  =  cDotProductCuda(dpar->cudaRitzVectors->Eigenvec(j), *W);
       //
       dpar->proj_matrix[j*dpar->ld+j] = alpha;
     }

     dpar->ResetDeflationCurrentDim(addednev);

     printfQuda("\n.. done.\n");

     delete W;
     delete W2;

     return;
  }


  void IncEigCG::DeflateSpinor(cudaColorSpinorField &x, cudaColorSpinorField &b, DeflationParam *dpar, bool set2zero)
  {
    if(set2zero) zeroCuda(x);
    if(dpar->cur_dim == 0) return;//nothing to do

    BlasMagmaArgs *magma_args = new BlasMagmaArgs(sizeof(double));//change precision..

    Complex  *vec   = new Complex[dpar->ld];

    double check_nrm2 = norm2(b);
    printfQuda("\nSource norm (gpu): %1.15e\n", sqrt(check_nrm2));


    for(int i = 0; i < dpar->cur_dim; i++)
    {
      vec[i] = cDotProductCuda(dpar->cudaRitzVectors->Eigenvec(i), b);//<i, b>
    }    

    magma_args->SolveProjMatrix((void*)vec, dpar->ld,  dpar->cur_dim, (void*)dpar->proj_matrix, dpar->ld);

    for(int i = 0; i < dpar->cur_dim; i++)
    {
      caxpyCuda(vec[i], dpar->cudaRitzVectors->Eigenvec(i), x); //a*i+x
    }

    check_nrm2 = norm2(x);
    printfQuda("\nDeflated guess spinor norm (gpu): %1.15e\n", sqrt(check_nrm2));


    delete magma_args;

    delete [] vec;

    return;
  }

//!!!!
//copy EigCG ritz vectors.
  void IncEigCG::SaveEigCGRitzVecs(DeflationParam *dpar, bool cleanEigCGResources)
  {
     const int first_idx = dpar->cur_dim; 

     if(dpar->cudaRitzVectors->EigvDim() < (first_idx+param.nev)) errorQuda("\nNot enough space to copy %d vectors..\n", param.nev); 

     else if(!eigcg_alloc || !dpar->cuda_ritz_alloc) errorQuda("\nEigCG resources were cleaned.\n"); 

     for(int i = 0; i < param.nev; i++) copyCuda(dpar->cudaRitzVectors->Eigenvec(first_idx+i), Vm->Eigenvec(i));
     
     if(cleanEigCGResources)
     {
       delete Vm;
       Vm = 0;
       eigcg_alloc = false;
     }
     else//just call zeroCuda..
     {
       zeroCuda(*Vm);
     }

     return;
  }

  void IncEigCG::CleanResources()
  {
    if(eigcg_alloc)
    {
       delete Vm;
       Vm = 0;
       eigcg_alloc = false;
    }
    if(defl_param != 0)
    {
       DeleteDeflationSpace(defl_param);
       defl_param = 0;
    }

    return;
  } 

  void IncEigCG::operator()(cudaColorSpinorField *out, cudaColorSpinorField *in) 
  {
     if(defl_param == 0) CreateDeflationSpace(*in, defl_param);

     //if this operator applied during the first stage of the incremental eigCG (to construct deflation space):
     //then: call eigCG inverter 
     if(use_eigcg){

        const bool use_mixed_prec = (eigcg_precision != param.precision); 

        //deflate initial guess:
        DeflateSpinor(*out, *in, defl_param);

        cudaColorSpinorField *outSloppy = 0;
        cudaColorSpinorField *inSloppy  = 0;

        double ext_tol = param.tol;

        if(use_mixed_prec)
        {
           ColorSpinorParam cudaParam(*out);
           //
           cudaParam.create = QUDA_ZERO_FIELD_CREATE;
           //
           cudaParam.setPrecision(eigcg_precision);

           outSloppy = new cudaColorSpinorField(cudaParam);
           inSloppy  = new cudaColorSpinorField(cudaParam);

           copyCuda(*inSloppy, *in);//input is outer residual
           copyCuda(*outSloppy, *out);

           param.tol = 1e-7;//single precision eigcg tolerance
        }
        else//full precision solver:
        {
           outSloppy = out;
           inSloppy  = in;
        }

        EigCG(*outSloppy, *inSloppy);

        if(use_mixed_prec)
        {
           double b2   = norm2(*in);
           double stop = b2*ext_tol*ext_tol;
      
           param.tol   = 5e-3;//initcg sloppy precision tolerance

           cudaColorSpinorField y(*in);//full precision accumulator
           cudaColorSpinorField r(*in);//full precision residual

           Solver *initCG = 0;

           initCGparam.tol       = param.tol;
           initCGparam.precision = eigcg_precision;//the same as eigcg
           //
           initCGparam.precision_sloppy = QUDA_HALF_PRECISION; //may not be half, in general?    
           initCGparam.use_sloppy_partial_accumulator=false;   //more stable single-half solver
     
           //no reliable updates?

           initCG = new CG(matSloppy, matCGSloppy, initCGparam, profile);

           //
           copyCuda(*out, *outSloppy);
           //
           mat(r, *out, y); // here we can use y as tmp
           //
           double r2 = xmyNormCuda(*in, r);//new residual (and RHS)
          
           while(r2 > stop)
           {
              zeroCuda(y);//deflate initial guess:
              //
              DeflateSpinor(y, r, defl_param);
              //
              copyCuda(*inSloppy, r);
              //
              copyCuda(*outSloppy, y);
              // 
              (*initCG)(*outSloppy, *inSloppy);

              copyCuda(y, *outSloppy);
              //
              xpyCuda(y, *out); //accumulate solution
              //
              mat(r, *out, y);  //here we can use y as tmp
              //
              r2 = xmyNormCuda(*in, r);//new residual (and RHS)
           }

           //clean objects:
           //
           delete initCG;
           //
           delete outSloppy;
           //
           delete inSloppy;
        }

	//store computed Ritz vectors:
        SaveEigCGRitzVecs(defl_param);

        //Construct(extend) projection matrix:
        ExpandDeflationSpace(defl_param, param.nev);

        //copy solver statistics:
        param.iter   += initCGparam.iter;
        //
        param.secs   += initCGparam.secs;
        //
        param.gflops += initCGparam.gflops;

     }
     //else: use deflated CG solver with proper restarting. 
     else{
        double full_tol    = initCGparam.tol;

        double restart_tol = initCGparam.tol_restart;

        ColorSpinorParam cudaParam(*out);

        cudaParam.create = QUDA_ZERO_FIELD_CREATE;

        cudaParam.eigv_dim  = 0;

        cudaParam.eigv_id   = -1;

        cudaColorSpinorField *W   = new cudaColorSpinorField(cudaParam);

        cudaColorSpinorField tmp (*W, cudaParam);

        Solver *initCG = 0;

        DeflateSpinor(*out, *in, defl_param);

        //launch initCG:
        while(restart_tol > full_tol)//currently just one restart, think about better algorithm for the restarts. 
        {
          initCGparam.tol = restart_tol; 

          initCG = new CG(mat, matCGSloppy, initCGparam, profile);

          (*initCG)(*out, *in);           

          delete initCG;

          matDefl(*W, *out, tmp);

          xpayCuda(*in, -1, *W); 

          DeflateSpinor(*out, *W, defl_param, false);

          restart_tol = full_tol;//one restart.                              
        }

        initCGparam.tol = full_tol; 

        initCG = new CG(mat, matCGSloppy, initCGparam, profile);

        (*initCG)(*out, *in);           

        delete initCG;

        //copy solver statistics:
        param.iter   = initCGparam.iter;
        //
        param.secs   = initCGparam.secs;
        //
        param.gflops = initCGparam.gflops;

        delete W;
     } 

     //compute true residual: 
     ColorSpinorParam cudaParam(*out);
     //
     cudaParam.create = QUDA_ZERO_FIELD_CREATE;
     //
     cudaColorSpinorField   *final_r = new cudaColorSpinorField(cudaParam);
     cudaColorSpinorField   *tmp2    = new cudaColorSpinorField(cudaParam);
           
     
     mat(*final_r, *out, *tmp2);
    
     param.true_res = sqrt(xmyNormCuda(*in, *final_r) / norm2(*in));
    
     delete final_r;
     delete tmp2;

     param.rhs_idx += 1;

     return;
  }


} // namespace quda
