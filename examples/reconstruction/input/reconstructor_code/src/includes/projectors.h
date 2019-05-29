
#ifndef _PROJECTORS_
#define _PROJECTORS_

#include <stdio.h>
#include <stdlib.h>
#include <cmath>

struct LORstruct{
  float in[3],out[3]; //Points that create the LOR
  double tin,tout;  //timestamps
  float w; //geometric factor
};

class projector{

 protected:
  // workspace
  static const char* pname;
  int nVoxels[4];
  int nvox;
  double voxelSize[3];
  double voxelsOrigin[3];

  int tbins;
  float dtbins;
  float* movMatrix;
  float cameraOrigin[3];

  bool geometryCreated;
  bool enableMov;

  bool initialized;
  
 public:

  projector();

  //Shared projector's functions
  
  int setWorkspace(int, int, int, float, float, float,
		   int, float, float*, float*);

  int movCorrection(float*, float*, float);

  unsigned project(float TOFres, unsigned long* voxIndexes, double* voxWeights, LORstruct& lor, double& l);
  
  // Virtual functions 
  
  virtual int init(){initialized = true; return 0;}

  virtual int LOR(float*, float*, unsigned long*,
		  double*, unsigned*, double*) const = 0;
  
  virtual int LORTOF(float* in,
		     float* out,
		     double t1,
		     double t2,
		     float timeResolution,
		     unsigned long *voxIndexes,
		     double *voxWeight,
		     unsigned *voxNumber,
		     double *voxLenghts) = 0;

  
  
  virtual void clean(){initialized = false;}

  virtual const char* name() const {return pname;}
 
  virtual ~projector(){};
 
};

class jacobs : public projector{

 private:
  static const char* pname;
  //Jacobs variables
  int Nx;
  int Ny;
  int Nz;  
  
  float* Xplane;
  float* Yplane;
  float* Zplane;


 public:

  jacobs();
  int init();
  
  int LOR(float*, float*, unsigned long*,
	  double*, unsigned*, double*) const;

  int LORTOF(float* in,
	     float* out,
	     double t1,
	     double t2,
	     float timeResolution,
	     unsigned long *voxIndexes,
	     double *voxWeight,
	     unsigned *voxNumber,
	     double *voxLenghts);

  int LORGauss(float* in,
	       float* out,
	       unsigned long *voxIndexes,
	       float sigma,
	       float mu,
	       double *voxWeight,
	       unsigned *voxNumber,
	       double *voxLenghts) const;
  
  void clean();
  const char* name() const {return pname;}
  ~jacobs();
};

//Auxiliar functions


void imageNormalize(double*, int*, double*);

void euler2quaternion(double*, double, double*, double, double*, double, double*);

int createMovMatrix(double*, float*&, int, float*);

template <class T> void Qprod(T* q1, T* q2, T* qout)
{
  // Perform the Hamilton quaternion product q1*q2 and
  // store the resulting quaternion in qout
  //
  //          The format assumef for
  //          quaternions is as follows,
  //
  //          b i + c j + d k + a
  //
  //          with scalar part at last position
  
  qout[0] =  q1[3]*q2[0] + q1[0]*q2[3] + q1[1]*q2[2] - q1[2]*q2[1];
  qout[1] =  q1[3]*q2[1] - q1[0]*q2[2] + q1[1]*q2[3] + q1[2]*q2[0];
  qout[2] =  q1[3]*q2[2] + q1[0]*q2[1] - q1[1]*q2[0] + q1[2]*q2[3];

  qout[3] =  q1[3]*q2[3] - q1[0]*q2[0] - q1[1]*q2[1] - q1[2]*q2[2];
}

template <class T1, class T2> void rotateQV(T1* q, T2*v)
{
  //Rotate the vector (3D) 'v' using the rotation specified
  //by quaternion 'q'.
  //
  //          The format assumef for
  //          quaternions is as follows,
  //
  //          b i + c j + d k + a
  //
  //          with scalar part at last position
  
  // v' = qvq^-1

  T1 iq[4] = {-q[0],-q[1],-q[2],q[3]}; 
  
  //Perform qv:

  T1 qp[4];

  qp[0] =  q[3]*v[0]             + q[1]*v[2] - q[2]*v[1];
  qp[1] =  q[3]*v[1] - q[0]*v[2]             + q[2]*v[0];
  qp[2] =  q[3]*v[2] + q[0]*v[1] - q[1]*v[0]            ;

  qp[3] =            - q[0]*v[0] - q[1]*v[1] - q[2]*v[2];

  //Perform (qv)q^-1:

  v[0] =  qp[3]*iq[0] + qp[0]*iq[3] + qp[1]*iq[2] - qp[2]*iq[1];
  v[1] =  qp[3]*iq[1] - qp[0]*iq[2] + qp[1]*iq[3] + qp[2]*iq[0];
  v[2] =  qp[3]*iq[2] + qp[0]*iq[1] - qp[1]*iq[0] + qp[2]*iq[3];
  
}

int printMatrixXY(FILE* file, int* nVoxels, double* voxelSize, double* voxelIn);

int printMatrixYZ(FILE* file, int* nVoxels, double* voxelSize, double* voxelIn);

int printMatrixXZ(FILE* file, int* nVoxels, double* voxelSize, double* voxelIn);

#endif
