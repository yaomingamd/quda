// *** CUDA DSLASH ***

#define DSLASH_SHARED_FLOATS_PER_THREAD 0


#if (CUDA_VERSION >= 4010)
#define VOLATILE
#else
#define VOLATILE volatile
#endif
// input spinor
#ifdef SPINOR_DOUBLE
#define spinorFloat double
// workaround for C++11 bug in CUDA 6.5/7.0
#if CUDA_VERSION >= 6050 && CUDA_VERSION < 7050
#define POW(a, b) pow(a, static_cast<spinorFloat>(b))
#else
#define POW(a, b) pow(a, b)
#endif

#define i00_re I0.x
#define i00_im I0.y
#define i01_re I1.x
#define i01_im I1.y
#define i02_re I2.x
#define i02_im I2.y
#define i10_re I3.x
#define i10_im I3.y
#define i11_re I4.x
#define i11_im I4.y
#define i12_re I5.x
#define i12_im I5.y
#define i20_re I6.x
#define i20_im I6.y
#define i21_re I7.x
#define i21_im I7.y
#define i22_re I8.x
#define i22_im I8.y
#define i30_re I9.x
#define i30_im I9.y
#define i31_re I10.x
#define i31_im I10.y
#define i32_re I11.x
#define i32_im I11.y
#define m5 m5_d
#define mdwf_b5 mdwf_b5_d
#define mdwf_c5 mdwf_c5_d
#define mferm param.mferm
#define a param.a
#define b param.b
#else
#define spinorFloat float
#define POW(a, b) __fast_pow(a, b)
#define i00_re I0.x
#define i00_im I0.y
#define i01_re I0.z
#define i01_im I0.w
#define i02_re I1.x
#define i02_im I1.y
#define i10_re I1.z
#define i10_im I1.w
#define i11_re I2.x
#define i11_im I2.y
#define i12_re I2.z
#define i12_im I2.w
#define i20_re I3.x
#define i20_im I3.y
#define i21_re I3.z
#define i21_im I3.w
#define i22_re I4.x
#define i22_im I4.y
#define i30_re I4.z
#define i30_im I4.w
#define i31_re I5.x
#define i31_im I5.y
#define i32_re I5.z
#define i32_im I5.w
#define m5 m5_f
#define mdwf_b5 mdwf_b5_f
#define mdwf_c5 mdwf_c5_f
#define mferm param.mferm_f
#define a param.a
#define b param.b
#endif // SPINOR_DOUBLE

// gauge link
#ifdef GAUGE_FLOAT2
#define g00_re G0.x
#define g00_im G0.y
#define g01_re G1.x
#define g01_im G1.y
#define g02_re G2.x
#define g02_im G2.y
#define g10_re G3.x
#define g10_im G3.y
#define g11_re G4.x
#define g11_im G4.y
#define g12_re G5.x
#define g12_im G5.y
#define g20_re G6.x
#define g20_im G6.y
#define g21_re G7.x
#define g21_im G7.y
#define g22_re G8.x
#define g22_im G8.y

#else
#define g00_re G0.x
#define g00_im G0.y
#define g01_re G0.z
#define g01_im G0.w
#define g02_re G1.x
#define g02_im G1.y
#define g10_re G1.z
#define g10_im G1.w
#define g11_re G2.x
#define g11_im G2.y
#define g12_re G2.z
#define g12_im G2.w
#define g20_re G3.x
#define g20_im G3.y
#define g21_re G3.z
#define g21_im G3.w
#define g22_re G4.x
#define g22_im G4.y

#endif // GAUGE_DOUBLE

// conjugated gauge link
#define gT00_re (+g00_re)
#define gT00_im (-g00_im)
#define gT01_re (+g10_re)
#define gT01_im (-g10_im)
#define gT02_re (+g20_re)
#define gT02_im (-g20_im)
#define gT10_re (+g01_re)
#define gT10_im (-g01_im)
#define gT11_re (+g11_re)
#define gT11_im (-g11_im)
#define gT12_re (+g21_re)
#define gT12_im (-g21_im)
#define gT20_re (+g02_re)
#define gT20_im (-g02_im)
#define gT21_re (+g12_re)
#define gT21_im (-g12_im)
#define gT22_re (+g22_re)
#define gT22_im (-g22_im)

// output spinor
VOLATILE spinorFloat o00_re;
VOLATILE spinorFloat o00_im;
VOLATILE spinorFloat o01_re;
VOLATILE spinorFloat o01_im;
VOLATILE spinorFloat o02_re;
VOLATILE spinorFloat o02_im;
VOLATILE spinorFloat o10_re;
VOLATILE spinorFloat o10_im;
VOLATILE spinorFloat o11_re;
VOLATILE spinorFloat o11_im;
VOLATILE spinorFloat o12_re;
VOLATILE spinorFloat o12_im;
VOLATILE spinorFloat o20_re;
VOLATILE spinorFloat o20_im;
VOLATILE spinorFloat o21_re;
VOLATILE spinorFloat o21_im;
VOLATILE spinorFloat o22_re;
VOLATILE spinorFloat o22_im;
VOLATILE spinorFloat o30_re;
VOLATILE spinorFloat o30_im;
VOLATILE spinorFloat o31_re;
VOLATILE spinorFloat o31_im;
VOLATILE spinorFloat o32_re;
VOLATILE spinorFloat o32_im;

#ifdef SPINOR_DOUBLE
#if (__COMPUTE_CAPABILITY__ >= 200)
#define SHARED_STRIDE 16 // to avoid bank conflicts on Fermi
#else
#define SHARED_STRIDE 8 // to avoid bank conflicts on G80 and GT200
#endif
#else
#if (__COMPUTE_CAPABILITY__ >= 200)
#define SHARED_STRIDE 32 // to avoid bank conflicts on Fermi
#else
#define SHARED_STRIDE 16 // to avoid bank conflicts on G80 and GT200
#endif
#endif

#include "read_gauge.h"
#include "io_spinor.h"

int sid = ((blockIdx.y*blockDim.y + threadIdx.y)*gridDim.x + blockIdx.x)*blockDim.x + threadIdx.x;
if (sid >= param.threads*param.Ls) return;


int X, coord[5];

int face_idx;
if (kernel_type == INTERIOR_KERNEL) {




  // Assume even dimensions
  coordsFromIndex<5,QUDA_4D_PC,EVEN_X>(X, coord, sid, param);

 o00_re = 0; o00_im = 0;
 o01_re = 0; o01_im = 0;
 o02_re = 0; o02_im = 0;
 o10_re = 0; o10_im = 0;
 o11_re = 0; o11_im = 0;
 o12_re = 0; o12_im = 0;
 o20_re = 0; o20_im = 0;
 o21_re = 0; o21_im = 0;
 o22_re = 0; o22_im = 0;
 o30_re = 0; o30_im = 0;
 o31_re = 0; o31_im = 0;
 o32_re = 0; o32_im = 0;

} else { // exterior kernel

const int face_volume = (param.threads*param.Ls >> 1); // volume of one face
const int face_num = (sid >= face_volume); // is this thread updating face 0 or 1
face_idx = sid - face_num*face_volume; // index into the respective face

// ghostOffset is scaled to include body (includes stride) and number of FloatN arrays (SPINOR_HOP)
// face_idx not sid since faces are spin projected and share the same volume index (modulo UP/DOWN reading)
//sp_idx = face_idx + param.ghostOffset[dim];

coordsFromFaceIndex<5,QUDA_4D_PC,kernel_type,1>(X, sid, coord, face_idx, face_num, param);

READ_INTERMEDIATE_SPINOR(INTERTEX, param.sp_stride, sid, sid);
 o00_re = i00_re; o00_im = i00_im;
 o01_re = i01_re; o01_im = i01_im;
 o02_re = i02_re; o02_im = i02_im;
 o10_re = i10_re; o10_im = i10_im;
 o11_re = i11_re; o11_im = i11_im;
 o12_re = i12_re; o12_im = i12_im;
 o20_re = i20_re; o20_im = i20_im;
 o21_re = i21_re; o21_im = i21_im;
 o22_re = i22_re; o22_im = i22_im;
 o30_re = i30_re; o30_im = i30_im;
 o31_re = i31_re; o31_im = i31_im;
 o32_re = i32_re; o32_im = i32_im;
}

// declare G## here and use ASSN below instead of READ
#ifdef GAUGE_FLOAT2
#if (DD_PREC==0) //temporal hack
double2 G0;
double2 G1;
double2 G2;
double2 G3;
double2 G4;
double2 G5;
double2 G6;
double2 G7;
double2 G8;
#else
float2 G0;
float2 G1;
float2 G2;
float2 G3;
float2 G4;
float2 G5;
float2 G6;
float2 G7;
float2 G8;
#endif
#else
float4 G0;
float4 G1;
float4 G2;
float4 G3;
float4 G4;
#endif



#ifdef MULTI_GPU
if ( (kernel_type == INTERIOR_KERNEL && (!param.ghostDim[0] || coord[0]<(param.X[0]-1))) ||
 (kernel_type == EXTERIOR_KERNEL_X && coord[0]==(param.X[0]-1)) )
#endif
{
 // Projector P0-
 //  1  0  0 -i
 //  0  1 -i  0
 //  0  i  1  0
 //  i  0  0  1

#ifdef MULTI_GPU
 const int sp_idx = (kernel_type == INTERIOR_KERNEL) ? (coord[0]==(param.X[0]-1) ? X-(param.X[0]-1) : X+1) >> 1 :
  face_idx + param.ghostOffset[static_cast<int>(kernel_type)][1];
#if (DD_PREC==2) // half precision
 const int sp_norm_idx = face_idx + param.ghostNormOffset[static_cast<int>(kernel_type)][1];
#endif
#else
 const int sp_idx = (coord[0]==(param.X[0]-1) ? X-(param.X[0]-1) : X+1) >> 1;
#endif

 const int ga_idx = sid % param.volume4CB;

 // read gauge matrix from device memory
 ASSN_GAUGE_MATRIX(G, GAUGE0TEX, 0, ga_idx, param.gauge_stride);

 spinorFloat a0_re, a0_im;
 spinorFloat a1_re, a1_im;
 spinorFloat a2_re, a2_im;
 spinorFloat b0_re, b0_im;
 spinorFloat b1_re, b1_im;
 spinorFloat b2_re, b2_im;

#ifdef MULTI_GPU
 if (kernel_type == INTERIOR_KERNEL) {
#endif

  // read spinor from device memory
  READ_SPINOR(SPINORTEX, param.sp_stride, sp_idx, sp_idx);

  // project spinor into half spinors
  a0_re = +i00_re+i30_im;
  a0_im = +i00_im-i30_re;
  a1_re = +i01_re+i31_im;
  a1_im = +i01_im-i31_re;
  a2_re = +i02_re+i32_im;
  a2_im = +i02_im-i32_re;
  b0_re = +i10_re+i20_im;
  b0_im = +i10_im-i20_re;
  b1_re = +i11_re+i21_im;
  b1_im = +i11_im-i21_re;
  b2_re = +i12_re+i22_im;
  b2_im = +i12_im-i22_re;

#ifdef MULTI_GPU
 } else {

  const int sp_stride_pad = param.Ls*param.ghostFace[static_cast<int>(kernel_type)];

  // read half spinor from device memory
  READ_SPINOR_GHOST(GHOSTSPINORTEX, sp_stride_pad, sp_idx, sp_norm_idx, 0);

  a0_re = i00_re; a0_im = i00_im;
  a1_re = i01_re; a1_im = i01_im;
  a2_re = i02_re; a2_im = i02_im;
  b0_re = i10_re; b0_im = i10_im;
  b1_re = i11_re; b1_im = i11_im;
  b2_re = i12_re; b2_im = i12_im;

 }
#endif // MULTI_GPU

 // reconstruct gauge matrix
 RECONSTRUCT_GAUGE_MATRIX(0);

 // multiply row 0
 spinorFloat A0_re = 0;
 A0_re += g00_re * a0_re;
 A0_re -= g00_im * a0_im;
 A0_re += g01_re * a1_re;
 A0_re -= g01_im * a1_im;
 A0_re += g02_re * a2_re;
 A0_re -= g02_im * a2_im;
 spinorFloat A0_im = 0;
 A0_im += g00_re * a0_im;
 A0_im += g00_im * a0_re;
 A0_im += g01_re * a1_im;
 A0_im += g01_im * a1_re;
 A0_im += g02_re * a2_im;
 A0_im += g02_im * a2_re;
 spinorFloat B0_re = 0;
 B0_re += g00_re * b0_re;
 B0_re -= g00_im * b0_im;
 B0_re += g01_re * b1_re;
 B0_re -= g01_im * b1_im;
 B0_re += g02_re * b2_re;
 B0_re -= g02_im * b2_im;
 spinorFloat B0_im = 0;
 B0_im += g00_re * b0_im;
 B0_im += g00_im * b0_re;
 B0_im += g01_re * b1_im;
 B0_im += g01_im * b1_re;
 B0_im += g02_re * b2_im;
 B0_im += g02_im * b2_re;

 // multiply row 1
 spinorFloat A1_re = 0;
 A1_re += g10_re * a0_re;
 A1_re -= g10_im * a0_im;
 A1_re += g11_re * a1_re;
 A1_re -= g11_im * a1_im;
 A1_re += g12_re * a2_re;
 A1_re -= g12_im * a2_im;
 spinorFloat A1_im = 0;
 A1_im += g10_re * a0_im;
 A1_im += g10_im * a0_re;
 A1_im += g11_re * a1_im;
 A1_im += g11_im * a1_re;
 A1_im += g12_re * a2_im;
 A1_im += g12_im * a2_re;
 spinorFloat B1_re = 0;
 B1_re += g10_re * b0_re;
 B1_re -= g10_im * b0_im;
 B1_re += g11_re * b1_re;
 B1_re -= g11_im * b1_im;
 B1_re += g12_re * b2_re;
 B1_re -= g12_im * b2_im;
 spinorFloat B1_im = 0;
 B1_im += g10_re * b0_im;
 B1_im += g10_im * b0_re;
 B1_im += g11_re * b1_im;
 B1_im += g11_im * b1_re;
 B1_im += g12_re * b2_im;
 B1_im += g12_im * b2_re;

 // multiply row 2
 spinorFloat A2_re = 0;
 A2_re += g20_re * a0_re;
 A2_re -= g20_im * a0_im;
 A2_re += g21_re * a1_re;
 A2_re -= g21_im * a1_im;
 A2_re += g22_re * a2_re;
 A2_re -= g22_im * a2_im;
 spinorFloat A2_im = 0;
 A2_im += g20_re * a0_im;
 A2_im += g20_im * a0_re;
 A2_im += g21_re * a1_im;
 A2_im += g21_im * a1_re;
 A2_im += g22_re * a2_im;
 A2_im += g22_im * a2_re;
 spinorFloat B2_re = 0;
 B2_re += g20_re * b0_re;
 B2_re -= g20_im * b0_im;
 B2_re += g21_re * b1_re;
 B2_re -= g21_im * b1_im;
 B2_re += g22_re * b2_re;
 B2_re -= g22_im * b2_im;
 spinorFloat B2_im = 0;
 B2_im += g20_re * b0_im;
 B2_im += g20_im * b0_re;
 B2_im += g21_re * b1_im;
 B2_im += g21_im * b1_re;
 B2_im += g22_re * b2_im;
 B2_im += g22_im * b2_re;

 o00_re += A0_re;
 o00_im += A0_im;
 o10_re += B0_re;
 o10_im += B0_im;
 o20_re -= B0_im;
 o20_im += B0_re;
 o30_re -= A0_im;
 o30_im += A0_re;

 o01_re += A1_re;
 o01_im += A1_im;
 o11_re += B1_re;
 o11_im += B1_im;
 o21_re -= B1_im;
 o21_im += B1_re;
 o31_re -= A1_im;
 o31_im += A1_re;

 o02_re += A2_re;
 o02_im += A2_im;
 o12_re += B2_re;
 o12_im += B2_im;
 o22_re -= B2_im;
 o22_im += B2_re;
 o32_re -= A2_im;
 o32_im += A2_re;
}

#ifdef MULTI_GPU
if ( (kernel_type == INTERIOR_KERNEL && (!param.ghostDim[0] || coord[0]>0)) ||
 (kernel_type == EXTERIOR_KERNEL_X && coord[0]==0) )
#endif
{
 // Projector P0+
 //  1  0  0  i
 //  0  1  i  0
 //  0 -i  1  0
 // -i  0  0  1

#ifdef MULTI_GPU
 const int sp_idx = (kernel_type == INTERIOR_KERNEL) ? (coord[0]==0 ? X+(param.X[0]-1) : X-1) >> 1 :
  face_idx + param.ghostOffset[static_cast<int>(kernel_type)][0];
#if (DD_PREC==2) // half precision
 const int sp_norm_idx = face_idx + param.ghostNormOffset[static_cast<int>(kernel_type)][0];
#endif
#else
 const int sp_idx = (coord[0]==0 ? X+(param.X[0]-1) : X-1) >> 1;
#endif

#ifdef MULTI_GPU
 const int ga_idx = ((kernel_type == INTERIOR_KERNEL) ? sp_idx % param.volume4CB : param.volume4CB+(face_idx % param.ghostFace[static_cast<int>(kernel_type)]));
#else
 const int ga_idx = sp_idx % param.volume4CB;
#endif

 // read gauge matrix from device memory
 ASSN_GAUGE_MATRIX(G, GAUGE1TEX, 1, ga_idx, param.gauge_stride);

 spinorFloat a0_re, a0_im;
 spinorFloat a1_re, a1_im;
 spinorFloat a2_re, a2_im;
 spinorFloat b0_re, b0_im;
 spinorFloat b1_re, b1_im;
 spinorFloat b2_re, b2_im;

#ifdef MULTI_GPU
 if (kernel_type == INTERIOR_KERNEL) {
#endif

  // read spinor from device memory
  READ_SPINOR(SPINORTEX, param.sp_stride, sp_idx, sp_idx);

  // project spinor into half spinors
  a0_re = +i00_re-i30_im;
  a0_im = +i00_im+i30_re;
  a1_re = +i01_re-i31_im;
  a1_im = +i01_im+i31_re;
  a2_re = +i02_re-i32_im;
  a2_im = +i02_im+i32_re;
  b0_re = +i10_re-i20_im;
  b0_im = +i10_im+i20_re;
  b1_re = +i11_re-i21_im;
  b1_im = +i11_im+i21_re;
  b2_re = +i12_re-i22_im;
  b2_im = +i12_im+i22_re;

#ifdef MULTI_GPU
 } else {

  const int sp_stride_pad = param.Ls*param.ghostFace[static_cast<int>(kernel_type)];

  // read half spinor from device memory
  READ_SPINOR_GHOST(GHOSTSPINORTEX, sp_stride_pad, sp_idx, sp_norm_idx, 1);

  a0_re = i00_re; a0_im = i00_im;
  a1_re = i01_re; a1_im = i01_im;
  a2_re = i02_re; a2_im = i02_im;
  b0_re = i10_re; b0_im = i10_im;
  b1_re = i11_re; b1_im = i11_im;
  b2_re = i12_re; b2_im = i12_im;

 }
#endif // MULTI_GPU

 // reconstruct gauge matrix
 RECONSTRUCT_GAUGE_MATRIX(1);

 // multiply row 0
 spinorFloat A0_re = 0;
 A0_re += gT00_re * a0_re;
 A0_re -= gT00_im * a0_im;
 A0_re += gT01_re * a1_re;
 A0_re -= gT01_im * a1_im;
 A0_re += gT02_re * a2_re;
 A0_re -= gT02_im * a2_im;
 spinorFloat A0_im = 0;
 A0_im += gT00_re * a0_im;
 A0_im += gT00_im * a0_re;
 A0_im += gT01_re * a1_im;
 A0_im += gT01_im * a1_re;
 A0_im += gT02_re * a2_im;
 A0_im += gT02_im * a2_re;
 spinorFloat B0_re = 0;
 B0_re += gT00_re * b0_re;
 B0_re -= gT00_im * b0_im;
 B0_re += gT01_re * b1_re;
 B0_re -= gT01_im * b1_im;
 B0_re += gT02_re * b2_re;
 B0_re -= gT02_im * b2_im;
 spinorFloat B0_im = 0;
 B0_im += gT00_re * b0_im;
 B0_im += gT00_im * b0_re;
 B0_im += gT01_re * b1_im;
 B0_im += gT01_im * b1_re;
 B0_im += gT02_re * b2_im;
 B0_im += gT02_im * b2_re;

 // multiply row 1
 spinorFloat A1_re = 0;
 A1_re += gT10_re * a0_re;
 A1_re -= gT10_im * a0_im;
 A1_re += gT11_re * a1_re;
 A1_re -= gT11_im * a1_im;
 A1_re += gT12_re * a2_re;
 A1_re -= gT12_im * a2_im;
 spinorFloat A1_im = 0;
 A1_im += gT10_re * a0_im;
 A1_im += gT10_im * a0_re;
 A1_im += gT11_re * a1_im;
 A1_im += gT11_im * a1_re;
 A1_im += gT12_re * a2_im;
 A1_im += gT12_im * a2_re;
 spinorFloat B1_re = 0;
 B1_re += gT10_re * b0_re;
 B1_re -= gT10_im * b0_im;
 B1_re += gT11_re * b1_re;
 B1_re -= gT11_im * b1_im;
 B1_re += gT12_re * b2_re;
 B1_re -= gT12_im * b2_im;
 spinorFloat B1_im = 0;
 B1_im += gT10_re * b0_im;
 B1_im += gT10_im * b0_re;
 B1_im += gT11_re * b1_im;
 B1_im += gT11_im * b1_re;
 B1_im += gT12_re * b2_im;
 B1_im += gT12_im * b2_re;

 // multiply row 2
 spinorFloat A2_re = 0;
 A2_re += gT20_re * a0_re;
 A2_re -= gT20_im * a0_im;
 A2_re += gT21_re * a1_re;
 A2_re -= gT21_im * a1_im;
 A2_re += gT22_re * a2_re;
 A2_re -= gT22_im * a2_im;
 spinorFloat A2_im = 0;
 A2_im += gT20_re * a0_im;
 A2_im += gT20_im * a0_re;
 A2_im += gT21_re * a1_im;
 A2_im += gT21_im * a1_re;
 A2_im += gT22_re * a2_im;
 A2_im += gT22_im * a2_re;
 spinorFloat B2_re = 0;
 B2_re += gT20_re * b0_re;
 B2_re -= gT20_im * b0_im;
 B2_re += gT21_re * b1_re;
 B2_re -= gT21_im * b1_im;
 B2_re += gT22_re * b2_re;
 B2_re -= gT22_im * b2_im;
 spinorFloat B2_im = 0;
 B2_im += gT20_re * b0_im;
 B2_im += gT20_im * b0_re;
 B2_im += gT21_re * b1_im;
 B2_im += gT21_im * b1_re;
 B2_im += gT22_re * b2_im;
 B2_im += gT22_im * b2_re;

 o00_re += A0_re;
 o00_im += A0_im;
 o10_re += B0_re;
 o10_im += B0_im;
 o20_re += B0_im;
 o20_im -= B0_re;
 o30_re += A0_im;
 o30_im -= A0_re;

 o01_re += A1_re;
 o01_im += A1_im;
 o11_re += B1_re;
 o11_im += B1_im;
 o21_re += B1_im;
 o21_im -= B1_re;
 o31_re += A1_im;
 o31_im -= A1_re;

 o02_re += A2_re;
 o02_im += A2_im;
 o12_re += B2_re;
 o12_im += B2_im;
 o22_re += B2_im;
 o22_im -= B2_re;
 o32_re += A2_im;
 o32_im -= A2_re;
}

#ifdef MULTI_GPU
if ( (kernel_type == INTERIOR_KERNEL && (!param.ghostDim[1] || coord[1]<(param.X[1]-1))) ||
 (kernel_type == EXTERIOR_KERNEL_Y && coord[1]==(param.X[1]-1)) )
#endif
{
 // Projector P1-
 //  1  0  0 -1
 //  0  1  1  0
 //  0  1  1  0
 // -1  0  0  1

#ifdef MULTI_GPU
 const int sp_idx = (kernel_type == INTERIOR_KERNEL) ? (coord[1]==(param.X[1]-1) ? X-param.X2X1mX1 : X+param.X[0]) >> 1 :
  face_idx + param.ghostOffset[static_cast<int>(kernel_type)][1];
#if (DD_PREC==2) // half precision
 const int sp_norm_idx = face_idx + param.ghostNormOffset[static_cast<int>(kernel_type)][1];
#endif
#else
 const int sp_idx = (coord[1]==(param.X[1]-1) ? X-param.X2X1mX1 : X+param.X[0]) >> 1;
#endif

 const int ga_idx = sid % param.volume4CB;

 // read gauge matrix from device memory
 ASSN_GAUGE_MATRIX(G, GAUGE0TEX, 2, ga_idx, param.gauge_stride);

 spinorFloat a0_re, a0_im;
 spinorFloat a1_re, a1_im;
 spinorFloat a2_re, a2_im;
 spinorFloat b0_re, b0_im;
 spinorFloat b1_re, b1_im;
 spinorFloat b2_re, b2_im;

#ifdef MULTI_GPU
 if (kernel_type == INTERIOR_KERNEL) {
#endif

  // read spinor from device memory
  READ_SPINOR(SPINORTEX, param.sp_stride, sp_idx, sp_idx);

  // project spinor into half spinors
  a0_re = +i00_re-i30_re;
  a0_im = +i00_im-i30_im;
  a1_re = +i01_re-i31_re;
  a1_im = +i01_im-i31_im;
  a2_re = +i02_re-i32_re;
  a2_im = +i02_im-i32_im;
  b0_re = +i10_re+i20_re;
  b0_im = +i10_im+i20_im;
  b1_re = +i11_re+i21_re;
  b1_im = +i11_im+i21_im;
  b2_re = +i12_re+i22_re;
  b2_im = +i12_im+i22_im;

#ifdef MULTI_GPU
 } else {

  const int sp_stride_pad = param.Ls*param.ghostFace[static_cast<int>(kernel_type)];

  // read half spinor from device memory
  READ_SPINOR_GHOST(GHOSTSPINORTEX, sp_stride_pad, sp_idx, sp_norm_idx, 2);

  a0_re = i00_re; a0_im = i00_im;
  a1_re = i01_re; a1_im = i01_im;
  a2_re = i02_re; a2_im = i02_im;
  b0_re = i10_re; b0_im = i10_im;
  b1_re = i11_re; b1_im = i11_im;
  b2_re = i12_re; b2_im = i12_im;

 }
#endif // MULTI_GPU

 // reconstruct gauge matrix
 RECONSTRUCT_GAUGE_MATRIX(2);

 // multiply row 0
 spinorFloat A0_re = 0;
 A0_re += g00_re * a0_re;
 A0_re -= g00_im * a0_im;
 A0_re += g01_re * a1_re;
 A0_re -= g01_im * a1_im;
 A0_re += g02_re * a2_re;
 A0_re -= g02_im * a2_im;
 spinorFloat A0_im = 0;
 A0_im += g00_re * a0_im;
 A0_im += g00_im * a0_re;
 A0_im += g01_re * a1_im;
 A0_im += g01_im * a1_re;
 A0_im += g02_re * a2_im;
 A0_im += g02_im * a2_re;
 spinorFloat B0_re = 0;
 B0_re += g00_re * b0_re;
 B0_re -= g00_im * b0_im;
 B0_re += g01_re * b1_re;
 B0_re -= g01_im * b1_im;
 B0_re += g02_re * b2_re;
 B0_re -= g02_im * b2_im;
 spinorFloat B0_im = 0;
 B0_im += g00_re * b0_im;
 B0_im += g00_im * b0_re;
 B0_im += g01_re * b1_im;
 B0_im += g01_im * b1_re;
 B0_im += g02_re * b2_im;
 B0_im += g02_im * b2_re;

 // multiply row 1
 spinorFloat A1_re = 0;
 A1_re += g10_re * a0_re;
 A1_re -= g10_im * a0_im;
 A1_re += g11_re * a1_re;
 A1_re -= g11_im * a1_im;
 A1_re += g12_re * a2_re;
 A1_re -= g12_im * a2_im;
 spinorFloat A1_im = 0;
 A1_im += g10_re * a0_im;
 A1_im += g10_im * a0_re;
 A1_im += g11_re * a1_im;
 A1_im += g11_im * a1_re;
 A1_im += g12_re * a2_im;
 A1_im += g12_im * a2_re;
 spinorFloat B1_re = 0;
 B1_re += g10_re * b0_re;
 B1_re -= g10_im * b0_im;
 B1_re += g11_re * b1_re;
 B1_re -= g11_im * b1_im;
 B1_re += g12_re * b2_re;
 B1_re -= g12_im * b2_im;
 spinorFloat B1_im = 0;
 B1_im += g10_re * b0_im;
 B1_im += g10_im * b0_re;
 B1_im += g11_re * b1_im;
 B1_im += g11_im * b1_re;
 B1_im += g12_re * b2_im;
 B1_im += g12_im * b2_re;

 // multiply row 2
 spinorFloat A2_re = 0;
 A2_re += g20_re * a0_re;
 A2_re -= g20_im * a0_im;
 A2_re += g21_re * a1_re;
 A2_re -= g21_im * a1_im;
 A2_re += g22_re * a2_re;
 A2_re -= g22_im * a2_im;
 spinorFloat A2_im = 0;
 A2_im += g20_re * a0_im;
 A2_im += g20_im * a0_re;
 A2_im += g21_re * a1_im;
 A2_im += g21_im * a1_re;
 A2_im += g22_re * a2_im;
 A2_im += g22_im * a2_re;
 spinorFloat B2_re = 0;
 B2_re += g20_re * b0_re;
 B2_re -= g20_im * b0_im;
 B2_re += g21_re * b1_re;
 B2_re -= g21_im * b1_im;
 B2_re += g22_re * b2_re;
 B2_re -= g22_im * b2_im;
 spinorFloat B2_im = 0;
 B2_im += g20_re * b0_im;
 B2_im += g20_im * b0_re;
 B2_im += g21_re * b1_im;
 B2_im += g21_im * b1_re;
 B2_im += g22_re * b2_im;
 B2_im += g22_im * b2_re;

 o00_re += A0_re;
 o00_im += A0_im;
 o10_re += B0_re;
 o10_im += B0_im;
 o20_re += B0_re;
 o20_im += B0_im;
 o30_re -= A0_re;
 o30_im -= A0_im;

 o01_re += A1_re;
 o01_im += A1_im;
 o11_re += B1_re;
 o11_im += B1_im;
 o21_re += B1_re;
 o21_im += B1_im;
 o31_re -= A1_re;
 o31_im -= A1_im;

 o02_re += A2_re;
 o02_im += A2_im;
 o12_re += B2_re;
 o12_im += B2_im;
 o22_re += B2_re;
 o22_im += B2_im;
 o32_re -= A2_re;
 o32_im -= A2_im;
}

#ifdef MULTI_GPU
if ( (kernel_type == INTERIOR_KERNEL && (!param.ghostDim[1] || coord[1]>0)) ||
 (kernel_type == EXTERIOR_KERNEL_Y && coord[1]==0) )
#endif
{
 // Projector P1+
 //  1  0  0  1
 //  0  1 -1  0
 //  0 -1  1  0
 //  1  0  0  1

#ifdef MULTI_GPU
 const int sp_idx = (kernel_type == INTERIOR_KERNEL) ? (coord[1]==0 ? X+param.X2X1mX1 : X-param.X[0]) >> 1 :
  face_idx + param.ghostOffset[static_cast<int>(kernel_type)][0];
#if (DD_PREC==2) // half precision
 const int sp_norm_idx = face_idx + param.ghostNormOffset[static_cast<int>(kernel_type)][0];
#endif
#else
 const int sp_idx = (coord[1]==0 ? X+param.X2X1mX1 : X-param.X[0]) >> 1;
#endif

#ifdef MULTI_GPU
 const int ga_idx = ((kernel_type == INTERIOR_KERNEL) ? sp_idx % param.volume4CB : param.volume4CB+(face_idx % param.ghostFace[static_cast<int>(kernel_type)]));
#else
 const int ga_idx = sp_idx % param.volume4CB;
#endif

 // read gauge matrix from device memory
 ASSN_GAUGE_MATRIX(G, GAUGE1TEX, 3, ga_idx, param.gauge_stride);

 spinorFloat a0_re, a0_im;
 spinorFloat a1_re, a1_im;
 spinorFloat a2_re, a2_im;
 spinorFloat b0_re, b0_im;
 spinorFloat b1_re, b1_im;
 spinorFloat b2_re, b2_im;

#ifdef MULTI_GPU
 if (kernel_type == INTERIOR_KERNEL) {
#endif

  // read spinor from device memory
  READ_SPINOR(SPINORTEX, param.sp_stride, sp_idx, sp_idx);

  // project spinor into half spinors
  a0_re = +i00_re+i30_re;
  a0_im = +i00_im+i30_im;
  a1_re = +i01_re+i31_re;
  a1_im = +i01_im+i31_im;
  a2_re = +i02_re+i32_re;
  a2_im = +i02_im+i32_im;
  b0_re = +i10_re-i20_re;
  b0_im = +i10_im-i20_im;
  b1_re = +i11_re-i21_re;
  b1_im = +i11_im-i21_im;
  b2_re = +i12_re-i22_re;
  b2_im = +i12_im-i22_im;

#ifdef MULTI_GPU
 } else {

  const int sp_stride_pad = param.Ls*param.ghostFace[static_cast<int>(kernel_type)];

  // read half spinor from device memory
  READ_SPINOR_GHOST(GHOSTSPINORTEX, sp_stride_pad, sp_idx, sp_norm_idx, 3);

  a0_re = i00_re; a0_im = i00_im;
  a1_re = i01_re; a1_im = i01_im;
  a2_re = i02_re; a2_im = i02_im;
  b0_re = i10_re; b0_im = i10_im;
  b1_re = i11_re; b1_im = i11_im;
  b2_re = i12_re; b2_im = i12_im;

 }
#endif // MULTI_GPU

 // reconstruct gauge matrix
 RECONSTRUCT_GAUGE_MATRIX(3);

 // multiply row 0
 spinorFloat A0_re = 0;
 A0_re += gT00_re * a0_re;
 A0_re -= gT00_im * a0_im;
 A0_re += gT01_re * a1_re;
 A0_re -= gT01_im * a1_im;
 A0_re += gT02_re * a2_re;
 A0_re -= gT02_im * a2_im;
 spinorFloat A0_im = 0;
 A0_im += gT00_re * a0_im;
 A0_im += gT00_im * a0_re;
 A0_im += gT01_re * a1_im;
 A0_im += gT01_im * a1_re;
 A0_im += gT02_re * a2_im;
 A0_im += gT02_im * a2_re;
 spinorFloat B0_re = 0;
 B0_re += gT00_re * b0_re;
 B0_re -= gT00_im * b0_im;
 B0_re += gT01_re * b1_re;
 B0_re -= gT01_im * b1_im;
 B0_re += gT02_re * b2_re;
 B0_re -= gT02_im * b2_im;
 spinorFloat B0_im = 0;
 B0_im += gT00_re * b0_im;
 B0_im += gT00_im * b0_re;
 B0_im += gT01_re * b1_im;
 B0_im += gT01_im * b1_re;
 B0_im += gT02_re * b2_im;
 B0_im += gT02_im * b2_re;

 // multiply row 1
 spinorFloat A1_re = 0;
 A1_re += gT10_re * a0_re;
 A1_re -= gT10_im * a0_im;
 A1_re += gT11_re * a1_re;
 A1_re -= gT11_im * a1_im;
 A1_re += gT12_re * a2_re;
 A1_re -= gT12_im * a2_im;
 spinorFloat A1_im = 0;
 A1_im += gT10_re * a0_im;
 A1_im += gT10_im * a0_re;
 A1_im += gT11_re * a1_im;
 A1_im += gT11_im * a1_re;
 A1_im += gT12_re * a2_im;
 A1_im += gT12_im * a2_re;
 spinorFloat B1_re = 0;
 B1_re += gT10_re * b0_re;
 B1_re -= gT10_im * b0_im;
 B1_re += gT11_re * b1_re;
 B1_re -= gT11_im * b1_im;
 B1_re += gT12_re * b2_re;
 B1_re -= gT12_im * b2_im;
 spinorFloat B1_im = 0;
 B1_im += gT10_re * b0_im;
 B1_im += gT10_im * b0_re;
 B1_im += gT11_re * b1_im;
 B1_im += gT11_im * b1_re;
 B1_im += gT12_re * b2_im;
 B1_im += gT12_im * b2_re;

 // multiply row 2
 spinorFloat A2_re = 0;
 A2_re += gT20_re * a0_re;
 A2_re -= gT20_im * a0_im;
 A2_re += gT21_re * a1_re;
 A2_re -= gT21_im * a1_im;
 A2_re += gT22_re * a2_re;
 A2_re -= gT22_im * a2_im;
 spinorFloat A2_im = 0;
 A2_im += gT20_re * a0_im;
 A2_im += gT20_im * a0_re;
 A2_im += gT21_re * a1_im;
 A2_im += gT21_im * a1_re;
 A2_im += gT22_re * a2_im;
 A2_im += gT22_im * a2_re;
 spinorFloat B2_re = 0;
 B2_re += gT20_re * b0_re;
 B2_re -= gT20_im * b0_im;
 B2_re += gT21_re * b1_re;
 B2_re -= gT21_im * b1_im;
 B2_re += gT22_re * b2_re;
 B2_re -= gT22_im * b2_im;
 spinorFloat B2_im = 0;
 B2_im += gT20_re * b0_im;
 B2_im += gT20_im * b0_re;
 B2_im += gT21_re * b1_im;
 B2_im += gT21_im * b1_re;
 B2_im += gT22_re * b2_im;
 B2_im += gT22_im * b2_re;

 o00_re += A0_re;
 o00_im += A0_im;
 o10_re += B0_re;
 o10_im += B0_im;
 o20_re -= B0_re;
 o20_im -= B0_im;
 o30_re += A0_re;
 o30_im += A0_im;

 o01_re += A1_re;
 o01_im += A1_im;
 o11_re += B1_re;
 o11_im += B1_im;
 o21_re -= B1_re;
 o21_im -= B1_im;
 o31_re += A1_re;
 o31_im += A1_im;

 o02_re += A2_re;
 o02_im += A2_im;
 o12_re += B2_re;
 o12_im += B2_im;
 o22_re -= B2_re;
 o22_im -= B2_im;
 o32_re += A2_re;
 o32_im += A2_im;
}

#ifdef MULTI_GPU
if ( (kernel_type == INTERIOR_KERNEL && (!param.ghostDim[2] || coord[2]<(param.X[2]-1))) ||
 (kernel_type == EXTERIOR_KERNEL_Z && coord[2]==(param.X[2]-1)) )
#endif
{
 // Projector P2-
 //  1  0 -i  0
 //  0  1  0  i
 //  i  0  1  0
 //  0 -i  0  1

#ifdef MULTI_GPU
 const int sp_idx = (kernel_type == INTERIOR_KERNEL) ? (coord[2]==(param.X[2]-1) ? X-param.X3X2X1mX2X1 : X+param.X2X1) >> 1 :
  face_idx + param.ghostOffset[static_cast<int>(kernel_type)][1];
#if (DD_PREC==2) // half precision
 const int sp_norm_idx = face_idx + param.ghostNormOffset[static_cast<int>(kernel_type)][1];
#endif
#else
 const int sp_idx = (coord[2]==(param.X[2]-1) ? X-param.X3X2X1mX2X1 : X+param.X2X1) >> 1;
#endif

 const int ga_idx = sid % param.volume4CB;

 // read gauge matrix from device memory
 ASSN_GAUGE_MATRIX(G, GAUGE0TEX, 4, ga_idx, param.gauge_stride);

 spinorFloat a0_re, a0_im;
 spinorFloat a1_re, a1_im;
 spinorFloat a2_re, a2_im;
 spinorFloat b0_re, b0_im;
 spinorFloat b1_re, b1_im;
 spinorFloat b2_re, b2_im;

#ifdef MULTI_GPU
 if (kernel_type == INTERIOR_KERNEL) {
#endif

  // read spinor from device memory
  READ_SPINOR(SPINORTEX, param.sp_stride, sp_idx, sp_idx);

  // project spinor into half spinors
  a0_re = +i00_re+i20_im;
  a0_im = +i00_im-i20_re;
  a1_re = +i01_re+i21_im;
  a1_im = +i01_im-i21_re;
  a2_re = +i02_re+i22_im;
  a2_im = +i02_im-i22_re;
  b0_re = +i10_re-i30_im;
  b0_im = +i10_im+i30_re;
  b1_re = +i11_re-i31_im;
  b1_im = +i11_im+i31_re;
  b2_re = +i12_re-i32_im;
  b2_im = +i12_im+i32_re;

#ifdef MULTI_GPU
 } else {

  const int sp_stride_pad = param.Ls*param.ghostFace[static_cast<int>(kernel_type)];

  // read half spinor from device memory
  READ_SPINOR_GHOST(GHOSTSPINORTEX, sp_stride_pad, sp_idx, sp_norm_idx, 4);

  a0_re = i00_re; a0_im = i00_im;
  a1_re = i01_re; a1_im = i01_im;
  a2_re = i02_re; a2_im = i02_im;
  b0_re = i10_re; b0_im = i10_im;
  b1_re = i11_re; b1_im = i11_im;
  b2_re = i12_re; b2_im = i12_im;

 }
#endif // MULTI_GPU

 // reconstruct gauge matrix
 RECONSTRUCT_GAUGE_MATRIX(4);

 // multiply row 0
 spinorFloat A0_re = 0;
 A0_re += g00_re * a0_re;
 A0_re -= g00_im * a0_im;
 A0_re += g01_re * a1_re;
 A0_re -= g01_im * a1_im;
 A0_re += g02_re * a2_re;
 A0_re -= g02_im * a2_im;
 spinorFloat A0_im = 0;
 A0_im += g00_re * a0_im;
 A0_im += g00_im * a0_re;
 A0_im += g01_re * a1_im;
 A0_im += g01_im * a1_re;
 A0_im += g02_re * a2_im;
 A0_im += g02_im * a2_re;
 spinorFloat B0_re = 0;
 B0_re += g00_re * b0_re;
 B0_re -= g00_im * b0_im;
 B0_re += g01_re * b1_re;
 B0_re -= g01_im * b1_im;
 B0_re += g02_re * b2_re;
 B0_re -= g02_im * b2_im;
 spinorFloat B0_im = 0;
 B0_im += g00_re * b0_im;
 B0_im += g00_im * b0_re;
 B0_im += g01_re * b1_im;
 B0_im += g01_im * b1_re;
 B0_im += g02_re * b2_im;
 B0_im += g02_im * b2_re;

 // multiply row 1
 spinorFloat A1_re = 0;
 A1_re += g10_re * a0_re;
 A1_re -= g10_im * a0_im;
 A1_re += g11_re * a1_re;
 A1_re -= g11_im * a1_im;
 A1_re += g12_re * a2_re;
 A1_re -= g12_im * a2_im;
 spinorFloat A1_im = 0;
 A1_im += g10_re * a0_im;
 A1_im += g10_im * a0_re;
 A1_im += g11_re * a1_im;
 A1_im += g11_im * a1_re;
 A1_im += g12_re * a2_im;
 A1_im += g12_im * a2_re;
 spinorFloat B1_re = 0;
 B1_re += g10_re * b0_re;
 B1_re -= g10_im * b0_im;
 B1_re += g11_re * b1_re;
 B1_re -= g11_im * b1_im;
 B1_re += g12_re * b2_re;
 B1_re -= g12_im * b2_im;
 spinorFloat B1_im = 0;
 B1_im += g10_re * b0_im;
 B1_im += g10_im * b0_re;
 B1_im += g11_re * b1_im;
 B1_im += g11_im * b1_re;
 B1_im += g12_re * b2_im;
 B1_im += g12_im * b2_re;

 // multiply row 2
 spinorFloat A2_re = 0;
 A2_re += g20_re * a0_re;
 A2_re -= g20_im * a0_im;
 A2_re += g21_re * a1_re;
 A2_re -= g21_im * a1_im;
 A2_re += g22_re * a2_re;
 A2_re -= g22_im * a2_im;
 spinorFloat A2_im = 0;
 A2_im += g20_re * a0_im;
 A2_im += g20_im * a0_re;
 A2_im += g21_re * a1_im;
 A2_im += g21_im * a1_re;
 A2_im += g22_re * a2_im;
 A2_im += g22_im * a2_re;
 spinorFloat B2_re = 0;
 B2_re += g20_re * b0_re;
 B2_re -= g20_im * b0_im;
 B2_re += g21_re * b1_re;
 B2_re -= g21_im * b1_im;
 B2_re += g22_re * b2_re;
 B2_re -= g22_im * b2_im;
 spinorFloat B2_im = 0;
 B2_im += g20_re * b0_im;
 B2_im += g20_im * b0_re;
 B2_im += g21_re * b1_im;
 B2_im += g21_im * b1_re;
 B2_im += g22_re * b2_im;
 B2_im += g22_im * b2_re;

 o00_re += A0_re;
 o00_im += A0_im;
 o10_re += B0_re;
 o10_im += B0_im;
 o20_re -= A0_im;
 o20_im += A0_re;
 o30_re += B0_im;
 o30_im -= B0_re;

 o01_re += A1_re;
 o01_im += A1_im;
 o11_re += B1_re;
 o11_im += B1_im;
 o21_re -= A1_im;
 o21_im += A1_re;
 o31_re += B1_im;
 o31_im -= B1_re;

 o02_re += A2_re;
 o02_im += A2_im;
 o12_re += B2_re;
 o12_im += B2_im;
 o22_re -= A2_im;
 o22_im += A2_re;
 o32_re += B2_im;
 o32_im -= B2_re;
}

#ifdef MULTI_GPU
if ( (kernel_type == INTERIOR_KERNEL && (!param.ghostDim[2] || coord[2]>0)) ||
 (kernel_type == EXTERIOR_KERNEL_Z && coord[2]==0) )
#endif
{
 // Projector P2+
 //  1  0  i  0
 //  0  1  0 -i
 // -i  0  1  0
 //  0  i  0  1

#ifdef MULTI_GPU
 const int sp_idx = (kernel_type == INTERIOR_KERNEL) ? (coord[2]==0 ? X+param.X3X2X1mX2X1 : X-param.X2X1) >> 1 :
  face_idx + param.ghostOffset[static_cast<int>(kernel_type)][0];
#if (DD_PREC==2) // half precision
 const int sp_norm_idx = face_idx + param.ghostNormOffset[static_cast<int>(kernel_type)][0];
#endif
#else
 const int sp_idx = (coord[2]==0 ? X+param.X3X2X1mX2X1 : X-param.X2X1) >> 1;
#endif

#ifdef MULTI_GPU
 const int ga_idx = ((kernel_type == INTERIOR_KERNEL) ? sp_idx % param.volume4CB : param.volume4CB+(face_idx % param.ghostFace[static_cast<int>(kernel_type)]));
#else
 const int ga_idx = sp_idx % param.volume4CB;
#endif

 // read gauge matrix from device memory
 ASSN_GAUGE_MATRIX(G, GAUGE1TEX, 5, ga_idx, param.gauge_stride);

 spinorFloat a0_re, a0_im;
 spinorFloat a1_re, a1_im;
 spinorFloat a2_re, a2_im;
 spinorFloat b0_re, b0_im;
 spinorFloat b1_re, b1_im;
 spinorFloat b2_re, b2_im;

#ifdef MULTI_GPU
 if (kernel_type == INTERIOR_KERNEL) {
#endif

  // read spinor from device memory
  READ_SPINOR(SPINORTEX, param.sp_stride, sp_idx, sp_idx);

  // project spinor into half spinors
  a0_re = +i00_re-i20_im;
  a0_im = +i00_im+i20_re;
  a1_re = +i01_re-i21_im;
  a1_im = +i01_im+i21_re;
  a2_re = +i02_re-i22_im;
  a2_im = +i02_im+i22_re;
  b0_re = +i10_re+i30_im;
  b0_im = +i10_im-i30_re;
  b1_re = +i11_re+i31_im;
  b1_im = +i11_im-i31_re;
  b2_re = +i12_re+i32_im;
  b2_im = +i12_im-i32_re;

#ifdef MULTI_GPU
 } else {

  const int sp_stride_pad = param.Ls*param.ghostFace[static_cast<int>(kernel_type)];

  // read half spinor from device memory
  READ_SPINOR_GHOST(GHOSTSPINORTEX, sp_stride_pad, sp_idx, sp_norm_idx, 5);

  a0_re = i00_re; a0_im = i00_im;
  a1_re = i01_re; a1_im = i01_im;
  a2_re = i02_re; a2_im = i02_im;
  b0_re = i10_re; b0_im = i10_im;
  b1_re = i11_re; b1_im = i11_im;
  b2_re = i12_re; b2_im = i12_im;

 }
#endif // MULTI_GPU

 // reconstruct gauge matrix
 RECONSTRUCT_GAUGE_MATRIX(5);

 // multiply row 0
 spinorFloat A0_re = 0;
 A0_re += gT00_re * a0_re;
 A0_re -= gT00_im * a0_im;
 A0_re += gT01_re * a1_re;
 A0_re -= gT01_im * a1_im;
 A0_re += gT02_re * a2_re;
 A0_re -= gT02_im * a2_im;
 spinorFloat A0_im = 0;
 A0_im += gT00_re * a0_im;
 A0_im += gT00_im * a0_re;
 A0_im += gT01_re * a1_im;
 A0_im += gT01_im * a1_re;
 A0_im += gT02_re * a2_im;
 A0_im += gT02_im * a2_re;
 spinorFloat B0_re = 0;
 B0_re += gT00_re * b0_re;
 B0_re -= gT00_im * b0_im;
 B0_re += gT01_re * b1_re;
 B0_re -= gT01_im * b1_im;
 B0_re += gT02_re * b2_re;
 B0_re -= gT02_im * b2_im;
 spinorFloat B0_im = 0;
 B0_im += gT00_re * b0_im;
 B0_im += gT00_im * b0_re;
 B0_im += gT01_re * b1_im;
 B0_im += gT01_im * b1_re;
 B0_im += gT02_re * b2_im;
 B0_im += gT02_im * b2_re;

 // multiply row 1
 spinorFloat A1_re = 0;
 A1_re += gT10_re * a0_re;
 A1_re -= gT10_im * a0_im;
 A1_re += gT11_re * a1_re;
 A1_re -= gT11_im * a1_im;
 A1_re += gT12_re * a2_re;
 A1_re -= gT12_im * a2_im;
 spinorFloat A1_im = 0;
 A1_im += gT10_re * a0_im;
 A1_im += gT10_im * a0_re;
 A1_im += gT11_re * a1_im;
 A1_im += gT11_im * a1_re;
 A1_im += gT12_re * a2_im;
 A1_im += gT12_im * a2_re;
 spinorFloat B1_re = 0;
 B1_re += gT10_re * b0_re;
 B1_re -= gT10_im * b0_im;
 B1_re += gT11_re * b1_re;
 B1_re -= gT11_im * b1_im;
 B1_re += gT12_re * b2_re;
 B1_re -= gT12_im * b2_im;
 spinorFloat B1_im = 0;
 B1_im += gT10_re * b0_im;
 B1_im += gT10_im * b0_re;
 B1_im += gT11_re * b1_im;
 B1_im += gT11_im * b1_re;
 B1_im += gT12_re * b2_im;
 B1_im += gT12_im * b2_re;

 // multiply row 2
 spinorFloat A2_re = 0;
 A2_re += gT20_re * a0_re;
 A2_re -= gT20_im * a0_im;
 A2_re += gT21_re * a1_re;
 A2_re -= gT21_im * a1_im;
 A2_re += gT22_re * a2_re;
 A2_re -= gT22_im * a2_im;
 spinorFloat A2_im = 0;
 A2_im += gT20_re * a0_im;
 A2_im += gT20_im * a0_re;
 A2_im += gT21_re * a1_im;
 A2_im += gT21_im * a1_re;
 A2_im += gT22_re * a2_im;
 A2_im += gT22_im * a2_re;
 spinorFloat B2_re = 0;
 B2_re += gT20_re * b0_re;
 B2_re -= gT20_im * b0_im;
 B2_re += gT21_re * b1_re;
 B2_re -= gT21_im * b1_im;
 B2_re += gT22_re * b2_re;
 B2_re -= gT22_im * b2_im;
 spinorFloat B2_im = 0;
 B2_im += gT20_re * b0_im;
 B2_im += gT20_im * b0_re;
 B2_im += gT21_re * b1_im;
 B2_im += gT21_im * b1_re;
 B2_im += gT22_re * b2_im;
 B2_im += gT22_im * b2_re;

 o00_re += A0_re;
 o00_im += A0_im;
 o10_re += B0_re;
 o10_im += B0_im;
 o20_re += A0_im;
 o20_im -= A0_re;
 o30_re -= B0_im;
 o30_im += B0_re;

 o01_re += A1_re;
 o01_im += A1_im;
 o11_re += B1_re;
 o11_im += B1_im;
 o21_re += A1_im;
 o21_im -= A1_re;
 o31_re -= B1_im;
 o31_im += B1_re;

 o02_re += A2_re;
 o02_im += A2_im;
 o12_re += B2_re;
 o12_im += B2_im;
 o22_re += A2_im;
 o22_im -= A2_re;
 o32_re -= B2_im;
 o32_im += B2_re;
}

#ifdef MULTI_GPU
if ( (kernel_type == INTERIOR_KERNEL && (!param.ghostDim[3] || coord[3]<(param.X[3]-1))) ||
 (kernel_type == EXTERIOR_KERNEL_T && coord[3]==(param.X[3]-1)) )
#endif
{
 // Projector P3-
 //  0  0  0  0
 //  0  0  0  0
 //  0  0  2  0
 //  0  0  0  2

#ifdef MULTI_GPU
 const int sp_idx = (kernel_type == INTERIOR_KERNEL) ? (coord[3]==(param.X[3]-1) ? X-param.X4X3X2X1mX3X2X1 : X+param.X3X2X1) >> 1 :
  face_idx + param.ghostOffset[static_cast<int>(kernel_type)][1];
#if (DD_PREC==2) // half precision
 const int sp_norm_idx = face_idx + param.ghostNormOffset[static_cast<int>(kernel_type)][1];
#endif
#else
 const int sp_idx = (coord[3]==(param.X[3]-1) ? X-param.X4X3X2X1mX3X2X1 : X+param.X3X2X1) >> 1;
#endif

 const int ga_idx = sid % param.volume4CB;

 if (param.gauge_fixed && ga_idx < param.X4X3X2X1hmX3X2X1h)
 {
  spinorFloat a0_re, a0_im;
  spinorFloat a1_re, a1_im;
  spinorFloat a2_re, a2_im;
  spinorFloat b0_re, b0_im;
  spinorFloat b1_re, b1_im;
  spinorFloat b2_re, b2_im;

#ifdef MULTI_GPU
  if (kernel_type == INTERIOR_KERNEL) {
#endif

   // read spinor from device memory
   READ_SPINOR_DOWN(SPINORTEX, param.sp_stride, sp_idx, sp_idx);

   // project spinor into half spinors
   a0_re = +2*i20_re;
   a0_im = +2*i20_im;
   a1_re = +2*i21_re;
   a1_im = +2*i21_im;
   a2_re = +2*i22_re;
   a2_im = +2*i22_im;
   b0_re = +2*i30_re;
   b0_im = +2*i30_im;
   b1_re = +2*i31_re;
   b1_im = +2*i31_im;
   b2_re = +2*i32_re;
   b2_im = +2*i32_im;

#ifdef MULTI_GPU
  } else {

   const int sp_stride_pad = param.Ls*param.ghostFace[static_cast<int>(kernel_type)];
   const int t_proj_scale = TPROJSCALE;

   // read half spinor from device memory
   READ_SPINOR_GHOST(GHOSTSPINORTEX, sp_stride_pad, sp_idx, sp_norm_idx, 6);

   a0_re = t_proj_scale*i00_re; a0_im = t_proj_scale*i00_im;
   a1_re = t_proj_scale*i01_re; a1_im = t_proj_scale*i01_im;
   a2_re = t_proj_scale*i02_re; a2_im = t_proj_scale*i02_im;
   b0_re = t_proj_scale*i10_re; b0_im = t_proj_scale*i10_im;
   b1_re = t_proj_scale*i11_re; b1_im = t_proj_scale*i11_im;
   b2_re = t_proj_scale*i12_re; b2_im = t_proj_scale*i12_im;

  }
#endif // MULTI_GPU

  // identity gauge matrix
  spinorFloat A0_re = a0_re; spinorFloat A0_im = a0_im;
  spinorFloat B0_re = b0_re; spinorFloat B0_im = b0_im;
  spinorFloat A1_re = a1_re; spinorFloat A1_im = a1_im;
  spinorFloat B1_re = b1_re; spinorFloat B1_im = b1_im;
  spinorFloat A2_re = a2_re; spinorFloat A2_im = a2_im;
  spinorFloat B2_re = b2_re; spinorFloat B2_im = b2_im;

  o20_re += A0_re;
  o20_im += A0_im;
  o30_re += B0_re;
  o30_im += B0_im;

  o21_re += A1_re;
  o21_im += A1_im;
  o31_re += B1_re;
  o31_im += B1_im;

  o22_re += A2_re;
  o22_im += A2_im;
  o32_re += B2_re;
  o32_im += B2_im;
 } else {
  // read gauge matrix from device memory
  ASSN_GAUGE_MATRIX(G, GAUGE0TEX, 6, ga_idx, param.gauge_stride);

  spinorFloat a0_re, a0_im;
  spinorFloat a1_re, a1_im;
  spinorFloat a2_re, a2_im;
  spinorFloat b0_re, b0_im;
  spinorFloat b1_re, b1_im;
  spinorFloat b2_re, b2_im;

#ifdef MULTI_GPU
  if (kernel_type == INTERIOR_KERNEL) {
#endif

   // read spinor from device memory
   READ_SPINOR_DOWN(SPINORTEX, param.sp_stride, sp_idx, sp_idx);

   // project spinor into half spinors
   a0_re = +2*i20_re;
   a0_im = +2*i20_im;
   a1_re = +2*i21_re;
   a1_im = +2*i21_im;
   a2_re = +2*i22_re;
   a2_im = +2*i22_im;
   b0_re = +2*i30_re;
   b0_im = +2*i30_im;
   b1_re = +2*i31_re;
   b1_im = +2*i31_im;
   b2_re = +2*i32_re;
   b2_im = +2*i32_im;

#ifdef MULTI_GPU
  } else {

   const int sp_stride_pad = param.Ls*param.ghostFace[static_cast<int>(kernel_type)];
   const int t_proj_scale = TPROJSCALE;

   // read half spinor from device memory
   READ_SPINOR_GHOST(GHOSTSPINORTEX, sp_stride_pad, sp_idx, sp_norm_idx, 6);

   a0_re = t_proj_scale*i00_re; a0_im = t_proj_scale*i00_im;
   a1_re = t_proj_scale*i01_re; a1_im = t_proj_scale*i01_im;
   a2_re = t_proj_scale*i02_re; a2_im = t_proj_scale*i02_im;
   b0_re = t_proj_scale*i10_re; b0_im = t_proj_scale*i10_im;
   b1_re = t_proj_scale*i11_re; b1_im = t_proj_scale*i11_im;
   b2_re = t_proj_scale*i12_re; b2_im = t_proj_scale*i12_im;

  }
#endif // MULTI_GPU

  // reconstruct gauge matrix
  RECONSTRUCT_GAUGE_MATRIX(6);

  // multiply row 0
  spinorFloat A0_re = 0;
  A0_re += g00_re * a0_re;
  A0_re -= g00_im * a0_im;
  A0_re += g01_re * a1_re;
  A0_re -= g01_im * a1_im;
  A0_re += g02_re * a2_re;
  A0_re -= g02_im * a2_im;
  spinorFloat A0_im = 0;
  A0_im += g00_re * a0_im;
  A0_im += g00_im * a0_re;
  A0_im += g01_re * a1_im;
  A0_im += g01_im * a1_re;
  A0_im += g02_re * a2_im;
  A0_im += g02_im * a2_re;
  spinorFloat B0_re = 0;
  B0_re += g00_re * b0_re;
  B0_re -= g00_im * b0_im;
  B0_re += g01_re * b1_re;
  B0_re -= g01_im * b1_im;
  B0_re += g02_re * b2_re;
  B0_re -= g02_im * b2_im;
  spinorFloat B0_im = 0;
  B0_im += g00_re * b0_im;
  B0_im += g00_im * b0_re;
  B0_im += g01_re * b1_im;
  B0_im += g01_im * b1_re;
  B0_im += g02_re * b2_im;
  B0_im += g02_im * b2_re;

  // multiply row 1
  spinorFloat A1_re = 0;
  A1_re += g10_re * a0_re;
  A1_re -= g10_im * a0_im;
  A1_re += g11_re * a1_re;
  A1_re -= g11_im * a1_im;
  A1_re += g12_re * a2_re;
  A1_re -= g12_im * a2_im;
  spinorFloat A1_im = 0;
  A1_im += g10_re * a0_im;
  A1_im += g10_im * a0_re;
  A1_im += g11_re * a1_im;
  A1_im += g11_im * a1_re;
  A1_im += g12_re * a2_im;
  A1_im += g12_im * a2_re;
  spinorFloat B1_re = 0;
  B1_re += g10_re * b0_re;
  B1_re -= g10_im * b0_im;
  B1_re += g11_re * b1_re;
  B1_re -= g11_im * b1_im;
  B1_re += g12_re * b2_re;
  B1_re -= g12_im * b2_im;
  spinorFloat B1_im = 0;
  B1_im += g10_re * b0_im;
  B1_im += g10_im * b0_re;
  B1_im += g11_re * b1_im;
  B1_im += g11_im * b1_re;
  B1_im += g12_re * b2_im;
  B1_im += g12_im * b2_re;

  // multiply row 2
  spinorFloat A2_re = 0;
  A2_re += g20_re * a0_re;
  A2_re -= g20_im * a0_im;
  A2_re += g21_re * a1_re;
  A2_re -= g21_im * a1_im;
  A2_re += g22_re * a2_re;
  A2_re -= g22_im * a2_im;
  spinorFloat A2_im = 0;
  A2_im += g20_re * a0_im;
  A2_im += g20_im * a0_re;
  A2_im += g21_re * a1_im;
  A2_im += g21_im * a1_re;
  A2_im += g22_re * a2_im;
  A2_im += g22_im * a2_re;
  spinorFloat B2_re = 0;
  B2_re += g20_re * b0_re;
  B2_re -= g20_im * b0_im;
  B2_re += g21_re * b1_re;
  B2_re -= g21_im * b1_im;
  B2_re += g22_re * b2_re;
  B2_re -= g22_im * b2_im;
  spinorFloat B2_im = 0;
  B2_im += g20_re * b0_im;
  B2_im += g20_im * b0_re;
  B2_im += g21_re * b1_im;
  B2_im += g21_im * b1_re;
  B2_im += g22_re * b2_im;
  B2_im += g22_im * b2_re;

  o20_re += A0_re;
  o20_im += A0_im;
  o30_re += B0_re;
  o30_im += B0_im;

  o21_re += A1_re;
  o21_im += A1_im;
  o31_re += B1_re;
  o31_im += B1_im;

  o22_re += A2_re;
  o22_im += A2_im;
  o32_re += B2_re;
  o32_im += B2_im;
 }
}

#ifdef MULTI_GPU
if ( (kernel_type == INTERIOR_KERNEL && (!param.ghostDim[3] || coord[3]>0)) ||
 (kernel_type == EXTERIOR_KERNEL_T && coord[3]==0) )
#endif
{
 // Projector P3+
 //  2  0  0  0
 //  0  2  0  0
 //  0  0  0  0
 //  0  0  0  0

#ifdef MULTI_GPU
 const int sp_idx = (kernel_type == INTERIOR_KERNEL) ? (coord[3]==0 ? X+param.X4X3X2X1mX3X2X1 : X-param.X3X2X1) >> 1 :
  face_idx + param.ghostOffset[static_cast<int>(kernel_type)][0];
#if (DD_PREC==2) // half precision
 const int sp_norm_idx = face_idx + param.ghostNormOffset[static_cast<int>(kernel_type)][0];
#endif
#else
 const int sp_idx = (coord[3]==0 ? X+param.X4X3X2X1mX3X2X1 : X-param.X3X2X1) >> 1;
#endif

#ifdef MULTI_GPU
 const int ga_idx = ((kernel_type == INTERIOR_KERNEL) ? sp_idx % param.volume4CB : param.volume4CB+(face_idx % param.ghostFace[static_cast<int>(kernel_type)]));
#else
 const int ga_idx = sp_idx % param.volume4CB;
#endif

 if (param.gauge_fixed && ga_idx < param.X4X3X2X1hmX3X2X1h)
 {
  spinorFloat a0_re, a0_im;
  spinorFloat a1_re, a1_im;
  spinorFloat a2_re, a2_im;
  spinorFloat b0_re, b0_im;
  spinorFloat b1_re, b1_im;
  spinorFloat b2_re, b2_im;

#ifdef MULTI_GPU
  if (kernel_type == INTERIOR_KERNEL) {
#endif

   // read spinor from device memory
   READ_SPINOR_UP(SPINORTEX, param.sp_stride, sp_idx, sp_idx);

   // project spinor into half spinors
   a0_re = +2*i00_re;
   a0_im = +2*i00_im;
   a1_re = +2*i01_re;
   a1_im = +2*i01_im;
   a2_re = +2*i02_re;
   a2_im = +2*i02_im;
   b0_re = +2*i10_re;
   b0_im = +2*i10_im;
   b1_re = +2*i11_re;
   b1_im = +2*i11_im;
   b2_re = +2*i12_re;
   b2_im = +2*i12_im;

#ifdef MULTI_GPU
  } else {

   const int sp_stride_pad = param.Ls*param.ghostFace[static_cast<int>(kernel_type)];
   const int t_proj_scale = TPROJSCALE;

   // read half spinor from device memory
   READ_SPINOR_GHOST(GHOSTSPINORTEX, sp_stride_pad, sp_idx, sp_norm_idx, 7);

   a0_re = t_proj_scale*i00_re; a0_im = t_proj_scale*i00_im;
   a1_re = t_proj_scale*i01_re; a1_im = t_proj_scale*i01_im;
   a2_re = t_proj_scale*i02_re; a2_im = t_proj_scale*i02_im;
   b0_re = t_proj_scale*i10_re; b0_im = t_proj_scale*i10_im;
   b1_re = t_proj_scale*i11_re; b1_im = t_proj_scale*i11_im;
   b2_re = t_proj_scale*i12_re; b2_im = t_proj_scale*i12_im;

  }
#endif // MULTI_GPU

  // identity gauge matrix
  spinorFloat A0_re = a0_re; spinorFloat A0_im = a0_im;
  spinorFloat B0_re = b0_re; spinorFloat B0_im = b0_im;
  spinorFloat A1_re = a1_re; spinorFloat A1_im = a1_im;
  spinorFloat B1_re = b1_re; spinorFloat B1_im = b1_im;
  spinorFloat A2_re = a2_re; spinorFloat A2_im = a2_im;
  spinorFloat B2_re = b2_re; spinorFloat B2_im = b2_im;

  o00_re += A0_re;
  o00_im += A0_im;
  o10_re += B0_re;
  o10_im += B0_im;

  o01_re += A1_re;
  o01_im += A1_im;
  o11_re += B1_re;
  o11_im += B1_im;

  o02_re += A2_re;
  o02_im += A2_im;
  o12_re += B2_re;
  o12_im += B2_im;
 } else {
  // read gauge matrix from device memory
  ASSN_GAUGE_MATRIX(G, GAUGE1TEX, 7, ga_idx, param.gauge_stride);

  spinorFloat a0_re, a0_im;
  spinorFloat a1_re, a1_im;
  spinorFloat a2_re, a2_im;
  spinorFloat b0_re, b0_im;
  spinorFloat b1_re, b1_im;
  spinorFloat b2_re, b2_im;

#ifdef MULTI_GPU
  if (kernel_type == INTERIOR_KERNEL) {
#endif

   // read spinor from device memory
   READ_SPINOR_UP(SPINORTEX, param.sp_stride, sp_idx, sp_idx);

   // project spinor into half spinors
   a0_re = +2*i00_re;
   a0_im = +2*i00_im;
   a1_re = +2*i01_re;
   a1_im = +2*i01_im;
   a2_re = +2*i02_re;
   a2_im = +2*i02_im;
   b0_re = +2*i10_re;
   b0_im = +2*i10_im;
   b1_re = +2*i11_re;
   b1_im = +2*i11_im;
   b2_re = +2*i12_re;
   b2_im = +2*i12_im;

#ifdef MULTI_GPU
  } else {

   const int sp_stride_pad = param.Ls*param.ghostFace[static_cast<int>(kernel_type)];
   const int t_proj_scale = TPROJSCALE;

   // read half spinor from device memory
   READ_SPINOR_GHOST(GHOSTSPINORTEX, sp_stride_pad, sp_idx, sp_norm_idx, 7);

   a0_re = t_proj_scale*i00_re; a0_im = t_proj_scale*i00_im;
   a1_re = t_proj_scale*i01_re; a1_im = t_proj_scale*i01_im;
   a2_re = t_proj_scale*i02_re; a2_im = t_proj_scale*i02_im;
   b0_re = t_proj_scale*i10_re; b0_im = t_proj_scale*i10_im;
   b1_re = t_proj_scale*i11_re; b1_im = t_proj_scale*i11_im;
   b2_re = t_proj_scale*i12_re; b2_im = t_proj_scale*i12_im;

  }
#endif // MULTI_GPU

  // reconstruct gauge matrix
  RECONSTRUCT_GAUGE_MATRIX(7);

  // multiply row 0
  spinorFloat A0_re = 0;
  A0_re += gT00_re * a0_re;
  A0_re -= gT00_im * a0_im;
  A0_re += gT01_re * a1_re;
  A0_re -= gT01_im * a1_im;
  A0_re += gT02_re * a2_re;
  A0_re -= gT02_im * a2_im;
  spinorFloat A0_im = 0;
  A0_im += gT00_re * a0_im;
  A0_im += gT00_im * a0_re;
  A0_im += gT01_re * a1_im;
  A0_im += gT01_im * a1_re;
  A0_im += gT02_re * a2_im;
  A0_im += gT02_im * a2_re;
  spinorFloat B0_re = 0;
  B0_re += gT00_re * b0_re;
  B0_re -= gT00_im * b0_im;
  B0_re += gT01_re * b1_re;
  B0_re -= gT01_im * b1_im;
  B0_re += gT02_re * b2_re;
  B0_re -= gT02_im * b2_im;
  spinorFloat B0_im = 0;
  B0_im += gT00_re * b0_im;
  B0_im += gT00_im * b0_re;
  B0_im += gT01_re * b1_im;
  B0_im += gT01_im * b1_re;
  B0_im += gT02_re * b2_im;
  B0_im += gT02_im * b2_re;

  // multiply row 1
  spinorFloat A1_re = 0;
  A1_re += gT10_re * a0_re;
  A1_re -= gT10_im * a0_im;
  A1_re += gT11_re * a1_re;
  A1_re -= gT11_im * a1_im;
  A1_re += gT12_re * a2_re;
  A1_re -= gT12_im * a2_im;
  spinorFloat A1_im = 0;
  A1_im += gT10_re * a0_im;
  A1_im += gT10_im * a0_re;
  A1_im += gT11_re * a1_im;
  A1_im += gT11_im * a1_re;
  A1_im += gT12_re * a2_im;
  A1_im += gT12_im * a2_re;
  spinorFloat B1_re = 0;
  B1_re += gT10_re * b0_re;
  B1_re -= gT10_im * b0_im;
  B1_re += gT11_re * b1_re;
  B1_re -= gT11_im * b1_im;
  B1_re += gT12_re * b2_re;
  B1_re -= gT12_im * b2_im;
  spinorFloat B1_im = 0;
  B1_im += gT10_re * b0_im;
  B1_im += gT10_im * b0_re;
  B1_im += gT11_re * b1_im;
  B1_im += gT11_im * b1_re;
  B1_im += gT12_re * b2_im;
  B1_im += gT12_im * b2_re;

  // multiply row 2
  spinorFloat A2_re = 0;
  A2_re += gT20_re * a0_re;
  A2_re -= gT20_im * a0_im;
  A2_re += gT21_re * a1_re;
  A2_re -= gT21_im * a1_im;
  A2_re += gT22_re * a2_re;
  A2_re -= gT22_im * a2_im;
  spinorFloat A2_im = 0;
  A2_im += gT20_re * a0_im;
  A2_im += gT20_im * a0_re;
  A2_im += gT21_re * a1_im;
  A2_im += gT21_im * a1_re;
  A2_im += gT22_re * a2_im;
  A2_im += gT22_im * a2_re;
  spinorFloat B2_re = 0;
  B2_re += gT20_re * b0_re;
  B2_re -= gT20_im * b0_im;
  B2_re += gT21_re * b1_re;
  B2_re -= gT21_im * b1_im;
  B2_re += gT22_re * b2_re;
  B2_re -= gT22_im * b2_im;
  spinorFloat B2_im = 0;
  B2_im += gT20_re * b0_im;
  B2_im += gT20_im * b0_re;
  B2_im += gT21_re * b1_im;
  B2_im += gT21_im * b1_re;
  B2_im += gT22_re * b2_im;
  B2_im += gT22_im * b2_re;

  o00_re += A0_re;
  o00_im += A0_im;
  o10_re += B0_re;
  o10_im += B0_im;

  o01_re += A1_re;
  o01_im += A1_im;
  o11_re += B1_re;
  o11_im += B1_im;

  o02_re += A2_re;
  o02_im += A2_im;
  o12_re += B2_re;
  o12_im += B2_im;
 }
}

#if defined MULTI_GPU && defined DSLASH_XPAY

int incomplete = 0; // Have all 8 contributions been computed for this site?

switch(kernel_type) { // intentional fall-through
case INTERIOR_KERNEL:
  incomplete = incomplete || (param.commDim[3] && (coord[3]==0 || coord[3]==(param.X[3]-1)));
case EXTERIOR_KERNEL_T:
  incomplete = incomplete || (param.commDim[2] && (coord[2]==0 || coord[2]==(param.X[2]-1)));
case EXTERIOR_KERNEL_Z:
  incomplete = incomplete || (param.commDim[1] && (coord[1]==0 || coord[1]==(param.X[1]-1)));
case EXTERIOR_KERNEL_Y:
  incomplete = incomplete || (param.commDim[0] && (coord[0]==0 || coord[0]==(param.X[0]-1)));
}

if (!incomplete)
#endif // MULTI_GPU
{

#ifdef DSLASH_XPAY
 READ_ACCUM(ACCUMTEX, param.sp_stride)
 VOLATILE spinorFloat coeff;

#ifdef MDWF_mode
 coeff = static_cast<spinorFloat>(0.5)*a/(mdwf_b5[coord[4]]*(m5+static_cast<spinorFloat>(4.0)) + static_cast<spinorFloat>(1.0));
#else
 coeff = a;
#endif

#ifdef SPINOR_DOUBLE
 o00_re = coeff*o00_re + accum0.x;
 o00_im = coeff*o00_im + accum0.y;
 o01_re = coeff*o01_re + accum1.x;
 o01_im = coeff*o01_im + accum1.y;
 o02_re = coeff*o02_re + accum2.x;
 o02_im = coeff*o02_im + accum2.y;
 o10_re = coeff*o10_re + accum3.x;
 o10_im = coeff*o10_im + accum3.y;
 o11_re = coeff*o11_re + accum4.x;
 o11_im = coeff*o11_im + accum4.y;
 o12_re = coeff*o12_re + accum5.x;
 o12_im = coeff*o12_im + accum5.y;
 o20_re = coeff*o20_re + accum6.x;
 o20_im = coeff*o20_im + accum6.y;
 o21_re = coeff*o21_re + accum7.x;
 o21_im = coeff*o21_im + accum7.y;
 o22_re = coeff*o22_re + accum8.x;
 o22_im = coeff*o22_im + accum8.y;
 o30_re = coeff*o30_re + accum9.x;
 o30_im = coeff*o30_im + accum9.y;
 o31_re = coeff*o31_re + accum10.x;
 o31_im = coeff*o31_im + accum10.y;
 o32_re = coeff*o32_re + accum11.x;
 o32_im = coeff*o32_im + accum11.y;
#else
 o00_re = coeff*o00_re + accum0.x;
 o00_im = coeff*o00_im + accum0.y;
 o01_re = coeff*o01_re + accum0.z;
 o01_im = coeff*o01_im + accum0.w;
 o02_re = coeff*o02_re + accum1.x;
 o02_im = coeff*o02_im + accum1.y;
 o10_re = coeff*o10_re + accum1.z;
 o10_im = coeff*o10_im + accum1.w;
 o11_re = coeff*o11_re + accum2.x;
 o11_im = coeff*o11_im + accum2.y;
 o12_re = coeff*o12_re + accum2.z;
 o12_im = coeff*o12_im + accum2.w;
 o20_re = coeff*o20_re + accum3.x;
 o20_im = coeff*o20_im + accum3.y;
 o21_re = coeff*o21_re + accum3.z;
 o21_im = coeff*o21_im + accum3.w;
 o22_re = coeff*o22_re + accum4.x;
 o22_im = coeff*o22_im + accum4.y;
 o30_re = coeff*o30_re + accum4.z;
 o30_im = coeff*o30_im + accum4.w;
 o31_re = coeff*o31_re + accum5.x;
 o31_im = coeff*o31_im + accum5.y;
 o32_re = coeff*o32_re + accum5.z;
 o32_im = coeff*o32_im + accum5.w;
#endif // SPINOR_DOUBLE
#endif // DSLASH_XPAY
}

// write spinor field back to device memory
WRITE_SPINOR(param.sp_stride);

// undefine to prevent warning when precision is changed
#undef m5
#undef mdwf_b5
#undef mdwf_c5
#undef mferm
#undef a
#undef b
#undef spinorFloat
#undef POW
#undef SHARED_STRIDE

#undef g00_re
#undef g00_im
#undef g01_re
#undef g01_im
#undef g02_re
#undef g02_im
#undef g10_re
#undef g10_im
#undef g11_re
#undef g11_im
#undef g12_re
#undef g12_im
#undef g20_re
#undef g20_im
#undef g21_re
#undef g21_im
#undef g22_re
#undef g22_im

#undef i00_re
#undef i00_im
#undef i01_re
#undef i01_im
#undef i02_re
#undef i02_im
#undef i10_re
#undef i10_im
#undef i11_re
#undef i11_im
#undef i12_re
#undef i12_im
#undef i20_re
#undef i20_im
#undef i21_re
#undef i21_im
#undef i22_re
#undef i22_im
#undef i30_re
#undef i30_im
#undef i31_re
#undef i31_im
#undef i32_re
#undef i32_im



#undef VOLATILE
