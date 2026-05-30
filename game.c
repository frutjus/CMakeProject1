#include "platform.h"

// implementing Conway's Game of Life

#define GRID_SIZE 200
#define VIEWPORT_SIZE 540

typedef enum {
  dead,
  alive,
} tile;

typedef union {
  struct {
    unsigned char b, g, r, a;
  };
  unsigned int v;
} pixel;

#define RGB(r,g,b) ((pixel){.v=(((unsigned int)((r) * 255.0f) << 16) + ((unsigned int)((g) * 255.0f) << 8) + (unsigned int)((b) * 255.0f))})
// for some reason the below is much slower than the above method
//#define RGB(r1,g1,b1) ((pixel){ \
//  .r=(unsigned char)((r1) * 255.0f), \
//  .g=(unsigned char)((g1) * 255.0f), \
//  .b=(unsigned char)((b1) * 255.0f), \
//  .a=255})

int roundfi(float f)
{
  return (int)(f + 0.5f);
}

typedef struct {
  float x, y, w, h;
} rectf;

typedef struct {
  int x, y, w, h;
} recti;

typedef struct {
  pixel* pixels;
  int width;
  int height;
  int stride;
} window;

pixel* pixel_at(window* win, int x, int y)
{
  return win->pixels + y * win->stride + x;
}

window div(window* w, recti r)
{
  window newwin = *w;

  newwin.pixels = pixel_at(w, r.x, r.y);
  newwin.width = r.w;
  newwin.height = r.h;

  return newwin;
}

window window_from_pixel_buffer(pixel_buffer* pixels)
{
  window newwin;
  newwin.pixels = (pixel*)pixels->buffer;
  newwin.width = pixels->width;
  newwin.height = pixels->height;
  newwin.stride = pixels->stride / pixels->bytesPerPixel;
  return newwin;
}

typedef struct {
  float x, y;
} vecf2;

typedef struct {
  bool is_initialised;

  tile* grid;
  tile grids[2][GRID_SIZE][GRID_SIZE];
  int grid_index;

  window main_window;
  window viewport;

  vecf2 camera;

  float time_since_last_update;

  bool paused;
} game_state;

void game_init(game_memory* memory, pixel_buffer* pixels)
{
  ASSERT(sizeof(game_state) <= memory->size)
  game_state *state = (game_state*)memory->buffer;

  state->main_window = window_from_pixel_buffer(pixels);

  state->grid_index = 0;
  state->grid = state->grids[state->grid_index];

  for (int i = 0; i < GRID_SIZE; i++)
  {
    for (int j = 0; j < GRID_SIZE; j++)
    {
      state->grid[i * GRID_SIZE + j] = dead;
    }
  }

  int glider_gun_width = 36;
  int glider_gun_height = 9;
  tile glider_gun[9][36] = {
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1},
    {0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,1,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1},
    {1,1,0,0,0,0,0,0,0,0,1,0,0,0,0,0,1,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {1,1,0,0,0,0,0,0,0,0,1,0,0,0,1,0,1,1,0,0,0,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  };

  for (int i = 0; i < glider_gun_height; i++)
  {
    for (int j = 0; j < glider_gun_width; j++)
    {
      state->grid[(i+40) * GRID_SIZE + j+2] = glider_gun[i][j];
    }
  }

  /*state->grid[1][3] = alive;
  state->grid[2][1] = alive;
  state->grid[2][3] = alive;
  state->grid[3][2] = alive;
  state->grid[3][3] = alive;*/

  state->viewport = div(&state->main_window, (recti){ .x = 0, .y = 0, .w = VIEWPORT_SIZE, .h = VIEWPORT_SIZE });

  state->camera = (vecf2){0.0f, 0.0f};

  state->time_since_last_update = 0.0f;

  state->paused = true;

  state->is_initialised = true;
}

bool is_alive(tile grid[GRID_SIZE][GRID_SIZE], int i, int j)
{
  if (false)
  {
    if (i < 0)
    {
      return false;
    }
    if (i >= GRID_SIZE)
    {
      return false;
    }
    if (j < 0)
    {
      return false;
    }
    if (j >= GRID_SIZE)
    {
      return false;
    }
  }
  else
  {
    i = (i + GRID_SIZE) % GRID_SIZE;
    j = (j + GRID_SIZE) % GRID_SIZE;
  }

  return grid[i][j] == alive;
}

void game_update(Input *input, game_state* state)
{
  // handle input

  event* e = input->events;
  for (int i = 0; i < input->events_count; i++)
  {
    switch (e->t)
    {
    case EVENT_KEYPRESS:
    {
      switch (e->k)
      {
      case KEY_P:
        state->paused = !state->paused;
        break;
      case KEY_R:
        state->is_initialised = false;
        break;
      }
    } break;
    }

    e++;
  }
  input->events = e;

  if (input->keys[KEY_UP].isdown)
    state->camera.y -= 5.f;
  if (input->keys[KEY_DOWN].isdown)
    state->camera.y += 5.f;
  if (input->keys[KEY_RIGHT].isdown)
    state->camera.x += 5.f;
  if (input->keys[KEY_LEFT].isdown)
    state->camera.x -= 5.f;

  // simulate

  const float update_frequency = 40.0f;
  const float update_time = 1.0f / update_frequency;

  state->time_since_last_update += input->dt;

  bool should_update = !state->paused;// && state->time_since_last_update >= update_time;

  if (should_update)
  {
    tile* new_grid = state->grids[(state->grid_index + 1) % 2];

    for (int i = 0; i < GRID_SIZE; i++)
    {
      for (int j = 0; j < GRID_SIZE; j++)
      {
        tile old_tile = state->grid[i * GRID_SIZE + j];

        int neighbour_count = 0;

        for (int k = -1; k <= 1; k++)
        {
          for (int l = -1; l <= 1; l++)
          {
            if (!(k == 0 && l == 0) && is_alive(state->grid, i + k, j + l))
            {
              neighbour_count++;
            }
          }
        }

        tile new_tile;

        if (old_tile == alive)
        {
          if (neighbour_count < 2 || neighbour_count > 3)
          {
            new_tile = dead;
          }
          else
          {
            new_tile = alive;
          }
        }
        else
        {
          if (neighbour_count == 3)
          {
            new_tile = alive;
          }
          else
          {
            new_tile = dead;
          }
        }

        new_grid[i * GRID_SIZE + j] = new_tile;
      }
    }

    state->grid_index = (state->grid_index + 1) % 2;
    state->grid = new_grid;

    state->time_since_last_update = 0.0f;
  }
}

void draw_rectangle(window* win, rectf r, pixel c)
{
  int minx = roundfi(r.x);
  int miny = roundfi(r.y);
  int maxx = roundfi(r.x + r.w);
  int maxy = roundfi(r.y + r.h);

  if (minx < 0)
  {
    minx = 0;
  }
  if (miny < 0)
  {
    miny = 0;
  }
  if (maxx > win->width)
  {
    maxx = win->width;
  }
  if (maxy > win->height)
  {
    maxy = win->height;
  }

  for (int y = miny; y < maxy; y++)
  {
    for (int x = minx; x < maxx; x++)
    {
      *pixel_at(win, x, y) = c;
    }
  }
}

void clear_background(window* win, pixel c)
{
  draw_rectangle(win, (rectf){ .x = 0.0f, .y = 0.0f, .w = (float)win->width, .h = (float)win->height }, c);
}

void render_grid(window* win, vecf2 camera, game_state* state)
{
  float pixels_per_tile = (float)VIEWPORT_SIZE / (float)GRID_SIZE * 2.0f;

  for (int tile_y = 0; tile_y < GRID_SIZE; tile_y++)
  {
    for (int tile_x = 0; tile_x < GRID_SIZE; tile_x++)
    {
      pixel p;

      if (is_alive(state->grid, tile_y, tile_x))
      {
        p = RGB(220.0f/255.0f, 191.0f/255.0f, 1.0f/255.0f);
      }
      else
      {
        p = RGB(88.0f/255.0f, 57.0f/255.0f, 39.0f/255.0f);
      }

      rectf r = {
        .x = (float)tile_x * pixels_per_tile - camera.x,
        .y = (float)tile_y * pixels_per_tile - camera.y,
        .w = (float)pixels_per_tile,
        .h = (float)pixels_per_tile,
      };

      draw_rectangle(win, r, p);
    }
  }
}

void render_pause_icon(window* win)
{
  int pause_icon_width = 20;
  int pause_icon_height = 20;
  int pause_icon_pos_x = win->width - pause_icon_width - 10;
  int pause_icon_pos_y = 10;
  rectf pause_l = {(float)pause_icon_pos_x, (float)pause_icon_pos_y, (float)6, (float)20};
  rectf pause_r = {(float)pause_icon_pos_x + 14, (float)pause_icon_pos_y, (float)6, (float)20};
  draw_rectangle(win, pause_l, RGB(1.0,1.0,1.0));
  draw_rectangle(win, pause_r, RGB(1.0,1.0,1.0));
}

void game_render(pixel_buffer* pixels, game_state* state)
{
  clear_background(&state->main_window, RGB(0.1, 0.1, 0.1));

  render_grid(&state->main_window, state->camera, state);

  if (state->paused)
  {
    render_pause_icon(&state->main_window);
  }
}

void game_main(Input *input, pixel_buffer* pixels, game_memory* memory)
{
  game_state *state = (game_state*)memory->buffer;

  if (!state->is_initialised)
  {
    game_init(memory, pixels);
  }

  game_update(input, state);

  game_render(pixels, state);
}