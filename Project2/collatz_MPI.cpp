#include <cstdio>
#include <algorithm>
#include <sys/time.h>
#include <mpi.h>

static int collatz(const long upper,int my_rank, int comm_sz)
{
  // compute sequence lengths
  int maxlen = 0;
  //cyclic assignment of iterations
  for (int i = my_rank+1; i <= upper; i += comm_sz){
    long val = i;
    int len = 1;
    while (val != 1) {
      len++;
      if ((val % 2) == 0) {
        val = val / 2;  // even
      }
      else {
        val = 3 * val + 1;  // odd
      }
    }
    maxlen = std::max(maxlen, len);
    }
  return maxlen;
}

int main(int argc, char *argv[])
{
  //Setup
  MPI_Init(NULL, NULL);
  int comm_sz, my_rank;
  MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);
  MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
  //will reduce max of all local maxlens into this global maxlen
  int global_maxlen = 0;

  if(my_rank == 0) printf("Collatz v1.2\n");

  //command line argument check
  if (argc != 2) {fprintf(stderr, "USAGE: %s upper_bound\n", argv[0]); exit(-1);}
  const long upper = atol(argv[1]);
  if (upper < 5) {fprintf(stderr, "ERROR: upper_bound must be at least 5\n"); exit(-1);}
  if ((upper % 2) != 1) {fprintf(stderr, "ERROR: upper_bound must be an odd number\n"); exit(-1);}

  if (my_rank == 0) {
    printf("Upper Bound: %ld\n", upper);
    printf("processes running: %d\n", comm_sz);
  }

  // start time
  timeval start, end;
  MPI_Barrier(MPI_COMM_WORLD);
  gettimeofday(&start, NULL);

  // execute timed code
  int maxlen = collatz(upper,my_rank,comm_sz);

  //Finding the longest sequence by using MPI_MAX as the reduction operation
  MPI_Reduce(&maxlen, &global_maxlen, 1, MPI_INT, MPI_MAX, 0, MPI_COMM_WORLD);

  // end time
  gettimeofday(&end, NULL);
  const double runtime = end.tv_sec - start.tv_sec + (end.tv_usec - start.tv_usec) / 1000000.0;
  //prints compute time
  if (my_rank == 0){
  printf("compute time: %.4f s\n", runtime);
  // print result
  printf("longest sequence: %d elements\n", global_maxlen);
  }

  //Cleanse
  MPI_Finalize();
}
