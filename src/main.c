#include "./cust_type/vector.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_gpu.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_timer.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

bool is_overlapping(float px, float py, float tx, float ty, float r1, float r2);
float distance(float x1, float x2, float y1, float y2);
float cap(float val, float max);
typedef struct circle circle;
typedef struct vector_M vector_M;

struct vector_M {
  double x, y;
};

struct circle {
  double xpos, ypos;
  double vx, vy;
  double ax, ay;
  double radius;
  double mass;
  SDL_Color color;

  int id;
};

typedef struct collision_pair collision_pair;
struct collision_pair {
  circle *ballA, *ballB;
};

void handle_input(SDL_Event *, int *, circle *, int);
void handle_collision(collision_pair pair);
vector_M scale_vector(double vx, double vy);

const int WIN_WIDTH = 1000;
const int WIN_HEIGHT = 700;

int main() {
  GPU_Target *screen;
  int is_running;
  int min_radius, max_radius;
  int num_circles;
  int max_velocity;
  uint32_t start_time;
  uint32_t end_time;
  double elapsed_time;
  vector *vec_collisions;
  int collision_buffer;

  screen = GPU_Init(WIN_WIDTH, WIN_HEIGHT,
                    GPU_INIT_DISABLE_VSYNC | GPU_DEFAULT_INIT_FLAGS);
  is_running = 1;
  num_circles = 20;
  min_radius = 25;
  max_radius = 30;
  max_velocity = 500;
  collision_buffer = num_circles * 2;
  vec_collisions = make(sizeof(collision_pair), 30);

  printf("Initializing Screen...\n");

  if (screen == NULL) {
    GPU_LogError("Failed to initalize screen\n");
    return 3;
  }

  printf("Renderer ID %d.%d\n", screen->renderer->id.major_version,
         screen->renderer->id.minor_version);
  SDL_Event ev;
  SDL_Color bg = {0x00, 0x00, 0x00, SDL_ALPHA_OPAQUE};
  SDL_Color color = {
      0xFF,
      0x34,
      0x00,
      SDL_ALPHA_OPAQUE,
  };

  circle *circles = (circle *)malloc(sizeof(circle) * num_circles);
  srand(time(NULL));
  for (int i = 0; i < num_circles; i++) {
    circle c = {
        .xpos = min_radius + rand() % screen->w,
        .ypos = min_radius + rand() % screen->h,
        .vx = rand() % max_velocity,
        .vy = rand() % max_velocity,
        .ax = 0,
        .ay = 0,
        .radius = min_radius + rand() % max_radius,
        .color = {rand() % 256, rand() % 256, rand() % 256, SDL_ALPHA_OPAQUE},
        .id = i};
    c.mass = M_PI * c.radius * c.radius;
    circles[i] = c;
  }
  SDL_Color black = {0x00, 0x00, 0x00, SDL_ALPHA_OPAQUE};
  while (is_running) {
    start_time = SDL_GetTicks();
    handle_input(&ev, &is_running, circles, num_circles);
    GPU_ClearColor(screen, bg);
    for (int i = 0; i < num_circles; i++) {
      GPU_CircleFilled(screen, circles[i].xpos, circles[i].ypos,
                       circles[i].radius, circles[i].color);
      vector_M unt_vec = scale_vector(circles[i].vx, circles[i].vy);
      printf("Unit Vec %f %f\n", unt_vec.x, unt_vec.y);
      printf("Vector   %f %f\n", circles[i].vx, circles[i].vy);
      GPU_Line(screen, circles[i].xpos, circles[i].ypos,
               circles[i].xpos + (unt_vec.x * circles[i].radius),
               circles[i].ypos + (unt_vec.y * circles[i].radius), black);

      circles[i].ax = -circles[i].vx * 0.08f;
      circles[i].ay = -circles[i].vy * 0.08f;

      circles[i].vx += circles[i].ax * elapsed_time;
      circles[i].vy += circles[i].ay * elapsed_time;

      circles[i].vx = cap(circles[i].vx, max_velocity);
      circles[i].vy = cap(circles[i].vy, max_velocity);

      circles[i].xpos += circles[i].vx * elapsed_time;
      circles[i].ypos += circles[i].vy * elapsed_time;

      for (int circA = 0; circA < num_circles; circA++) {
        if ((circles[i].id != circles[circA].id) &&
            is_overlapping(circles[i].xpos, circles[i].ypos,
                           circles[circA].xpos, circles[circA].ypos,
                           circles[i].radius, circles[circA].radius)) {
          collision_pair pair = {&circles[i], &circles[circA]};
          append(vec_collisions, &pair);
          float distA_B = distance(circles[i].xpos, circles[circA].xpos,
                                   circles[i].ypos, circles[circA].ypos);
          float overlap =
              0.5f * (distA_B - circles[i].radius - circles[circA].radius);

          // displace the current ball
          circles[i].xpos -=
              overlap * (circles[i].xpos - circles[circA].xpos) / distA_B;
          circles[i].ypos -=
              overlap * (circles[i].ypos - circles[circA].ypos) / distA_B;

          // displace the target ball
          circles[circA].xpos +=
              overlap * (circles[i].xpos - circles[circA].xpos) / distA_B;
          circles[circA].ypos +=
              overlap * (circles[i].ypos - circles[circA].ypos) / distA_B;
        }
      }
      // handle collision
      for (int collisions = 0; collisions < vec_collisions->len; collisions++) {
        collision_pair *pair = pop(vec_collisions);
        handle_collision(*pair);
      }

      if (fabs((pow(circles[i].vx, 2) + pow(circles[i].vy, 2))) < 0.01f) {
        circles[i].vx = 0;
        circles[i].vy = 0;
      }

      if (circles[i].xpos + circles[i].radius >= screen->w) {
        circles[i].vx = -1 * circles[i].vx;
        circles[i].ax = -1 * circles[i].ax;
      }
      if (circles[i].xpos - circles[i].radius < 0) {
        circles[i].vx = fabs(circles[i].vx);
        circles[i].ax = fabs(circles[i].ax);
      }

      if (circles[i].ypos + circles[i].radius >= screen->h) {
        circles[i].vy = -1 * circles[i].vy;
        circles[i].ay = -1 * circles[i].ay;
      }
      if (circles[i].ypos - circles[i].radius < 0) {
        circles[i].vy = fabs(circles[i].vy);
        circles[i].ay = fabs(circles[i].ay);
      }
    }

    GPU_Flip(screen);
    SDL_Delay(20);
    end_time = SDL_GetTicks();
    elapsed_time = (end_time - start_time) / 1000.0;
  }
  GPU_Quit();
}

circle *current_circle = NULL;
bool is_circle_selected = false;

void handle_input(SDL_Event *ev, int *is_running, circle *circles,
                  int num_circles) {
  float mouseX;
  float mouseY;
  float dist_mouse_circ;
  while (SDL_PollEvent(ev)) {
    switch (ev->type) {
    case SDL_KEYDOWN:
      switch (ev->key.keysym.sym) {
      case SDLK_q:
        *is_running = 0;
        break;
      default:;
      }
    case SDL_MOUSEBUTTONDOWN:
      if (is_circle_selected) {
        is_circle_selected = false;
        current_circle = NULL;
        break;
      }
      printf("Mouse click at %d,%d \n", ev->button.x, ev->button.y);

      mouseX = ev->button.x;
      mouseY = ev->button.y;
      for (int i = 0; i <= num_circles; i++) {
        dist_mouse_circ =
            distance(mouseX, circles[i].xpos, mouseY, circles[i].ypos);
        if (dist_mouse_circ <= circles[i].radius) {
          printf("Inside circle %d\n", circles[i].id);
          current_circle = &circles[i];
          is_circle_selected = true;
          break;
        }
      }
    case SDL_MOUSEMOTION:
      if (current_circle != NULL) {
        printf("current circle: %d\n", current_circle->id);
        current_circle->xpos = ev->motion.x;
        current_circle->ypos = ev->motion.y;
      }
    default:;
    }
  }
}

void handle_collision(collision_pair pair) {
  printf("A: %d B: %d\n", pair.ballA->id, pair.ballB->id);
  circle *ball_A = pair.ballA;
  circle *ball_B = pair.ballB;

  float fDistance = sqrt(pow(ball_A->xpos - ball_B->xpos, 2.0) +
                         pow(ball_A->ypos - ball_B->ypos, 2.0));

  float nx = (ball_B->xpos - ball_A->xpos) / fDistance;
  float ny = (ball_B->ypos - ball_A->ypos) / fDistance;

  float tx = -ny;
  float ty = nx;

  float tanDP_A = ball_A->vx * tx + ball_A->vy * ty;
  float tanDP_B = ball_B->vx * tx + ball_B->vy * ty;

  float dpNorm_A = ball_A->vx * nx + ball_A->vy * ny;
  float dpNorm_B = ball_B->vx * nx + ball_B->vy * ny;

  float m1 = (dpNorm_A * (ball_A->mass - ball_B->mass) +
              2.0f * ball_B->mass * dpNorm_B) /
             (ball_A->mass + ball_B->mass);
  float m2 = (dpNorm_B * (ball_B->mass - ball_A->mass) +
              2.0f * ball_A->mass * dpNorm_A) /
             (ball_A->mass + ball_B->mass);

  ball_A->vx = tx * tanDP_A + nx * m1;
  ball_A->vy = ty * tanDP_A + ny * m1;

  ball_B->vx = tx * tanDP_B + nx * m2;
  ball_B->vy = ty * tanDP_B + ny * m2;

  ball_B->color = ball_A->color;
}

bool is_overlapping(float px, float py, float tx, float ty, float r1,
                    float r2) {
  return (px - tx) * (px - tx) + (py - ty) * (py - ty) <= (r1 + r2) * (r1 + r2);
}

float distance(float x1, float x2, float y1, float y2) {
  float distance;
  float dx = (x1 - x2);
  float dy = (y1 - y2);
  distance = sqrtf(powf(dx, 2) + powf(dy, 2));
  return distance;
}

float cap(float val, float max) { return val >= max ? max : val; }

vector_M scale_vector(double vx, double vy) {
  vector_M unt_vec;
  double mag_vec = sqrtf(vx * vx + vy * vy);
  unt_vec.x = (vx / mag_vec);
  unt_vec.y = (vy / mag_vec);
  return unt_vec;
}
