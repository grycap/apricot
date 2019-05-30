 
#include "input.h"

//Create file formats
static const char* format0 = " %*d %*d %e %e %e %*e %le %*d %*d %e %e %e %*e %le"; //PENELOPE format
static const char* format1 = " %e %e %e %e %e %e %*d %*d "; //Victor format without time
static const char* format2 = " %*d %*d %e %e %e %*e %le %*d %*d %e %e %e %*e %le %e"; //PENELOPE format with w factor to create sensitivity image

int parseArguments(int argc, char** argv, char*& LORsFilename, char*& movimentFilename, projector*& pProj,
		   int &LORformat, int* nvox, double* dvox, int& nChunks, int& kernelDim, int& nThreads, float& TOFres,
           char*& sensfilename, int& nIterations)
{
  //This function parse the input program arguments and store the corresponding
  //values.

  // return  0 on succes
  // return <0 on failure and print the corresponding error missage


  for(int i = 1; i < argc; i++)
    {
      if(strcmp(argv[i],"--help") == 0 || strcmp(argv[i],"-h") == 0)
	{
	  //Display help and exit

	  printf("Reconstruction program usage:\n");
	  printf("\t%s LORs-file\n",argv[0]);

	  printf("Configuration options:\n");
	  printf("\t\t%20s\tSpecifies filename with moviment data\n","--mov");
	  printf("\t\t%20s\tSpecifies the LOR file format\n","--LOR-format");
	  printf("\t\t%20c\t 0 -> (module1 crystal1 x1 y1 z1 E1 t1 module2 crystal2 x2 y2 z2 E2 t2)\n",' ');
	  printf("\t\t%20s\tSpecifies the projector to use, valid projectors are:\n","--projector");
	  printf("\t\t%20c\t%5c %15s\n",' ',' ',"jacob");
	  printf("\t\t%20s\tSpecifies the number of voxels in each axis nx ny nz\n","--nvox");
	  printf("\t\t%20s\tSpecifies the dimension of voxels in each axis dx dy dz\n","--dvox");
	  printf("\t\t%20s\tSpecifies the number of LOR chunks\n","--nChunks");
	  printf("\t\t%20s\tSpecifies the size (dim) of smooth kernel (dim x dim x dim)\n","--kdim");
	  printf("\t\t%20s\tSpecifies the number of threads to uses with openMP\n","--nthreads");
	  printf("\t\t%20s\tSpecifies TOF resolution. Set it negative to disable TOF (default option).\n","--TOF");  
	  printf("\t\t%20s\tSpecifies the number iterations with all data\n","--niter");
	  printf("\t\t%20s\tSpecifies the sensitivity image filename\n","--sensitivity");
	  return 1;
	}
	      else if(strcmp(argv[i],"--sensitivity") == 0)
	{
	  //sensitivity filename specified
	  if(i > argc-2)
	    {
	      //No more arguments, print an error
	      printf("Error: '--sensitivity' option enabled but no name specified\n");
	      return -11;
	    }

        sensfilename = argv[i+1];
	  //Skip niterations argument
	  i += 1;        
    }	
	      else if(strcmp(argv[i],"--niter") == 0)
	{
	  //Number of iterations specified
	  if(i > argc-2)
	    {
	      //No more arguments, print an error
	      printf("Error: '--niter' option enabled but no number specified\n");
	      return -10;
	    }

	  sscanf(argv[i+1]," %d",&nIterations);	  

	  //Skip niterations argument
	  i += 1;        
    }
      else if(strcmp(argv[i],"--mov") == 0)
	{
	  //Check if has been already introduced
	  if(movimentFilename != 0)
	    {
	      //Moviment filename introduced twice
	      printf("Error: Moviment filename introduced twice (--mov)\n");
	      return -2;
	    }
	  
	  //Moviment file specified
	  if(i >= argc-1)
	    {
	      //No more arguments, print an error
	      printf("Error: '--mov' option enabled but no moviment file specified\n");
	      return -2;
	    }
	  //Store pointer to moviment filename
	  movimentFilename = argv[i+1];
	  i++; //skip moviment filename argument
	}
      else if(strcmp(argv[i],"--LOR-format") == 0)
	{
	  //Check if has been already introduced
	  if(LORformat >= 0)
	    {
	      //Moviment filename introduced twice
	      printf("Error: LOR format introduced twice (--LOR-format)\n");
	      return -3;
	    }
	  
	  //LOR format file specified
	  if(i >= argc-1)
	    {
	      //No more arguments, print an error
	      printf("Error: '--LOR-format' option enabled but no format number specified\n");
	      return -3;
	    }
	  sscanf(argv[i+1]," %d",&LORformat);
	  
	  i++; //Skip format number argument
	}
      else if(strcmp(argv[i],"--projector") == 0)
	{
	  //Check if has been already introduced
	  if(pProj != 0)
	    {
	      //Projector introduced twice
	      printf("Error: Projector introduced twice (--projector)\n");
	      return -4;
	    }

	  //Projector specified
	  if(i >= argc-1)
	    {
	      //No more arguments, print an error
	      printf("Error: '--projector' option enabled but no name specified\n");
	      return -4;
	    }

	  if(strcmp(argv[i+1],"jacobs") == 0)
	    {
	      pProj = new jacobs;
	    }

	    i++; //Skip projector name
	}
      else if(strcmp(argv[i],"--nvox") == 0)
	{
	  //Number of voxels specified
	  if(i > argc-4)
	    {
	      //No more arguments, print an error
	      printf("Error: '--nvox' option enabled but no number specified\n");
	      return -5;
	    }

	  sscanf(argv[i+1]," %d",&nvox[0]);	  
	  sscanf(argv[i+2]," %d",&nvox[1]);	  
	  sscanf(argv[i+3]," %d",&nvox[2]);

	  //Skip nx,ny,nz arguments
	  i += 3;
	  
	}
      else if(strcmp(argv[i],"--dvox") == 0)
	{
	  //Size of voxels specified
	  if(i > argc-4)
	    {
	      //No more arguments, print an error
	      printf("Error: '--dvox' option enabled but no size specified\n");
	      return -5;
	    }

	  sscanf(argv[i+1]," %lf",&dvox[0]);	  
	  sscanf(argv[i+2]," %lf",&dvox[1]);	  
	  sscanf(argv[i+3]," %lf",&dvox[2]);

	  //Skip dx,dy,dz arguments
	  i += 3;
	  
	}
      else if(strcmp(argv[i],"--nChunks") == 0)
	{
	  //Number of chunks specified
	  if(i > argc-1)
	    {
	      //No more arguments, print an error
	      printf("Error: '--nChunks' option enabled but no number specified\n");
	      return -7;
	    }

	  sscanf(argv[i+1]," %d",&nChunks);	  

	  //Skip nChunks arguments
	  i++;
	  
	}
      else if(strcmp(argv[i],"--kdim") == 0)
	{
	  //Number of chunks specified
	  if(i > argc-1)
	    {
	      //No more arguments, print an error
	      printf("Error: '--kdim' option enabled but no number specified\n");
	      return -7;
	    }

	  sscanf(argv[i+1]," %d",&kernelDim);	  

	  //Skip nChunks arguments
	  i++;
	  
	}
      else if(strcmp(argv[i],"--nthreads") == 0)
	{
	  //Number of threads specified
	  if(i > argc-1)
	    {
	      //No more arguments, print an error
	      printf("Error: '--nthreads' option enabled but no number specified\n");
	      return -8;
	    }

	  sscanf(argv[i+1]," %d",&nThreads);	  

	  //Skip nChunks arguments
	  i++;
	}
      else if(strcmp(argv[i],"--TOF") == 0)
	{
	  //Number of threads specified
	  if(i > argc-1)
	    {
	      //No more arguments, print an error
	      printf("Error: '--TOF' option enabled but no number specified\n");
	      return -9;
	    }

	  sscanf(argv[i+1]," %f",&TOFres);	  

	  //Skip TOF arguments
	  i++;
	}      
      else
	{
	  //Lors filename specified
	  LORsFilename = argv[i];
	}
    }
  return 0;
}

int readMovFile(const char* fname, int& nbins, float& dbin, float*& out, float* initPos)
{

  // This function read and process moviment information from
  // specified filename 'fname'.
  //
  // Input data must have a header with the format,
  //
  //   nbins(int) dbin(float)
  //
  // followed by data tuples with the format,
  //
  // x,y,z,alpha,beta,gamma
  //
  // where all are 'double' variables. x,y,z
  // specifies the position at this bin. Alpha
  // beta and gamma specifies the rotation at
  // this point,
  //
  // R(alpha,beta,gamma) = Rz(gamma)Ry(beta)Rz(alpha)
  //
  // return  0 on succes
  // return -1 if can't open specified file
  // return -2 if can't allocate memory
  // return -3 on reading data error
  //
  
  FILE* fmov = 0;
  fmov = fopen(fname,"rb");
  if(fmov == 0)
    {
      return -1;
    }

  //Read number of bins
  fread(&nbins,sizeof(int),1,fmov);
  //Read bin size
  fread(&dbin,sizeof(float),1,fmov);

  //Allocate memory
  double* rawMov = 0;
  rawMov = (double*) malloc(sizeof(double)*nbins*6);
  if(rawMov == 0){return -2;}
  
  //Read raw data
  int nread = fread(rawMov,sizeof(double),6*nbins,fmov);
  if(nread != 6*nbins){return -3;}

  //Close file
  fclose(fmov);

  //Create moviment matrix
  int err = createMovMatrix(rawMov, out, nbins, initPos);
  if(err != 0){return -2;}

  //Free memory
  free(rawMov);
  
  return 0;
}

int readLORs(FILE* fin, LORstruct* lors, int nread, int format)
{
  // Read as maximum 'nread' LORs from file 'fin'
  // and store it in 'lors' array.
  //
  // return the number of readed LORs on succes
  // return -1 if invalid format has been specified
  //

  const char* pformat = 0;
  if(format == 0)
    {
      pformat = format0;
    }
  else if(format == 2)
    {
      pformat = format2;
    }
  else if(format == 1)
    {
      pformat = format1;
    }
  else
    {
      //Invalid format
      return -1;
    }

  //Check end of file
  if(feof(fin)){return 0;}
  
  //Read LORs

  int i = 0;
  while(i < nread)
    {
      if(format == 0) //to reconstruct an image. Input is a coincidendes file of specific simulation
	{
	  lors[i].w = 1.0;
	  if(fscanf(fin,pformat,
		    &lors[i].in[0] ,&lors[i].in[1] ,&lors[i].in[2] ,&lors[i].tin,
		    &lors[i].out[0],&lors[i].out[1],&lors[i].out[2],&lors[i].tout) != 8)
	    {
	      break;
	    }
	}
      else if(format == 2) //to create sensitivity matrix. input is a phantom simulation with w values
	{
	  
	  if(fscanf(fin,pformat,  
		    &lors[i].in[0] ,&lors[i].in[1] ,&lors[i].in[2] ,&lors[i].tin,
		    &lors[i].out[0],&lors[i].out[1],&lors[i].out[2],&lors[i].tout, &lors[i].w) != 9)
	    {
	      break;
	    }
	}
      else if(format == 1)
	{
	  lors[i].w = 1.0;
	  lors[i].tin = 0.0;
	  lors[i].tout = 0.0;
	  if(fscanf(fin,pformat,
		    &lors[i].in[0],&lors[i].in[1],&lors[i].in[2],
		    &lors[i].out[0],&lors[i].out[1],&lors[i].out[2]) != 6)
	    {
	      break;
	    }
	}
      i++;
    }

  return i;
}

int countLines(const char* filename)
{
  //Count number of lines in file specified
  //by filename

    unsigned int number_of_lines = 0;
    int ch = 'a';
    FILE* infile = 0;
    infile = fopen(filename, "r");
    if(infile == 0)
      return -1;
    

    while (EOF != (ch=getc(infile)))
      if ('\n' == ch)
	++number_of_lines;
    return number_of_lines;  
}

int readSensitivity(const char* filename, int* nvox, double* dvox, double*& s)
{
    
    FILE* fin = 0;
    fin = fopen(filename,"rb");
    if(fin == 0)
        return -1;

    int sensVox[3];
    double sensdVox[3];
    fread(sensVox,sizeof(int),3,fin);
    fread(sensdVox,sizeof(double),3,fin);
    
    //Check number of voxels in each axis match 
    if(sensVox[0] != nvox[0] || sensVox[1] != nvox[1] || sensVox[2] != nvox[2])
    {
        fclose(fin);
        return -2;
    }
    //Check size of voxels in each axis match 
    if(sensdVox[0] != dvox[0] || sensdVox[1] != dvox[1] || sensdVox[2] != dvox[2])
    {
        fclose(fin);
        return -3;    
    }  
    
    if(s != 0)
    {
        free(s);
        s = 0;
    }
    s = (double*) calloc(nvox[0]*nvox[1]*nvox[2],sizeof(double));
    
    int nread = fread(s,sizeof(double),nvox[0]*nvox[1]*nvox[2],fin);
    
    fclose(fin);
    
    if(nread != nvox[0]*nvox[1]*nvox[2])
        return -4;
    
    return 0;
}
