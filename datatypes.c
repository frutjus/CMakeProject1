#include "datatypes.h"

vec2f add_vec2f(vec2f v1, vec2f v2)
{
  vec2f v3;
  v3.x = v1.x + v2.x;
  v3.y = v1.y + v2.y;
  return v3;
}

vec2f subtract_vec2f(vec2f v1, vec2f v2)
{
  vec2f v3;
  v3.x = v1.x - v2.x;
  v3.y = v1.y - v2.y;
  return v3;
}

vec2f multiply_vec2f(vec2f v, float f)
{
  vec2f v2;
  v2.x = v.x * f;
  v2.y = v.y * f;
  return v2;
}

vec2f divide_vec2f(vec2f v, float f)
{
  vec2f v2;
  v2.x = v.x / f;
  v2.y = v.y / f;
  return v2;
}

vec2i add_vec2i(vec2i v1, vec2i v2)
{
  vec2i v3;
  v3.x = v1.x + v2.x;
  v3.y = v1.y + v2.y;
  return v3;
}

vec2i subtract_vec2i(vec2i v1, vec2i v2)
{
  vec2i v3;
  v3.x = v1.x - v2.x;
  v3.y = v1.y - v2.y;
  return v3;
}

vec2i multiply_vec2i(vec2i v, int i)
{
  vec2i v2;
  v2.x = v.x * i;
  v2.y = v.y * i;
  return v2;
}

vec2i divide_vec2i(vec2i v, int i)
{
  vec2i v2;
  v2.x = v.x / i;
  v2.y = v.y / i;
  return v2;
}

vec2i vec2f_to_veci2(vec2f v)
{
  vec2i vi;
  vi.x = (int)v.x;
  vi.y = (int)v.y;
  return vi;
}

vec2f vec2i_to_vec2f(vec2i v)
{
  vec2f vf;
  vf.x = (float)v.x;
  vf.y = (float)v.y;
  return vf;
}
