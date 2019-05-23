
#include "cuda_runtime.h"
#include "device_launch_parameters.h"
#include <math.h>
#include <stdio.h>
#define HEAVY 10000

cudaError_t CounterCuda(int * ArrNumbers, int n, int myid, int * results);
void FreeMethod(int * arr, int * result);


__device__ double f_gpu(int i) {
	int j;
	double value;
	double result = 0;

	for (j = 1; j < HEAVY; j++) {
		value = (i + 1)*(j % 10);
		result += cos(value);
	}
	return cos(result);
}

__global__ void CounterKernel(int *Arr_dev,  int * results_dev)
{
	int i = blockIdx.x * 1024 +  threadIdx.x;
	
		if (f_gpu(Arr_dev[i]) > 0)
		{
			results_dev[i] = 1;
		}
		else
		{
			results_dev[i] = 0;
		}
	

	
}



// Helper function for using CUDA to add vectors in parallel.

cudaError_t CounterCuda(int * ArrNumbers, int n, int myid, int * results)
{
	int *dev_ArrNumbers = 0;
	int *dev_results = 0;
	cudaError_t cudaStatus;

	// Choose which GPU to run on, change this on a multi-GPU system.
	cudaStatus = cudaSetDevice(0);
	if (cudaStatus != cudaSuccess) {
		fprintf(stderr, "cudaSetDevice failed!  Do you have a CUDA-capable GPU installed?");
		FreeMethod(dev_ArrNumbers, dev_results);
		return cudaStatus;

	}


	// Allocate GPU buffers for three vectors (two input, one output)    .
	cudaStatus = cudaMalloc((void**)&dev_ArrNumbers, (n/2) * sizeof(int));
	if (cudaStatus != cudaSuccess) {
		fprintf(stderr, "cudaMalloc failed!");
		FreeMethod(dev_ArrNumbers, dev_results);
		return cudaStatus;
	}

	cudaStatus = cudaMalloc((void**)&dev_results, (n/2) * sizeof(int));
	if (cudaStatus != cudaSuccess) {
		fprintf(stderr, "cudaMalloc failed!");
		FreeMethod(dev_ArrNumbers, dev_results);
		return cudaStatus;
	}


	// process 1 contains the first half of array

	// process 0 second half array 

	if (myid == 0)
	{											// second half
		cudaStatus = cudaMemcpy(dev_ArrNumbers, ArrNumbers + (n/2), (n / 2) * sizeof(int), cudaMemcpyHostToDevice);
		if (cudaStatus != cudaSuccess) {
			fprintf(stderr, "cudaMemcpy failed!");
			FreeMethod(dev_ArrNumbers, dev_results);
			return cudaStatus;
		}
	}
	else
	{
		if (myid == 1)
		{											// first half
			cudaStatus = cudaMemcpy(dev_ArrNumbers, ArrNumbers, (n / 2) * sizeof(int), cudaMemcpyHostToDevice);
			if (cudaStatus != cudaSuccess) {
				fprintf(stderr, "cudaMemcpy failed!");
				FreeMethod(dev_ArrNumbers, dev_results);
				return cudaStatus;
			}
		}
	}

	int dim_blocks = ((n/2) / 1024) + 1; // 49

	CounterKernel <<< dim_blocks , 1024 >>>(dev_ArrNumbers,dev_results);

	// Copy input vectors from host memory to GPU buffers.

	// Check for any errors launching the kernel
	cudaStatus = cudaGetLastError();
	if (cudaStatus != cudaSuccess) {
		fprintf(stderr, "CounterKernel launch failed: %s\n", cudaGetErrorString(cudaStatus));
		FreeMethod(dev_ArrNumbers, dev_results);
		return cudaStatus;
	}

	// cudaDeviceSynchronize waits for the kernel to finish, and returns
	// any errors encountered during the launch.
	cudaStatus = cudaDeviceSynchronize();
	if (cudaStatus != cudaSuccess) {
		fprintf(stderr, "cudaDeviceSynchronize returned error code %d after launching addKernel!\n", cudaStatus);
		FreeMethod(dev_ArrNumbers, dev_results);
		return cudaStatus;
	}

	// Copy output vector from GPU buffer to host memory.
	cudaStatus = cudaMemcpy(results, dev_results, (n/2) * sizeof(int), cudaMemcpyDeviceToHost);
	if (cudaStatus != cudaSuccess) {
		fprintf(stderr, "cudaMemcpy failed!");
		FreeMethod(dev_ArrNumbers, dev_results);
		return cudaStatus;
	}
	

	FreeMethod(dev_ArrNumbers, dev_results);

	return cudaStatus;



}


void FreeMethod(int * arr, int * result)
{
	cudaFree(arr);
	cudaFree(result);
}
