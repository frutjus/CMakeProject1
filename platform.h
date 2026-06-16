#pragma once

#include "defs.h"

/* ------------------------------------- *
                Graphics
* ------------------------------------- */

/* ------------------------------------- *
                 Input
* ------------------------------------- */

typedef enum {
  KEY_INVALID,
  KEY_A,
  KEY_B,
  KEY_C,
  KEY_D,
  KEY_E,
  KEY_F,
  KEY_G,
  KEY_H,
  KEY_I,
  KEY_J,
  KEY_K,
  KEY_L,
  KEY_M,
  KEY_N,
  KEY_O,
  KEY_P,
  KEY_Q,
  KEY_R,
  KEY_S,
  KEY_T,
  KEY_U,
  KEY_V,
  KEY_W,
  KEY_X,
  KEY_Y,
  KEY_Z,
  KEY_1,
  KEY_2,
  KEY_3,
  KEY_4,
  KEY_5,
  KEY_6,
  KEY_7,
  KEY_8,
  KEY_9,
  KEY_0,
  KEY_ENTER,
  KEY_ESCAPE,
  KEY_BACKSPACE,
  KEY_TAB,
  KEY_SPACE,
  KEY_DASH,
  KEY_EQUALS,
  KEY_LBRACE,
  KEY_RBRACE,
  KEY_BACKSLASH,
  KEY_COLON,
  KEY_APOSTROPHE,
  KEY_GRAVE,
  KEY_COMMA,
  KEY_PERIOD,
  KEY_FORWARDSLASH,
  KEY_CAPSLOCK,
  KEY_F1,
  KEY_F2,
  KEY_F3,
  KEY_F4,
  KEY_F5,
  KEY_F6,
  KEY_F7,
  KEY_F8,
  KEY_F9,
  KEY_F10,
  KEY_F11,
  KEY_F12,
  KEY_PRINTSCREEN,
  KEY_SYSRQ,
  KEY_SCROLLLOCK,
  KEY_BREAK,
  KEY_PAUSE,
  KEY_INSERT,
  KEY_HOME,
  KEY_PAGEUP,
  KEY_DELETE,
  KEY_END,
  KEY_PAGEDOWN,
  KEY_RIGHT,
  KEY_LEFT,
  KEY_DOWN,
  KEY_UP,
  KEY_NP_NUMLOCK,
  KEY_NP_FORWARDSLASH,
  KEY_NP_STAR,
  KEY_NP_DASH,
  KEY_NP_PLUS,
  KEY_NP_ENTER,
  KEY_NP_1,
  KEY_NP_2,
  KEY_NP_3,
  KEY_NP_4,
  KEY_NP_5,
  KEY_NP_6,
  KEY_NP_7,
  KEY_NP_8,
  KEY_NP_9,
  KEY_NP_0,
  KEY_NP_PERIOD,
  KEY_APP,
  KEY_LCONTROL,
  KEY_LSHIFT,
  KEY_LALT,
  KEY_LSUPER,
  KEY_RCONTROL,
  KEY_RSHIFT,
  KEY_RALT,
  KEY_RSUPER,
  KEY_MOUSEL,
  KEY_MOUSER,
  KEY_MOUSEM,
  KEY_MOUSEX1,
  KEY_MOUSEX2,
  KEY_COUNT
} Key;

INTERNAL char *key_names[] = {
  "KEY_INVALID",
  "KEY_A",
  "KEY_B",
  "KEY_C",
  "KEY_D",
  "KEY_E",
  "KEY_F",
  "KEY_G",
  "KEY_H",
  "KEY_I",
  "KEY_J",
  "KEY_K",
  "KEY_L",
  "KEY_M",
  "KEY_N",
  "KEY_O",
  "KEY_P",
  "KEY_Q",
  "KEY_R",
  "KEY_S",
  "KEY_T",
  "KEY_U",
  "KEY_V",
  "KEY_W",
  "KEY_X",
  "KEY_Y",
  "KEY_Z",
  "KEY_1",
  "KEY_2",
  "KEY_3",
  "KEY_4",
  "KEY_5",
  "KEY_6",
  "KEY_7",
  "KEY_8",
  "KEY_9",
  "KEY_0",
  "KEY_ENTER",
  "KEY_ESCAPE",
  "KEY_BACKSPACE",
  "KEY_TAB",
  "KEY_SPACE",
  "KEY_DASH",
  "KEY_EQUALS",
  "KEY_LBRACE",
  "KEY_RBRACE",
  "KEY_BACKSLASH",
  "KEY_COLON",
  "KEY_APOSTROPHE",
  "KEY_GRAVE",
  "KEY_COMMA",
  "KEY_PERIOD",
  "KEY_FORWARDSLASH",
  "KEY_CAPSLOCK",
  "KEY_F1",
  "KEY_F2",
  "KEY_F3",
  "KEY_F4",
  "KEY_F5",
  "KEY_F6",
  "KEY_F7",
  "KEY_F8",
  "KEY_F9",
  "KEY_F10",
  "KEY_F11",
  "KEY_F12",
  "KEY_PRINTSCREEN",
  "KEY_SYSRQ",
  "KEY_SCROLLLOCK",
  "KEY_BREAK",
  "KEY_PAUSE",
  "KEY_INSERT",
  "KEY_HOME",
  "KEY_PAGEUP",
  "KEY_DELETE",
  "KEY_END",
  "KEY_PAGEDOWN",
  "KEY_RIGHT",
  "KEY_LEFT",
  "KEY_DOWN",
  "KEY_UP",
  "KEY_NP_NUMLOCK",
  "KEY_NP_FORWARDSLASH",
  "KEY_NP_STAR",
  "KEY_NP_DASH",
  "KEY_NP_PLUS",
  "KEY_NP_ENTER",
  "KEY_NP_1",
  "KEY_NP_2",
  "KEY_NP_3",
  "KEY_NP_4",
  "KEY_NP_5",
  "KEY_NP_6",
  "KEY_NP_7",
  "KEY_NP_8",
  "KEY_NP_9",
  "KEY_NP_0",
  "KEY_NP_PERIOD",
  "KEY_APP",
  "KEY_LCONTROL",
  "KEY_LSHIFT",
  "KEY_LALT",
  "KEY_LSUPER",
  "KEY_RCONTROL",
  "KEY_RSHIFT",
  "KEY_RALT",
  "KEY_RSUPER",
  "KEY_MOUSEL",
  "KEY_MOUSER",
  "KEY_MOUSEM",
  "KEY_MOUSEX1",
  "KEY_MOUSEX2",
  "KEY_COUNT"
};

typedef enum {
  EVENT_KEYPRESS,
  EVENT_COUNT
} event_type;

typedef struct {
  event_type t;
  union {
    struct {
      Key k;
      struct {
        int x;
        int y;
      } mouse;
      bool control, shift, alt;
    };
  };
} event;

#define MAX_EVENTS 2

typedef struct {
  struct {
    bool isdown;
  } keys[KEY_COUNT];

  event events[MAX_EVENTS];
  int events_count;

  struct {
    int x;
    int y;
  } mouse;

  int mousewheel_delta;
  int mousewheelh_delta;

  float dt;

  struct {
    int x;
    int y;
  } window_resolution;
} Input;

/* ------------------------------------- *
                 Memory
* ------------------------------------- */

typedef struct {
  void* buffer;
  int size;
} game_memory;

/* ------------------------------------- *
               Game calls
* ------------------------------------- */

void game_main(Input *input, game_memory* memory);
