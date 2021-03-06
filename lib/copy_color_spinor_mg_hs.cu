#include <copy_color_spinor_mg.cuh>

namespace quda {
  
  void copyGenericColorSpinorMGHS(ColorSpinorField &dst, const ColorSpinorField &src, 
				  QudaFieldLocation location, void *Dst, void *Src, 
				  void *dstNorm, void *srcNorm) {

#if defined(GPU_MULTIGRID) && (QUDA_PRECISION & 2)
    short *dst_ptr = static_cast<short*>(Dst);
    float *src_ptr = static_cast<float*>(Src);

    INSTANTIATE_COLOR;
#else
    errorQuda("Half precision has not been enabled");
#endif

  }

} // namespace quda
