 

#include <cstring>

#include "projectors.h"
#include "reconstruction.h"
#include "input.h"

#ifdef _RECONSTRUCT_OMP_
#include <omp.h>
#endif

int main(int argc, char** argv)
{
  
  //*****************************//
  //* Check number of arguments *//
  //*****************************//
  if(argc < 2)
    {
      printf("Usage:\n");
      printf("%s LORs-file\n",argv[0]);
      return -1;
    }

  //****************************//
  //*    Declare variables     *//
  //****************************//
  
  //Buffers
  LORstruct* LORbuffer = 0; //Store LORs
  unsigned long* voxIndexes = 0; //Voxel indexes buffer (used by projection)
  double *voxWeights = 0;        //Voxel weights buffer (used by projection)

  double* image = 0;             //Buffer with actual image
  double* s = 0;                 //Buffer with sensitivity matrix
  double* sConv = 0;             //Buffer with convoluted sensitivity matrix
  double* c = 0;                 //Correction matrix
  double* Hn = 0;                //Image convoluted with physical corrections
  double* smoothKernel = 0;      //Smooth kernel
  double* rho = 0;               //Correction kernel
  
  //Workspace variables
  int nvox[3] = {0,0,0};        //Number of voxels in each axis
  double dvox[3] = {0.0,0.0,0.0};  //Voxel size in each axis
  
  //Moviment variables
  int tbins = 0;                //Number of temporal bins
  float dtbin = 0;              //Size of temporal bin
  float* movMatrix = 0;         //Matrix with moviment information
  float initPos[3];             //Initial moviment position
  
  //Configuration variables
  char* sensfilename = 0;
  int nIterations = 1;
  int nThreads = 1;             //Number of threads to use with openMP
  float TOFres = -1.0;
  char* LORsFilename = 0;       //Filename where read LORs
  char* movimentFilename = 0;   //Filename with moviment information
  const int nLORformats = 3;    //Number of LOR formats
  int LORformat = -1;           //Used LOR format
  projector* pProj = 0;         //Pointer to used projector
  int nChunks = 10;             //Number of chunks for reconstruction algorithm
  int chunkSize = 0;            //Number of LORs in each chunk
  int nLORs = 0;      //Number of LORs in specified file
  int kernelDim = 5;
  float kernelSigma = 2.5;

#ifdef _RECONSTRUCT_OMP_
  //Calculate the number of threads to use by default.
  //One thread for each proc
  nThreads = omp_get_num_procs();
#endif
  
  //****************************//
  //*   Parse configuration    *//
  //****************************//
  if(parseArguments(argc, argv, LORsFilename, movimentFilename, pProj, LORformat, nvox, dvox, nChunks, kernelDim, nThreads, TOFres, sensfilename, nIterations) != 0)
    {
      printf("Stopping execution.\n");
      return 0;
    }

  //Check LORs file
  if(LORsFilename == 0)
    {
      printf("Error: No LOR filename specified.\n");
      printf("Stopping execution.\n");
      return -1;
    }

  //Check LOR format
  if(LORformat >= nLORformats || LORformat < 0)
    {
      printf("Error: Invalid specified LOR format %d\n",LORformat);
      printf("       Valid range: [0,%d]",nLORformats-1);
      return -3;
    }  

  //Check number of voxels
  if(nvox[0] <= 0 || nvox[1] <= 0 || nvox[2] <= 0)
    {
      printf("Error: Invalid number of voxels.\n");
      printf("       Voxels in each axis:\n");
      printf("        nx = %d\n",nvox[0]);
      printf("        ny = %d\n",nvox[1]);
      printf("        nz = %d\n",nvox[2]);
      return -4;
    }

  //Check voxel sizes
  if(nvox[0] <= 0 || nvox[1] <= 0 || nvox[2] <= 0)
    {
      printf("Error: Invalid voxel sizes.\n");
      printf("       Voxel size in each axis:\n");
      printf("        dx = %f\n",dvox[0]);
      printf("        dy = %f\n",dvox[1]);
      printf("        dz = %f\n",dvox[2]);
      return -5;
    }

  //Check chunk number
  if(nChunks <= 0)
    {
      printf("Warning: Invalid number of chunks: %d\n",nChunks);
      printf("         default value (10) will be used\n");
      nChunks = 10;
    }

  //Check projector
  if(pProj == 0)
    {
      printf("No projector specified, default projector will be used.\n");
      pProj = new jacobs;
    }

  if(pProj == 0)
    {
      printf("Error: Can't create projector!\n");
      return -3;
    }

  //Check LOR format
  if(LORformat < 0)
    {
      printf("No valid LOR format specified, default format will be used.\n");
      LORformat = 0;
    }
  
  //Check kernel dimension
  if(kernelDim <= 0)
    {
      printf("Kernel dimension set to 0, smooth disabled.\n");
      kernelDim = 0;
    }

  //Check number of threads
  if(nThreads <= 0)
    {
      printf("Warning: Number of threads must be positive and, at last, 1.\n");
      printf("         %d is not a valid value, seting 'nThreads' to 1.\n",nThreads);
      nThreads = 1;
    }
    
  //Check number of iterations
  if(nIterations <= 0)
    {
      printf("Warning: Number of iterations must be positive and, at last, 1.\n");
      printf("         %d is not a valid value, seting 'nIterations' to 1.\n",nIterations);
      nIterations = 1;
    }

#ifndef _RECONSTRUCT_OMP_
  if(nThreads > 1)
    {
      printf("Warning: Can't use more than one thread because openMP is not enabled.\n");
      printf("         Please, recompile the code with openMP option enabled.\n");
    }
#endif
  
  
  //****************************//
  //*   Print configuration    *//
  //****************************//
  printf("Projector    : %s\n",pProj->name());
  printf("LORs filename: %s\n",LORsFilename);
  printf("Sensitivity filename: %s\n",sensfilename);
  printf("\nVoxels in each axis:\n");
  printf("     nx = %d\n",nvox[0]);
  printf("     ny = %d\n",nvox[1]);
  printf("     nz = %d\n",nvox[2]);  
  printf("\nVoxel size in each axis:\n");
  printf("     dx = %f\n",dvox[0]);
  printf("     dy = %f\n",dvox[1]);
  printf("     dz = %f\n",dvox[2]);
  printf("\nNumber of iterations: %d\n", nIterations);
  printf("Number of LOR chunks: %d\n",nChunks);
  printf("kernel dimension: %dx%dx%d\n",kernelDim,kernelDim,kernelDim);
  printf("Number of threads: %d\n",nThreads);
  printf("TOF: %s\n", TOFres < 0.0 ? "disabled" : "enabled");
  if(TOFres >= 0.0)
    printf("Time resolution: %e s\n",TOFres);
  if(movimentFilename != 0)
    {
      printf("moviment filename: %s\n",movimentFilename);
      //read moviment variables and create moviment matrix
      int err = readMovFile(movimentFilename, tbins, dtbin, movMatrix, initPos);
      if(err != 0)
	{
	  printf("Error processing moviment data. Error code: %d\n",err);
	  return -2;
	}
      printf("Temporal bins and bin width:\n");
      printf(" %10d %.5e\n",tbins,dtbin);

      printf("Initial sensor position:\n");
      printf(" %11.5e %11.5e %11.5e\n",initPos[0],initPos[1],initPos[2]);
    }
  else
    {
      printf("no moviment specified.\n");
    }
  fflush(stdout);
  
  //****************************//
  //*     Create workspace     *//
  //****************************//

  printf("Creating workspace...\n");
  int errWS = pProj->setWorkspace(nvox[0],nvox[1],nvox[2],dvox[0],dvox[1],dvox[2],tbins,dtbin,movMatrix,initPos);
  if(errWS < 0)
    {
      printf("Error creating workspace. Error code: %d\n",errWS);
    }
  printf("Done!\n");
  fflush(stdout);

  //****************************//
  //*   Initialize projector   *//
  //****************************//

  printf("initializing projector...\n");
  
  pProj->init();

  printf("Done!\n");
  fflush(stdout);

  //****************************//
  //*    Create convolution    *//
  //****************************//

  convolution convObj;
  
  //****************************//
  //*   Create smooth kernel   *//
  //****************************//

  int smoothDim = kernelDim;
  if(kernelDim > 0)
    {
      printf("Creating smooth kernel...\n");
      //Allocate memory
      smoothKernel = (double*) malloc(sizeof(double)*kernelDim*kernelDim*kernelDim);
      if(smoothKernel == 0)
	{
	  printf("Error: Can't allocate memory for smooth kernel.\n");
	  return -8;
	}

      //Calculate sigma
      kernelSigma = kernelDim/2.0;
  
      //Fill kernel
      gaussianKernel(kernelDim,kernelSigma,smoothKernel);
      
      printf("Done!\n");
      fflush(stdout);
    }
  //****************************//
  //* Create correction kernel *//
  //****************************//

  printf("Creating correction kernel...\n");
  
  //Allocate memory
  rho = (double*) malloc(sizeof(double)*1);  

  rho[0] = 1.0;

  printf("Done!\n");
  fflush(stdout);

  //*********************************//
  //* Create/read sensitivity image *//
  //*********************************//
  
  if( sensfilename == 0 )
  {
      
    //Create sensitivity
      
    printf("Creating sensitivity matrix...\n");
    nLORs = sensitivity(nvox, TOFres, LORsFilename, LORformat, s, pProj, convObj, rho, 1);
    //nLORs of sensitivity file
    if(nLORs == -1)
    {
      printf("Error: Can't open LORs file %s\n",LORsFilename);
      return -8;
    }
    
    FILE* out2 = fopen("s.bin","wb");
    fwrite(nvox,sizeof(int),3,out2);
    fwrite(dvox,sizeof(double),3,out2);
    fwrite(s,sizeof(double),nvox[0]*nvox[1]*nvox[2],out2);
    fclose(out2);
    
  }
  else
  {
      if(strcmp(sensfilename,"NONE") == 0){
        printf("Using identity sensitivity matrix.\n");
        if(sensIdentity(nvox,s) != 0){
            printf("Error allocating memmory for sensitivity matrix.\n");
            return -9;
        }
      }
      else{
        //Read sensitivity
        printf("Reading sensitivity matrix...\n");
        int senserr = readSensitivity(sensfilename, nvox, dvox, s);
                
        if(senserr != 0)
        {
        printf("Error reading sensitivity matrix: %d \n",senserr);
        return -9;
        }
      }
      
    //Obtain nLORs of file to reconstruct 
    nLORs = countLines(LORsFilename);
    nLORs = nLORs/2;
      
  }


  printf("Done!\n");
  fflush(stdout);
    
  //****************************//
  //*     Allocate memory      *//
  //****************************//

  
  if(nLORs == 0)
    {
      printf("Error: No LORs in the specified file %s.\n",LORsFilename);
      return -4;
    }

  //Allocate memory for LOR buffer
  LORbuffer = (LORstruct*) malloc(sizeof(LORstruct)*nLORs);
  if(LORbuffer == 0)
    {
      printf("Error: Can't allocate memory for %d LORs.\n",nLORs);
      return -4;
    }    
  
  unsigned tnvox = nvox[0]*nvox[1]*nvox[2];
  //Allocate memory for image
  image = (double*) malloc(sizeof(double)*tnvox);
  for(unsigned i = 0; i < tnvox; i++)
    image[i] = 1.0;
  //Allocate memory for projection arays considering
  //the number of voxels in each axis
  int projBufferSize = 4*sqrt( pow(nvox[0],2) + pow(nvox[1],2) + pow(nvox[2],2));
  
  voxIndexes = (unsigned long*) malloc(sizeof(unsigned long)*projBufferSize*nThreads);
  voxWeights = (double*)        malloc(sizeof(double)*projBufferSize*nThreads);

  //Allocate memory for correction matrix
  c   = (double*) calloc(tnvox,sizeof(double));
  Hn  = (double*) calloc(tnvox,sizeof(double));
  
  //****************************//
  //* Read all LORs file       *//
  //****************************//
  
  //Open LORs file
  FILE* fLOR = 0;
  fLOR = fopen(LORsFilename,"r");
  if(fLOR == 0)
    {
      printf("Error: Can't open LORs file %s\n",LORsFilename);
      return -8;
    }
    
  //Read all LORs
  int nread = readLORs(fLOR, LORbuffer, nLORs, LORformat);
 
  
  if(nread != nLORs)
  {
      printf("Error: Number of readed LORs (%d) don't match with number of LORs used\n       to construct sensitivity matrix (%d).\n",nread,nLORs);
      return -9;      
  }
  
  fclose(fLOR);
  
  //****************************//
  //* Reconstruction algorithm *//
  //****************************//

   sConv  = (double*) calloc(tnvox,sizeof(double));
  
  printf("Reconstructing the image...\n");
  fflush(stdout);
  
  //Calculate chunk size
  chunkSize = nLORs/nChunks;

  printf("Number of LORs in a single chunk: %d\n",chunkSize);

  for(int iter = 0; iter < nIterations; iter++)
  {
    printf("Iteration %d...\n",iter);
    //Iterate for every chunk
    for(int i = 0; i < nChunks; i++)
        {
            printf("Processing chunk %d...\n",i);

            int firstChunkLOR = chunkSize*i;
            
            //Convolute actual image with rho
            convObj.convolute(image, Hn, nvox, rho, 1);

            //Init matrix c
            for(unsigned int j = 0; j < tnvox; j++)
            c[j] = 0.0;

            //Begin parallel region
        #pragma omp parallel num_threads(nThreads)
            {
                
            #ifdef _RECONSTRUCT_OMP_
                //Each thread must take his index
                int tid = omp_get_thread_num();
            #else
                //No multi-threading, his id must be 0
                int tid = 0;
            #endif
                //Take the pointer to respective array chunks
                unsigned long* pvoxIndexes = &voxIndexes[projBufferSize*tid];
                double* pvoxWeights = &voxWeights[projBufferSize*tid];
                
                //Project each read LOR
            #pragma omp for
                for(int j = 0; j < chunkSize; j++)
                {
                    double l;
                    unsigned nCrossed = pProj->project(TOFres, pvoxIndexes, pvoxWeights, LORbuffer[firstChunkLOR+j],l);

                    //Backproject the inverse of the forward prjection 
                    BPiFP(pvoxIndexes,pvoxWeights,nCrossed,c,Hn);
                    
                }
            }

            //Convolute resulting c with rho
            convObj.convolute(c, Hn, nvox, rho, 1);
                
            //Convolute resulting c with smooth kernel
            
            if(kernelDim > 0)
            {
                convObj.convolute(Hn, c, nvox, smoothKernel, smoothDim);
                //convObj.convolute(s, sConv, nvox, smoothKernel, smoothDim);
            }
            else
            {
                //Change c pointer by Hn pointer
                double* paux = c;
                c = Hn;
                Hn = paux;
                paux = 0;	  
            }
            
          /*  //Find maximum value in s
            double max = 0.0;
            for(unsigned k = 0; k < tnvox; k++)
            {
                if(sConv[k] > max)
                {
                    max = sConv[k];
                    
                }
            }
            
            //Set threshold value to avoid diveregences
            double threshold = (1.0/max)*0.5;
            
            for(unsigned k = 0; k < tnvox; k++)
            {
                if(sConv[k] > threshold)
                {
                    sConv[k] = sConv[k];
                }
                else
                sConv[k] = 0.0;
            }
             
            for(unsigned k = 0; k < tnvox; k++)
            {
                s[k] = sConv[k];
            }
            */        
            //Perform element by element product
            nextImage(image, s, c, tnvox);
        
            printf("Done!\n");
            fflush(stdout);      
        }
  }
  printf("Reconstruction done!\n");
  fflush(stdout);
  
   //***************************//
  //*         Normalize        *//
  //****************************//
  
  //imageNormalize(image, nvox, image);
  FILE* completeOut = NULL;
  completeOut = fopen("normalizedCompleteData.txt", "w");

  if(completeOut == NULL){
    printf("Error: Unable to open results file 'normalizedCompleteData.txt'\n");
    return -10;
  }
  
  fprintf(completeOut, "#Voxel   IntensityNorm\n");
  for(unsigned i = 0; i < tnvox; i++)
    fprintf(completeOut,"%i   %le\n", i, image[i]/double(nLORs));

  fclose(completeOut);

  //****************************//
  //*      Print results       *//
  //****************************//
  /*
  printf("Printing results...\n");
  FILE* out = 0;
  
  //Print Results ordered by plane Z
  out = fopen("resultXY.mat","w");
  printMatrixXY(out, nvox, dvox, image);
  fclose(out);
  
  //Print Results ordered by plane X
  out = fopen("resultYZ.mat","w");
  printMatrixYZ(out, nvox, dvox, image);
  fclose(out);
  
  //Print Results ordered by plane Y
  out = fopen("resultXZ.mat","w");
  printMatrixXZ(out, nvox, dvox, image);
  fclose(out);

  printf("Done!\n");
  fflush(stdout);
  */
  
  //****************************//
  //*          Clear           *//
  //****************************//

  printf("Clear memory...\n");
  fflush(stdout);
  
  //Free projector
  pProj->clean();
  delete pProj;
  //Free buffers
  free(voxWeights);
  
  free(voxIndexes);

  free(image);

  free(s);
  
  free(c);

  free(Hn);

  if(kernelDim > 0)
    free(smoothKernel);
  free(rho);

  free(LORbuffer);
  
  printf("Done!\n");
  fflush(stdout);
  return 0;
}
