#ifndef STRUCTS
#define STRUCTS


/* The vector structure */
typedef struct {
  float x, y, z;
}vector;

/* The sphere */
typedef struct {
  vector pos;
  float radius;
  int material;
}sphere;

/* The ray */
typedef struct {
  vector start;
  vector dir;
}ray;

/* Colour */
typedef struct {
  float red, green, blue;
}colour;

/* Material Definition */
typedef struct {
  colour diffuse;
  float reflection;
}material;

/* Lightsource definition */
typedef struct {
  vector pos;
  colour intensity;
}light;

#endif