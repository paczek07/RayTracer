#include <stdio.h>
#include <stdbool.h> /* Needed for boolean datatype */
#include <math.h>
#include "toolset.h"

/* Subtract two vectors and return the resulting vector */
vector vectorSub(vector * v1, vector * v2) {
  vector result = {
    v1 -> x - v2 -> x,
    v1 -> y - v2 -> y,
    v1 -> z - v2 -> z
  };
  return result;
}

/* Multiply two vectors and return the resulting scalar (dot product) */
float vectorDot(vector * v1, vector * v2) {
  return v1 -> x * v2 -> x + v1 -> y * v2 -> y + v1 -> z * v2 -> z;
}

/* Calculate Vector x Scalar and return resulting Vector*/
vector vectorScale(float c, vector * v) {
  vector result = {
    v -> x * c,
    v -> y * c,
    v -> z * c
  };
  return result;
}

/* Add two vectors and return the resulting vector */
vector vectorAdd(vector * v1, vector * v2) {
  vector result = {
    v1 -> x + v2 -> x,
    v1 -> y + v2 -> y,
    v1 -> z + v2 -> z
  };
  return result;
}
