#include <float.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>

// SIGNED
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

// UNSIGNED
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef struct {
  float x;
  float y;
} Vector2;

typedef struct {
  float x;
  float y;
  float z;
} Vector3;

Vector3 Vector3Scale(Vector3 v1, float scalar) {
  return (Vector3){
      v1.x * scalar,
      v1.y * scalar,
      v1.z * scalar,
  };
}

Vector3 Vector3AddValue(Vector3 v1, float value) {
  return (Vector3){
      v1.x * value,
      v1.y * value,
      v1.z * value,
  };
}

Vector3 Vector3Subtract(Vector3 v1, Vector3 v2) {
  return (Vector3){
      v1.x - v2.x,
      v1.y - v2.y,
      v1.z - v2.z,
  };
}

Vector3 Vector3Add(Vector3 v1, Vector3 v2) {
  return (Vector3){
      v1.x + v2.x,
      v1.y + v2.y,
      v1.z + v2.z,
  };
}

float Vector3DotProduct(Vector3 v1, Vector3 v2) {
  return (v1.x * v2.x) + (v1.y * v2.y) + (v1.z * v2.z);
};

float Vector3Length(Vector3 v1) { return sqrtf(Vector3DotProduct(v1, v1)); }

Vector3 Vector3Normalize(Vector3 v) {
  float len = Vector3Length(v);
  return (Vector3){v.x / len, v.y / len, v.z / len};
}

typedef struct {
  Vector3 center;
  float radius;
  Vector3 color;
} Sphere;

#define SCENE_COUNT 10
Sphere scene[SCENE_COUNT];

Vector2 intersect_ray_sphere(Vector3 origin, Vector3 direction,
                             Sphere *sphere) {
  Vector3 co = Vector3Subtract(origin, sphere->center);
  float a = Vector3DotProduct(direction, direction);
  float b = Vector3DotProduct(direction, co) * 2;
  float c = Vector3DotProduct(co, co) - (sphere->radius * sphere->radius);

  float discriminant = b * b - 4 * a * c;
  if (discriminant < 0) {
    return (Vector2){FLT_MAX, FLT_MAX};
  }

  float sqrt_disc = sqrtf(discriminant);
  float t1 = (-b - sqrt_disc) / (2.0f * a);
  float t2 = (-b + sqrt_disc) / (2.0f * a);

  return (Vector2){t1, t2};
}

Vector3 trace_ray(Vector3 origin, Vector3 direction, float t_min, float t_max) {
  Vector3 result;
  float closest_t = t_max;
  Sphere *closest_sphere = NULL;

  for (int i = 0; i < SCENE_COUNT; i++) {
    Sphere *sphere = &scene[i];
    Vector2 roots = intersect_ray_sphere(origin, direction, sphere);
    float t1 = roots.x;
    float t2 = roots.y;

    if (t1 >= t_min && t1 <= t_max && t1 < closest_t) {
      closest_t = t1;
      closest_sphere = sphere;
    }
    if (t2 >= t_min && t2 <= t_max && t2 < closest_t) {
      closest_t = t2;
      closest_sphere = sphere;
    }
  }
  if (closest_sphere == NULL) {
    return (Vector3){255, 255, 255};
  }

  return closest_sphere->color;
}

int main(void) {
  FILE *file = fopen("image.ppm", "w");
  if (!file) {
    fprintf(stderr, "Failed to open file for writing.\n");
    return 1;
  }

  const float VIEWPORT_WIDTH = 1.0f;
  const float VIEWPORT_HEIGHT = 1.0f;
  const float VIEWPORT_DISTANCE = 1.0f;

  const u16 IMAGE_HEIGHT = 256;
  const u16 IMAGE_WIDTH = 256;

  Vector3 camera = {0, 0, 0};

  float vx = 1 * (VIEWPORT_WIDTH / IMAGE_WIDTH);
  float vy = 1 * (VIEWPORT_HEIGHT / IMAGE_HEIGHT);
  Vector3 viewport = {vx, vy, VIEWPORT_DISTANCE};

  // Find the distance from the camera to the viewport
  // Vector3 ray = Vector3Subtract(viewport, camera);
  // // Find where we are on the ray based on t time
  // float time = 1;
  // ray = Vector3Scale(ray, time);
  // // Offset the ray by the origin, in our case the camera
  // ray = Vector3Add(camera, ray);
  //
  // // Where is the ray relative to our sphere origin
  // Vector3 intersection_point = Vector3Subtract(ray, sphere.center);
  // // Find the length of new Vector3
  // float length = Vector3DotProduct(intersection_point, intersection_point);

  scene[0] = (Sphere){
      .center = (Vector3){0, -1, 3}, .radius = 1, .color = {255, 0, 0}};
  scene[1] =
      (Sphere){.center = (Vector3){2, 0, 4}, .radius = 1, .color = {0, 255, 0}};
  scene[2] = (Sphere){
      .center = (Vector3){-2, 0, 4}, .radius = 1, .color = {0, 0, 255}};

  fprintf(file, "P3\n%i %i\n 255\n", IMAGE_WIDTH, IMAGE_HEIGHT);
  for (int y = 0; y < IMAGE_HEIGHT; y++) {
    for (int x = 0; x < IMAGE_WIDTH; x++) {
      float vx = (x - IMAGE_WIDTH / 2.0f) * (VIEWPORT_WIDTH / IMAGE_WIDTH);
      float vy = -(y - IMAGE_HEIGHT / 2.0f) * (VIEWPORT_HEIGHT / IMAGE_HEIGHT);
      Vector3 viewport_point =
          Vector3Normalize((Vector3){vx, vy, VIEWPORT_DISTANCE});

      Vector3 color = trace_ray(camera, Vector3Subtract(viewport_point, camera),
                                1, FLT_MAX);
      fprintf(file, "%i %i %i\n", (int)color.x, (int)color.y, (int)color.z);
    }
  }

  return 0;
}
