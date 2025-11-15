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

Vector3 Vector3Negate(Vector3 v) { return (Vector3){-v.x, -v.y, -v.z}; }

typedef enum {
  LIGHT_TYPE_POINT,
  LIGHT_TYPE_DIRECTIONAL,
  LIGHT_TYPE_AMBIENT
} Light_Type;

typedef struct {
  Light_Type type;
  float intensity;
  union {
    Vector3 position;  // For point light
    Vector3 direction; // For directional light
    // ambient doesn't use either
  };
} Light;

typedef struct {
  Vector3 center;
  float radius;
  Vector3 color;
  float specular;
} Sphere;

#define SPHERE_COUNT 10
Sphere spheres[SPHERE_COUNT];

#define LIGHT_COUNT 10
Light lights[LIGHT_COUNT];

Light create_point_light(float intensity, Vector3 position) {
  return (Light){
      .type = LIGHT_TYPE_POINT, .intensity = intensity, .position = position};
}

Light create_directional_light(float intensity, Vector3 direction) {
  return (Light){.type = LIGHT_TYPE_DIRECTIONAL,
                 .intensity = intensity,
                 .direction = direction};
}

Light create_ambient_light(float intensity) {
  return (Light){.type = LIGHT_TYPE_AMBIENT, .intensity = intensity};
}

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

typedef struct {
  Sphere *closest_sphere;
  float closest_t;
} ClosestIntersection;
ClosestIntersection closest_intersection(Vector3 origin, Vector3 direction,
                                         float t_min, float t_max) {
  float closest_t = t_max;
  Sphere *closest_sphere = NULL;

  for (int i = 0; i < SPHERE_COUNT; i++) {
    Sphere *sphere = &spheres[i];
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

  return (ClosestIntersection){.closest_sphere = closest_sphere,
                               .closest_t = closest_t};
}

float compute_lighting(Vector3 point, Vector3 normal, Vector3 view,
                       float specular) {
  float intensity = 0.0f;

  for (int j = 0; j < LIGHT_COUNT; j++) {
    Light *light = &lights[j];

    if (light->type == LIGHT_TYPE_AMBIENT) {
      intensity += light->intensity;
      continue;
    }

    Vector3 L;
    float t_max;
    if (light->type == LIGHT_TYPE_POINT) {
      L = Vector3Normalize(Vector3Subtract(light->position, point));
      t_max = 1;
    } else {
      L = Vector3Normalize(light->direction);
      t_max = FLT_MAX;
    }

    // Shadow Check
    ClosestIntersection intersection =
        closest_intersection(point, L, 0.001, t_max);
    if (intersection.closest_sphere != NULL) {
      continue;
    }

    // Diffuse
    float n_dot_l = Vector3DotProduct(normal, L);
    if (n_dot_l > 0) {
      intensity += light->intensity * n_dot_l /
                   (Vector3Length(normal) * Vector3Length(L));
    }

    if (specular > 0) {
      Vector3 R = Vector3Subtract(
          Vector3Scale(normal, 2.0f * Vector3DotProduct(normal, L)), L);
      float r_dot_v = Vector3DotProduct(R, view);
      if (r_dot_v > 0) {
        float angle_between_r_v =
            r_dot_v / (Vector3Length(R) * Vector3Length(view));
        intensity += light->intensity * powf(angle_between_r_v, specular);
      }
    }
  }
  if (intensity > 1.0f) {
    intensity = 1.0f;
  }

  return intensity;
}

Vector3 trace_ray(Vector3 origin, Vector3 direction, float t_min, float t_max) {
  Vector3 result;

  ClosestIntersection intersection =
      closest_intersection(origin, direction, t_min, t_max);
  Sphere *closest_sphere = intersection.closest_sphere;
  float closest_t = intersection.closest_t;

  if (closest_sphere == NULL) {
    return (Vector3){255, 255, 255};
  }

  Vector3 P = Vector3Scale(direction, closest_t);
  P = Vector3Add(P, origin);
  Vector3 N = Vector3Subtract(P, closest_sphere->center);
  N = Vector3Normalize(N);
  return Vector3Scale(closest_sphere->color,
                      compute_lighting(P, N, Vector3Negate(direction),
                                       closest_sphere->specular));
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

  const u16 IMAGE_HEIGHT = 256 * 10;
  const u16 IMAGE_WIDTH = 256 * 10;
  Vector3 camera = {0, 0, 0};

  float vx = 1 * (VIEWPORT_WIDTH / IMAGE_WIDTH);
  float vy = 1 * (VIEWPORT_HEIGHT / IMAGE_HEIGHT);
  Vector3 viewport = {vx, vy, VIEWPORT_DISTANCE};

  spheres[0] = (Sphere){.center = (Vector3){0, -1, 3},
                        .radius = 1,
                        .color = {255, 0, 0},
                        .specular = 500};
  spheres[1] = (Sphere){.center = (Vector3){2, 0, 4},
                        .radius = 1,
                        .color = {0, 255, 0},
                        .specular = 500};
  spheres[2] = (Sphere){.center = (Vector3){-2, 0, 4},
                        .radius = 1,
                        .color = {0, 0, 255},
                        .specular = 10};
  spheres[3] = (Sphere){.center = (Vector3){0, -5001, 0},
                        .radius = 5000,
                        .color = {255, 255, 0},
                        .specular = 1000};

  lights[0] = create_ambient_light(0.2);
  lights[1] = create_point_light(0.6, (Vector3){2, 1, 0});
  lights[2] = create_directional_light(0.2, (Vector3){1, 4, 4});

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
