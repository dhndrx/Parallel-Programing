#include <cstdlib>
#include <cstdio>
#include <sys/time.h>
#include "ECLgraph.h"

static const unsigned char in = 2;
static const unsigned char out = 1;
static const unsigned char undecided = 0;

// source of hash function: https://stackoverflow.com/questions/664014/what-integer-hash-function-are-good-that-accepts-an-integer-hash-key
static unsigned int hash(unsigned int val)
{
  val = ((val >> 16) ^ val) * 0x45d9f3b;
  val = ((val >> 16) ^ val) * 0x45d9f3b;
  return (val >> 16) ^ val;
}

static void mis(const ECLgraph g, unsigned char* const status, unsigned int* const random)
{
  // initialize arrays
  #pragma omp for
  for (int v = 0; v < g.nodes; v++) status[v] = undecided;
  #pragma omp for
  for (int v = 0; v < g.nodes; v++) random[v] = hash(v + 1);

  bool missing;
  // repeat until all nodes' status has been decided
  do {
    missing = false;
    // go over all the nodes

    //removes implicit barrier
    #pragma omp for nowait
    for (int v = 0; v < g.nodes; v++) {
      if (status[v] == undecided) {
        int i = g.nindex[v];
        // try to find a neighbor whose random number is lower
        while ((i < g.nindex[v + 1]) && ((status[g.nlist[i]] == out) || (random[v] < random[g.nlist[i]]) || ((random[v] == random[g.nlist[i]]) && (v < g.nlist[i])))) {
          i++;
        }
        if (i < g.nindex[v + 1]) {
          // found such a neighbor -> status still unknown
          missing = true;
        } else {
          // no such neighbor -> status is "in" and all neighbors are "out"
          status[v] = in;
          for (int i = g.nindex[v]; i < g.nindex[v + 1]; i++) {
            status[g.nlist[i]] = out;
          }
        }
      }
    }
  } while (missing);
}

int main(int argc, char* argv[])
{
  printf("Maximal Independent Set v1.3\n");

  // check command line
  if (argc != 3) {fprintf(stderr, "USAGE: %s input_file thread_count\n", argv[0]); exit(-1);}
  const int threads = atoi(argv[2]);
  if (threads < 1) {fprintf(stderr, "ERROR: thread_count must be at least 1\n"); exit(-1);}

  // read input
  ECLgraph g = readECLgraph(argv[1]);

  printf("configuration: %d nodes and %d edges (%s)\n", g.nodes, g.edges, argv[1]);
  printf("thread count: %d\n", threads);

  // allocate arrays
  unsigned char* const status = new unsigned char [g.nodes];
  unsigned int* const random = new unsigned int [g.nodes];

  // start time
  timeval start, end;
  gettimeofday(&start, NULL);

  // execute timed code
  #pragma omp parallel default(none) shared(g) num_threads(threads)
  mis(g, status, random);

  // end time
  gettimeofday(&end, NULL);
  const double runtime = end.tv_sec - start.tv_sec + (end.tv_usec - start.tv_usec) / 1000000.0;
  printf("compute time: %.4f s\n", runtime);

  // determine and print set size
  int count = 0;
  for (int v = 0; v < g.nodes; v++) {
    if (status[v] == in) {
      count++;
    }
  }
  printf("elements in set: %d (%.1f%%)\n", count, 100.0 * count / g.               nodes);

  // verify result
  for (int v = 0; v < g.nodes; v++) {
    if ((status[v] != in) && (status[v] != out)) {fprintf(stderr, "ERROR: found unprocessed node\n"); exit(-1);}
    if (status[v] == in) {
      for (int i = g.nindex[v]; i < g.nindex[v + 1]; i++) {
        if (status[g.nlist[i]] == in) {fprintf(stderr, "ERROR: found adjacent nodes in MIS\n"); exit(-1);}
      }
    } else {
      bool flag = true;
      for (int i = g.nindex[v]; i < g.nindex[v + 1]; i++) {
        if (status[g.nlist[i]] == in) {
          flag = false;
          break;
        }
      }
      if (flag) {fprintf(stderr, "ERROR: set is not maximal\n"); exit(-1);}
    }
  }

  // clean up
  freeECLgraph(g);
  delete [] status;
  delete [] random;
  return 0;
}
