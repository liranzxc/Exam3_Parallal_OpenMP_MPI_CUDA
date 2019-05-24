#define HEAVY 10000
#define MASTER 0 
#define SLAVE 1

#include "cuda_runtime.h"
#include "device_launch_parameters.h"
#include <math.h>
#include <stdio.h>
#include "mpi.h"
#include <omp.h>
#include <stdlib.h>

cudaError_t CounterCuda(int * ArrNumbers, int n, int myid, int * results);


int ReadFromFile(char * file_name, int ** ArrNumbers);
void WorkNormally(int n, int * ArrNumbers);
double f(int i);