#include <cstdio>
#include <algorithm>
#include <sys/time.h>
#include <pthread.h>

//Shared variables
static long threads;
static long upper;
static int maxlen;
//initialize mutex
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;

static void* collatz(void* arg)
{
  const long my_rank = (long)arg;
  int len = 1;
  // compute sequence lengths
  for(long i = my_rank * 2 + 1; i <= upper; i += threads * 2)
  {
    long val = i;
     while (val != 1) {
      len++;
      if ((val % 2) == 0) {
        val = val / 2;  // even
      } else {
        val = 3 * val + 1;  // odd
      }
    }
  }
  //update global variable
  pthread_mutex_lock(&mutex1);
  maxlen = std::max(maxlen, len);
  pthread_mutex_unlock(&mutex1);
  return NULL;
}

int main(int argc, char *argv[])
{
  printf("Collatz v1.2\n");

  // check command line
  if (argc != 3) {fprintf(stderr, "USAGE: %s upper_bound threads\n", argv[0]); exit(-1);}
  upper = atol(argv[1]);
  if (upper < 5) {fprintf(stderr, "ERROR: upper_bound must be at least 5\n"); exit(-1);}
  if ((upper % 2) != 1) {fprintf(stderr, "ERROR: upper_bound must be an odd number\n"); exit(-1);}
  threads = atol(argv[2]);
  if (threads < 1) {fprintf(stderr, "ERROR: threads must be at least 1\n"); exit(-1);}
  printf("threads: %ld\n", threads);

  // initialize pthread variables
  pthread_t* const handle = new pthread_t [threads - 1];
  maxlen = 0;
  // start time
  timeval start, end;
  gettimeofday(&start, NULL);

  // launch threads
  for (long thread = 0; thread < threads - 1; thread++) {
    pthread_create(&handle[thread], NULL, collatz, (void *)thread);
  }

  // work for master
  collatz((void*)(threads - 1));

  // join threads
  for (long thread = 0; thread < threads - 1; thread++) {
    pthread_join(handle[thread], NULL);
  }

  // end time
  gettimeofday(&end, NULL);
  const double runtime = end.tv_sec - start.tv_sec + (end.tv_usec - start.tv_usec) / 1000000.0;
  printf("compute time: %.4f s\n", runtime);

  // print result
  printf("longest sequence: %d elements\n", maxlen);

  // cleanup
  delete [] handle;
  // destroy mutex
  pthread_mutex_destroy(&mutex1);
  return 0;
}
