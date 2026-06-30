#include "conway.h"
#include "platform.h"
#include "graphics.h"
#include "defs.h"
#include "datatypes.h"
#include "allocator.h"

#include <math.h>

// implementing Conway's Game of Life

#define CONWAY_GRID_SIZE 200

typedef enum {
  dead,
  alive,
} tile;

typedef struct {
  // shader programs
  GLuint prog_cells, prog_borders, prog_rect;
  // buffer objects
  union {
    GLuint buffers;
    struct {
      GLuint
        vbo_grid, vbo_rect,
        ebo_cells, ebo_borders,
        ssbo;
    };
  };
  // state objects
  union {
    GLuint vertex_arrays;
    struct {
      GLuint
        vao_cells, vao_borders, vao_rect;
    };
  };
  // parameters
  vec2f camera;
  vec2f resolution;
  float pixels_per_tile;
} gl_state;

typedef struct {
  bool is_initialised;

  vec2f resolution;
  vec2f mouse;

  bool debug_break;

  allocator allctr;

  vec2f camera;

  tile *grid, *newgrid;
  tile grids[2 * CONWAY_GRID_SIZE * CONWAY_GRID_SIZE];

  float time_since_last_update;

  bool paused;

  gl_state gl_st;

  bool dragging;
  vec2f last_drag_pos;

  float pixels_per_tile;
} conway_game_state;

INTERNAL
tile* tile_at(tile* grid, int x, int y)
{
  return grid + y*CONWAY_GRID_SIZE + x;
}

INTERNAL
bool in_bounds(vec2i t)
{
  return t.x >= 0 && t.y >= 0 && t.x < CONWAY_GRID_SIZE && t.y < CONWAY_GRID_SIZE;
}

INTERNAL
vec2f origin_for_camera(vec2f camera, vec2f resolution, float pixels_per_tile)
{
  return subtract_vec2f(camera, divide_vec2f(divide_vec2f(resolution, 2.0f), pixels_per_tile));
}

INTERNAL
vec2i tile_at_pixel(vec2f p, vec2f camera, vec2f resolution, float pixels_per_tile)
{
  vec2f transform = add_vec2f(origin_for_camera(camera, resolution, pixels_per_tile), divide_vec2f(p, pixels_per_tile));
  vec2i fixup = { transform.x < 0.0 ? -1 : 0, transform.y < 0.0 ? -1 : 0 };
  return add_vec2i(vec2f_to_veci2(transform), fixup);
}

INTERNAL
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

INTERNAL
const char vs_str_2d_abs[] =
"#version 430 core\n"
"in vec2 coord2d;"
"uniform vec2 resolution;"
"void main(void) {"
"  gl_Position = vec4(coord2d/resolution*2.0-1.0, 0.0, 1.0);"
"}";

INTERNAL
const char fs_str_uniform[] =
"#version 430 core\n"
"out vec4 fragColor;"
"uniform vec4 colour;"
"void main(void) {"
"  fragColor = colour;"
"}";

INTERNAL
const char vs_str_tiles[] =
"#version 430 core\n"
"in vec3 coord;"
"uniform vec2 camera;"
"uniform vec2 resolution;"
"uniform float pixels_per_tile;"
"void main(void) {"
"  vec3 origin_for_camera = vec3(camera - resolution/2.0f/pixels_per_tile, 0.0);"
"  gl_Position = vec4((coord - origin_for_camera)*pixels_per_tile/vec3(resolution, 1.0)*2.0-1.0, 1.0);"
"}";

INTERNAL
const char fs_str_tiles[] =
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

INTERNAL
const char fs_str_borders[] =
"#version 430 core\n"
"out vec4 fragColor;"
"uniform float pixels_per_tile;"
"void main(void) {"
"  float alpha = clamp(pixels_per_tile / 100.0, 0.0, 1.0);"
"  fragColor = vec4(0.125, 0.125, 0.125, alpha);"
"}";

INTERNAL
void init_graphics(gl_state* state, allocator scratch, int grid_size)
{
  GLuint vs = new_shader(GL_VERTEX_SHADER, vs_str_tiles);
  GLuint fs = new_shader(GL_FRAGMENT_SHADER, fs_str_tiles);
  state->prog_cells = new_shader_program(vs, fs);

  fs = new_shader(GL_FRAGMENT_SHADER, fs_str_borders);
  state->prog_borders = new_shader_program(vs, fs);

  vs = new_shader(GL_VERTEX_SHADER, vs_str_2d_abs);
  fs = new_shader(GL_FRAGMENT_SHADER, fs_str_uniform);
  state->prog_rect = new_shader_program(vs, fs);

  long long int vertices_size = sizeof(vec3f) * (grid_size + 1) * (grid_size + 1);
  vec3f *vertices = alloc(&scratch, vertices_size);

  long long int cell_elements_size = 6 * sizeof(GLuint) * (grid_size) * (grid_size);
  struct { GLuint bl, tl1, br1, br2, tl2, tr; } *cell_elements = alloc(&scratch, cell_elements_size);

  long long int border_elements_size = 8 * sizeof(GLuint) * (grid_size) * (grid_size);
  struct { GLuint l1, l2, t1, t2, r1, r2, b1, b2; } *border_elements = alloc(&scratch, border_elements_size);

  for (int i = 0; i <= grid_size; i++)
  {
    for (int j = 0; j <= grid_size; j++)
    {
      vertices[i * (grid_size + 1) + j] = (vec3f){ (float)j, (float)i, 0.0f };

      if (i < grid_size && j < grid_size)
      {
        GLuint bl = (i)     * (grid_size + 1) + j;
        GLuint tl = (i + 1) * (grid_size + 1) + j;
        GLuint br = (i)     * (grid_size + 1) + j + 1;
        GLuint tr = (i + 1) * (grid_size + 1) + j + 1;

        cell_elements[i * grid_size + j].bl  = bl;
        cell_elements[i * grid_size + j].tl1 = tl;
        cell_elements[i * grid_size + j].tl2 = tl;
        cell_elements[i * grid_size + j].tr  = tr;
        cell_elements[i * grid_size + j].br1 = br;
        cell_elements[i * grid_size + j].br2 = br;

        border_elements[i * grid_size + j].l1 = bl;
        border_elements[i * grid_size + j].l2 = tl;
        border_elements[i * grid_size + j].t1 = tl;
        border_elements[i * grid_size + j].t2 = tr;
        border_elements[i * grid_size + j].r1 = tr;
        border_elements[i * grid_size + j].r2 = br;
        border_elements[i * grid_size + j].b1 = br;
        border_elements[i * grid_size + j].b2 = bl;
      }
    }
  }

  glGenBuffers(5, &state->buffers);
  CATCH_GL_ERROR(errstr);

  glGenVertexArrays(3, &state->vertex_arrays);

  glBindVertexArray(state->vao_cells);
  {
    glBindBuffer(GL_ARRAY_BUFFER, state->vbo_grid);
    glBufferData(GL_ARRAY_BUFFER, vertices_size, vertices, GL_STATIC_DRAW);
    CATCH_GL_ERROR(errstr);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, state->ebo_cells);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, cell_elements_size, cell_elements, GL_STATIC_DRAW);
    CATCH_GL_ERROR(errstr);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, state->ssbo);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, state->ssbo);

    glUseProgram(state->prog_cells);
    GLuint attr = glGetAttribLocation(state->prog_cells, "coord");
    CATCH_GL_ERROR(errstr);

    glVertexAttribPointer(attr, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    CATCH_GL_ERROR(errstr);

    glEnableVertexAttribArray(attr);
    CATCH_GL_ERROR(errstr);
  }

  glBindVertexArray(state->vao_borders);
  {
    glBindBuffer(GL_ARRAY_BUFFER, state->vbo_grid);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, state->ebo_borders);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, border_elements_size, border_elements, GL_STATIC_DRAW);
    CATCH_GL_ERROR(errstr);

    glUseProgram(state->prog_borders);
    GLuint attr = glGetAttribLocation(state->prog_borders, "coord");
    CATCH_GL_ERROR(errstr);

    glVertexAttribPointer(attr, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    CATCH_GL_ERROR(errstr);

    glEnableVertexAttribArray(attr);
    CATCH_GL_ERROR(errstr);
  }

  glBindVertexArray(state->vao_rect);
  {
    glBindBuffer(GL_ARRAY_BUFFER, state->vbo_rect);

    glUseProgram(state->prog_rect);
    GLuint attr = glGetAttribLocation(state->prog_rect, "coord2d");
    CATCH_GL_ERROR(errstr);

    glVertexAttribPointer(attr, 2, GL_FLOAT, GL_FALSE, 0, NULL);
    CATCH_GL_ERROR(errstr);

    glEnableVertexAttribArray(attr);
    CATCH_GL_ERROR(errstr);
  }

  glBindVertexArray(0);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

}

INTERNAL
void render_grid(gl_state* state, int* grid, int grid_size)
{
  glUseProgram(state->prog_cells);

  glUniform2f(glGetUniformLocation(state->prog_cells, "camera"), state->camera.x, state->camera.y);
  glUniform2f(glGetUniformLocation(state->prog_cells, "resolution"), state->resolution.x, state->resolution.y);
  glUniform1f(glGetUniformLocation(state->prog_cells, "pixels_per_tile"), state->pixels_per_tile);

  glBindVertexArray(state->vao_cells);

  glBufferData(GL_SHADER_STORAGE_BUFFER, grid_size * grid_size * sizeof(*grid), grid, GL_DYNAMIC_COPY);
  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

  glDrawElements(GL_TRIANGLES, 6 * grid_size * grid_size, GL_UNSIGNED_INT, NULL);
  CATCH_GL_ERROR(errstr);

  glBindVertexArray(0);
}

INTERNAL
void render_grid_borders(gl_state* state, int grid_size)
{
  glUseProgram(state->prog_borders);

  glUniform2f(glGetUniformLocation(state->prog_borders, "camera"), state->camera.x, state->camera.y);
  glUniform2f(glGetUniformLocation(state->prog_borders, "resolution"), state->resolution.x, state->resolution.y);
  glUniform1f(glGetUniformLocation(state->prog_borders, "pixels_per_tile"), state->pixels_per_tile);

  glBindVertexArray(state->vao_borders);

  glDrawElements(GL_LINES, 8 * grid_size * grid_size, GL_UNSIGNED_INT, NULL);
  CATCH_GL_ERROR(errstr);

  glBindVertexArray(0);
}

INTERNAL
void draw_rectangle(gl_state* state, rectf r, colour c)
{
  float vertices[] = {
    r.x, r.y,
    r.x, r.y + r.h,
    r.x + r.w, r.y,
    r.x, r.y + r.h,
    r.x + r.w, r.y + r.h,
    r.x + r.w, r.y,
  };
  glBindVertexArray(state->vao_rect);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
  CATCH_GL_ERROR(errstr);

  glUseProgram(state->prog_rect);

  glUniform2f(glGetUniformLocation(state->prog_rect, "resolution"), state->resolution.x, state->resolution.y);
  glUniform4f(glGetUniformLocation(state->prog_rect, "colour"), c.r, c.g, c.b, c.a);

  glDrawArrays(GL_TRIANGLES, 0, 6);
  CATCH_GL_ERROR(errstr);

  glBindVertexArray(0);
}
INTERNAL
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


INTERNAL
void render_mouse_tile(conway_game_state* state)
{
  vec2i tpos = tile_at_pixel(state->mouse, state->camera, state->resolution, state->pixels_per_tile);

  if (in_bounds(tpos))
  {
    rectf rec = pixels_for_tile(tpos, state->camera, state->resolution, state->pixels_per_tile);

    draw_rectangle(&state->gl_st, rec, (colour){ 0.8f, 0.8f, 0.8f, 0.8f });
  }
}

INTERNAL
void render_mouse(conway_game_state* state)
{
  rectf tile_area = {
    state->mouse.x - 2.5f,
    state->mouse.y - 2.5f,
    state->pixels_per_tile,
    state->pixels_per_tile,
  };
  draw_rectangle(&state->gl_st, tile_area, (colour){ 0.8f, 0.8f, 0.8f, 0.5f });
}

INTERNAL
void game_render(conway_game_state* state)
{
  clear_background((colour){ 0.1f, 0.1f, 0.1f, 1.0f });

  render_grid(&state->gl_st, (int*)state->grid, CONWAY_GRID_SIZE);

  if (state->pixels_per_tile > 20.0f)
  {
    render_grid_borders(&state->gl_st, CONWAY_GRID_SIZE);
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

INTERNAL
void game_init(game_memory* memory)
{
  ASSERT(sizeof(conway_game_state) <= memory->size)
    conway_game_state *state = (conway_game_state*)memory->buffer;

  state->grid = state->grids;
  state->newgrid = state->grids + CONWAY_GRID_SIZE*CONWAY_GRID_SIZE;

  for (int i = 0; i < CONWAY_GRID_SIZE; i++)
  {
    for (int j = 0; j < CONWAY_GRID_SIZE; j++)
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
  state->grid[i * CONWAY_GRID_SIZE + j] = structure[i][j];
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
      *tile_at(state->grid, j + 2, CONWAY_GRID_SIZE - i - 7) = glider_gun[i][j];
    }
  }

  /*state->grid[1][3] = alive;
  state->grid[2][1] = alive;
  state->grid[2][3] = alive;
  state->grid[3][2] = alive;
  state->grid[3][3] = alive;*/

  state->camera = (vec2f){ (float)CONWAY_GRID_SIZE / 2.0f, (float)CONWAY_GRID_SIZE / 2.0f};

  state->resolution = (vec2f){ 1.0f, 1.0f };

  state->time_since_last_update = 0.0f;

  state->paused = true;

  state->debug_break = false;

  state->pixels_per_tile = 5.0f;

  state->allctr.head = (char*)memory->buffer + sizeof(conway_game_state);
  state->allctr.space = memory->size - sizeof(conway_game_state);

  state->is_initialised = true;
}

/* ------------------------------------- *
                 Updating
* ------------------------------------- */

INTERNAL
bool is_alive(tile* grid, int i, int j)
{
  if (false)
  {
    if (i < 0)
    {
      return false;
    }
    if (i >= CONWAY_GRID_SIZE)
    {
      return false;
    }
    if (j < 0)
    {
      return false;
    }
    if (j >= CONWAY_GRID_SIZE)
    {
      return false;
    }
  }
  else
  {
    i = (i + CONWAY_GRID_SIZE) % CONWAY_GRID_SIZE;
    j = (j + CONWAY_GRID_SIZE) % CONWAY_GRID_SIZE;
  }

  return *tile_at(grid, j, i) == alive;
}

INTERNAL
void game_update(Input *input, conway_game_state* state)
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

    for (int i = 0; i < CONWAY_GRID_SIZE; i++)
    {
      for (int j = 0; j < CONWAY_GRID_SIZE; j++)
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

void conway_game_main(Input *input, game_memory* memory)
{
  conway_game_state *state = (conway_game_state*)memory->buffer;

  if (!state->is_initialised)
  {
    game_init(memory);

    init_graphics(&state->gl_st, state->allctr, CONWAY_GRID_SIZE);
  }

  game_update(input, state);

  game_render(state);
}
