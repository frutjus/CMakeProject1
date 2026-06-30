#include "conway.h"
#include "platform.h"
#include "graphics.h"
#include "defs.h"
#include "datatypes.h"
#include "allocator.h"

#include <math.h>

typedef enum {
  TT_invalid = -1,
  TT_empty,
  TT_grass,
  TT_desert,
  TT_mountain,
  TT_water
} Tile_terrain;

typedef struct {
  GLuint id;
  GLenum type;
  char* path;
  unsigned long long int last_file_time;
  bool needs_relink;
} gl_shader;

typedef struct {
  GLuint id;
  gl_shader *vs, *fs;
} gl_program;

typedef struct {
  // shader programs
  union { gl_program all[3];
    struct { gl_program
      cells, borders, rect;
    };
  } prog;

  union { gl_shader all[5];
    struct { gl_shader
      fs_tiles, vs_tiles, fs_borders, vs_2d_abs, fs_uniform;
    };
  } shdr;

  // buffer objects
  union { GLuint buffers;
    struct { GLuint
      vbo_grid, vbo_rect,
      ebo_cells, ebo_borders,
      ssbo;
    };
  };
  // state objects
  union { GLuint vertex_arrays;
    struct { GLuint
      vao_cells, vao_borders, vao_rect;
    };
  };
  // parameters
  vec2f camera;
  vec2f resolution;
  float pixels_per_tile;
} gl_state;

#define SHADER_FOLDER "../../../shaders/"

typedef struct {
  Tile_terrain *tiles;
  int width, height;
} Tilemap;

typedef struct {
  bool is_initialised;

  vec2f resolution;
  vec2f mouse;

  bool debug_break;

  allocator allctr;

  vec2f camera;

  Tilemap *tilemap;
  Tile_terrain cursor_terrain;

  bool paused;

  gl_state gl_st;

  bool dragging;
  vec2f last_drag_pos;

  float pixels_per_tile;
} game_state;

INTERNAL
Tile_terrain* tile_at(Tilemap* tilemap, int x, int y)
{
  return tilemap->tiles + y * tilemap->width + x;
}

INTERNAL
bool in_bounds(Tilemap* tilemap, vec2i t)
{
  return t.x >= 0 && t.y >= 0 && t.x < tilemap->width && t.y < tilemap->height;
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

/* ------------------------------------- */
/*             Initialisation            */
/* ------------------------------------- */

Tilemap* new_tilemap(allocator *allctr, int width, int height)
{
  Tilemap *tilemap = (Tilemap*)alloc(allctr, sizeof(Tilemap));
  tilemap->width = width;
  tilemap->height = height;
  tilemap->tiles = (Tile_terrain*)alloc(allctr, sizeof(Tile_terrain) * width * height);
  return tilemap;
}

INTERNAL
void game_init(game_memory* memory)
{
  ASSERT(sizeof(game_state) <= memory->size);
  game_state *state = (game_state*)memory->buffer;

  state->allctr.head = (char*)memory->buffer + sizeof(game_state);
  state->allctr.space = memory->size - sizeof(game_state);

  state->tilemap = new_tilemap(&state->allctr, 32, 32);

  Tile_terrain initial_terrain[] = {
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
    3,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,4,
    3,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,4,
    3,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,4,
    3,1,1,1,1,1,1,1,1,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,4,
    3,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,4,
    3,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,4,
    3,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,4,
    3,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,4,
    3,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,4,
    3,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,4,
    3,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,4,
    3,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,4,
    3,1,1,1,1,1,1,1,1,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,4,
    3,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,4,4,
    3,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,4,4,
    3,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,4,4,4,
    3,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,4,4,4,4,
    3,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,4,4,4,4,4,
    3,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,4,4,4,4,4,4,
    3,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,4,4,4,4,4,4,4,
    3,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,4,4,4,4,4,4,4,4,
    3,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,4,4,4,4,4,4,4,4,4,
    3,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,4,4,4,4,4,4,4,4,4,4,
    3,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,4,4,4,4,4,4,4,4,4,4,4,
    3,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,4,4,4,4,4,4,4,4,4,4,4,4,
    3,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
    3,1,1,1,1,1,1,1,1,1,1,1,1,1,1,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
    3,1,1,1,1,1,1,1,1,1,1,1,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
    3,1,1,1,1,1,1,1,1,1,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
    3,1,1,1,1,1,1,1,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
    3,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  };

  for (int i = 0; i < 32; i++)
  {
    for (int j = 0; j < 32; j++)
    {
      *tile_at(state->tilemap, j, 32 - i - 1) = initial_terrain[i * 32 + j];
    }
  }

  state->cursor_terrain = TT_invalid;

  state->camera = (vec2f){ (float)state->tilemap->width / 2.0f, (float)state->tilemap->height / 2.0f};

  state->resolution = (vec2f){ 1.0f, 1.0f };

  state->paused = true;

  state->debug_break = false;

  state->pixels_per_tile = 20.0f;

  state->is_initialised = true;
}

/* ------------------------------------- *
                 Graphics
* ------------------------------------- */

INTERNAL
void init_graphics(gl_state* state, allocator scratch, Tilemap *tilemap)
{
  state->shdr.vs_tiles.type   = GL_VERTEX_SHADER;
  state->shdr.fs_tiles.type   = GL_FRAGMENT_SHADER;
  state->shdr.fs_borders.type = GL_FRAGMENT_SHADER;
  state->shdr.vs_2d_abs.type  = GL_VERTEX_SHADER;
  state->shdr.fs_uniform.type = GL_FRAGMENT_SHADER;

  state->shdr.vs_tiles.path   = SHADER_FOLDER"tiles.vert";
  state->shdr.fs_tiles.path   = SHADER_FOLDER"tiles.frag";
  state->shdr.fs_borders.path = SHADER_FOLDER"borders.frag";
  state->shdr.vs_2d_abs.path  = SHADER_FOLDER"2d_abs.vert";
  state->shdr.fs_uniform.path = SHADER_FOLDER"uniform.frag";

  state->prog.cells.vs   = &state->shdr.vs_tiles;
  state->prog.cells.fs   = &state->shdr.fs_tiles;
  state->prog.borders.vs = &state->shdr.vs_tiles;
  state->prog.borders.fs = &state->shdr.fs_borders;
  state->prog.rect.vs    = &state->shdr.vs_2d_abs;
  state->prog.rect.fs    = &state->shdr.fs_uniform;

  for (int i = 0; i < ARRAY_COUNT(state->shdr.all); i++)
  {
    gl_shader *sh = &state->shdr.all[i];
    sh->last_file_time = file_modified_time(sh->path);
    sh->needs_relink = false;
    char* source = read_file(sh->path);
    sh->id = new_shader(sh->type, (const char*)source);
    free_file_memory(source);
  }

  for (int i = 0; i < ARRAY_COUNT(state->prog.all); i++)
  {
    gl_program *prog = &state->prog.all[i];
    prog->id = new_shader_program(prog->vs->id, prog->fs->id);
  }

  long long int vertices_size = sizeof(vec3f) * (tilemap->width + 1) * (tilemap->height + 1);
  vec3f *vertices = alloc(&scratch, vertices_size);

  long long int cell_elements_size = 6 * sizeof(GLuint) * (tilemap->width) * (tilemap->height);
  struct { GLuint bl, tl1, br1, br2, tl2, tr; } *cell_elements = alloc(&scratch, cell_elements_size);

  long long int border_elements_size = 8 * sizeof(GLuint) * (tilemap->width) * (tilemap->height);
  struct { GLuint l1, l2, t1, t2, r1, r2, b1, b2; } *border_elements = alloc(&scratch, border_elements_size);

  for (int i = 0; i <= tilemap->height; i++)
  {
    for (int j = 0; j <= tilemap->width; j++)
    {
      vertices[i * (tilemap->width + 1) + j] = (vec3f){ (float)j, (float)i, 0.0f };

      if (i < tilemap->height && j < tilemap->width)
      {
        GLuint bl = (i)     * (tilemap->width + 1) + j;
        GLuint tl = (i + 1) * (tilemap->width + 1) + j;
        GLuint br = (i)     * (tilemap->width + 1) + j + 1;
        GLuint tr = (i + 1) * (tilemap->width + 1) + j + 1;

        cell_elements[i * tilemap->width + j].bl  = bl;
        cell_elements[i * tilemap->width + j].tl1 = tl;
        cell_elements[i * tilemap->width + j].tl2 = tl;
        cell_elements[i * tilemap->width + j].tr  = tr;
        cell_elements[i * tilemap->width + j].br1 = br;
        cell_elements[i * tilemap->width + j].br2 = br;

        border_elements[i * tilemap->width + j].l1 = bl;
        border_elements[i * tilemap->width + j].l2 = tl;
        border_elements[i * tilemap->width + j].t1 = tl;
        border_elements[i * tilemap->width + j].t2 = tr;
        border_elements[i * tilemap->width + j].r1 = tr;
        border_elements[i * tilemap->width + j].r2 = br;
        border_elements[i * tilemap->width + j].b1 = br;
        border_elements[i * tilemap->width + j].b2 = bl;
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

    glUseProgram(state->prog.cells.id);
    GLuint attr = glGetAttribLocation(state->prog.cells.id, "coord");
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

    glUseProgram(state->prog.borders.id);
    GLuint attr = glGetAttribLocation(state->prog.borders.id, "coord");
    CATCH_GL_ERROR(errstr);

    glVertexAttribPointer(attr, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    CATCH_GL_ERROR(errstr);

    glEnableVertexAttribArray(attr);
    CATCH_GL_ERROR(errstr);
  }

  glBindVertexArray(state->vao_rect);
  {
    glBindBuffer(GL_ARRAY_BUFFER, state->vbo_rect);

    glUseProgram(state->prog.rect.id);
    GLuint attr = glGetAttribLocation(state->prog.rect.id, "coord2d");
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
void render_grid(gl_state* state, Tilemap *tilemap)
{
  glUseProgram(state->prog.cells.id);

  glUniform2f(glGetUniformLocation(state->prog.cells.id, "camera"), state->camera.x, state->camera.y);
  glUniform2f(glGetUniformLocation(state->prog.cells.id, "resolution"), state->resolution.x, state->resolution.y);
  glUniform1f(glGetUniformLocation(state->prog.cells.id, "pixels_per_tile"), state->pixels_per_tile);

  glBindVertexArray(state->vao_cells);

  glBufferData(GL_SHADER_STORAGE_BUFFER, tilemap->width * tilemap->height * sizeof(*tilemap->tiles), tilemap->tiles, GL_DYNAMIC_COPY);
  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

  glDrawElements(GL_TRIANGLES, 6 * tilemap->width * tilemap->height, GL_UNSIGNED_INT, NULL);
  CATCH_GL_ERROR(errstr);

  glBindVertexArray(0);
}

INTERNAL
void render_grid_borders(gl_state* state, Tilemap *tilemap)
{
  glUseProgram(state->prog.borders.id);

  glUniform2f(glGetUniformLocation(state->prog.borders.id, "camera"), state->camera.x, state->camera.y);
  glUniform2f(glGetUniformLocation(state->prog.borders.id, "resolution"), state->resolution.x, state->resolution.y);
  glUniform1f(glGetUniformLocation(state->prog.borders.id, "pixels_per_tile"), state->pixels_per_tile);

  glBindVertexArray(state->vao_borders);

  glDrawElements(GL_LINES, 8 * tilemap->width * tilemap->height, GL_UNSIGNED_INT, NULL);
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

  glUseProgram(state->prog.rect.id);

  glUniform2f(glGetUniformLocation(state->prog.rect.id, "resolution"), state->resolution.x, state->resolution.y);
  glUniform4f(glGetUniformLocation(state->prog.rect.id, "colour"), c.r, c.g, c.b, c.a);

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
void render_mouse_tile(game_state* state)
{
  vec2i tpos = tile_at_pixel(state->mouse, state->camera, state->resolution, state->pixels_per_tile);

  if (in_bounds(state->tilemap, tpos))
  {
    rectf rec = pixels_for_tile(tpos, state->camera, state->resolution, state->pixels_per_tile);

    colour c = {0.0f};

    switch (state->cursor_terrain)
    {
    case TT_invalid:
      c = (colour){ 0.8f, 0.8f, 0.8f, 0.8f };
      break;
    case TT_empty:
      c = (colour){ 0.0f, 0.0f, 0.0f, 0.8f };
      break;
    case TT_grass:
      c = (colour){ 91.0f/255.0f, 148.0f/255.0f, 82.0f/255.0f, 0.8f };
      break;
    case TT_desert:
      c = (colour){ 254.0f/255.0f, 246.0f/255.0f, 163.0f/255.0f, 0.8f };
      break;
    case TT_mountain:
      c = (colour){ 198.0f/255.0f, 195.0f/255.0f, 191.0f/255.0f, 0.8f };
      break;
    case TT_water:
      c = (colour){ 153.0f/255.0f, 192.0f/255.0f, 227.0f/255.0f, 0.8f };
      break;
    default:
      UNIMPLEMENTED("not all terrain types handled");
    }

    draw_rectangle(&state->gl_st, rec, c);
  }
}

INTERNAL
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

INTERNAL
void game_render(game_state* state)
{
  clear_background((colour){ 0.1f, 0.1f, 0.1f, 1.0f });

  render_grid(&state->gl_st, state->tilemap);

  //if (state->pixels_per_tile > 10.0f)
  {
    render_grid_borders(&state->gl_st, state->tilemap);
  }

  render_mouse_tile(state);

  if (state->paused)
  {
    render_pause_icon(&state->gl_st);
  }
}

/* ------------------------------------- *
                 Updating
* ------------------------------------- */

INTERNAL
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
      case KEY_0:
        state->cursor_terrain = TT_empty;
        break;
      case KEY_1:
        state->cursor_terrain = TT_grass;
        break;
      case KEY_2:
        state->cursor_terrain = TT_desert;
        break;
      case KEY_3:
        state->cursor_terrain = TT_mountain;
        break;
      case KEY_4:
        state->cursor_terrain = TT_water;
        break;
      case KEY_ESCAPE:
        state->cursor_terrain = TT_invalid;
        break;
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

  if (input->keys[KEY_MOUSEL].isdown && state->cursor_terrain != TT_invalid)
  {
    vec2i t = tile_at_pixel(state->mouse,state->camera, state->resolution, state->pixels_per_tile);
    if (in_bounds(state->tilemap, t))
    {
      *tile_at(state->tilemap, t.x, t.y) = state->cursor_terrain;
    }
  }

  state->pixels_per_tile *= expf((float)input->mousewheel_delta / 960.f);

  {
    vec2f newres = (vec2f){(float)input->window_resolution.x, (float)input->window_resolution.y};
    state->resolution = newres;
  }

  state->gl_st.resolution = state->resolution;
  state->gl_st.camera = state->camera;
  state->gl_st.pixels_per_tile = state->pixels_per_tile;

  for (int i = 0; i < ARRAY_COUNT(state->gl_st.shdr.all); i++)
  {
    gl_shader *sh = &state->gl_st.shdr.all[i];
    unsigned long long int new_file_time = file_modified_time(sh->path);
    if (new_file_time > sh->last_file_time)
    {
      char* source = read_file(sh->path);
      recompile_shader(sh->id, (const char*)source);
      free_file_memory(source);
      sh->last_file_time = new_file_time;
      sh->needs_relink = true;
    }
  }

  for (int i = 0; i < ARRAY_COUNT(state->gl_st.prog.all); i++)
  {
    gl_program *prog = &state->gl_st.prog.all[i];
    if (prog->vs->needs_relink || prog->fs->needs_relink)
    {
      relink_shader_program(prog->id);
    }
  }

  for (int i = 0; i < ARRAY_COUNT(state->gl_st.shdr.all); i++)
  {
    gl_shader *sh = &state->gl_st.shdr.all[i];
    sh->needs_relink = false;
  }

}

/* ------------------------------------- *
                   Main
* ------------------------------------- */

void rts_game_main(Input *input, game_memory* memory)
{
  game_state *state = (game_state*)memory->buffer;

  if (!state->is_initialised)
  {
    game_init(memory);

    init_graphics(&state->gl_st, state->allctr, state->tilemap);
  }

  game_update(input, state);

  game_render(state);
}
