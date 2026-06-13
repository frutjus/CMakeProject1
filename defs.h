#pragma once


#define ARRAY_COUNT(arr) (sizeof(arr) / sizeof(arr[0]))
#define ASSERT(cond) if (!(cond)) { *(char*)0 = 0; }

#define INTERNAL static

#define UNIMPLEMENTED(any) ASSERT(0)

typedef enum {
  false,
  true,
} bool;

#define OPENGL