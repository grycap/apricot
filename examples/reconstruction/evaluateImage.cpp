#include<stdio.h>
#include<stdlib.h>
#include<cmath>

using namespace std;

int main(int argc, char* argv[]){

   if(argc < 8)
    {
      printf("usage : %s reconstructedFile nvoxX nvoxY nvoxZ dx dy dz\n",argv[0]);
      return -1;
    }

   //Extract argument values
   char* reconstructedImage = argv[1];
   unsigned nvox[3];
   nvox[0] = atoi(argv[2]);
   nvox[1] = atoi(argv[3]);
   nvox[2] = atoi(argv[4]);
   unsigned nxy = nvox[0] * nvox[1];
   unsigned tnvox = nxy * nvox[2]; //Number of voxels  

   double dvox[3];
   dvox[0] = atof(argv[5]);
   dvox[1] = atof(argv[6]);
   dvox[2] = atof(argv[7]);

   //Translation from FOV's center
   double halfFOV[3];
   halfFOV[0] = (double)(nvox[0])*0.5*dvox[0];
   halfFOV[1] = (double)(nvox[1])*0.5*dvox[1];
   halfFOV[2] = (double)(nvox[2])*0.5*dvox[2];
       
  FILE* fin = NULL;
  fin = fopen(reconstructedImage, "r"); //reconstructed file
  
  if(fin == NULL)
    {
      printf("Error opening input files\n");
    }  
  
  
  if(tnvox <= 0)
  {
      printf("ERROR: Number of voxels must be positive\n");
      return 1;
  }


  //Cylinder dimensions (in cm)
  double base = -6.0 + halfFOV[2]; 
  double height = 6.0 + halfFOV[2];
  double radio = 5.0;
  double xCenter = 0.0 + halfFOV[0];
  double yCenter = 0.0 + halfFOV[1];
  double zCenter = 0.0 + halfFOV[2];
    
  double centerDistance;
    
    
  //Spheres dimensions (in cm)
  double radSphere1 = 0.25;
  double radSphere2 = 0.25; 
  double radSphere3 = 0.25;
  double radSphere4 = 0.25;
    
  double sphere2xshift = 3.0; //this spehre is located at 3 cm (x direction) of the center of the FOV
  double sphere3yshift = - 3.0; //this spehre is located at -3 cm (y direction) of the center of the FOV
  double sphere4yshift = 3.0; //this sphere is located at 3 cm (y direction) of the center of the halfFOV
    
  double sph1Distance;
  double sph2Distance;
  double sph3Distance;
  double sph4Distance;  
    
  double umbral = 0.0;
  
  double* idealImage = 0;
  double* imageDiff = 0;
  
  idealImage  = (double*) calloc(tnvox,sizeof(double));
  imageDiff    = (double*) calloc(tnvox,sizeof(double));

  int voxIndex = 0;
  double idealCummIntensity = 0.0;
  
  
  double idealIntensityNorm, recIntensityNorm;
  double sumRMSE = 0.0;
  double sumTrue = 0.0;
  double sumNMAD = 0.0;
  double sumNRMSD = 0.0;
  double maxIdeal = 0.0;
  double meanIdeal = 0.0;
  
  int roiVox = 0;
    
  fscanf(fin, "%*[^\n]");
  getc(fin);

  printf("Compare images...\n");
    for(int k = 0; k < nvox[2]; k++)
    {
        for(int j = 0; j < nvox[1]; j++)
        {
            for(int i = 0; i < nvox[0]; i++)
            {
	      double pos[3];
                pos[0] = i*dvox[0];
                pos[1] = j*dvox[1];
                pos[2] = k*dvox[2];
                
                voxIndex = i + j*nvox[0] + k*nxy; //voxel position
                
                if(pos[2] >= base && pos[2] <= height)
                {
                    centerDistance = sqrt(pow((pos[1] - yCenter),2) + pow((pos[0] - xCenter),2));
                    sph1Distance = sqrt(pow((pos[0] - xCenter),2) + pow((pos[1] - yCenter),2) + pow((pos[2] - zCenter),2));
                    sph2Distance = sqrt(pow((pos[0] - (xCenter+sphere2xshift)),2) + pow((pos[1] - yCenter),2) + pow((pos[2] - zCenter),2));
                    sph3Distance = sqrt(pow((pos[0] - xCenter),2) + pow((pos[1] - (yCenter+sphere3yshift)),2) + pow((pos[2] - zCenter),2));
                    sph4Distance = sqrt(pow((pos[0] - xCenter),2) + pow((pos[1] - (yCenter+sphere4yshift)),2) + pow((pos[2] - zCenter),2));
                    
                    
                    if(centerDistance <= radio)
                    {
                        //this position is located inside the cylinder intensity = 1.0
                        idealImage[voxIndex] = 1.0;
                        
                        if(sph1Distance <= radSphere1) //sphere1
                        {
                            idealImage[voxIndex] = 8.0;
                        }
                        else if(sph2Distance <= radSphere2) //sphere2
                        {
                            idealImage[voxIndex] = 8.0;
                        }
                        else if(sph3Distance <= radSphere3) //sphere3
                        {
                            idealImage[voxIndex] = 8.0;
                        }
                        else if(sph4Distance <= radSphere4) //sphere4
                        {
                            idealImage[voxIndex] = 0.0;
                        }
                    }
                    else 
                    {
                        idealImage[voxIndex] = 0.0;
                    }
                    
                }
                else
                {
                   idealImage[voxIndex] = 0.0;
                }
		idealCummIntensity += idealImage[voxIndex];

		if(idealImage[voxIndex] > umbral){
		  roiVox++;
		  meanIdeal += idealImage[voxIndex];
		  if(idealImage[voxIndex] > maxIdeal){
		    maxIdeal = idealImage[voxIndex];
		  }
		}
            }
        }
    }

    //mean value of ideal image
    meanIdeal /= idealCummIntensity;
    meanIdeal = meanIdeal/roiVox;

    //Normalize ideal image
    for(unsigned i = 0; i < tnvox; i++){
      idealImage[i] /= idealCummIntensity;
    }
    
    //Compare images
    printf("Comparing images...\n");
    for(unsigned i = 0; i < tnvox; i++){
      fscanf(fin, "%*d   %le%*[^\n]", &recIntensityNorm);
      getc(fin);
    
      imageDiff[i] =  recIntensityNorm - idealImage[i];
    
    }
    fclose(fin);
  

    printf("Calculating quality parameters...\n");
    for(unsigned i = 0; i < tnvox; i++)
      {
	if(idealImage[i] > umbral)
	  {        
	    sumRMSE += pow((imageDiff[i]),2);
	    sumNMAD += fabs(imageDiff[i]);
	    sumTrue += fabs(idealImage[i]);
	    sumNRMSD += pow((meanIdeal - idealImage[i]),2);

	  }
      }  


  //EVALUATIONS
  
  double RMSE;
  double PSNR;
  double NRMSD;
  double NMAD;

  
  FILE* evaluation = NULL;
  evaluation = fopen("evaluationResults.txt", "w");
  
  RMSE = sqrt(sumRMSE/roiVox);
  PSNR = 10.0*log10(pow(maxIdeal,2)/pow(RMSE,2));
  NRMSD = sqrt(sumRMSE/sumNRMSD);
  NMAD = sumNMAD/sumTrue;
  
  fprintf(evaluation, "    RMSE           PSNR           NRMSD          NMAD\n");
  
  fprintf(evaluation, "%le   %le   %le   %le\n", RMSE, PSNR, NRMSD, NMAD);
  
  fclose(evaluation);

  printf("RMSE  : %le\n",RMSE);
  printf("PSNR  : %le\n",PSNR);
  printf("NRMSD : %le\n",NRMSD);
  printf("NMAD  : %le\n",NMAD);
  
  printf("Done!\n");
    
}



