#include <host_utils.h>
#include <dslash_reference.h>
#include <wilson_dslash_reference.h>
#include <domain_wall_dslash_reference.h>
#include <command_line_params.h>

void verifyInversion(void *spinorOut, void **spinorOutMulti, void *spinorIn, void *spinorCheck, QudaGaugeParam &gauge_param, QudaInvertParam &inv_param, void **gauge, void *clover, void *clover_inv) {
  
  if (dslash_type == QUDA_DOMAIN_WALL_DSLASH ||
      dslash_type == QUDA_DOMAIN_WALL_4D_DSLASH ||
      dslash_type == QUDA_MOBIUS_DWF_DSLASH ) {
    verifyDomainWallTypeInversion(spinorOut, spinorOutMulti, spinorIn, spinorCheck, gauge_param, inv_param, gauge, clover, clover_inv);
  } else if (dslash_type == QUDA_WILSON_DSLASH || 
	     dslash_type == QUDA_CLOVER_WILSON_DSLASH ||
	     dslash_type == QUDA_TWISTED_MASS_DSLASH || 
	     dslash_type == QUDA_TWISTED_CLOVER_DSLASH ) {
    verifyWilsonTypeInversion(spinorOut, spinorOutMulti, spinorIn, spinorCheck, gauge_param, inv_param, gauge, clover, clover_inv);
  } 
}

void verifyDomainWallTypeInversion(void *spinorOut, void **spinorOutMulti, void *spinorIn, void *spinorCheck, QudaGaugeParam &gauge_param, QudaInvertParam &inv_param, void **gauge, void *clover, void *clover_inv) 
{
  if (inv_param.solution_type == QUDA_MAT_SOLUTION) {
    if (dslash_type == QUDA_DOMAIN_WALL_DSLASH) {
      dw_mat(spinorCheck, gauge, spinorOut, kappa5, inv_param.dagger, inv_param.cpu_prec, gauge_param, inv_param.mass);
    } else if (dslash_type == QUDA_DOMAIN_WALL_4D_DSLASH) {
      dw_4d_mat(spinorCheck, gauge, spinorOut, kappa5, inv_param.dagger, inv_param.cpu_prec, gauge_param, inv_param.mass);
    } else if (dslash_type == QUDA_MOBIUS_DWF_DSLASH) {
      double _Complex *kappa_b = (double _Complex *)malloc(Lsdim * sizeof(double _Complex));
      double _Complex *kappa_c = (double _Complex *)malloc(Lsdim * sizeof(double _Complex));
      for(int xs = 0 ; xs < Lsdim ; xs++)
	{
	  kappa_b[xs] = 1.0/(2*(inv_param.b_5[xs]*(4.0 + inv_param.m5) + 1.0));
	  kappa_c[xs] = 1.0/(2*(inv_param.c_5[xs]*(4.0 + inv_param.m5) - 1.0));
	  }
      mdw_mat(spinorCheck, gauge, spinorOut, kappa_b, kappa_c, inv_param.dagger, inv_param.cpu_prec, gauge_param, inv_param.mass, inv_param.b_5, inv_param.c_5);
      free(kappa_b);
      free(kappa_c);
    } else {
      errorQuda("Unsupported dslash_type");
    }
    
    if (inv_param.mass_normalization == QUDA_MASS_NORMALIZATION) {
      ax(0.5/kappa5, spinorCheck, V*spinor_site_size*inv_param.Ls, inv_param.cpu_prec);
    }
    
  } else if(inv_param.solution_type == QUDA_MATPC_SOLUTION) {
    
    //DOMAIN_WALL START
    if (dslash_type == QUDA_DOMAIN_WALL_DSLASH) {
      dw_matpc(spinorCheck, gauge, spinorOut, kappa5, inv_param.matpc_type, 0, inv_param.cpu_prec, gauge_param, inv_param.mass);
    } else if (dslash_type == QUDA_DOMAIN_WALL_4D_DSLASH) {
      dw_4d_matpc(spinorCheck, gauge, spinorOut, kappa5, inv_param.matpc_type, 0, inv_param.cpu_prec, gauge_param, inv_param.mass);
    } else if (dslash_type == QUDA_MOBIUS_DWF_DSLASH) {
      double _Complex *kappa_b = (double _Complex *)malloc(Lsdim * sizeof(double _Complex));
      double _Complex *kappa_c = (double _Complex *)malloc(Lsdim * sizeof(double _Complex));
      for(int xs = 0 ; xs < Lsdim ; xs++)
	{
	  kappa_b[xs] = 1.0/(2*(inv_param.b_5[xs]*(4.0 + inv_param.m5) + 1.0));
	  kappa_c[xs] = 1.0/(2*(inv_param.c_5[xs]*(4.0 + inv_param.m5) - 1.0));
	}
      mdw_matpc(spinorCheck, gauge, spinorOut, kappa_b, kappa_c, inv_param.matpc_type, 0, inv_param.cpu_prec, gauge_param, inv_param.mass, inv_param.b_5, inv_param.c_5);
      free(kappa_b);
      free(kappa_c);
      //DOMAIN_WALL END
    } else {
      errorQuda("Unsupported dslash_type");
    }
    
    if (inv_param.mass_normalization == QUDA_MASS_NORMALIZATION) {
      ax(0.25/(kappa5*kappa5), spinorCheck, V*spinor_site_size*inv_param.Ls, inv_param.cpu_prec);
    }
    
  } else if (inv_param.solution_type == QUDA_MATPCDAG_MATPC_SOLUTION) {
    
    void *spinorTmp = malloc(V*spinor_site_size*host_spinor_data_type_size*inv_param.Ls);	
    ax(0, spinorCheck, V*spinor_site_size, inv_param.cpu_prec);
    
    //DOMAIN_WALL START
    if (dslash_type == QUDA_DOMAIN_WALL_DSLASH) {
      dw_matpc(spinorTmp, gauge, spinorOut, kappa5, inv_param.matpc_type, 0, inv_param.cpu_prec, gauge_param, inv_param.mass);
      dw_matpc(spinorCheck, gauge, spinorTmp, kappa5, inv_param.matpc_type, 1, inv_param.cpu_prec, gauge_param, inv_param.mass);
    } else if (dslash_type == QUDA_DOMAIN_WALL_4D_DSLASH) {
      dw_4d_matpc(spinorTmp, gauge, spinorOut, kappa5, inv_param.matpc_type, 0, inv_param.cpu_prec, gauge_param, inv_param.mass);
      dw_4d_matpc(spinorCheck, gauge, spinorTmp, kappa5, inv_param.matpc_type, 1, inv_param.cpu_prec, gauge_param, inv_param.mass);
    } else if (dslash_type == QUDA_MOBIUS_DWF_DSLASH) {
      double _Complex *kappa_b = (double _Complex *)malloc(Lsdim * sizeof(double _Complex));
      double _Complex *kappa_c = (double _Complex *)malloc(Lsdim * sizeof(double _Complex));
      for(int xs = 0 ; xs < Lsdim ; xs++)
	{
	  kappa_b[xs] = 1.0/(2*(inv_param.b_5[xs]*(4.0 + inv_param.m5) + 1.0));
	  kappa_c[xs] = 1.0/(2*(inv_param.c_5[xs]*(4.0 + inv_param.m5) - 1.0));
	}
      mdw_matpc(spinorTmp, gauge, spinorOut, kappa_b, kappa_c, inv_param.matpc_type, 0, inv_param.cpu_prec, gauge_param, inv_param.mass, inv_param.b_5, inv_param.c_5);
      mdw_matpc(spinorCheck, gauge, spinorTmp, kappa_b, kappa_c, inv_param.matpc_type, 1, inv_param.cpu_prec, gauge_param, inv_param.mass, inv_param.b_5, inv_param.c_5);
      free(kappa_b);
      free(kappa_c);
      //DOMAIN_WALL END
    } else {
      errorQuda("Unsupported dslash_type");
    }
    
    if (inv_param.mass_normalization == QUDA_MASS_NORMALIZATION) {
      errorQuda("Mass normalization not implemented");
    }
    
    //free(spinorTmp);
    
  
    int vol = inv_param.solution_type == QUDA_MAT_SOLUTION ? V : Vh;
    mxpy(spinorIn, spinorCheck, vol*spinor_site_size*inv_param.Ls, inv_param.cpu_prec);
    double nrm2 = norm_2(spinorCheck, vol*spinor_site_size*inv_param.Ls, inv_param.cpu_prec);
    double src2 = norm_2(spinorIn, vol*spinor_site_size*inv_param.Ls, inv_param.cpu_prec);
    double l2r = sqrt(nrm2 / src2);
      
    printfQuda("Residuals: (L2 relative) tol %g, QUDA = %g, host = %g; (heavy-quark) tol %g, QUDA = %g\n",
	       inv_param.tol, inv_param.true_res, l2r, inv_param.tol_hq, inv_param.true_res_hq);      
  }
}


void verifyWilsonTypeInversion(void *spinorOut, void **spinorOutMulti, void *spinorIn, void *spinorCheck, QudaGaugeParam &gauge_param, QudaInvertParam &inv_param, void **gauge, void *clover, void *clover_inv) 
{
  if (multishift) {
    // ONLY WILSON/CLOVER/TWISTED TYPES
    if (inv_param.mass_normalization == QUDA_MASS_NORMALIZATION) {
      errorQuda("Mass normalization not supported for multi-shift solver in invert_test");
    }
    
    void *spinorTmp = malloc(V*spinor_site_size*host_spinor_data_type_size*inv_param.Ls);
    printfQuda("Host residuum checks: \n");
    for(int i=0; i < inv_param.num_offset; i++) {
      ax(0, spinorCheck, V*spinor_site_size, inv_param.cpu_prec);

      if (dslash_type == QUDA_TWISTED_MASS_DSLASH) {
	if (inv_param.twist_flavor != QUDA_TWIST_SINGLET) {
	  int tm_offset = Vh*spinor_site_size;
	  void *out0 = spinorCheck;
	  void *out1 = (char*)out0 + tm_offset*cpu_prec;
	  
	  void *tmp0 = spinorTmp;
	  void *tmp1 = (char*)tmp0 + tm_offset*cpu_prec;
	  
	  void *in0  = spinorOutMulti[i];
	  void *in1  = (char*)in0 + tm_offset*cpu_prec;
	  
	  tm_ndeg_matpc(tmp0, tmp1, gauge, in0, in1, inv_param.kappa, inv_param.mu, inv_param.epsilon, inv_param.matpc_type, 0, inv_param.cpu_prec, gauge_param);
	  tm_ndeg_matpc(out0, out1, gauge, tmp0, tmp1, inv_param.kappa, inv_param.mu, inv_param.epsilon, inv_param.matpc_type, 1, inv_param.cpu_prec, gauge_param);
	} else {
	  tm_matpc(spinorTmp, gauge, spinorOutMulti[i], inv_param.kappa, inv_param.mu, inv_param.twist_flavor,
		   inv_param.matpc_type, 0, inv_param.cpu_prec, gauge_param);
	  tm_matpc(spinorCheck, gauge, spinorTmp, inv_param.kappa, inv_param.mu, inv_param.twist_flavor,
		   inv_param.matpc_type, 1, inv_param.cpu_prec, gauge_param);
	}
      } else if (dslash_type == QUDA_TWISTED_CLOVER_DSLASH) {
	if (inv_param.twist_flavor != QUDA_TWIST_SINGLET)
	  errorQuda("Twisted mass solution type not supported");
	tmc_matpc(spinorTmp, gauge, spinorOutMulti[i], clover, clover_inv, inv_param.kappa, inv_param.mu,
		  inv_param.twist_flavor, inv_param.matpc_type, 0, inv_param.cpu_prec, gauge_param);
	tmc_matpc(spinorCheck, gauge, spinorTmp, clover, clover_inv, inv_param.kappa, inv_param.mu,
		  inv_param.twist_flavor, inv_param.matpc_type, 1, inv_param.cpu_prec, gauge_param);
      } else if (dslash_type == QUDA_WILSON_DSLASH) {
	wil_matpc(spinorTmp, gauge, spinorOutMulti[i], inv_param.kappa, inv_param.matpc_type, 0,
		  inv_param.cpu_prec, gauge_param);
	wil_matpc(spinorCheck, gauge, spinorTmp, inv_param.kappa, inv_param.matpc_type, 1,
		  inv_param.cpu_prec, gauge_param);
      } else if (dslash_type == QUDA_CLOVER_WILSON_DSLASH) {
	clover_matpc(spinorTmp, gauge, clover, clover_inv, spinorOutMulti[i], inv_param.kappa, inv_param.matpc_type, 0,
		     inv_param.cpu_prec, gauge_param);
	clover_matpc(spinorCheck, gauge, clover, clover_inv, spinorTmp, inv_param.kappa, inv_param.matpc_type, 1,
		     inv_param.cpu_prec, gauge_param);
      } else {
	printfQuda("Domain wall not supported for multi-shift\n");
	exit(-1);
      }
	
      axpy(inv_param.offset[i], spinorOutMulti[i], spinorCheck, Vh*spinor_site_size, inv_param.cpu_prec);
      mxpy(spinorIn, spinorCheck, Vh*spinor_site_size, inv_param.cpu_prec);
      double nrm2 = norm_2(spinorCheck, Vh*spinor_site_size, inv_param.cpu_prec);
      double src2 = norm_2(spinorIn, Vh*spinor_site_size, inv_param.cpu_prec);
      double l2r = sqrt(nrm2 / src2);
	
      printfQuda("Shift %d residuals: (L2 relative) tol %g, QUDA = %g, host = %g; (heavy-quark) tol %g, QUDA = %g\n",
		 i, inv_param.tol_offset[i], inv_param.true_res_offset[i], l2r, 
		 inv_param.tol_hq_offset[i], inv_param.true_res_hq_offset[i]);
    }
    free(spinorTmp);
    
  } else {
    // Non-multishift workflow
    if (inv_param.solution_type == QUDA_MAT_SOLUTION) {
      if (dslash_type == QUDA_TWISTED_MASS_DSLASH) {
	if(inv_param.twist_flavor == QUDA_TWIST_SINGLET) {
	  tm_mat(spinorCheck, gauge, spinorOut, inv_param.kappa, inv_param.mu, inv_param.twist_flavor, 0, inv_param.cpu_prec, gauge_param);
	} else {
	  int tm_offset = V*spinor_site_size;
	  void *evenOut = spinorCheck;
	  void *oddOut  = (char*)evenOut + tm_offset*cpu_prec;
	    
	  void *evenIn  = spinorOut;
	  void *oddIn   = (char*)evenIn + tm_offset*cpu_prec;
	    
	  tm_ndeg_mat(evenOut, oddOut, gauge, evenIn, oddIn, inv_param.kappa, inv_param.mu, inv_param.epsilon, 0, inv_param.cpu_prec, gauge_param);
	}
      } else if (dslash_type == QUDA_TWISTED_CLOVER_DSLASH) {
	tmc_mat(spinorCheck, gauge, clover, spinorOut, inv_param.kappa, inv_param.mu, inv_param.twist_flavor, 0,
		inv_param.cpu_prec, gauge_param);
      } else if (dslash_type == QUDA_WILSON_DSLASH) {
	wil_mat(spinorCheck, gauge, spinorOut, inv_param.kappa, 0, inv_param.cpu_prec, gauge_param);
      } else if (dslash_type == QUDA_CLOVER_WILSON_DSLASH) {
	clover_mat(spinorCheck, gauge, clover, spinorOut, inv_param.kappa, 0, inv_param.cpu_prec, gauge_param);
      } else {
	errorQuda("Unsupported dslash_type");
      }
      if (inv_param.mass_normalization == QUDA_MASS_NORMALIZATION) {
	if (dslash_type == QUDA_TWISTED_MASS_DSLASH && twist_flavor == QUDA_TWIST_NONDEG_DOUBLET) {
	  ax(0.5/inv_param.kappa, spinorCheck, 2*V*spinor_site_size, inv_param.cpu_prec);
	  //CAREFULL
	} else {
	  ax(0.5/inv_param.kappa, spinorCheck, V*spinor_site_size, inv_param.cpu_prec);
	}
      }
      
    } else if(inv_param.solution_type == QUDA_MATPC_SOLUTION) {
	
      if (dslash_type == QUDA_TWISTED_MASS_DSLASH) {
	if (inv_param.twist_flavor != QUDA_TWIST_SINGLET) {
	  int tm_offset = Vh*spinor_site_size;
	  void *out0 = spinorCheck;
	  void *out1 = (char*)out0 + tm_offset*cpu_prec;
	    
	  void *in0  = spinorOut;
	  void *in1  = (char*)in0 + tm_offset*cpu_prec;
	    
	  tm_ndeg_matpc(out0, out1, gauge, in0, in1, inv_param.kappa, inv_param.mu, inv_param.epsilon, inv_param.matpc_type, 0, inv_param.cpu_prec, gauge_param);
	} else {
	  tm_matpc(spinorCheck, gauge, spinorOut, inv_param.kappa, inv_param.mu, inv_param.twist_flavor,
		   inv_param.matpc_type, 0, inv_param.cpu_prec, gauge_param);
	}
      } else if (dslash_type == QUDA_TWISTED_CLOVER_DSLASH) {
	if (inv_param.twist_flavor != QUDA_TWIST_SINGLET)
	  errorQuda("Twisted mass solution type not supported");
	tmc_matpc(spinorCheck, gauge, spinorOut, clover, clover_inv, inv_param.kappa, inv_param.mu,
		  inv_param.twist_flavor, inv_param.matpc_type, 0, inv_param.cpu_prec, gauge_param);
      } else if (dslash_type == QUDA_WILSON_DSLASH) {
	wil_matpc(spinorCheck, gauge, spinorOut, inv_param.kappa, inv_param.matpc_type, 0,
		  inv_param.cpu_prec, gauge_param);
      } else if (dslash_type == QUDA_CLOVER_WILSON_DSLASH) {
	clover_matpc(spinorCheck, gauge, clover, clover_inv, spinorOut, inv_param.kappa, inv_param.matpc_type, 0,
		     inv_param.cpu_prec, gauge_param);
      } else {
	errorQuda("Unsupported dslash_type");
      }
	
      if (inv_param.mass_normalization == QUDA_MASS_NORMALIZATION) {
	ax(0.25/(inv_param.kappa*inv_param.kappa), spinorCheck, Vh*spinor_site_size, inv_param.cpu_prec);
      }
      
    } else if (inv_param.solution_type == QUDA_MATPCDAG_MATPC_SOLUTION) {
	
      void *spinorTmp = malloc(V*spinor_site_size*host_spinor_data_type_size*inv_param.Ls);	
      ax(0, spinorCheck, V*spinor_site_size, inv_param.cpu_prec);
      
      if (dslash_type == QUDA_TWISTED_MASS_DSLASH) {
	if (inv_param.twist_flavor != QUDA_TWIST_SINGLET) {
	  int tm_offset = Vh*spinor_site_size;
	  void *out0 = spinorCheck;
	  void *out1 = (char*)out0 + tm_offset*cpu_prec;
	    
	  void *tmp0 = spinorTmp;
	  void *tmp1 = (char*)tmp0 + tm_offset*cpu_prec;
	    
	  void *in0  = spinorOut;
	  void *in1  = (char*)in0 + tm_offset*cpu_prec;
	    
	  tm_ndeg_matpc(tmp0, tmp1, gauge, in0, in1, inv_param.kappa, inv_param.mu, inv_param.epsilon, inv_param.matpc_type, 0, inv_param.cpu_prec, gauge_param);
	  tm_ndeg_matpc(out0, out1, gauge, tmp0, tmp1, inv_param.kappa, inv_param.mu, inv_param.epsilon, inv_param.matpc_type, 1, inv_param.cpu_prec, gauge_param);
	} else {
	  tm_matpc(spinorTmp, gauge, spinorOut, inv_param.kappa, inv_param.mu, inv_param.twist_flavor,
		   inv_param.matpc_type, 0, inv_param.cpu_prec, gauge_param);
	  tm_matpc(spinorCheck, gauge, spinorTmp, inv_param.kappa, inv_param.mu, inv_param.twist_flavor,
		   inv_param.matpc_type, 1, inv_param.cpu_prec, gauge_param);
	}
      } else if (dslash_type == QUDA_TWISTED_CLOVER_DSLASH) {
	if (inv_param.twist_flavor != QUDA_TWIST_SINGLET)
	  errorQuda("Twisted mass solution type not supported");
	tmc_matpc(spinorTmp, gauge, spinorOut, clover, clover_inv, inv_param.kappa, inv_param.mu,
		  inv_param.twist_flavor, inv_param.matpc_type, 0, inv_param.cpu_prec, gauge_param);
	tmc_matpc(spinorCheck, gauge, spinorTmp, clover, clover_inv, inv_param.kappa, inv_param.mu,
		  inv_param.twist_flavor, inv_param.matpc_type, 1, inv_param.cpu_prec, gauge_param);
      } else if (dslash_type == QUDA_WILSON_DSLASH) {
	wil_matpc(spinorTmp, gauge, spinorOut, inv_param.kappa, inv_param.matpc_type, 0,
		  inv_param.cpu_prec, gauge_param);
	wil_matpc(spinorCheck, gauge, spinorTmp, inv_param.kappa, inv_param.matpc_type, 1,
		  inv_param.cpu_prec, gauge_param);
      } else if (dslash_type == QUDA_CLOVER_WILSON_DSLASH) {
	clover_matpc(spinorTmp, gauge, clover, clover_inv, spinorOut, inv_param.kappa,
		     inv_param.matpc_type, 0, inv_param.cpu_prec, gauge_param);
	clover_matpc(spinorCheck, gauge, clover, clover_inv, spinorTmp, inv_param.kappa,
		     inv_param.matpc_type, 1, inv_param.cpu_prec, gauge_param);
      } else {
	errorQuda("Unsupported dslash_type");
      }
      
      if (inv_param.mass_normalization == QUDA_MASS_NORMALIZATION) {
	errorQuda("Mass normalization not implemented");
      }
      
      free(spinorTmp);
    }
      
    int vol = inv_param.solution_type == QUDA_MAT_SOLUTION ? V : Vh;
    mxpy(spinorIn, spinorCheck, vol*spinor_site_size*inv_param.Ls, inv_param.cpu_prec);
    double nrm2 = norm_2(spinorCheck, vol*spinor_site_size*inv_param.Ls, inv_param.cpu_prec);
    double src2 = norm_2(spinorIn, vol*spinor_site_size*inv_param.Ls, inv_param.cpu_prec);
    double l2r = sqrt(nrm2 / src2);
      
    printfQuda("Residuals: (L2 relative) tol %g, QUDA = %g, host = %g; (heavy-quark) tol %g, QUDA = %g\n",
	       inv_param.tol, inv_param.true_res, l2r, inv_param.tol_hq, inv_param.true_res_hq);      
  }
}