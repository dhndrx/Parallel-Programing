
#include <cstdio>
#include <cmath>
#include <algorithm>
#include <sys/time.h>
#include <mpi.h>

#include "BMP43805351.h"

static void fractal(const int width, const int my_start_frame, const int my_end_frame, unsigned char* const pic)
{
  const double Delta = 0.002;
  const double xMid = 0.2315059;
  const double yMid = 0.5214880;

  // compute pixels of each frame
  double delta = Delta;
  for (int frame = my_start_frame; frame < my_end_frame; frame++) {  // frames
    //computing delta as  function of frame value
    delta = frame*.98 * pow(.98,frame);
    const double xMin = xMid - delta;
    const double yMin = yMid - delta;
    const double dw = 2.0 * delta / width;
    for (int row = 0; row < width; row++) {  // rows
      const double cy = yMin + row * dw;
      for (int col = 0; col < width; col++) {  // columns
        const double cx = xMin + col * dw;
        double x = cx;
        double y = cy;
        double x2, y2;
        int depth = 256;
        do {
          x2 = x * x;
          y2 = y * y;
          y = 2.0 * x * y + cy;
          x = x2 - y2 + cx;
          depth--;
        } while ((depth > 0) && ((x2 + y2) < 5.0));
        pic[frame * width * width + row * width + col] = (unsigned char)depth;
      }
    }
  }
}

int main(int argc, char *argv[])
{
  //Setup
  MPI_Init(NULL, NULL);
  int comm_sz, my_rank;
  MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);
  MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

  if (my_rank == 0) printf("Fractal v1.9\n");


  // check command line
  if (argc != 3) {fprintf(stderr, "USAGE: %s frame_width number_of_frames\n", argv[0]); exit(-1);}
  const int width = atoi(argv[1]);
  if (width < 10) {fprintf(stderr, "ERROR: frame_width must be at least 10\n"); exit(-1);}
  const int frames = atoi(argv[2]);
  if (frames < 1) {fprintf(stderr, "ERROR: number_of_frames must be at least 1\n"); exit(-1);}
  if ((frames % comm_sz) != 0) {fprintf(stderr, "ERROR: number_of_frames must be a multiple of the number of processes\n"); exit(-1);}

  if (my_rank == 0) {
  printf("width: %d\n", width);
  printf("frames: %d\n", frames);
  printf("processes running: %d\n", comm_sz);
  }

  // allocate pic array
  unsigned char* pic = new unsigned char [frames * width * width];

  // compute start and end frames for blocked assignments
  const int my_start_frame = my_rank * (long)frames / comm_sz;
  const int my_end_frame = (my_rank + 1) * (long)frames / comm_sz;
  const int frames_range = my_end_frame - my_start_frame;


  int* resulting_pic = NULL;
  if (my_rank == 0) res = new int [frames * width * width];

  //executing barrier before time is taken
  MPI_Barrier(MPI_COMM_WORLD);
  // start time
  timeval start, end;
  gettimeofday(&start, NULL);

  // execute timed code
  fractal(width,my_start_frame,my_end_frame, pic);

  MPI_Gather(&pic[my_start_frame], frames_range, MPI_INT, res, frames_range, MPI_INT, 0, MPI_COMM_WORLD);

  // end time
  gettimeofday(&end, NULL);
  const double runtime = end.tv_sec - start.tv_sec + (end.tv_usec - start.tv_usec) / 1000000.0;

  if (my_rank == 0){
    printf("compute time: %.4f s\n", runtime);
    // write result to BMP files
    if ((width <= 257) && (frames <= 60)) {
      for (int frame = 0; frame < frames; frame++) {
        BMP24 bmp(0, 0, width - 1, width - 1);
        for (int y = 0; y < width - 1; y++) {
          for (int x = 0; x < width - 1; x++) {
            const int p = res[frame * width * width + y * width + x];
            const int e = res[frame * width * width + y * width + (x + 1)];
            const int s = res[frame * width * width + (y + 1) * width + x];
            const int dx = std::min(2 * std::abs(e - p), 255);
            const int dy = std::min(2 * std::abs(s - p), 255);
            bmp.dot(x, y, dx * 0x000100 + dy * 0x000001);
          }
        }
        char name[32];
        sprintf(name, "fractal%d.bmp", frame + 1000);
        bmp.save(name);
      }
    }
    delete [] res;
  }

  // clean up
  MPI_Finalize();
  delete [] pic;
  return 0;
}
