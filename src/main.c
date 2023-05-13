#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_gpu.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_pixels.h>
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

typedef struct circle circle;

struct circle {
  float xpos, ypos;
  float vx, vy;
  float ax, ay;
  float radius;

  int id;
};

typedef struct collision_pair collision_pair;
struct collision_pair {
  circle *ballA, *ballB;
};

void handle_input(SDL_Event *, int *, circle *, int);

int main() {
  GPU_Target *screen;
  int is_running;
  int min_radius, max_radius;
  int num_circles;
  int max_velocity;
  uint32_t start_time;
  uint32_t end_time;
  double elapsed_time;
  collision_pair *collided_circles;
  int collision_buffer;

  screen = GPU_Init(600, 600, GPU_INIT_DISABLE_VSYNC | GPU_DEFAULT_INIT_FLAGS);
  is_running = 1;
  num_circles = 10;
  min_radius = 20;
  max_radius = 50;
  max_velocity = 5;
  collision_buffer = num_circles * 2;

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
  collided_circles =
      (collision_pair *)malloc(sizeof(collision_pair) * collision_buffer);
  srand(time(NULL));
  for (int i = 0; i < num_circles; i++) {
    circle c = {.xpos = min_radius + rand() % screen->w,
                .ypos = min_radius + rand() % screen->h,
                .vx = rand() % max_velocity,
                .vy = rand() % max_velocity,
                .ax = 0,
                .ay = 0,
                .radius = min_radius + rand() % max_radius,
                .id = i};
    circles[i] = c;
  }

  while (is_running) {
    start_time = SDL_GetTicks();
    handle_input(&ev, &is_running, circles, num_circles);
    GPU_ClearColor(screen, bg);

    for (int i = 0; i < num_circles; i++) {
      GPU_Circle(screen, circles[i].xpos, circles[i].ypos, circles[i].radius,
                 color);

      circles[i].xpos += circles[i].vx;
      circles[i].ypos += circles[i].vy;

      for (int circA = 0; circA < num_circles; circA++) {
        if ((circles[i].id != circles[circA].id) &&
            is_overlapping(circles[i].xpos, circles[i].ypos,
                           circles[circA].xpos, circles[circA].ypos,
                           circles[i].radius, circles[circA].radius)) {
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

      if (circles[i].xpos >= screen->w)
        circles[i].xpos -= screen->w;
      if (circles[i].xpos < 0)
        circles[i].xpos += screen->w;

      if (circles[i].ypos >= screen->h)
        circles[i].ypos -= screen->h;
      if (circles[i].ypos < 0)
        circles[i].ypos += screen->h;
    }

    GPU_Flip(screen);
    SDL_Delay(20);
    end_time = SDL_GetTicks();
    elapsed_time = end_time - start_time;
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
      for (int i = 0; i < num_circles; i++) {
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

void push(collision_pair *pair, int bfr_size, circle *circleA,
          circle *circleB) {
  for (int i = 0; i < bfr_size; i++) {
  }
}
