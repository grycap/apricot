 
#include "projectors.h"

//*******************//
//* projector class *//
//*******************//

const char* projector::pname = "dummy";

projector::projector()
{
  geometryCreated = false;
  enableMov = false;
  initialized = false;
  movMatrix = 0;
}

int projector::setWorkspace(int nx, int ny, int nz, float xS, float yS, float zS,
		  int nt, float tS, float* movement, float* camera)
{

  // setWorkspace init 3D projection voxel matrix parameters

  //  INPUT:
  //        nx,ny,nz -> number of voxels in each axi (x,y,z)
  //        xS,yS,zS -> voxel sides sizes (dx,dy,dz)
  //        nt -> number of temporal bins
  //        tS -> width of each temporal bin (s)
  //        movement -> pointer to matrix that stores movement information.
  //                    The format of this matrix is as follows,
  //                    (dx,dy,dz,alpha,beta,gamma) (all floats)
  //                    where dx, dy, dz are the relative deviation in
  //                    each axis and alpha, beta, gamma are the relative
  //                    rotation angles in the z, y and z axis respectively
  // 

  geometryCreated = false;
  enableMov = false;
  
  //Store number of pixels in each axis
  nVoxels[0] = nx;
  nVoxels[1] = ny;
  nVoxels[2] = nz;

  if(nVoxels[0] <= 0 || nVoxels[1] <= 0 || nVoxels[2] <= 0)
    {
      return -1;
    }
  //Precalculate the number of voxels in a Z plane and total number of voxels.
  nVoxels[3] = nVoxels[0]*nVoxels[1];
  nvox = nVoxels[3]*nVoxels[2];
  //Store voxel size dx,dy,dz
  voxelSize[0] = xS;
  voxelSize[1] = yS;
  voxelSize[2] = zS;

  if(voxelSize[0] <= 0 || voxelSize[1] <= 0 || voxelSize[2] <= 0)
    {
      return -2;
    }

  //Precalculate some values
  double halfFOVSize[3];
  halfFOVSize[0] = nVoxels[0]*voxelSize[0] / 2.0;
  halfFOVSize[1] = nVoxels[1]*voxelSize[1] / 2.0;
  halfFOVSize[2] = nVoxels[2]*voxelSize[2] / 2.0;

  voxelsOrigin[0] = -halfFOVSize[0];
  voxelsOrigin[1] = -halfFOVSize[1];
  voxelsOrigin[2] = -halfFOVSize[2];

  geometryCreated = true;
  
  //************//
  //* Movement *//
  //************//

  tbins = nt;
  dtbins = tS;
  movMatrix = movement;

  if(tbins < 0 || tS <= 0 || movMatrix == 0)
    {
      return 1;
    }

  cameraOrigin[0] = camera[0];
  cameraOrigin[1] = camera[1];
  cameraOrigin[2] = camera[2];

  enableMov = true;
  return 0;
}

int projector::movCorrection(float* in, float* out, float timestamp)
{
  //Correct the input LOR specified by 'in' and 'out' using the
  //provided object moviment information in 'movMatrix'.
  //
  //  input:
  //    in     -> (x,y,z) of first LOR point
  //    out    -> (x,y,z) of second LOR point
  // timestamp -> LOR mesured timestamp
  //
  // return -1 if moviment is not enabled
  // return  0 on succes
  //

  if(!enableMov){return -1;}
  
  //Calculate time bin
  int bin =  int(timestamp/dtbins);
  int fpos = bin*7;

  //Extract moviment information
  float translation[3] = {movMatrix[fpos],movMatrix[fpos+1],movMatrix[fpos+2]};
  
  //Extract rotation quaternion
  float qrot[4] = {
    movMatrix[fpos+3], movMatrix[fpos+4] , movMatrix[fpos+5] , movMatrix[fpos+6]
  };

  //Apply point translation
  in[0] -= translation[0];
  in[1] -= translation[1];
  in[2] -= translation[2];

  out[0] -= translation[0];
  out[1] -= translation[1];
  out[2] -= translation[2];
  
  //Apply rotation considering the camera origin
  in[0] -= cameraOrigin[0];
  in[1] -= cameraOrigin[1];
  in[2] -= cameraOrigin[2];

  out[0] -= cameraOrigin[0];
  out[1] -= cameraOrigin[1];
  out[2] -= cameraOrigin[2];

  rotateQV(qrot, in);
  rotateQV(qrot,out);

  //Translate back
  in[0] += cameraOrigin[0];
  in[1] += cameraOrigin[1];
  in[2] += cameraOrigin[2];

  out[0] += cameraOrigin[0];
  out[1] += cameraOrigin[1];
  out[2] += cameraOrigin[2];
  
  return 0;
}

unsigned projector::project(float TOFres, unsigned long* voxIndexes, double* voxWeights, LORstruct& lor, double& l)
{
  ////////////////////////////////////////////////////////////////////////////////////
  //                                                                                //
  //   array with voxels weights                                                    //
  //                                                                                //
  //   INPUT:                                                                       //
  //                                                                                //
  //                                                                                //
  //   OUTPUT                                                                       //   
  //         voxIndexes -> array with voxels indexes with non-zero weights          //
  //         voxWeights -> array with voxels acumulated weights (only non-zero)     //
  //         return number of crossed voxels                                        //
  //                                                                                //
  ////////////////////////////////////////////////////////////////////////////////////
    
    
  //For each voxel we sum LOR's weights that crossed this voxel

  //Apply moviment corrections
  movCorrection(lor.in,lor.out,lor.tin);	  
  
  //Perform the calculus of sum(w*x) of all lors for each voxel
  unsigned nCrossed = 0;
  l = 0.0;
  if(TOFres < 0.0)
    {
      //TOF disabled	
      //Calculate the projection
      LOR(lor.in, lor.out,
	  voxIndexes, voxWeights, &nCrossed, &l);
    }
  else
    {
      //TOF enabled	
      //Calculate the projection
      LORTOF(lor.in, lor.out,
		    lor.tin, lor.tout,
		    TOFres, voxIndexes, voxWeights, &nCrossed, &l);
    }

  //Normalize weights
  for(unsigned int k = 0; k < nCrossed; k++)
    {
      voxWeights[k] /= l;
    }
  
  return nCrossed;
}


//*******************//
//*   jacob class   *//
//*******************//

const char* jacobs::pname = "jacobs";

jacobs::jacobs()
{
  geometryCreated = false;
  enableMov = false;
  movMatrix = 0;
  
  Xplane = 0;
  Yplane = 0;
  Zplane = 0;
  initialized = 0;  
}

int jacobs::init()
{
  //Initialize jacob.

  if(!geometryCreated){return -1;}
  
  //Allocate memory for siddon method
  clean();   //Check for previous initialization

  //Reserve memory

  Nx = nVoxels[0] + 1;
  Ny = nVoxels[1] + 1;
  Nz = nVoxels[2] + 1;  
  
  Xplane = (float*)malloc(sizeof(float)*Nx);
  Yplane = (float*)malloc(sizeof(float)*Ny);
  Zplane = (float*)malloc(sizeof(float)*Nz);
  
  //Calculate X, Y, Z planes values
  for(int i = 0; i < Nx; i++)
    {
      Xplane[i] = voxelsOrigin[0] + i*voxelSize[0];
    }
  for(int i = 0; i < Ny; i++)
    {
      Yplane[i] = voxelsOrigin[1] + i*voxelSize[1];
    }
  for(int i = 0; i < Nz; i++)
    {
      Zplane[i] = voxelsOrigin[2] + i*voxelSize[2];
    }
  
  initialized = true;
  return 0;
}

void jacobs::clean()
{
  //Free dinamic memory
  
  if(Xplane != 0)
    free(Xplane);
  if(Yplane != 0)
    free(Yplane);
  if(Zplane != 0)
    free(Zplane);

  Xplane = 0;
  Yplane = 0;
  Zplane = 0;
  
  initialized = false;
}

int jacobs::LOR(float* in, float* out, unsigned long *voxIndexes,
	       double *voxWeight, unsigned *voxNumber, double *voxLenghts) const
{
  //////////////////////////////////////////////////////////////////
  //                                                              
  //  Add LOR contribution to voxels weights matrix (voxWeights) using siddon algorithm 
  //
  //  INPUT:
  //        in      -> Position of init LOR point (x,y,z).
  //        out     -> Position of end  LOR point (x,y,z).
  //        nVoxels -> Array with number of voxels in each axis (nx,ny,nz).
  //        voxSize -> 3 dimensional array with voxels size in each axis (dx,dy,dz).
  //
  //  OUTPUT:
  //       return 0 on succes
  //       voxIndexes -> Array with with voxels indexes with non zero weights.
  //       voxWeights -> Array with with voxels acumulated weights (only non zero weights).
  //       voxNumber  -> Number of voxels with non zero weights.
  //       voxLenghts -> Total voxel distance crossed by LOR (excludes out of FOV LOR segments) 
  //

  if(!geometryCreated || !initialized)
    return -1;
  
  int nxy = nVoxels[3];
  const double* voxSize = voxelSize;
  (*voxNumber) = 0;
  
  //Parametrize the line between point1 and point2
  // x = x1 + alpha*(x2-x1)
  // y = y1 + alpha*(y2-y1)
  // z = z1 + alpha*(z2-z1)
      
  double u[3] = {out[0] - in[0], out[1] - in[1], out[2] - in[2]};  //line vector

  double alphaMin;
  double alphaMax;

  double XalphaMin;
  double YalphaMin;
  double ZalphaMin;

  double XalphaMax;
  double YalphaMax;
  double ZalphaMax;
      
  //Calculate alpha values for first and last plane in each axis

  //X axis
  if(fabs(u[0]) > 1.0e-9)
    {
      double Xalpha1 = (Xplane[0]-in[0])/u[0];
      double XalphaN = (Xplane[Nx-1]-in[0])/u[0];

      if(Xalpha1 < XalphaN)
	{
	  XalphaMin = Xalpha1;
	  XalphaMax = XalphaN;
	}
      else
	{
	  XalphaMin = XalphaN;
	  XalphaMax = Xalpha1;	  
	}

    }
  else
    {
      XalphaMin = -1.0;
      XalphaMax =  2.0;
    }
      
  //Y axis
  if(fabs(u[1]) > 1.0e-9)
    {
      double Yalpha1 = (Yplane[0]-in[1])/u[1];
      double YalphaN = (Yplane[Ny-1]-in[1])/u[1];

      if(Yalpha1 < YalphaN)
	{
	  YalphaMin = Yalpha1;
	  YalphaMax = YalphaN;
	}
      else
	{
	  YalphaMin = YalphaN;
	  YalphaMax = Yalpha1;	  
	}	  
    }
  else
    {
      YalphaMin = -1.0;
      YalphaMax =  2.0;
    }

  //Z axis
  if(fabs(u[2]) > 1.0e-9)
    {      
      
      double Zalpha1 = (Zplane[0]-in[2])/u[2];
      double ZalphaN = (Zplane[Nz-1]-in[2])/u[2];

      if(Zalpha1 < ZalphaN)
	{
	  ZalphaMin = Zalpha1;
	  ZalphaMax = ZalphaN;
	}
      else
	{
	  ZalphaMin = ZalphaN;
	  ZalphaMax = Zalpha1;	  
	}
    }
  else
    {
      ZalphaMin = -1.0;
      ZalphaMax =  2.0;
    }      
      
  //Calculate alpha min and alpha max
  alphaMin = XalphaMin;
  if(alphaMin < YalphaMin){alphaMin = YalphaMin;}
  if(alphaMin < ZalphaMin){alphaMin = ZalphaMin;}
  if(alphaMin < 0.0)      {alphaMin = 0.0;      }

  alphaMax = XalphaMax;
  if(alphaMax > YalphaMax){alphaMax = YalphaMax;}
  if(alphaMax > ZalphaMax){alphaMax = ZalphaMax;}
  if(alphaMax > 1.0)      {alphaMax = 1.0;      }


  if(alphaMax < alphaMin)
    {
      return 1; // LOR out of FOV	  
    }

      
  //Calculate min and max index i,j,k for LOR n
  int nIntX = 0;
  int nIntY = 0;
  int nIntZ = 0;

  double alpha0[3] = {1.0e30,1.0e30,1.0e30};; // Stores first alfa values where LOR cuts first plane of each axis (x,y,z)  
  int indexU[3] = {0,0,0}; //Store index updates (x,y,z)

  int imin = 0, imax = 0, jmin = 0, jmax = 0, kmin = 0, kmax = 0;
  
  if(u[0] > 0.0)
    {
      imin = Nx - 1 - (int)((Xplane[Nx-1] - alphaMin*u[0] - in[0])/voxSize[0]);
      imax =  (int)((in[0] + alphaMax*u[0]-Xplane[0])/voxSize[0]);
	  
      imin = imin < 0 ? 0 : imin;
      imax = imax > Nx ? Nx-1 : imax;
      nIntX = imax - imin + 1;
	  
      alpha0[0] = (Xplane[imin]-in[0])/u[0];

      indexU[0] = 1;
    }
  else if(u[0] < 0.0)
    {
      imin = Nx - 1 - (int)((Xplane[Nx-1] - alphaMax*u[0] - in[0])/voxSize[0]);
      imax = (int)((in[0] + alphaMin*u[0]-Xplane[0])/voxSize[0]);

      imin = imin < 0 ? 0 : imin;
      imax = imax > Nx ? Nx-1 : imax;
      nIntX = imax - imin +1;

      alpha0[0] = (Xplane[imax]-in[0])/u[0];
	  
      indexU[0] = -1;
    }      
      
  if(u[1] > 0.0)
    {
      jmin = Ny - 1 - (int)((Yplane[Ny-1] - alphaMin*u[1] - in[1])/voxSize[1]);
      jmax = (int)((in[1] + alphaMax*u[1]-Yplane[0])/voxSize[1]);

      jmin = jmin < 0 ? 0 : jmin;
      jmax = jmax >= Ny ? Ny-1 : jmax;
      nIntY = jmax - jmin + 1;

      alpha0[1] = (Yplane[jmin]-in[1])/u[1];

      indexU[1] = 1;
    }
  else if(u[1] < 0.0)
    {
      jmin = Ny - 1 - (int)((Yplane[Ny-1] - alphaMax*u[1] - in[1])/voxSize[1]);
      jmax = (int)((in[1] + alphaMin*u[1]-Yplane[0])/voxSize[1]);

      jmin = jmin < 0 ? 0 : jmin;
      jmax = jmax >= Ny ? Ny-1 : jmax;
      nIntY = jmax - jmin + 1;

      alpha0[1] = (Yplane[jmax]-in[1])/u[1];
      
      indexU[1] = -1;
    }
      
      
  if(u[2] > 0.0)
    {
      kmin = Nz - 1 - (int)((Zplane[Nz-1] - alphaMin*u[2] - in[2])/voxSize[2]);
      kmax = (int)((in[2] + alphaMax*u[2]-Zplane[0])/voxSize[2]);
	  
      kmin = kmin < 0 ? 0 : kmin;
      kmax = kmax >= Nz ? Nz-1 : kmax;      
      nIntZ = kmax - kmin + 1;

      alpha0[2] = (Zplane[kmin]-in[2])/u[2];
      
      indexU[2] = 1;
    }
  else if(u[2] < 0.0)
    {
      kmin = Nz - 1 - (int)((Zplane[Nz-1] - alphaMax*u[2] - in[2])/voxSize[2]);
      kmax = (int)((in[2] + alphaMin*u[2]-Zplane[0])/voxSize[2]);

      kmin = kmin < 0 ? 0 : kmin;
      kmax = kmax >= Nz ? Nz-1 : kmax;      
      nIntZ = kmax - kmin + 1;

      alpha0[2] = (Zplane[kmax]-in[2])/u[2];

      indexU[2] = -1;
    }

  // Number of crossed planes
  int nAlphas = nIntX + nIntY + nIntZ;

  //Calculate alpha updates
  double alphaU[3];
  alphaU[0] = fabs(voxSize[0]/u[0]);
  alphaU[1] = fabs(voxSize[1]/u[1]);
  alphaU[2] = fabs(voxSize[2]/u[2]);
    
  double dLOR = sqrt(pow(u[0],2.0) + pow(u[1],2.0) + pow(u[2],2.0));
  double d12 = 0.0;

  // The program need care of if some alpha value is equal to 'alphaMin'.
  // If alpha min is equal to first crossed plane, we must
  // avoid calcule the step between first alpha and first
  // crossed plane (Because they are the same). So we have
  // one less alpha. This must be check in every axis.

  double alphaC = alphaMin;
  for(int m = 0; m < 3; m++)
    {
      if(alpha0[m]-alphaMin < 1.0e-9)
	{
	  //First alpha is equal to alpha min.
	  //Increment first alpha0 to avoid empty segment
	  alpha0[m] += alphaU[m];
	  nAlphas--;	  
	}
    }

  //Check first crosed axis
  int firstAlpha = alpha0[0] < alpha0[1] ? 0 : 1;
  firstAlpha = alpha0[firstAlpha] < alpha0[2] ? firstAlpha : 2;
  
  //Calculate first i,j,k indexes
  int i,j,k;
  i = (int)((in[0] + (alphaC+alpha0[firstAlpha])*0.5*u[0] - Xplane[0])/voxSize[0]);
  j = (int)((in[1] + (alphaC+alpha0[firstAlpha])*0.5*u[1] - Yplane[0])/voxSize[1]);
  k = (int)((in[2] + (alphaC+alpha0[firstAlpha])*0.5*u[2] - Zplane[0])/voxSize[2]);
  
  if(i < 0){i = 0;}
  else if(i >= nVoxels[0]){i = nVoxels[0]-1;}

  if(j < 0){j = 0;}
  else if(j >= nVoxels[1]){j = nVoxels[1]-1;}

  if(k < 0){k = 0;}
  else if(k >= nVoxels[2]){k = nVoxels[2]-1;}
  
  for(int m = 0; m < nAlphas; m++)
    {
      double l;
      int index = 0;
      
      if(alpha0[0] < alpha0[1] && alpha0[0] < alpha0[2])
	{
	  //Next plane crossed: X plane
	  l = (alpha0[0]-alphaC)*dLOR;
	  d12 += l;

	  //Calculate index
	  index = k*nxy + j*nVoxels[0] + i;
	  
	  i += indexU[0];
	  alphaC = alpha0[0];

	  //Avoid possible rounding errors. This is because,
	  //if some index values are on the FOV edges, taking
	  //one axis or other can cause a negative index (-1) or
	  //equal to 'nvox[]'. This only will be caused when the
	  //alpha0 values of two axis are aproximately equal and
	  //one of the index are on the border. 
	  if(i == -1 || i == nVoxels[0])
	    {
	      break;
	    }

	  alpha0[0] += alphaU[0];	      
	}
      else if(alpha0[1] < alpha0[2])
	{
	  //Next plane crossed: X plane
	  l = (alpha0[1]-alphaC)*dLOR;
	  d12 += l;

	  //Calculate index
	  index = k*nxy + j*nVoxels[0] + i;

	  j += indexU[1];
	  alphaC = alpha0[1];

	  //Avoid possible rounding errors. This is because,
	  //if some index values are on the FOV edges, taking
	  //one axis or other can cause a negative index (-1) or
	  //equal to 'nvox[]'. This only will be caused when the
	  //alpha0 values of two axis are aproximately equal and
	  //one of the index are on the border. 
	  if(j == -1 || j == nVoxels[1])
	    {
	      break;
	    }

	  alpha0[1] += alphaU[1];	      
	}
      else
	{
	  //Next plane crossed: X plane
	  l = (alpha0[2]-alphaC)*dLOR;
	  d12 += l;

	  //Calculate index
	  index = k*nxy + j*nVoxels[0] + i;
	  
	  k += indexU[2];
	  alphaC = alpha0[2];

	  //Avoid possible rounding errors. This is because,
	  //if some index values are on the FOV edges, taking
	  //one axis or other can cause a negative index (-1) or
	  //equal to 'nvox[]'. This only will be caused when the
	  //alpha0 values of two axis are aproximately equal and
	  //one of the index are on the border. 
	  if(k == -1 || k == nVoxels[2])
	    {
	      break;
	    }
	  
	  alpha0[2] += alphaU[2];	      
	  
	}
      
      //Each voxel only can be crossed one time, so add this index with no check
      voxIndexes[(*voxNumber)] = index;
      voxWeight[(*voxNumber)] = l;
      (*voxNumber)++;      
    }

  // Add the contribution from last alpha to alphaMax.
  // Take in account that no plane will be crossed, so
  // the voxel index will be the last used.

  // If the alpha max is on a plane, the voxel index will be
  // out of fov. So, check first the i,j,k indexes.

  if(i < 0 || i >= nVoxels[0] || j < 0 || j >= nVoxels[1] || k < 0 || k >= nVoxels[2])
    {
      //Alpha max is on border plane. Doesn't do any contribution.
    }
  else
    {
      //Add the contribution of alpha max
      double l = (alphaMax-alphaC)*dLOR;
      if(l > 1.0e-9)
	{
	  d12 += l;
	  
	  //Calculate index
	  int index = k*nxy + j*nVoxels[0] + i;
	  
	  //Each voxel only can be crossed one time, so add this index with no check
	  voxIndexes[(*voxNumber)] = index;
	  voxWeight[(*voxNumber)] = l;
	  (*voxNumber)++;
	}
    }
  
  (*voxLenghts) = d12;
  
  return 0;
}

//Function to obtain new shortened LOR with TOF information
int jacobs::LORTOF(float* in, float* out, double t1, double t2, float timeResolution, 
           unsigned long *voxIndexes, double *voxWeight, unsigned *voxNumber, double *voxLenghts)
{
  ////////////////////////////////////////////////////////////////////////////////////////////////
  //                                                                                            //
  //  Determine initial and final point of the LOR and sigma of the Gaussian kernel             //
  //  using time of flight (TOF)                                                                //
  //                                                                                            //
  //  INPUT:                                                                                    //
  //        inX,inY,inZ -> point of the first photon of the coincidence. (cm)                   //
  //        outX,outY,outZ -> point of the second photon of the coincidence. (cm)               //
  //        deltaTemp -> time difference in detection of the two photons in coincidence. (s)    //
  //        timeResolution -> time resolution of the system. (s)                                //  
  //                                                                                            //
  //                                                                                            //
  //  OUTPUT:                                                                                   //        
  //        return 0 on succes                                                                  //  
  //                                                                                            //
  ////////////////////////////////////////////////////////////////////////////////////////////////


   
    const double c=2.99792458E10;                             //Speed of light (cm/s) the same units that the input
    double deltaTemp=t2-t1;                                   //Time difference
    double d=deltaTemp*c/2.0;                                 //Distance of annihilation point from the center of LOR
    
    //The LOR from point 1 to point 2 may be represented parametrically as parameter alpha. 
    //Alpha is zero at point 1 and unity at point 2
    // x(alpha) = x1 + alpha(x2-x1),
    // y(alpha) = y1 + alpha(y2-y1),
    // z(alpha) = z1 + alpha(z2-z1),
    
    double u[3] = {out[0] - in[0], out[1] - in[1], out[2] - in[2]};       //Line vector of LOR
    double r=sqrt(pow(u[0],2)+pow(u[1],2)+pow(u[2],2));       //Distance from point 1 to point 2 (LOR length): module of u
    double deltaAlpha=d/r;                                    //Normalized distance of annihilation point from the center of LOR in terms of alpha
    double alpha = 0.5 - deltaAlpha;                          //Annihilation point in terms of alpha
    
    float sigmaNorm = (timeResolution*c/(2.0*2.35))/r;
    
    //Values for the limits of Gaussian distribution. 
    //Out of these alpha values ​​the probability of occurrence of annihilation point is assumed as zero 
    double alphaMin = alpha - 3.0*sigmaNorm;                 
    double alphaMax = alpha + 3.0*sigmaNorm;
 
    //Limits of the shortened LOR.
    float min[3], max[3];
    min[0] = in[0] + alphaMin*u[0];
    min[1] = in[1] + alphaMin*u[1];
    min[2] = in[2] + alphaMin*u[2];
    max[0] = in[0] + alphaMax*u[0];
    max[1] = in[1] + alphaMax*u[1];
    max[2] = in[2] + alphaMax*u[2];

    //printf("deltaT: %e  d: %e\n",deltaTemp,d);
    //printf("min: (%e,%e,%e) max: (%e,%e,%e) centre: (%e,%e,%e)\n",min[0],min[1],min[2],max[0],max[1],max[2],(min[0]+max[0])*0.5,(min[1]+max[1])*0.5,(min[2]+max[2])*0.5);
    
    double uNew[3] = {max[0] - min[0], max[1] - min[1], max[2] - min[2]};          //New line vector calculated from Min and Max values.
    double rNew = sqrt(pow(uNew[0],2)+pow(uNew[1],2)+pow(uNew[2],2));  //Module of new u
    
    float sigmaNew = sigmaNorm*r/rNew;                                 //New value of sigma
    float mu = 0.5;                                                    //Expected value of the Gaussian function
                                                                       //Assumed at the center of the shortende LOR
    
    
    //Function to image reconstruction with modified Siddon algorithm
    LORGauss(min, max, voxIndexes, sigmaNew, 
             mu, voxWeight, voxNumber, voxLenghts);      
    
    return 0;
}

int jacobs::LORGauss(float* in, float* out, unsigned long *voxIndexes, float sigma, float mu,
	       double *voxWeight, unsigned *voxNumber, double *voxLenghts) const
{
  //////////////////////////////////////////////////////////////////
  //                                                              
  //  Add LOR contribution to voxels weights matrix (voxWeights) using siddon algorithm 
  //
  //  INPUT:
  //        in      -> Position of init LOR point (x,y,z).
  //        out     -> Position of end  LOR point (x,y,z).
  //        nVoxels -> Array with number of voxels in each axis (nx,ny,nz).
  //        voxSize -> 3 dimensional array with voxels size in each axis (dx,dy,dz).
  //
  //  OUTPUT:
  //       return 0 on succes
  //       voxIndexes -> Array with with voxels indexes with non zero weights.
  //       voxWeights -> Array with with voxels acumulated weights (only non zero weights).
  //       voxNumber  -> Number of voxels with non zero weights.
  //       voxLenghts -> Total voxel distance crossed by LOR (excludes out of FOV LOR segments) 
  //

  if(!geometryCreated || !initialized)
    return -1;
  
  int nxy = nVoxels[3];
  const double* voxSize = voxelSize;
  (*voxNumber) = 0;
  
  //Parametrize the line between point1 and point2
  // x = x1 + alpha*(x2-x1)
  // y = y1 + alpha*(y2-y1)
  // z = z1 + alpha*(z2-z1)
      
  double u[3] = {out[0] - in[0], out[1] - in[1], out[2] - in[2]};  //line vector

  double alphaMin;
  double alphaMax;

  double XalphaMin;
  double YalphaMin;
  double ZalphaMin;

  double XalphaMax;
  double YalphaMax;
  double ZalphaMax;
      
  //Calculate alpha values for first and last plane in each axis

  //X axis
  if(fabs(u[0]) > 1.0e-9)
    {
      double Xalpha1 = (Xplane[0]-in[0])/u[0];
      double XalphaN = (Xplane[Nx-1]-in[0])/u[0];

      if(Xalpha1 < XalphaN)
	{
	  XalphaMin = Xalpha1;
	  XalphaMax = XalphaN;
	}
      else
	{
	  XalphaMin = XalphaN;
	  XalphaMax = Xalpha1;	  
	}
    }
  else
    {
      XalphaMin = -1.0;
      XalphaMax =  2.0;
    }
      
  //Y axis
  if(fabs(u[1]) > 1.0e-9)
    {
      double Yalpha1 = (Yplane[0]-in[1])/u[1];
      double YalphaN = (Yplane[Ny-1]-in[1])/u[1];

      if(Yalpha1 < YalphaN)
	{
	  YalphaMin = Yalpha1;
	  YalphaMax = YalphaN;
	}
      else
	{
	  YalphaMin = YalphaN;
	  YalphaMax = Yalpha1;	  
	}	  
    }
  else
    {
      YalphaMin = -1.0;
      YalphaMax =  2.0;
    }

  //Z axis
  if(fabs(u[2]) > 1.0e-9)
    {      
      
      double Zalpha1 = (Zplane[0]-in[2])/u[2];
      double ZalphaN = (Zplane[Nz-1]-in[2])/u[2];

      if(Zalpha1 < ZalphaN)
	{
	  ZalphaMin = Zalpha1;
	  ZalphaMax = ZalphaN;
	}
      else
	{
	  ZalphaMin = ZalphaN;
	  ZalphaMax = Zalpha1;	  
	}
    }
  else
    {
      ZalphaMin = -1.0;
      ZalphaMax =  2.0;
    }      
      
  //Calculate alpha min and alpha max
  alphaMin = XalphaMin;
  if(alphaMin < YalphaMin){alphaMin = YalphaMin;}
  if(alphaMin < ZalphaMin){alphaMin = ZalphaMin;}
  if(alphaMin < 0.0)      {alphaMin = 0.0;      }

  alphaMax = XalphaMax;
  if(alphaMax > YalphaMax){alphaMax = YalphaMax;}
  if(alphaMax > ZalphaMax){alphaMax = ZalphaMax;}
  if(alphaMax > 1.0)      {alphaMax = 1.0;      }


  if(alphaMax < alphaMin)
    {
      return 1; // LOR out of FOV	  
    }

      
  //Calculate min and max index i,j,k for LOR n
  int nIntX = 0;
  int nIntY = 0;
  int nIntZ = 0;

  double alpha0[3] = {1.0e30,1.0e30,1.0e30};; // Stores first alfa values where LOR cuts first plane of each axis (x,y,z)  
  int indexU[3] = {0,0,0}; //Store index updates (x,y,z)

  int imin = 0, imax = 0, jmin = 0, jmax = 0, kmin = 0, kmax = 0;
  
  if(u[0] > 0.0)
    {
      imin = Nx - 1 - (int)((Xplane[Nx-1] - alphaMin*u[0] - in[0])/voxSize[0]);
      imax =  (int)((in[0] + alphaMax*u[0]-Xplane[0])/voxSize[0]);
	  
      imin = imin < 0 ? 0 : imin;
      imax = imax > Nx ? Nx-1 : imax;
      nIntX = imax - imin + 1;
	  
      alpha0[0] = (Xplane[imin]-in[0])/u[0];

      indexU[0] = 1;
    }
  else if(u[0] < 0.0)
    {
      imin = Nx - 1 - (int)((Xplane[Nx-1] - alphaMax*u[0] - in[0])/voxSize[0]);
      imax = (int)((in[0] + alphaMin*u[0]-Xplane[0])/voxSize[0]);

      imin = imin < 0 ? 0 : imin;
      imax = imax > Nx ? Nx-1 : imax;
      nIntX = imax - imin +1;

      alpha0[0] = (Xplane[imax]-in[0])/u[0];
	  
      indexU[0] = -1;
    }      
      
  if(u[1] > 0.0)
    {
      jmin = Ny - 1 - (int)((Yplane[Ny-1] - alphaMin*u[1] - in[1])/voxSize[1]);
      jmax = (int)((in[1] + alphaMax*u[1]-Yplane[0])/voxSize[1]);

      jmin = jmin < 0 ? 0 : jmin;
      jmax = jmax >= Ny ? Ny-1 : jmax;
      nIntY = jmax - jmin + 1;

      alpha0[1] = (Yplane[jmin]-in[1])/u[1];

      indexU[1] = 1;
    }
  else if(u[1] < 0.0)
    {
      jmin = Ny - 1 - (int)((Yplane[Ny-1] - alphaMax*u[1] - in[1])/voxSize[1]);
      jmax = (int)((in[1] + alphaMin*u[1]-Yplane[0])/voxSize[1]);

      jmin = jmin < 0 ? 0 : jmin;
      jmax = jmax >= Ny ? Ny-1 : jmax;
      nIntY = jmax - jmin + 1;

      alpha0[1] = (Yplane[jmax]-in[1])/u[1];
      
      indexU[1] = -1;
    }
      
      
  if(u[2] > 0.0)
    {
      kmin = Nz - 1 - (int)((Zplane[Nz-1] - alphaMin*u[2] - in[2])/voxSize[2]);
      kmax = (int)((in[2] + alphaMax*u[2]-Zplane[0])/voxSize[2]);
	  
      kmin = kmin < 0 ? 0 : kmin;
      kmax = kmax >= Nz ? Nz-1 : kmax;      
      nIntZ = kmax - kmin + 1;

      alpha0[2] = (Zplane[kmin]-in[2])/u[2];
      
      indexU[2] = 1;
    }
  else if(u[2] < 0.0)
    {
      kmin = Nz - 1 - (int)((Zplane[Nz-1] - alphaMax*u[2] - in[2])/voxSize[2]);
      kmax = (int)((in[2] + alphaMin*u[2]-Zplane[0])/voxSize[2]);

      kmin = kmin < 0 ? 0 : kmin;
      kmax = kmax >= Nz ? Nz-1 : kmax;      
      nIntZ = kmax - kmin + 1;

      alpha0[2] = (Zplane[kmax]-in[2])/u[2];

      indexU[2] = -1;
    }

  // Number of crossed planes
  int nAlphas = nIntX + nIntY + nIntZ;

  //Calculate alpha updates
  double alphaU[3];
  alphaU[0] = fabs(voxSize[0]/u[0]);
  alphaU[1] = fabs(voxSize[1]/u[1]);
  alphaU[2] = fabs(voxSize[2]/u[2]);
    
  double dLOR = sqrt(pow(u[0],2.0) + pow(u[1],2.0) + pow(u[2],2.0));
  double d12 = 0.0;

  // The program need care of if some alpha value is equal to 'alphaMin'.
  // If alpha min is equal to first crossed plane, we must
  // avoid calcule the step between first alpha and first
  // crossed plane (Because they are the same). So we have
  // one less alpha. This must be check in every axis.

  double alphaC = alphaMin;
  for(int m = 0; m < 3; m++)
    {
      if(alpha0[m]-alphaMin < 1.0e-9)
	{
	  //First alpha is equal to alpha min.
	  //Increment first alpha0 to avoid empty segment
	  alpha0[m] += alphaU[m];
	  nAlphas--;	  
	}
    }

  //Check first crosed axis
  int firstAlpha = alpha0[0] < alpha0[1] ? 0 : 1;
  firstAlpha = alpha0[firstAlpha] < alpha0[2] ? firstAlpha : 2;
  
  //Calculate first i,j,k indexes
  int i,j,k;
  i = (int)((in[0] + (alphaC+alpha0[firstAlpha])*0.5*u[0] - Xplane[0])/voxSize[0]);
  j = (int)((in[1] + (alphaC+alpha0[firstAlpha])*0.5*u[1] - Yplane[0])/voxSize[1]);
  k = (int)((in[2] + (alphaC+alpha0[firstAlpha])*0.5*u[2] - Zplane[0])/voxSize[2]);
  
  if(i < 0){i = 0;}
  else if(i >= nVoxels[0]){i = nVoxels[0]-1;}

  if(j < 0){j = 0;}
  else if(j >= nVoxels[1]){j = nVoxels[1]-1;}

  if(k < 0){k = 0;}
  else if(k >= nVoxels[2]){k = nVoxels[2]-1;}
  
  
  const float var = 2.0*pow(sigma,2);  //Gaussian function parameters
  const float A = 1.0/(sqrt(var*M_PI));
  
  for(int m = 0; m < nAlphas; m++)
    {
      double l;
      int index = 0;
      
      if(alpha0[0] < alpha0[1] && alpha0[0] < alpha0[2])
	{
	  float alphaMid = (alpha0[0]+alphaC)*0.5;  
	  //Next plane crossed: X plane
	  l = (alpha0[0]-alphaC)*dLOR*A*exp(-pow((alphaMid-mu),2)/var);
	  d12 += l;

	  //Calculate index
	  index = k*nxy + j*nVoxels[0] + i;
	  
	  i += indexU[0];
	  alphaC = alpha0[0];

	  //Avoid possible rounding errors. This is because,
	  //if some index values are on the FOV edges, taking
	  //one axis or other can cause a negative index (-1) or
	  //equal to 'nvox[]'. This only will be caused when the
	  //alpha0 values of two axis are aproximately equal and
	  //one of the index are on the border. 
	  if(i == -1 || i == nVoxels[0])
	    {
	      break;
	    }

	  alpha0[0] += alphaU[0];	      
	}
      else if(alpha0[1] < alpha0[2])
	{
	  float alphaMid = (alpha0[1]+alphaC)*0.5;  
	  //Next plane crossed: X plane
	  l = (alpha0[1]-alphaC)*dLOR*A*exp(-pow((alphaMid-mu),2)/var);
	  d12 += l;

	  //Calculate index
	  index = k*nxy + j*nVoxels[0] + i;

	  j += indexU[1];
	  alphaC = alpha0[1];

	  //Avoid possible rounding errors. This is because,
	  //if some index values are on the FOV edges, taking
	  //one axis or other can cause a negative index (-1) or
	  //equal to 'nvox[]'. This only will be caused when the
	  //alpha0 values of two axis are aproximately equal and
	  //one of the index are on the border. 
	  if(j == -1 || j == nVoxels[1])
	    {
	      break;
	    }

	  alpha0[1] += alphaU[1];	      
	}
      else
	{
	  float alphaMid = (alpha0[2]+alphaC)*0.5;  
	  //Next plane crossed: X plane
	  l = (alpha0[2]-alphaC)*dLOR*A*exp(-pow((alphaMid-mu),2)/var);
	  d12 += l;

	  //Calculate index
	  index = k*nxy + j*nVoxels[0] + i;
	  
	  k += indexU[2];
	  alphaC = alpha0[2];

	  //Avoid possible rounding errors. This is because,
	  //if some index values are on the FOV edges, taking
	  //one axis or other can cause a negative index (-1) or
	  //equal to 'nvox[]'. This only will be caused when the
	  //alpha0 values of two axis are aproximately equal and
	  //one of the index are on the border. 
	  if(k == -1 || k == nVoxels[2])
	    {
	      break;
	    }
	  
	  alpha0[2] += alphaU[2];	      
	  
	}
      
      //Each voxel only can be crossed one time, so add this index with no check
      voxIndexes[(*voxNumber)] = index;
      voxWeight[(*voxNumber)] = l;
      (*voxNumber)++;      
    }

  // Add the contribution from last alpha to alphaMax.
  // Take in account that no plane will be crossed, so
  // the voxel index will be the last used.

  // If the alpha max is on a plane, the voxel index will be
  // out of fov. So, check first the i,j,k indexes.

  if(i < 0 || i >= nVoxels[0] || j < 0 || j >= nVoxels[1] || k < 0 || k >= nVoxels[2])
    {
      //Alpha max is on border plane. Doesn't do any contribution.
    }
  else
    {
      //Add the contribution of alpha max
      float alphaMid = (alphaMax+alphaC)*0.5;
      double l = (alphaMax-alphaC)*dLOR*A*exp(-pow((alphaMid-mu),2)/var);      
      if(l > 1.0e-9)
	{
	  d12 += l;
	  
	  //Calculate index
	  int index = k*nxy + j*nVoxels[0] + i;
	  
	  //Each voxel only can be crossed one time, so add this index with no check
	  voxIndexes[(*voxNumber)] = index;
	  voxWeight[(*voxNumber)] = l;
	  (*voxNumber)++;
	}
    }
  
  (*voxLenghts) = d12;
  
  return 0;
}

jacobs::~jacobs()
{
  clean();
}


//**********************//
//* Auxiliar functions *//
//**********************//


void imageNormalize(double* imageIn, int* nvox, double* imageOut)
{
    
    //Function to normalize image after reconstruction 
    //   
    //  input:
    //   imageIn  -> reconstructed image
    //   tnVox    -> total number of voxels 
    //   
    //  output:
    //   imageOut -> resulting normalized image

       
    int tnvox = nvox[0]*nvox[1]*nvox[2];
    double sum = 0.0;
  
    
    for(int i = 0; i < tnvox; i++)
    {
       sum += imageIn[i];  //sum for intensities of all voxels
    }
    
    for(int j = 0; j < tnvox; j++)
    {
        imageOut[j] = imageIn[j]/sum;  //normalize 
    }
}


void euler2quaternion(double* axis1, double alpha, double* axis2, double beta, double* axis3, double gamma, double* q)
{
  //Extract rotation quaternion from the
  //specified Euler angles and rotation axis.
  //
  //
  //  input:
  //   axis1 -> axis of first rotation (x,y,z)
  //   axis2 -> axis of second rotation (x,y,z)
  //   axis3 -> axis of third rotation (x,y,z)
  //   alpha -> angle of first rotation (rad)
  //   beta  -> angle of second rotation (rad)
  //   gamma -> angle of third rotation (rad)
  //
  // Output:
  //    q  -> resulting quaternion rotation. The format of
  //          quaternion is as follows,
  //
  //          ax i + ay j + az k + a
  //
  //          with scalar part at last position
  //

  //Precalculate some values

  const double twoPI = 2.0*M_PI;
  
  while(alpha >= twoPI)
    alpha -= twoPI;

  while(beta >= twoPI)
    beta -= twoPI;

  while(gamma >= twoPI)
    gamma -= twoPI;
  
  const double alpha05 = alpha*0.5;
  const double beta05 = beta*0.5;
  const double gamma05 = gamma*0.5;

  const double sinAlpha05 = sin(alpha05);
  const double sinBeta05  = sin(beta05);
  const double sinGamma05 = sin(gamma05);
  
  const double modAxis1 = sqrt(pow(axis1[0],2) + pow(axis1[1],2) + pow(axis1[2],2));
  const double modAxis2 = sqrt(pow(axis2[0],2) + pow(axis2[1],2) + pow(axis2[2],2));
  const double modAxis3 = sqrt(pow(axis3[0],2) + pow(axis3[1],2) + pow(axis3[2],2));
  
  //Normalize rotation axis
  double v1[3];
  v1[0] = axis1[0]/modAxis1;
  v1[1] = axis1[1]/modAxis1;
  v1[2] = axis1[2]/modAxis1;

  double v2[3];
  v2[0] = axis2[0]/modAxis2;
  v2[1] = axis2[1]/modAxis2;
  v2[2] = axis2[2]/modAxis2;

  double v3[3];
  v3[0] = axis3[0]/modAxis3;
  v3[1] = axis3[1]/modAxis3;
  v3[2] = axis3[2]/modAxis3;
  
  //Construct partial rotation quaternions
  double q1[4];
  q1[0] = v1[0]*sinAlpha05;
  q1[1] = v1[1]*sinAlpha05;
  q1[2] = v1[2]*sinAlpha05;
  q1[3] = cos(alpha05);

  double q2[4];
  q2[0] = v2[0]*sinBeta05;
  q2[1] = v2[1]*sinBeta05;
  q2[2] = v2[2]*sinBeta05;
  q2[3] = cos(beta05);

  double q3[4];
  q3[0] = v3[0]*sinGamma05;
  q3[1] = v3[1]*sinGamma05;
  q3[2] = v3[2]*sinGamma05;
  q3[3] = cos(gamma05);

  //Calculate global quaternion rotation
  double qaux[4];
  Qprod(q3  ,q2,qaux);
  Qprod(qaux,q1,   q);
}

int createMovMatrix(double* in, float*& out, int nbins, float* initPos)
{
  //Create moviment information from input data (in) with
  //format,
  //
  // (x,y,z,alpha,beta,gamma)
  //
  // where x,y,z are the spatial positions in each bin
  // and alpha beta gamma are the Euler angles to
  // perform the rotation,
  //
  // Rt(alpha,beta,gamma) = Rz(gamma)Ry(beta)Rz(alpha)
  //
  //
  // Store the output matrix in 'out' and initial position
  // in 'initPos'
  //
  // return  0 on succes
  // return -1 if can't allocate memory

  double z[3] = {0.0,0.0,1.0};
  double y[3] = {0.0,1.0,0.0};

  out = 0;
  out = (float*) malloc(sizeof(float)*nbins*7);
  if(out == 0){return -1;}
  
  //Take initial position
  initPos[0] = in[0];
  initPos[1] = in[1];
  initPos[2] = in[2];

  //Take initial rotation
  double qinit[4];
  euler2quaternion(z,in[3],y,in[4],z,in[5],qinit);

  //Calculate the inverse of qinit
  double iqinit[4];
  iqinit[0] = -qinit[0];
  iqinit[1] = -qinit[1];
  iqinit[2] = -qinit[2];
  iqinit[3] =  qinit[3];
  
  for(int i = 0; i < nbins; i++)
    {
      int posIn  = i*6;
      int posOut = i*7;

      out[posOut  ] = in[posIn  ] - initPos[0];
      out[posOut+1] = in[posIn+1] - initPos[1];
      out[posOut+2] = in[posIn+2] - initPos[2];

      //Extract quaternion rotation
      double alpha = in[posIn+3];
      double beta  = in[posIn+4];
      double gamma = in[posIn+5];
      double q2[4];
      euler2quaternion(z,alpha,y,beta,z,gamma,q2);

      //Apply the inverse rotation of first position
      double q[4];
      Qprod(q2,iqinit,q);      
      
      //Store final inverted rotation quaternion
      out[posOut+3] = -(float)q[0];
      out[posOut+4] = -(float)q[1];
      out[posOut+5] = -(float)q[2];
      out[posOut+6] =  (float)q[3];

    }
  return 0;
}


//Matrix print

int printMatrixXY(FILE* file, int* nVox, double* voxSize, double* voxelIn)
{
  // Create a file to represent XY planes of matrix "voxelIn" with gnuplot
  //
  // Using:
  //        splot 'filename' index <plane number> matrix nonuniform notitle
  //
  
  if(file == NULL)
    {
      return 1;
    }

  int nxy = nVox[0]*nVox[1];
  
  for(int k = 0; k < nVox[2]; k++)
    {
      
      fprintf(file, "# Plane Z: %d \n", k);
      fprintf(file, " %14d ", nVox[1]);
      for(int i = 0; i < nVox[0]; i++)
	{
	  fprintf(file, " %14.5E ", (i+0.5)*voxSize[0]);
	}
      fprintf(file, "\n");
      
      for(int j = 0; j < nVox[1]; j++)
	{
	  fprintf(file, " %14.5E ", (j+0.5)*voxSize[1]);

	  for(int i = 0; i < nVox[0]; i++)
	    {
	      fprintf(file, " %14.5E ", voxelIn[k*nxy+j*nVox[0]+i]);
	    }
	  fprintf(file, "\n");
	}
      fprintf(file, "\n");
    }
  return 0;
}

int printMatrixYZ(FILE* file, int* nVox, double* voxSize, double* voxelIn)
{
  // Create a file to represent YZ planes of matrix "voxelIn" with gnuplot
  //
  // Using:
  //        splot 'filename' index <plane number> matrix nonuniform notitle
  //
  
  if(file == NULL)
    {
      return 1;
    }

  int nxy = nVox[0]*nVox[1];
  
  for(int i = 0; i < nVox[0]; i++)
    {
      
      fprintf(file, "# Plane X: %d \n", i);
      fprintf(file, " %14d ", nVox[2]);
      for(int j = 0; j < nVox[1]; j++)
	{
	  fprintf(file, " %14.5E ", (j+0.5)*voxSize[1]);
	}
      fprintf(file, "\n");
      
      for(int k = 0; k < nVox[2]; k++)
	{

	  fprintf(file, " %14.5E ", (k+0.5)*voxSize[2]);

	  for(int j = 0; j < nVox[1]; j++)
	    {
	      fprintf(file, " %14.5E ", voxelIn[k*nxy+j*nVox[0]+i]);
	    }
	  fprintf(file, "\n");
	}
      fprintf(file, "\n");
    }
  return 0;
}

int printMatrixXZ(FILE* file, int* nVox, double* voxSize, double* voxelIn)
{
  // Create a file to represent XZ planes of matrix "voxelIn" with gnuplot
  //
  // Using:
  //        splot 'filename' index <plane number> matrix nonuniform notitle
  //

  if(file == NULL)
    {
      return 1;
    }

  int nxy = nVox[0]*nVox[1];
  
  for(int j = 0; j < nVox[1]; j++)
    {
      
      fprintf(file, "# Plane Y: %d \n", j);
      fprintf(file, " %14d ", nVox[2]);
      for(int i = 0; i < nVox[0]; i++)
	{
	  fprintf(file, " %14.5E ", (i+0.5)*voxSize[0]);
	}
      fprintf(file, "\n");
      
      for(int k = 0; k < nVox[2]; k++)
	{
	  fprintf(file, " %14.5E ", (k+0.5)*voxSize[2]);

	  for(int i = 0; i < nVox[0]; i++)
	    {
	      fprintf(file, " %14.5E ", voxelIn[k*nxy+j*nVox[0]+i]);
	    }
	  fprintf(file, "\n");
	}
      fprintf(file, "\n");
    }
  return 0;
}
