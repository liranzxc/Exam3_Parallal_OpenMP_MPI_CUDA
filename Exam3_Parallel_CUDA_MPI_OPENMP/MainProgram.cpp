#include "MainProgram.h"



int main(int argc, char *argv[]) {

	int myid, numprocs;
	MPI_Status status;

	omp_set_nested(1); // nesting omp turn on
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
	MPI_Comm_rank(MPI_COMM_WORLD, &myid);

	int n;
	int * ArrNumbers = NULL;
	cudaError_t cudaStatus;


	if (numprocs != 2)
	{
		printf("Program must be two processes ,please set to 2 ");
		MPI_Finalize();
		return 1;
	}


	if (myid == MASTER) // master
	{
		char * file_name = argv[1];

		if (n = ReadFromFile(file_name, &ArrNumbers)) // reading from file
		{
			// sending N number to p1 
			//Sending half array  to  p1 
			MPI_Send(&n, 1, MPI_INT, SLAVE, 0, MPI_COMM_WORLD);
			MPI_Send(ArrNumbers, n / 2, MPI_INT, SLAVE, 0, MPI_COMM_WORLD);
		}



	}
	else if (myid == SLAVE)
	{

		MPI_Recv(&n, 1, MPI_INT, MASTER, 0, MPI_COMM_WORLD, &status);

		ArrNumbers = (int*)malloc(sizeof(int) * (n / 2)); // slave have N/2 array of number

		MPI_Recv(ArrNumbers, n / 2, MPI_INT, MASTER, 0, MPI_COMM_WORLD, &status);

		//  p1 first half array 


	}



	//status 
	// p1 works on first half of array
	// p0 works on second half of array
	double s_time = MPI_Wtime();

	int  * results_Cuda_quarter = (int*)malloc(sizeof(int) * (n / 4)); // create results array n/4 Cuda
	int  * results_OpenMP_quarter = (int*)malloc(sizeof(int) * (n / 4)); // create results array n/4 openMP




#pragma omp parallel 
	{
		#pragma omp sections // for have two section . Cuda and OpenMP in parallel
		{
			#pragma omp section
			{
				//call cuda n / 4 
				cudaStatus = CounterCuda(ArrNumbers, n, myid, results_Cuda_quarter);
			}

			#pragma omp section
			{
				// call openMP n / 4 
				OpenMPCounter(myid, n, ArrNumbers, results_OpenMP_quarter);
			}
		}
	}

	// check cuda status
	if (cudaStatus != cudaSuccess) {
		fprintf(stderr, "CounterCuda failed!");
		MPI_Finalize();
		return 1;

	}

	// each process counter the array result parallel by openMP

	int final_sum = 0;
#pragma omp parallel for shared( results_Cuda_quarter,results_OpenMP_quarter) reduction(+: final_sum)
	for (int i = 0; i < n / 4; i++)
	{

		final_sum += results_Cuda_quarter[i] + results_OpenMP_quarter[i];


	}

	//final stage - sum all results from processes , messaging data
	// p1 send to p0 results 

	if (myid == MASTER)
	{
		// master collect result from p1 
		int sumP1 = 0;
		MPI_Recv(&sumP1, 1, MPI_INT, SLAVE, 0, MPI_COMM_WORLD, &status);

		double end_time = MPI_Wtime();

		printf("Number of positive in arr by f function  :  %d \n", final_sum + sumP1);
		printf("Time is  :  %f \n", end_time - s_time);

		/*	 //Seq soultion for testing
			s_time = MPI_Wtime();
			WorkNormally(n, ArrNumbers);
			end_time = MPI_Wtime();
			printf("Time seq is  :  %f \n", end_time - s_time);
	*/

	}
	else if (myid == SLAVE)
	{
		// p1 send result to p0 
		MPI_Send(&final_sum, 1, MPI_INT, MASTER, 0, MPI_COMM_WORLD);

	}

	//all processes do finalize
	MPI_Finalize();

	// free arrays
	free(ArrNumbers);
	free(results_Cuda_quarter);
	free(results_OpenMP_quarter);

	return 0;
}


void WorkNormally(int n, int * ArrNumbers)
{
	int counter = 0;
#pragma omp parallel for shared(ArrNumbers) reduction(+: counter)
	for (int i = 0; i < n; i++)
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


void OpenMPCounter(int myid, int n, int * ArrNumbers, int * results_OpenMP_quarter)
{
	int start, end, indexer;

	if (myid == MASTER) //p0
	{
		start = (3 * n) / 4;
		end = n - 1;
	}
	else if (myid == SLAVE) // p1
	{
		start = n / 4;
		end = (n / 2) - 1;
	}


	indexer = 0;
	for (int i = start; i <= end; i++)
	{
		if (f(ArrNumbers[i]) > 0)
		{
			results_OpenMP_quarter[indexer] = 1;
		}
		else
		{
			results_OpenMP_quarter[indexer] = 0;
		}
		++indexer;
	}

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