#include "platform.h"
#include "opengl.h"
#include "defs.h"
#include "datatypes.h"

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

  struct {
    GLuint vs, fs, prog, vbo_coord2d, attr_coord2d, vbo_alive, attr_alive, ssbo;
    struct { vec2f tl, tr1, bl1, bl2, tr2, br; } vertices[GRID_SIZE][GRID_SIZE];
  } gl;

  gl_state gl_st;

  bool debug_break;

  bool dragging;
  vec2f last_drag_pos;

  float pixels_per_tile;
} game_state;

tile* tile_at(tile* grid, int x, int y)
{
  return grid + y*GRID_SIZE + x;
}

bool in_bounds(vec2i t)
{
  return t.x >= 0 && t.y >= 0 && t.x < GRID_SIZE && t.y < GRID_SIZE;
}

vec2i tile_at_pixel(vec2f p, vec2f camera, float pixels_per_tile)
{
  return vec2f_to_veci2(add_vec2f(camera, divide_vec2f(p, pixels_per_tile)));
}

rectf pixels_for_tile(vec2i t, vec2f camera, float pixels_per_tile)
{
  vec2f coords = multiply_vec2f(subtract_vec2f(vec2i_to_vec2f(t), camera), pixels_per_tile);
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

const char vs_str[] =
"#version 430 core\n"
"in vec2 coord2d;"
"uniform vec2 camera;"
"uniform vec2 resolution;"
"uniform float pixels_per_tile;"
"void main(void) {"
"  gl_Position = vec4((coord2d - camera)*pixels_per_tile/resolution*2.0-1.0, 0.0, 1.0);"
"}";

const char fs_str[] =
"#version 430 core\n"
"layout(std430, binding = 0) buffer ssbo {"
"  int grid[];"
"};"
"out vec4 fragColor;"
"void main(void) {"
"  int alive = grid[gl_PrimitiveID/2];"
"  if (alive == 1) {"
"    fragColor = vec4(220.0f/255.0f, 191.0f/255.0f, 1.0f/255.0f, 1.0);"
"  } else {"
"    fragColor = vec4(88.0f/255.0f, 57.0f/255.0f, 39.0f/255.0f, 1.0);"
"  }"
"}";

void init_graphics(game_state* state)
{
  state->gl.vs = new_shader(GL_VERTEX_SHADER, vs_str);
  state->gl.fs = new_shader(GL_FRAGMENT_SHADER, fs_str);
  state->gl.prog = new_shader_program(state->gl.vs, state->gl.fs);

  glGenBuffers(1, &state->gl.vbo_coord2d);
  CATCH_GL_ERROR(errstr);

  for (int i = 0; i < GRID_SIZE; i++)
  {
    for (int j = 0; j < GRID_SIZE; j++)
    {
      state->gl.vertices[i][j].tl  = (vec2f){ (float)(j)  , (float)(i)   };
      state->gl.vertices[i][j].tr1 = (vec2f){ (float)(j+1), (float)(i)   };
      state->gl.vertices[i][j].bl1 = (vec2f){ (float)(j)  , (float)(i+1) };
      state->gl.vertices[i][j].br  = (vec2f){ (float)(j+1), (float)(i+1) };
      state->gl.vertices[i][j].bl2 = state->gl.vertices[i][j].bl1;
      state->gl.vertices[i][j].tr2 = state->gl.vertices[i][j].tr1;
    }
  }

  glBindBuffer(GL_ARRAY_BUFFER,state->gl.vbo_coord2d);
  glBufferData(GL_ARRAY_BUFFER, sizeof(state->gl.vertices), state->gl.vertices, GL_STATIC_DRAW);
  CATCH_GL_ERROR(errstr);;

  glGenBuffers(1, &state->gl.ssbo);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, state->gl.ssbo);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, state->gl.ssbo);

  init_graphics_generic(&state->gl_st);
}

void render_grid(game_state* state)
{
  glUseProgram(state->gl.prog);

  glUniform2f(glGetUniformLocation(state->gl.prog, "camera"), state->camera.x, state->camera.y);
  glUniform2f(glGetUniformLocation(state->gl.prog, "resolution"), state->resolution.x, state->resolution.y);
  glUniform1f(glGetUniformLocation(state->gl.prog, "pixels_per_tile"), state->pixels_per_tile);

  glBufferData(GL_SHADER_STORAGE_BUFFER, GRID_SIZE * GRID_SIZE * sizeof(tile), state->grid, GL_DYNAMIC_COPY);
  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

  glUniform1iv(glGetUniformLocation(state->gl.prog, "grid"), GRID_SIZE * GRID_SIZE, (const GLint*)state->grid);

  glBindBuffer(GL_ARRAY_BUFFER,state->gl.vbo_coord2d);
  state->gl.attr_coord2d = glGetAttribLocation(state->gl.prog, "coord2d");
  CATCH_GL_ERROR(errstr);

  glVertexAttribPointer(
    state->gl.attr_coord2d,  // attribute
    2,                  // number of elements per vertex; here (x,y)
    GL_FLOAT,           // type of each element
    GL_FALSE,           // take our values as-is
    0,                  // no extra data between each position
    0                   // offset of first element    
  );
  CATCH_GL_ERROR(errstr);

  glEnableVertexAttribArray(state->gl.attr_coord2d);
  CATCH_GL_ERROR(errstr);

  glDrawArrays(GL_TRIANGLES, 0, 6 * GRID_SIZE * GRID_SIZE);
  CATCH_GL_ERROR(errstr);

  glDisableVertexAttribArray(state->gl.attr_coord2d);
  CATCH_GL_ERROR(errstr);
}

void render_pause_icon(gl_state* gl_st)
{
  float pause_icon_width = 20.0f;
  float pause_icon_height = 20.0f;
  float pause_icon_pos_x = gl_st->resolution.x - pause_icon_width - 10.0f;
  float pause_icon_pos_y = gl_st->resolution.y - pause_icon_height - 10.0f;
  rectf pause_l = {pause_icon_pos_x,      pause_icon_pos_y, 6.0f, 20.0f};
  rectf pause_r = {pause_icon_pos_x + 14, pause_icon_pos_y, 6.0f, 20.0f};
  draw_rectangle(gl_st, pause_l, (colour){ 1.0f, 1.0f, 1.0f, 1.0f });
  draw_rectangle(gl_st, pause_r, (colour){ 1.0f, 1.0f, 1.0f, 1.0f });
}

void render_mouse_tile(game_state* state)
{
  if (state->debug_break)
    state->debug_break = false;
  
  vec2i tpos = tile_at_pixel(state->mouse, state->camera, state->pixels_per_tile);

  if (in_bounds(tpos))
  {
    rectf rec = pixels_for_tile(tpos, state->camera, state->pixels_per_tile);

    draw_rectangle(&state->gl_st, rec, (colour){ 0.8f, 0.8f, 0.8f, 0.8f });
  }
}

void render_mouse(game_state* state)
{
  if (state->debug_break)
    state->debug_break = false;

  rectf tile_area = {
    state->mouse.x - 2.5f,
    state->mouse.y - 2.5f,
    state->pixels_per_tile,
    state->pixels_per_tile,
  };
  draw_rectangle(&state->gl_st, tile_area, (colour){ 0.8f, 0.8f, 0.8f, 0.8f });
}

void game_render(game_state* state)
{
  clear_background((colour){ 0.1f, 0.1f, 0.1f, 1.0f });
  CATCH_GL_ERROR(errstr);

  render_grid(state);

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

  state->camera = (vec2f){ 0.0f, 0.0f };

  state->time_since_last_update = 0.0f;

  state->paused = true;

  state->debug_break = false;

  state->pixels_per_tile = 5.0f;

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
      case KEY_MOUSEL:
      {
        vec2f mouse = { (float)e.mouse.x, (float)e.mouse.y };
        vec2i tpos = tile_at_pixel(mouse, state->camera, state->pixels_per_tile);
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

  state->resolution.x = (float)input->window_resolution.x;
  state->resolution.y = (float)input->window_resolution.y;
  state->gl_st.resolution = state->resolution;

  // simulate

  const float update_frequency = 40.0f;
  const float update_time = 1.0f / update_frequency;

  state->time_since_last_update += input->dt;

  bool should_update = !state->paused;// && state->time_since_last_update >= update_time;

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

    init_graphics(state);
  }

  game_update(input, state);

  game_render(state);
}