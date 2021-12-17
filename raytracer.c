#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h> /* Needed for boolean datatype */
#include <math.h>
#include <string.h>
#include "toolset.h"


#include <pthread.h>

#define min(a, b)(((a) < (b)) ? (a) : (b))

/* Width and height of out image */
#define WIDTH 800
#define HEIGHT 600
#define NUMTHREADS 4

unsigned char *img = NULL;
int sectionsize = 0;


material materials[3];
sphere spheres[3];
light lights[3];
/* Renderthread prototype */
void *renderThread(void *arg);

/* Thread information structure */
typedef struct {
  /* Used as argument to thread_start() */
  pthread_t thread_id; /* ID returned by pthread_create() */
  int thread_num; /* Application-defined thread # */
}thread_info;


/* Check if the ray and sphere intersect */
bool intersectRaySphere(ray * r, sphere * s, float * t) {
  bool retval = false;
  /* A = d.d, the vector dot product of the direction */
  float A = vectorDot( &r -> dir, &r -> dir);
  /* We need a vector representing the distance between the start of 
   * the ray and the position of the circle.
   * This is the term (p0 - c) 
   */
  vector dist = vectorSub( &r -> start, &s -> pos);
  /* 2d.(p0 - c) */
  float B = 2 * vectorDot( &r -> dir, &dist);
  /* (p0 - c).(p0 - c) - r^2 */
  float C = vectorDot( &dist, &dist) - (s -> radius * s -> radius);
  /* Solving the discriminant */
  float discr = B * B - 4 * A * C;
  /* If the discriminant is negative, there are no real roots.
   * Returnfalse in that case as the ray misses the sphere.
   * Return true in all other cases (can be one or two intersections)
   * t represents the distance between the start of the ray and
   * the point on the sphere where it intersects.
   */
  if (discr < 0)
    retval = false;
  else {
    float sqrtdiscr = sqrtf(discr);
    float t0 = (-B + sqrtdiscr) / (2);
    float t1 = (-B - sqrtdiscr) / (2);
  /* We want the closest one */
    if (t0 > t1)
      t0 = t1;
  /* Verify t1 larger than 0 and less than the original t */
    if ((t0 > 0.001f) && (t0 < * t)) {
      * t = t0;
      retval = true;
    } else
      retval = false;
  }
  return retval;
}

/* Output data as PPMfile */
void saveppm(char * filename, unsigned char * img, int width, int height) {
  /* FILE pointer */
  FILE * f;
  /* Openfilefor writing */
  f = fopen(filename, "wb");
  /* PPM header info, including the size of the image */
  fprintf(f, "P6 %d %d %d\n", width, height, 255);
  /* Write the image data to thefile - remember 3 byte per pixel */
  fwrite(img, 3, width * height, f);
  /* Make sure you close thefile */
  fclose(f);
}

void *renderThread(void *arg){

  int x,y;
  thread_info* tinfo = (thread_info *)arg;

  /* Calculate which part to render based on thread id */
  int limits[]={(tinfo->thread_num*sectionsize),(tinfo->thread_num*sectionsize)+sectionsize};

  for (y = limits[0]; y < limits[1]; ++y) {
    for (x = 0; WIDTH; ++x) {
      ray r;
      colour output = {0.0f, 0.0f, 0.0f};
      double fragmentx, fragmenty;
      float red = 0;
      float green = 0;
      float blue = 0;
      int level = 0;
      float coef = 1.0;

      r.start.x = x;
      r.start.y = y;
      r.start.z = -2000;

      r.dir.x = 0;
      r.dir.y = 0;
      r.dir.z = 1;

      do {
        /* Find closest intersection */
        float t = 20000.0f;
        int currentSphere = -1;

        unsigned int i;
        for (i = 0; i < 3; i++) {
          if (intersectRaySphere( &r, &spheres[i], &t))
            currentSphere = i;
        }
        if (currentSphere == -1) break;

        vector scaled = vectorScale(t, &r.dir);
        vector newStart = vectorAdd( &r.start, &scaled);

        /* Find the normalfor this new vector at the point of intersection */
        vector n = vectorSub( &newStart, &spheres[currentSphere].pos);
        float temp = vectorDot( &n, &n);
        if (temp == 0) break;

        temp = 1.0f / sqrtf(temp);
        n = vectorScale(temp, &n);

        /* Find the material to determine the colour */
        material currentMat = materials[spheres[currentSphere].material];

        /* Find the value of the light at this point */
        unsigned int j;
        for (j = 0; j < 3; j++) {
          light currentLight = lights[j];
          vector dist = vectorSub( &currentLight.pos, &newStart);
          if (vectorDot( &n, &dist) <= 0.0f) continue;
          float t = sqrtf(vectorDot( &dist, &dist));
          if (t <= 0.0f) continue;

          ray lightRay;
          lightRay.start = newStart;
          lightRay.dir = vectorScale((1 / t), &dist);

          /* Lambert diffusion */
          float lambert = vectorDot( &lightRay.dir, &n) * coef;
          red += lambert * currentLight.intensity.red * currentMat.diffuse.red;
          green += lambert * currentLight.intensity.green * currentMat.diffuse.green;
          blue += lambert * currentLight.intensity.blue * currentMat.diffuse.blue;
        }
        /* Iterate over the reflection */
        coef *= currentMat.reflection;

        /* The reflected ray start and direction */
        r.start = newStart;
        float reflect = 2.0f * vectorDot( &r.dir, &n);
        vector tmp = vectorScale(reflect, &n);
        r.dir = vectorSub( &r.dir, &tmp);

        level++;

      } 
      while ((coef > 0.0f) && (level < 15));
        img[(x + y * WIDTH) * 3 + 0] = (unsigned char) min(red * 255.0f, 255.0f);
        img[(x + y * WIDTH) * 3 + 1] = (unsigned char) min(green * 255.0f, 255.0f);
        img[(x + y * WIDTH) * 3 + 2] = (unsigned char) min(blue * 255.0f, 255.0f);
    }
      
  }

  pthread_exit((void *) arg);
}


int main(int argc, char * argv[]) {
  
  /* Threads */
  thread_info *tinfo;
  pthread_attr_t attr;
  int t;
  /* Allocate memory for pthread_create() arguments */
  tinfo = calloc(NUMTHREADS, sizeof(thread_info));
  if (tinfo == NULL) {
    printf("Error allocating memory for pthread_create() arguments. \n");
    return -1;
  }
  
  materials[0].diffuse.red = 1;
  materials[0].diffuse.green = 0;
  materials[0].diffuse.blue = 0;
  materials[0].reflection = 0.2;

  materials[1].diffuse.red = 0;
  materials[1].diffuse.green = 1;
  materials[1].diffuse.blue = 0;
  materials[1].reflection = 0.5;
  
  materials[2].diffuse.red = 1;
  materials[2].diffuse.green = 0;
  materials[2].diffuse.blue = 1;
  materials[2].reflection = 1;

  
  spheres[0].pos.x = 200;
  spheres[0].pos.y = 300;
  spheres[0].pos.z = 0;
  spheres[0].radius = 100;
  spheres[0].material = 0;
  
  spheres[1].pos.x = 400;
  spheres[1].pos.y = 400;
  spheres[1].pos.z = 0;
  spheres[1].radius = 100;
  spheres[1].material = 1;
  
  spheres[2].pos.x = 500;
  spheres[2].pos.y = 140;
  spheres[2].pos.z = 0;
  spheres[2].radius = 100;
  spheres[2].material = 2;
  
  
  lights[0].pos.x = 100;
  lights[0].pos.y = 240;
  lights[0].pos.z = -100;
  lights[0].intensity.red = 1;
  lights[0].intensity.green = 1;
  lights[0].intensity.blue = 1;

  lights[1].pos.x = 3200;
  lights[1].pos.y = 3000;
  lights[1].pos.z = -1000;
  lights[1].intensity.red = 0.6;
  lights[1].intensity.green = 0.7;
  lights[1].intensity.blue = 1;

  lights[2].pos.x = 600;
  lights[2].pos.y = 0;
  lights[2].pos.z = -100;
  lights[2].intensity.red = 0.3;
  lights[2].intensity.green = 0.5;
  lights[2].intensity.blue = 1;
  
  if (img)
    free(img);
  img = (unsigned char *) malloc (3 * WIDTH * HEIGHT);
  memset(img, 0, 3* WIDTH * HEIGHT);

  /* Calculate section size per thread */
  sectionsize = HEIGHT/NUMTHREADS;





  /* Pthread options */
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

  /* Create the render threads */
  for (t=0; t<NUMTHREADS; t++) {
    tinfo[t].thread_num = t;
    if (pthread_create(&tinfo[t].thread_id, &attr, renderThread, &tinfo[t])) {
      printf("Creation of thread %d failed. \n", t);
      exit(-1);
    }
  }

  if (pthread_attr_destroy(&attr)) {
    printf("Error destroying thread attributes. \n");
  }

  /* Wait for render threads to finish */
  for (t=0; t<NUMTHREADS; t++) {
    if (pthread_join(tinfo[t].thread_id, NULL)) {
    printf("Error waiting for thread. \n");
    }
  }
  /* Will contain the raw image */
  /*
  unsigned char img[3 * WIDTH * HEIGHT];
  int x, y;
  for (y = 0; y < HEIGHT; y++) {
    for (x = 0; x < WIDTH; x++) {
      float red = 0;
      float green = 0;
      float blue = 0;
      int level = 0;
      float coef = 1.0;
      r.start.x = x;
      r.start.y = y;
      r.start.z = -2000;
      r.dir.x = 0;
      r.dir.y = 0;
      r.dir.z = 1;
      do {
        
        float t = 20000.0f;
        int currentSphere = -1;

        unsigned int i;
        for (i = 0; i < 3; i++) {
          if (intersectRaySphere( &r, &spheres[i], &t))
            currentSphere = i;
        }
        if (currentSphere == -1) break;

        vector scaled = vectorScale(t, &r.dir);
        vector newStart = vectorAdd( &r.start, &scaled);

        
        vector n = vectorSub( &newStart, &spheres[currentSphere].pos);
        float temp = vectorDot( &n, &n);
        if (temp == 0) break;

        temp = 1.0f / sqrtf(temp);
        n = vectorScale(temp, &n);

        
        material currentMat = materials[spheres[currentSphere].material];

        
        unsigned int j;
        for (j = 0; j < 3; j++) {
          light currentLight = lights[j];
          vector dist = vectorSub( &currentLight.pos, &newStart);
          if (vectorDot( &n, &dist) <= 0.0f) continue;
          float t = sqrtf(vectorDot( &dist, &dist));
          if (t <= 0.0f) continue;

          ray lightRay;
          lightRay.start = newStart;
          lightRay.dir = vectorScale((1 / t), &dist);

          
          float lambert = vectorDot( &lightRay.dir, &n) * coef;
          red += lambert * currentLight.intensity.red * currentMat.diffuse.red;
          green += lambert * currentLight.intensity.green * currentMat.diffuse.green;
          blue += lambert * currentLight.intensity.blue * currentMat.diffuse.blue;
        }
        
        coef *= currentMat.reflection;

        
        r.start = newStart;
        float reflect = 2.0f * vectorDot( &r.dir, &n);
        vector tmp = vectorScale(reflect, &n);
        r.dir = vectorSub( &r.dir, &tmp);

        level++;

      } while ((coef > 0.0f) && (level < 15));

      
      img[(x + y * WIDTH) * 3 + 0] = (unsigned char) min(red * 255.0f, 255.0f);
      img[(x + y * WIDTH) * 3 + 1] = (unsigned char) min(green * 255.0f, 255.0f);
      img[(x + y * WIDTH) * 3 + 2] = (unsigned char) min(blue * 255.0f, 255.0f);
    }
  }

  */
  saveppm("image.ppm", img, WIDTH, HEIGHT);
  /* Free allocated memory */
  // if (img)
  //   free(img);

  /* Exit */
  pthread_exit(NULL);

  if (tinfo)
    free(tinfo);
  
  return 0;
}
