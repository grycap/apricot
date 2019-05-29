 
#include "reconstruction.h"


void convolution::convolute(const double* in, double* out, const int* nvox, const double* kernel, const int dim)
{
  if(kernel != 0 && dim > 0)
    {
      CPUconvolute(in, nvox[0], nvox[1], nvox[2], kernel, dim, out);
    }
}

void convolution::CPUconvolute(const double *in, const int nx, const int ny, const int nz, const double *kernel, const int dim, double *out)
{
    //////////////////////////////////////////////////////////////////////////////////////
    //                                                                                  //
    //   Convolute function                                                             //
    //                                                                                  //
    //   INPUT:                                                                         //
    //         in -> original matrix (image)                                            //
    //         nx, ny, nz -> number of voxel in each coordenate (image dimension)       //
    //         kernel -> kernel matrix to convolute with original image                 //
    //         dim -> dimension of the kernel matrix(dim x dim x dim)                   //
    //                                                                                  //
    //   OUTPUT                                                                         //   
    //         out -> pointer where convoluted image will be saved                      //
    //                                                                                  //
    //////////////////////////////////////////////////////////////////////////////////////
    
    int kCenter = dim/2;                       // It is the same for profundity, rows and columns (dim x dim x dim)
 
    int m = 0;                                 // Output index

    
    const int nxy = nx*ny;
    const int nxyz = nxy*nz;
    const int dim2 = dim*dim;

    for(int k = 0; k < nxyz; k++)
      out[k] = 0.0;
    
    for(int k=0; k < nz; k++)                    // Profundity of image
    {
        for(int j=0; j < ny; j++)                // Rows of image
        {
            for(int i=0; i < nx; i++)            // Columns of image
            {
                for(int z=0; z < dim; z++)                // Profundity of kernel
                {                    
                    for(int y=0; y < dim; y++)            // Rowns of kernel
                    {
                        for(int x=0; x < dim; x++)        // Columns of kernel
                        {
                            // Index of input signal (image), used for checking boundary
                            int kk = k + (z - kCenter);
                            int jj = j + (y - kCenter);
                            int ii = i + (x - kCenter);
                            
                            // Ignore input samples which are out of bound
                            if(kk >= 0 && kk < nz && jj >= 0 && jj < ny && ii >= 0 && ii < nx)
                            {
                                out[m] += in[(kk*nxy) + (jj*nx) + ii]*kernel[(z*dim2) + y*dim + x];        // Convolution operation with corresponding matrix elements
                            } 
                              
                        }
                    }
                    
                }
                m++;
            }
                
        }
    }
}

void gaussianKernel(int dim, float sigma, double *kernel)
{
    //////////////////////////////////////////////////////////////////////
    //                                                                  //
    //   Gaussian 3D Kernel                                             //
    //                                                                  //
    //   INPUT:                                                         //
    //         dim -> dimension of the kernel matrix(dim x dim x dim)   //
    //         sigma -> global sigma to obtain Gaussian function        //
    //                                                                  //
    //   OUTPUT                                                         //   
    //         kernel -> pointer where calculated kernel will be saved  //
    //                                                                  //
    //////////////////////////////////////////////////////////////////////
    
    int kernelDim = dim*dim*dim;         // Size of kernel vector
    float sum = 0.0;                     // For accumulating the kernel values
    float mu = float(dim)/2.0;                  // Centered mu of the Gaussian function
                                         // Is the same value for each coordenate (x,y,z)
    int i=0;
    for(int z=0; z < dim; z++)
    {
        for(int y=0; y < dim; y++)
        {
            for(int x=0; x < dim; x++)
            {
                kernel[i] = gauss3D(mu, sigma, x+0.5, y+0.5, z+0.5);  // Gaussian function 3D
                //Accumulate the kernel values
                sum += kernel[i];
                i++;
            }
        }
    }
    
    // Kernel normalization
    for (i=0; i < kernelDim; i++)
    {
       kernel[i] /= sum;
    }
}

float gauss3D(float mu, float sigma, float x, float y, float z)
{
    //////////////////////////////////////////////////////////////////////
    //                                                                  //
    //   3D Gaussian funtion                                            //
    //                                                                  //
    //   INPUT:                                                         //
    //         mu -> global mean of centered Gaussian function          //
    //         sigma -> global sigma to obtain Gaussian function        //
    //         x, y, z -> coordenates of the matrix element             //
    //                                                                  //
    //   OUTPUT                                                         //   
    //         return 3D Gaussian function value                        //
    //                                                                  //
    //////////////////////////////////////////////////////////////////////
    
    const float PI2e305 = sqrt(pow(2.0*M_PI,3));
    
    float sigma2 = sigma*sigma;
    float sigma3 = sigma2*sigma;
    
    float N = 1.0/(PI2e305*sigma3);
    float expon = exp(-(pow(x-mu,2)+pow(y-mu,2)+pow(z-mu,2))/(2.0*sigma2));
        
    return N*expon;
}

void BPiFP(const unsigned long* voxIndexes, const double* voxWeights, int voxNumber, double* ck, const double* nXrho)
{

  //This function backproject the inverse of the forward projection
  //of the projected LOR.
  //The forward projection will be done using 'nXrho' matrix.
  //The backprojection will be added to 'ck' matrix.
  //
  // input:
  //    voxIndexes -> array with indexes crossed by LOR.
  //    voxWeights -> array with weights corresponing to each voxel
  //                  specified in voxInexes.
  //    voxNumber  -> number of crossed voxels
  //    nXrho      -> matrix to perform forward projection
  // output:
  //    ck         -> matrix to store backward projection.
  //
  // Important: This function is thread safe. Notice the atomic pragma.
  //
  
  //La suma de los voxeles
  double sum = 0.0;

  // Calcular la suma de los voxeles que atraviesa la lor
  // para la matriz nk X rho
  for(int i = 0; i < voxNumber; i++)
    {
      sum += nXrho[voxIndexes[i]]*voxWeights[i];
    }
  // rellenanar ck (sin convolucion con rho) 
  for (int i = 0; i < voxNumber; i++)
    {
      if(sum != 0.0)
	{
          #pragma omp atomic
	  ck[voxIndexes[i]] += voxWeights[i]/sum;
	}
    }
}

int sensIdentity(int* nvox, double*& s)
{
  
  unsigned long tnvox  = nvox[0]*nvox[1]*nvox[2];  //Calculate number of voxels
  s = nullptr;
  s             = (double*) malloc(sizeof(double)*tnvox);
  if(s == nullptr)
    return -1;
  
  for(unsigned i = 0; i < tnvox; i++)
    s[i] = 1.0;

  return 0;
}

int sensitivity(int* nvox, float TOFres, const char* filename, const int LORformat, double*& s, projector* pProj, convolution& convObj, const double* h, const int dim)
{   
    
  ////////////////////////////////////////////////////////////////////////////////////
  //                                                                                //
  //   sensitivity matrix                                                           //
  //                                                                                //
  //   INPUT:                                                                       //
  //         nx, ny, nz -> number of voxel in each coordenate (image dimension)     //
  //         filename   -> filename where LORs are stored                           //
  //         h          -> convolution kernel with neighbors influence of physics   //
  //                       effects                                                  //
  //         dim        -> Convolution kernel "h" dimensions
  //                                                                                //
  //   OUTPUT                                                                       // 
  //         s          -> sensitivity matrix                                       //
  //                                                                                //
  ////////////////////////////////////////////////////////////////////////////////////

  if(s != 0)
    {
      free(s);
      s = 0;
    }
  
  //Allocate memory for sensitivity matrix
  unsigned long tnvox  = nvox[0]*nvox[1]*nvox[2];  //Calculate number of voxels
  s             = (double*) calloc(tnvox,sizeof(double));
  double* sumwx = (double*) calloc(tnvox,sizeof(double));
  
  //Allocate memory for projection arays considering
  //the number of voxels in each axis
  int projBufferSize = 4*sqrt( pow(nvox[0],2) + pow(nvox[1],2) + pow(nvox[2],2));
  
  unsigned long* voxIndexes = (unsigned long*) malloc(sizeof(unsigned long)*projBufferSize);
  double*        voxWeights = (double*)        malloc(sizeof(double)*projBufferSize);

  //Declare lor buffer
  const int nReadLors = 50000;
  LORstruct LORbuffer[nReadLors];

  //Open LORs file
  FILE* fLOR = 0;
  fLOR = fopen(filename,"r");
  if(fLOR == 0)
    {
      return -1;
    }

  //---------------------------
  //First calculate sum{w*X}
  //---------------------------

  
  //Read first LOR chunk
  int nread = readLORs(fLOR, LORbuffer, nReadLors, LORformat);
  int nLORs = 0;
  
  while(nread > 0)
    {
      nLORs += nread;

      // Calculate contribution of each LOR
      for(int i = 0; i < nread; i++)
	{
	  // Project LOR
        double l;
	  int nCrosed = pProj->project(TOFres, voxIndexes, voxWeights, LORbuffer[i], l);

	  for(int j = 0; j < nCrosed; j++)
	    {
	      sumwx[voxIndexes[j]] += LORbuffer[i].w*voxWeights[j];
	    }	  
	  
	}
      
      //Read next LOR chunk
      nread = readLORs(fLOR, LORbuffer, nReadLors, LORformat);
    }

  //-------------
  // Calculate S
  //-------------
  
  convObj.convolute(sumwx, s, nvox, h, dim);
  
  //Find maximum value in s
  double max = 0.0;
  for(unsigned i = 0; i < tnvox; i++)
  {
    if(s[i] > max)
    {
        max = s[i];
        
    }
  }
  
  //Set threshold value to avoid diveregences
  double threshold = (1.0/max)*0.5;
  
  for(unsigned i = 0; i < tnvox; i++)
  {
      if(s[i] > threshold)
      {
        s[i] = 1.0/s[i];
      }
      else
      s[i] = 0.0;
  }

  //Free memory
  free(sumwx);
  free(voxIndexes);
  free(voxWeights);

  return nLORs;
  
}

void nextImage(double* image, const double* s, const double* c, const unsigned nvox)
{
  #pragma omp parallel for
  for(unsigned int k = 0; k < nvox; k++)
    {
        image[k] = image[k]*s[k]*c[k];
    }  
}
