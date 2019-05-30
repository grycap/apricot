 
#ifndef __RECONSTRUCTION__
#define __RECONSTRUCTION__

#include <cmath>
#include "input.h"

class convolution{
 public:
  
  void CPUconvolute(const double *in, const int nx, const int ny, const int nz, const double *kernel, const int dim, double *out);

  void convolute(const double* in, double* out, const int* nvox, const double* kernel = 0, const int dim = 0);
  
};

void gaussianKernel(int, float, double*);
float gauss3D(float, float, float, float, float);
void BPiFP(const unsigned long*, const double*, int, double*, const double*);

void nextImage(double* image, const double* s, const double* c, const unsigned nvox);

int sensIdentity(int*, double*&);
int sensitivity(int* nvox, float TOFres, const char* filename, const int LORformat, double*& s, projector* pProj, convolution& convObj, const double* h = 0, const int dim = 0);

#endif
