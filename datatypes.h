#pragma once

typedef struct {
  float x, y, w, h;
} rectf;

typedef struct {
  int x, y, w, h;
} recti;

typedef struct {
  float x, y;
} vec2f;

typedef struct {
  int x, y;
} vec2i;

vec2f add_vec2f(vec2f v1, vec2f v2);
vec2f subtract_vec2f(vec2f v1, vec2f v2);
vec2f multiply_vec2f(vec2f v, float f);
vec2f divide_vec2f(vec2f v, float f);
vec2i vec2f_to_veci2(vec2f v);
vec2f vec2i_to_vec2f(vec2i v);