/*
 * autocolor.c
 * by: aiden olsen
 * TODO: 
 */

#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <stdarg.h>

#define CANVAS_AT(c, x, y) c.pixels[(x) + ((y) * (c).width)]
#define PRINT_COLOR(c) print_color(c, #c)
#define PIXEL_SIZE(c) sizeof(*c.pixels) * c.width * c.height
#define color_not_equal(a, b) !color_equal(a, b)
#define DO(x) for (int i = 0; i < x; i++)
#define MAX(...) max(__VA_ARGS__, 0)

const Color ALPHA_GRAY = {0, 0, 0, 100};

typedef enum {
  VON_NEUMANN,
  MOORE,
} neighborhood;

typedef struct {
  int count;
  Color avg_color;
} NeighborInfo;

typedef struct {
  int change_factor;
  int width;
  int height;
  Color *pixels;
  neighborhood neighborhood;
  int dim_factor;
} Canvas;

typedef struct {
  int width;
  int height;
  int fps;
  int spawn_num;
  int speed;
} Window;

Canvas create_canvas(int w, int h, neighborhood n, int change_factor, int dim_factor);
void fill_canvas(Canvas c, Color color);
void fill_canvas_random(Canvas c);
void draw_canvas(Canvas c, Window w);
void copy_canvas(Canvas dst, Canvas src);
void print_color(Color color, const char *name);
void update_canvas(Canvas c);
int color_equal(Color a, Color b);
NeighborInfo count_neighbors(Canvas c, int i, int j);
Color random_color(void);
void draw_random_point(Canvas c, Color color);
Color average_neighbors_colors(Color colors[8], int count);
Color tinge_color(Color color, int nudge, int scale);
int clamp(int i, int min, int max);
Color dim_color(Color a, int factor);
void draw_pause_screen(void);
void draw_at_mouse(Canvas c, Window w);
int map(int in, int Amin, int Amax, int Bmin, int Bmax);
void draw_info(Window w, int steps);
int max(int first, ...);

int main(void)
{
  float ratio = 16.f / 10.f; // 16:9 aspect ratio 
  Window w = {
    .width = 1000, 
    .height = w.width * 1 / ratio, 
    .fps = 30, 
    .spawn_num = 20,
    .speed = 1,
  };
  
  Canvas c = create_canvas(w.width, w.height, MOORE, 3, 0);
  c.neighborhood = MOORE;

  CANVAS_AT(c, c.width/2, c.height/2) = WHITE;

  SetConfigFlags(FLAG_WINDOW_RESIZABLE);
  InitWindow(w.width, w.height, "autocolor");
  SetTargetFPS(w.fps);

  int frames = 0;
  int steps = 0;
  bool paused = false;
  bool random_spawn = false;

  while (!WindowShouldClose()) {

    if (IsWindowResized()) {
      w.width = GetScreenWidth();
      w.height = GetScreenHeight();
    }

    if (IsKeyPressed(KEY_C)) fill_canvas(c, BLACK);
    if (IsKeyPressed(KEY_S)) { 
      fill_canvas(c, BLACK);
      for (int i= 0; i < w.spawn_num; i++) draw_random_point(c, random_color());
    }
    if (IsKeyPressed(KEY_R)) random_spawn = !random_spawn;
    if (IsKeyPressed(KEY_SPACE)) paused = !paused;
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) draw_at_mouse(c, w);

    BeginDrawing();

      if (frames % w.fps == 0 && random_spawn) draw_random_point(c, random_color());

      draw_canvas(c, w);

      if (frames % w.speed == 0 && frames != 0 && !paused) { 
        update_canvas(c); 
        steps++; 
      }

      if (paused) draw_pause_screen();
      else draw_info(w, steps);

    EndDrawing();

    frames++;
  }

  CloseWindow();
}

int max(int first, ...)
{
  int max;
  int num = 0;
  va_list args;
  va_start(args, first);
  while ((num = va_arg(args, int)) != 0) {
    if (num > max) max = num;
  }
  va_end(args);
  return max;
}

void draw_info(Window w, int steps)
{
  int fps = GetFPS();
  const char *fps_str  = TextFormat("FPS:  %d", fps);
  const char *step_str = TextFormat("Step: %d", steps);
  int font_size = w.width / 30;
  int fps_len = MeasureText(fps_str, font_size);
  int step_len = MeasureText(step_str, font_size);
  int max_length = MAX(fps_len, step_len);
  DrawRectangle(0, 0, max_length + 10, font_size * 2, ALPHA_GRAY);
  DrawText(fps_str, 5, 0, font_size, WHITE);
  DrawText(step_str, 5, font_size, font_size, WHITE);
}

int map(int in, int minA, int maxA, int minB, int maxB)
{
  float new = minA + ((float)(maxB - minB) / (minA - maxA)) * (in - minA);
  if (new < 0) new *= -1;
  return new;
}

void draw_at_mouse(Canvas c, Window w)
{
  int x_cell = map(GetMouseX(), 0, w.width, 0, c.width);
  int y_cell = map(GetMouseY(), 0, w.height, 0, c.height);
  CANVAS_AT(c, x_cell, y_cell) = random_color();
}

void draw_pause_screen(void) 
{
  int screenWidth = GetScreenWidth();
  int screenHeight = GetScreenHeight();
  Color backgroundColor = ALPHA_GRAY;
  const char* pausedStr = "PAUSED";
  const char* resumeStr = "PRESS SPACE TO RESUME";
  int pausedFontSize = screenWidth / 10;
  int resumeFontSize = screenWidth / 50;
  
  DrawRectangle(0, 0, screenWidth, screenHeight, backgroundColor);
  
  int pausedWidth = MeasureText(pausedStr, pausedFontSize);
  int pausedX = (screenWidth - pausedWidth) / 2;
  int pausedY = (screenHeight - pausedFontSize) / 2;
  DrawText(pausedStr, pausedX, pausedY, pausedFontSize, WHITE);
  
  int resumeWidth = MeasureText(resumeStr, resumeFontSize);
  int resumeX = (screenWidth - resumeWidth) / 2;
  int resumeY = (screenHeight / 2) + (pausedFontSize / 2);
  DrawText(resumeStr, resumeX, resumeY, resumeFontSize, WHITE);
}

Color dim_color(Color a, int factor)
{
  Color dimmed_color = a;
  dimmed_color.r = clamp(a.r - factor, 0, 255);
  dimmed_color.g = clamp(a.g - factor, 0, 255);
  dimmed_color.b = clamp(a.b - factor, 0, 255);
  return dimmed_color;
}

int clamp(int i, int min, int max)
{
  if (i < min) { i = min; }
  if (i > max) { i = max; }
  return i;
}

Color tinge_color(Color color, int nudge, int scale)
{
  int delta = nudge * scale;
  Color tinged_color = {0};
  tinged_color.r = clamp(color.r + GetRandomValue(-delta, delta), 0, 255);
  tinged_color.g = clamp(color.g + GetRandomValue(-delta, delta), 0, 255);
  tinged_color.b = clamp(color.b + GetRandomValue(-delta, delta), 0, 255);
  tinged_color.a = 255;
  return tinged_color;
}

void draw_random_point(Canvas c, Color color)
{
  int max_attempts = 5;
  int attempts = 0;
  bool valid_spot = false;
  int rx, ry;
  while (!valid_spot && attempts <= max_attempts) {
    rx = GetRandomValue(0, c.width  - 1);
    ry = GetRandomValue(0, c.height - 1);
    if (color_equal(CANVAS_AT(c, rx, ry), BLACK)) valid_spot = true;
    else attempts++;
  }
  if (attempts <= max_attempts) CANVAS_AT(c, rx, ry) = color;
}

void copy_canvas(Canvas dst, Canvas src)
{
  for (int i = 0; i < dst.width; i++) {
    for (int j = 0; j < dst.height; j++) {
      CANVAS_AT(dst, i, j) = CANVAS_AT(src, i, j);
    }
  }
}

int color_equal(Color a, Color b)
{
  if (ColorToInt(a) == ColorToInt(b)) return 1;
  else return 0;
}

Color average_neighbors_colors(Color colors[8], int count)
{
  int rsum = 0;
  int gsum = 0;
  int bsum = 0;
  for (int i = 0; i < count; i++) {
    rsum += colors[i].r;
    gsum += colors[i].g;
    bsum += colors[i].b;
  }
  Color avg_color = (Color) {rsum/count, gsum/count, bsum/count, 255};
  return avg_color;
}

NeighborInfo count_neighbors(Canvas c, int i, int j)
{

  NeighborInfo ni = {0};
  Color colors[8] = {0};

  const bool has_left  = (i - 1 >= 0);
  const bool has_right = (i + 1 < c.width);
  const bool has_top  = (j - 1 >= 0);
  const bool has_bttm = (j + 1 < c.height);

  switch (c.neighborhood) {

    case VON_NEUMANN:
      if (has_left  && color_not_equal(CANVAS_AT(c, i - 1, j), BLACK)) colors[ni.count++] = CANVAS_AT(c, i - 1, j);
      if (has_right && color_not_equal(CANVAS_AT(c, i + 1, j), BLACK)) colors[ni.count++] = CANVAS_AT(c, i + 1, j);
      if (has_top   && color_not_equal(CANVAS_AT(c, i, j - 1), BLACK)) colors[ni.count++] = CANVAS_AT(c, i, j - 1);
      if (has_bttm  && color_not_equal(CANVAS_AT(c, i, j + 1), BLACK)) colors[ni.count++] = CANVAS_AT(c, i, j + 1);
      break;

    case MOORE:
      if (has_left  && has_top  && color_not_equal(CANVAS_AT(c, i - 1, j - 1), BLACK)) colors[ni.count++] = CANVAS_AT(c, i - 1, j - 1); 
      if (has_right && has_bttm && color_not_equal(CANVAS_AT(c, i + 1, j + 1), BLACK)) colors[ni.count++] = CANVAS_AT(c, i + 1, j + 1); 
      if (has_left  && has_bttm && color_not_equal(CANVAS_AT(c, i - 1, j + 1), BLACK)) colors[ni.count++] = CANVAS_AT(c, i - 1, j + 1); 
      if (has_right && has_top  && color_not_equal(CANVAS_AT(c, i + 1, j - 1), BLACK)) colors[ni.count++] = CANVAS_AT(c, i + 1, j - 1); 
      if (has_left  && color_not_equal(CANVAS_AT(c, i - 1, j), BLACK)) colors[ni.count++] = CANVAS_AT(c, i - 1, j);
      if (has_right && color_not_equal(CANVAS_AT(c, i + 1, j), BLACK)) colors[ni.count++] = CANVAS_AT(c, i + 1, j);
      if (has_top   && color_not_equal(CANVAS_AT(c, i, j - 1), BLACK)) colors[ni.count++] = CANVAS_AT(c, i, j - 1);
      if (has_bttm  && color_not_equal(CANVAS_AT(c, i, j + 1), BLACK)) colors[ni.count++] = CANVAS_AT(c, i, j + 1);
      break;

    default:
      printf("ERROR: code unreachable...\n");
      exit(0);
      break;
  }

  ni.avg_color = ni.count > 0 ? average_neighbors_colors(colors, ni.count) : BLACK;
  return ni;
}

void update_canvas(Canvas c)
{
  Canvas c_next = create_canvas(c.width, c.height, c.neighborhood, c.change_factor, c.dim_factor);
  Color *curr_pixel;
  Color *next_pixel;

  for (int i = 0; i < c.width; i++) {
    for (int j = 0; j < c.height; j++) {
      curr_pixel = &CANVAS_AT(c, i, j);
      next_pixel = &CANVAS_AT(c_next, i, j);
      if (color_equal(*curr_pixel, BLACK)) {
        NeighborInfo ni = count_neighbors(c, i, j);
        if (ni.count > 0) {
          *next_pixel = GetRandomValue(1, 4) <= ni.count ? tinge_color(ni.avg_color, ni.count, c.change_factor) : BLACK;
        }
      }
      else {
        *next_pixel = dim_color(*curr_pixel, c.dim_factor);
      }
    }
  }
  
  copy_canvas(c, c_next);
}

void print_color(Color color, const char *name)
{
  printf("%s = (%d, %d, %d, %d)\n", name, color.r, color.g, color.b, color.a);
}

Color random_color(void)
{
  int r = GetRandomValue(0, 255);
  int g = GetRandomValue(0, 255);
  int b = GetRandomValue(0, 255);
  Color random_color = {r, g, b, 255};
  return random_color;
}

void fill_canvas_random(Canvas c)
{
  for (int i = 0; i < c.width; i++) {
    for (int j = 0; j < c.height; j++) {
      CANVAS_AT(c, i, j) = random_color();
    }
  }
}

void draw_canvas(Canvas c, Window w)
{
  float x_px = (float)w.width / c.width;
  float y_px = (float)w.height / c.height;
  for (int i = 0; i < c.width; i++) {
    for (int j = 0; j < c.height; j++) {
      DrawRectangle(i * x_px, j * y_px, ceil(x_px), ceil(y_px), CANVAS_AT(c, i, j));
    }
  }
}

void fill_canvas(Canvas c, Color color)
{
  for (int i = 0; i < c.width; i++) {
    for (int j = 0; j < c.height; j++) {
      CANVAS_AT(c, i, j) = color;
    }
  }
}

Canvas create_canvas(int w, int h, neighborhood n, int change_factor, int dim_factor)
{
  Canvas c = {0};
  c.change_factor = change_factor;
  c.dim_factor = dim_factor;
  c.neighborhood = n;
  c.width = w;
  c.height = h;
  c.pixels = malloc(PIXEL_SIZE(c));
  assert(c.pixels != NULL);
  fill_canvas(c, BLACK);
  return c;
}
