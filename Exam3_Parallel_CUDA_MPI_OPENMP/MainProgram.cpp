#include "MainProgram.h"
#include "cuda_runtime.h"
#include "device_launch_parameters.h"
#include <math.h>
#include <stdio.h>
#include "mpi.h"
#include <omp.h>
#include <stdlib.h>
#define HEAVY 10000


cudaError_t CounterCuda(int * ArrNumbers,int n, int myid,int * results);


int ReadFromFile(char * file_name, int ** ArrNumbers);
void WorkNormally(int n , int * ArrNumbers);
double f(int i);

int main(int argc, char *argv[]) {

	int myid, numprocs;
	MPI_Status status;

	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
	MPI_Comm_rank(MPI_COMM_WORLD, &myid);

	int n;
	int * ArrNumbers = NULL;


	if (myid == 0) // master
	{
		char * file_name = "D://data.txt";
		if ( n = ReadFromFile(file_name, &ArrNumbers))
		{

			//Sending half array  to process 1 
			MPI_Send(&n, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);

			MPI_Send(ArrNumbers, n/2 , MPI_INT, 1, 0, MPI_COMM_WORLD);


			// process 1 contains the first half of array

			// process 0 second half array 


		}

	}
	else
	{
		if (myid == 1)
		{
			MPI_Recv(&n, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);

			ArrNumbers = (int*)malloc(sizeof(int) * n / 2 );

			MPI_Recv(ArrNumbers, n/2, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);

			//  p1 first half array 
		}
	}
	
	double s_time = MPI_Wtime();

	int  * results = (int*)malloc(sizeof(int) * n / 2); // create results 

	cudaError_t cudaStatus = CounterCuda(ArrNumbers, n, myid,results);
	if (cudaStatus != cudaSuccess) {
		fprintf(stderr, "CounterCuda failed!");
		return 1;
	}

	int sum = 0;
	#pragma omp parallel for shared(results) reduction(+: sum)
	for (auto i = 0; i < n / 2 ; i++)
	{
		sum += results[i];
	}



	// p1 send to p0 results 

	if (myid == 0)
	{
		int sumP1 = 0;
		MPI_Recv(&sumP1, 1, MPI_INT, 1, 0, MPI_COMM_WORLD, &status);

		double end_time = MPI_Wtime();

		printf("Results :  %d \n", sum+sumP1);
		printf("Time is  :  %f \n", end_time-s_time);

		s_time = MPI_Wtime();
		WorkNormally(n, ArrNumbers);
		end_time = MPI_Wtime();

		printf("Time seq is  :  %f \n", end_time - s_time);


	}
	else if (myid == 1)
	{
		MPI_Send(&sum, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);

	}
	

	MPI_Finalize();

	free(ArrNumbers);
	free(results);

	return 0;
}


void WorkNormally(int n, int * ArrNumbers)
{
	int counter = 0;
	#pragma omp parallel for shared(ArrNumbers) reduction(+: counter)
	for (auto i = 0; i < n; i++)
	{
		if (f(ArrNumbers[i]) > 0)
		{
			counter++;
		}
		
	}

	printf("Seq : counter is %d , number of postive in arr \n ", counter);
}
double f(int i) {
	int j;
	double value;
	double result = 0;

	for (j = 1; j < HEAVY; j++) {
		value = (i + 1)*(j % 10);
		result += cos(value);
	}
	return cos(result);
}

int ReadFromFile(char * file_name, int ** ArrNumbers)
{

	FILE* file = fopen(file_name, "r");
	int n = 0;

	fscanf(file, "%d", &n);

	if (n == 0)
	{
		printf("must be more than zero number in file");
		return 0;
	}

	 *ArrNumbers = (int*)malloc(sizeof(int) * n);


	int indexer = 0;
	while (!feof(file))
	{
		fscanf(file, "%d", &(*ArrNumbers)[indexer]);
		indexer++;
	}
	fclose(file);

	return n;
}