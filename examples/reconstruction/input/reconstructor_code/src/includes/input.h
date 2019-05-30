 
#ifndef __RECONSTRUCTION__INPUT
#define __RECONSTRUCTION__INPUT

#include "projectors.h"
#include <cstring>

int parseArguments(int, char**, char*&, char*&, projector*&, int& , int*, double*, int&, int&, int&, float&, char*&, int&);
int readMovFile(const char*, int&, float&, float*& , float*);
int readLORs(FILE*, LORstruct*, int, int);
int countLines(const char*);
int readSensitivity(const char* filename, int* nvox, double* dvox, double*& s);

#endif
