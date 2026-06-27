#include "platform.h"
#include "graphics.h"
#include "defs.h"
#include "datatypes.h"
#include "allocator.h"

#include <math.h>

// implementing Conway's Game of Life

#define GRID_SIZE 200

int roundfi(float f)
{
  return (int)(f + 0.5f);
}

typedef enum {
  dead,
  alive,
} tile;

typedef struct {
  bool is_initialised;

  tile *grid, *newgrid;
  tile grids[2 * GRID_SIZE * GRID_SIZE];

  vec2f camera;
  vec2f resolution;
  vec2f mouse;

  float time_since_last_update;

  bool paused;

  gl_state gl_st;

  bool debug_break;

  bool dragging;
  vec2f last_drag_pos;

  float pixels_per_tile;

  allocator allctr;
} game_state;

tile* tile_at(tile* grid, int x, int y)
{
  return grid + y*GRID_SIZE + x;
}

bool in_bounds(vec2i t)
{
  return t.x >= 0 && t.y >= 0 && t.x < GRID_SIZE && t.y < GRID_SIZE;
}

vec2f origin_for_camera(vec2f camera, vec2f resolution, float pixels_per_tile)
{
  return subtract_vec2f(camera, divide_vec2f(divide_vec2f(resolution, 2.0f), pixels_per_tile));
}

vec2i tile_at_pixel(vec2f p, vec2f camera, vec2f resolution, float pixels_per_tile)
{
  vec2f transform = add_vec2f(origin_for_camera(camera, resolution, pixels_per_tile), divide_vec2f(p, pixels_per_tile));
  vec2i fixup = { transform.x < 0.0 ? -1 : 0, transform.y < 0.0 ? -1 : 0 };
  return add_vec2i(vec2f_to_veci2(transform), fixup);
}

rectf pixels_for_tile(vec2i t, vec2f camera, vec2f resolution, float pixels_per_tile)
{
  vec2f coords = multiply_vec2f(subtract_vec2f(vec2i_to_vec2f(t), origin_for_camera(camera, resolution, pixels_per_tile)), pixels_per_tile);
  rectf tile_area = {
    coords.x,
    coords.y,
    pixels_per_tile,
    pixels_per_tile,
  };
  return tile_area;
}

/* ------------------------------------- *
                Graphics
* ------------------------------------- */

void render_mouse_tile(game_state* state)
{
  vec2i tpos = tile_at_pixel(state->mouse, state->camera, state->resolution, state->pixels_per_tile);

  if (in_bounds(tpos))
  {
    rectf rec = pixels_for_tile(tpos, state->camera, state->resolution, state->pixels_per_tile);

    draw_rectangle(&state->gl_st, rec, (colour){ 0.8f, 0.8f, 0.8f, 0.8f });
  }
}

void render_mouse(game_state* state)
{
  rectf tile_area = {
    state->mouse.x - 2.5f,
    state->mouse.y - 2.5f,
    state->pixels_per_tile,
    state->pixels_per_tile,
  };
  draw_rectangle(&state->gl_st, tile_area, (colour){ 0.8f, 0.8f, 0.8f, 0.5f });
}

void game_render(game_state* state)
{
  clear_background((colour){ 0.1f, 0.1f, 0.1f, 1.0f });

  render_grid(&state->gl_st, (int*)state->grid, GRID_SIZE);

  if (state->pixels_per_tile > 20.0f)
  {
    render_grid_borders(&state->gl_st, GRID_SIZE);
  }

  render_mouse_tile(state);

  if (state->paused)
  {
    render_pause_icon(&state->gl_st);
  }
}

/* ------------------------------------- *
            Initialisation
* ------------------------------------- */

void game_init(game_memory* memory)
{
  ASSERT(sizeof(game_state) <= memory->size)
  game_state *state = (game_state*)memory->buffer;

  state->grid = state->grids;
  state->newgrid = state->grids + GRID_SIZE*GRID_SIZE;

  for (int i = 0; i < GRID_SIZE; i++)
  {
    for (int j = 0; j < GRID_SIZE; j++)
    {
      *tile_at(state->grid, j, i) = dead;
    }
  }

  /*int structure_width = 4;
  int structure_height = 4;
  tile structure[4][4] = {
  {1,0,0,1},
  {0,1,0,1},
  {1,0,1,0},
  {1,1,1,1},
  };

  for (int i = 0; i < structure_height; i++)
  {
    for (int j = 0; j < structure_width; j++)
    {
      state->grid[i * GRID_SIZE + j] = structure[i][j];
    }
  }*/

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
      *tile_at(state->grid, j + 2, GRID_SIZE - i - 7) = glider_gun[i][j];
    }
  }

  /*state->grid[1][3] = alive;
  state->grid[2][1] = alive;
  state->grid[2][3] = alive;
  state->grid[3][2] = alive;
  state->grid[3][3] = alive;*/

  state->camera = (vec2f){ (float)GRID_SIZE / 2.0f, (float)GRID_SIZE / 2.0f};

  state->resolution = (vec2f){ 1.0f, 1.0f };

  state->time_since_last_update = 0.0f;

  state->paused = true;

  state->debug_break = false;

  state->pixels_per_tile = 5.0f;

  state->allctr.head = (char*)memory->buffer + sizeof(game_state);
  state->allctr.space = memory->size - sizeof(game_state);

  state->is_initialised = true;
}

/* ------------------------------------- *
               Updating
* ------------------------------------- */

bool is_alive(tile* grid, int i, int j)
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

  return *tile_at(grid, j, i) == alive;
}

void game_update(Input *input, game_state* state)
{
  // handle input
  bool step_once = false;

  for (int i = 0; i < input->events_count; i++)
  {
    event e = input->events[i];
    switch (e.t)
    {
    case EVENT_KEYPRESS:
    {
      switch (e.k)
      {
      case KEY_P:
        state->paused = !state->paused;
        break;
      case KEY_R:
        state->is_initialised = false;
        break;
      case KEY_B:
        state->debug_break = true;
        break;
      case KEY_S:
        step_once = true;
        break;
      case KEY_MOUSEL:
      {
        vec2f mouse = { (float)e.mouse.x, (float)e.mouse.y };
        vec2i tpos = tile_at_pixel(mouse, state->camera, state->resolution, state->pixels_per_tile);
        if (in_bounds(tpos))
        {
          *tile_at(state->grid, tpos.x, tpos.y) ^= alive;
        }
      } break;
      }
    } break;
    }
  }

  state->mouse.x = (float)input->mouse.x;
  state->mouse.y = (float)input->mouse.y;

  if (input->keys[KEY_MOUSER].isdown)
    state->dragging = true;
  else
    state->dragging = false;

  if (state->dragging)
  {
    state->camera = subtract_vec2f(state->camera, divide_vec2f(subtract_vec2f(state->mouse, state->last_drag_pos), state->pixels_per_tile));
  }
  else
  {
    if (input->keys[KEY_UP].isdown)
      state->camera.y += 1.5f;
    if (input->keys[KEY_DOWN].isdown)
      state->camera.y -= 1.5f;
    if (input->keys[KEY_RIGHT].isdown)
      state->camera.x += 1.5f;
    if (input->keys[KEY_LEFT].isdown)
      state->camera.x -= 1.5f;
  }
  state->last_drag_pos = state->mouse;

  state->pixels_per_tile *= expf((float)input->mousewheel_delta / 960.f);

  {
    vec2f newres = (vec2f){(float)input->window_resolution.x, (float)input->window_resolution.y};

    /*if (state->resolution.x == 1.0f)
    {
      state->camera = add_vec2f(state->camera, divide_vec2f(subtract_vec2f(newres, state->resolution), state->pixels_per_tile * 2.0f));
    }*/

    state->resolution = newres;
  }

  state->gl_st.resolution = state->resolution;
  state->gl_st.camera = state->camera;
  state->gl_st.pixels_per_tile = state->pixels_per_tile;

  // simulate

  const float update_frequency = 40.0f;
  const float update_time = 1.0f / update_frequency;

  state->time_since_last_update += input->dt;

  bool should_update = !state->paused || step_once;// && state->time_since_last_update >= update_time;

  if (should_update)
  {
    tile* newgrid = state->newgrid;

    for (int i = 0; i < GRID_SIZE; i++)
    {
      for (int j = 0; j < GRID_SIZE; j++)
      {
        tile old_tile = *tile_at(state->grid, j, i);

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

        *tile_at(newgrid, j, i) = new_tile;
      }
    }

    // swop grids
    {
      tile* tmp = state->grid;
      state->grid = newgrid;
      state->newgrid = tmp;
    }

    state->time_since_last_update = 0.0f;
  }
}

/* ------------------------------------- *
                 Main
* ------------------------------------- */

void game_main(Input *input, game_memory* memory)
{
  game_state *state = (game_state*)memory->buffer;

  if (!state->is_initialised)
  {
    game_init(memory);

    init_graphics(&state->gl_st, state->allctr, GRID_SIZE);
  }

  game_update(input, state);

  game_render(state);
}