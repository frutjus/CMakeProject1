#include "platform.h"
#include "defs.h"

#include <Windows.h>
#pragma comment(lib, "mincore")
#pragma comment(lib, "Winmm")

#include <stdio.h>

DWORD report_error(char *context)
{
  OutputDebugStringA("Error ");
  OutputDebugStringA(context);
  OutputDebugStringA(": ");

  DWORD err = GetLastError();

  char* buffer;

  DWORD message_length = FormatMessageA(
    FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
    NULL, err, 0, (LPSTR)&buffer, 0, NULL);

  if (message_length > 0)
  {
    OutputDebugStringA(buffer);
    OutputDebugStringA("\n");
  }
  else
  {
    OutputDebugStringA("Failed to retrieve error\n");
  }

  return err;
}

void report_error_exit(char *context)
{
  ExitProcess(report_error(context));
}

long long int get_tick()
{
  LARGE_INTEGER t;
  QueryPerformanceCounter(&t);
  return t.QuadPart;
}

/* ------------------------------------- *
            Main Win32 Handling
 * ------------------------------------- */

typedef struct {
  void* buffer;
  int width; //in pixels
  int height; //in pixels
  int bytesPerPixel;
  int stride; //in bytes
  int size; //in bytes
  BITMAPINFOHEADER bmi;
} win32_pixel_buffer;

typedef struct {
  void* start;
  void* write;
  void* read;
  int size;
  int size_used;
} ring_buffer;

void create_pixel_buffer(win32_pixel_buffer* pixels, int width, int height)
{
  int bytesPerPixel = 4;
  int stride = width * bytesPerPixel;
  int size = stride * height;

  void* buffer = VirtualAlloc(NULL, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
  if (buffer == NULL) report_error_exit("allocating pixel buffer\n");

  pixels->buffer = buffer;
  pixels->width = width;
  pixels->height = height;
  pixels->bytesPerPixel = bytesPerPixel;
  pixels->stride = stride;
  pixels->size = size;
  pixels->bmi.biSize = sizeof(pixels->bmi);
  pixels->bmi.biWidth = width;
  pixels->bmi.biHeight = -height;
  pixels->bmi.biPlanes = 1;
  pixels->bmi.biBitCount = bytesPerPixel * 8;
  pixels->bmi.biCompression = BI_RGB;
  pixels->bmi.biSizeImage = 0;
  pixels->bmi.biXPelsPerMeter = 0;
  pixels->bmi.biYPelsPerMeter = 0;
  pixels->bmi.biClrUsed = 0;
  pixels->bmi.biClrImportant = 0;
}

void destroy_pixel_buffer(win32_pixel_buffer* pixels)
{
  if (pixels->buffer != NULL)
  {
    VirtualFree(pixels->buffer, 0, MEM_RELEASE);
  }
}

void flip_buffers(HDC hdc, int screenWidth, int screenHeight, win32_pixel_buffer* pixels)
{
  StretchDIBits(
    hdc,
    0, 0,
    screenWidth, screenHeight,
    0, 0,
    pixels->width, pixels->height,
    pixels->buffer,
    (BITMAPINFO*)&pixels->bmi,
    0,
    SRCCOPY
  );
}

Key key_from_scancode(WORD scancode)
{
  static const Key scancode_map[] = {
    KEY_INVALID,
    KEY_ESCAPE,
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
    KEY_DASH,
    KEY_EQUALS,
    KEY_BACKSPACE,
    KEY_TAB,
    KEY_Q,
    KEY_W,
    KEY_E,
    KEY_R,
    KEY_T,
    KEY_Y,
    KEY_U,
    KEY_I,
    KEY_O,
    KEY_P,
    KEY_LBRACE,
    KEY_RBRACE,
    KEY_ENTER,
    KEY_LCONTROL,
    KEY_A,
    KEY_S,
    KEY_D,
    KEY_F,
    KEY_G,
    KEY_H,
    KEY_J,
    KEY_K,
    KEY_L,
    KEY_COLON,
    KEY_APOSTROPHE,
    KEY_GRAVE,
    KEY_LSHIFT,
    KEY_BACKSLASH,
    KEY_Z,
    KEY_X,
    KEY_C,
    KEY_V,
    KEY_B,
    KEY_N,
    KEY_M,
    KEY_COMMA,
    KEY_PERIOD,
    KEY_FORWARDSLASH,
    KEY_RSHIFT,
    KEY_NP_STAR,
    KEY_LALT,
    KEY_SPACE,
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
    KEY_PAUSE,
    KEY_SCROLLLOCK,
    KEY_NP_7,
    KEY_NP_8,
    KEY_NP_9,
    KEY_NP_DASH,
    KEY_NP_4,
    KEY_NP_5,
    KEY_NP_6,
    KEY_NP_PLUS,
    KEY_NP_1,
    KEY_NP_2,
    KEY_NP_3,
    KEY_NP_0,
    KEY_NP_PERIOD,
    KEY_SYSRQ,
    KEY_INVALID,
    KEY_INVALID,
    KEY_F11,
    KEY_F12,
  };

  static const Key scancode_map_ext[] = {
    KEY_INVALID,
    KEY_INVALID,
    KEY_INVALID,
    KEY_INVALID,
    KEY_INVALID,
    KEY_INVALID,
    KEY_INVALID,
    KEY_INVALID,
    KEY_INVALID,
    KEY_INVALID,
    KEY_INVALID,
    KEY_INVALID,
    KEY_INVALID,
    KEY_INVALID,
    KEY_INVALID,
    KEY_INVALID,
    KEY_INVALID,
    KEY_INVALID,
    KEY_INVALID,
    KEY_INVALID,
    KEY_INVALID,
    KEY_INVALID,
    KEY_INVALID,
    KEY_INVALID,
    KEY_INVALID,
    KEY_INVALID,
    KEY_INVALID,
    KEY_INVALID,
    KEY_NP_ENTER,
    KEY_RCONTROL,
    KEY_INVALID,
    KEY_INVALID,
    KEY_INVALID,
    KEY_INVALID,
    KEY_INVALID,
    KEY_INVALID,
    KEY_INVALID,
    KEY_INVALID,
    KEY_INVALID,
    KEY_INVALID,
    KEY_INVALID,
    KEY_INVALID,
    KEY_INVALID,
    KEY_INVALID,
    KEY_INVALID,
    KEY_INVALID,
    KEY_INVALID,
    KEY_INVALID,
    KEY_INVALID,
    KEY_INVALID,
    KEY_INVALID,
    KEY_INVALID,
    KEY_INVALID,
    KEY_NP_FORWARDSLASH,
    KEY_INVALID,
    KEY_PRINTSCREEN,
    KEY_RALT,
    KEY_INVALID,
    KEY_INVALID,
    KEY_INVALID,
    KEY_INVALID,
    KEY_INVALID,
    KEY_INVALID,
    KEY_INVALID,
    KEY_INVALID,
    KEY_INVALID,
    KEY_INVALID,
    KEY_INVALID,
    KEY_INVALID,
    KEY_NP_NUMLOCK,
    KEY_BREAK,
    KEY_HOME,
    KEY_UP,
    KEY_PAGEUP,
    KEY_INVALID,
    KEY_LEFT,
    KEY_INVALID,
    KEY_RIGHT,
    KEY_INVALID,
    KEY_END,
    KEY_DOWN,
    KEY_PAGEDOWN,
    KEY_INSERT,
    KEY_DELETE,
    KEY_INVALID,
    KEY_INVALID,
    KEY_INVALID,
    KEY_INVALID,
    KEY_INVALID,
    KEY_INVALID,
    KEY_INVALID,
    KEY_LSUPER,
    KEY_RSUPER,
    KEY_APP,
  };

  BYTE code = LOBYTE(scancode);
  BYTE ext = HIBYTE(scancode);

  if (ext == 0x00)
  {
    return code < ARRAY_COUNT(scancode_map) ?
      scancode_map[code] : KEY_INVALID;
  }
  if (ext == 0xE0)
  {
    return code < ARRAY_COUNT(scancode_map_ext) ?
      scancode_map_ext[code] : KEY_INVALID;
  }

  return KEY_INVALID;
}

bool alloc_ring_buffer(ring_buffer *buf, int size)
{
  // this function provided courtesy of the Microsoft docs on VirtualAlloc2
  // todo: this function relies on relatively recent Windows functionality;
  // i.e. VirtualAlloc2 and MapViewOfFile3 which are provided in mincore.dll
  // it would be nice to have a fallback version which would still work if that is unavailable

  SYSTEM_INFO sysInfo;
  GetSystemInfo(&sysInfo);

  if ((size % sysInfo.dwAllocationGranularity) != 0) {
    return false;
  }

  //
  // Reserve a placeholder region where the buffer will be mapped.
  //

  void* placeholder1 = VirtualAlloc2(
    NULL,
    NULL,
    2 * size,
    MEM_RESERVE | MEM_RESERVE_PLACEHOLDER,
    PAGE_NOACCESS,
    NULL, 0
  );

  if (placeholder1 == NULL) {
    report_error_exit("allocating ring buffer\n");
  }

  //
  // Split the placeholder region into two regions of equal size.
  //

  BOOL result = VirtualFree (
    placeholder1,
    size,
    MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER
  );

  if (result == FALSE) {
    report_error_exit("allocating ring buffer\n");
  }

  void* placeholder2 = (void*)((ULONG_PTR)placeholder1 + size);

  //
  // Create a pagefile-backed section for the buffer.
  //

  HANDLE section = CreateFileMappingA(
    INVALID_HANDLE_VALUE,
    NULL,
    PAGE_READWRITE,
    0,
    size, NULL
  );

  if (section == NULL) {
    report_error_exit("allocating ring buffer\n");
  }

  //
  // Map the section into the first placeholder region.
  //

  void* view1 = MapViewOfFile3 (
    section,
    NULL,
    placeholder1,
    0,
    size,
    MEM_REPLACE_PLACEHOLDER,
    PAGE_READWRITE,
    NULL, 0
  );

  if (view1 == NULL) {
    report_error_exit("allocating ring buffer\n");
  }

  //
  // Map the section into the second placeholder region.
  //

  void* view2 = MapViewOfFile3 (
    section,
    NULL,
    placeholder2,
    0,
    size,
    MEM_REPLACE_PLACEHOLDER,
    PAGE_READWRITE,
    NULL, 0
  );

  if (view2 == NULL) {
    report_error_exit("allocating ring buffer\n");
  }

  //
  // Success
  //

  buf->start = view1;
  buf->write = view1;
  buf->read = view1;
  buf->size = size;
  buf->size_used = 0;

  CloseHandle(section);

  return true;
}

void free_ring_buffer(ring_buffer *buf)
{
  if (buf->start != NULL) {
    void* page1 = buf->start;
    void* page2 = (void*)((ULONG_PTR)buf->start + buf->size);
    UnmapViewOfFileEx(page1, 0);
    UnmapViewOfFileEx(page2, 0);
    VirtualFree(page1, 0, MEM_RELEASE);
    VirtualFree(page2, 0, MEM_RELEASE);
  }

  buf->start = 0;
  buf->size = 0;
  buf->read = 0;
  buf->write = 0;
}

bool is_ring_buffer_full(ring_buffer* buf)
{
  return buf->size_used >= buf->size;
}

#define bump_ring_buffer_with(buf, type, var) \
for (type* var = (type*)(buf)->write; \
  var == (type*)(buf)->write; \
  (buf)->write = ((ULONG_PTR)(buf)->write >= (ULONG_PTR)(buf)->start + (ULONG_PTR)(buf)->size) \
    ? (void*)((ULONG_PTR)(buf)->write - (ULONG_PTR)(buf)->size + sizeof(type)) \
    : (void*)((ULONG_PTR)(buf)->write + sizeof(type)), \
  (buf)->size_used += sizeof(type) \
)

LRESULT process_keystroke(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, Input* game_input, ring_buffer* earray)
{
  WORD vkCode = LOWORD(wParam);
  WORD keyFlags = HIWORD(lParam);
  WORD scanCode = LOBYTE(keyFlags);
  
  BOOL isExtendedKey = (keyFlags & KF_EXTENDED) == KF_EXTENDED;
  BOOL wasDown       = (keyFlags & KF_REPEAT  ) == KF_REPEAT;
  BOOL isReleased    = (keyFlags & KF_UP      ) == KF_UP;
  BOOL isAltDown     = (keyFlags & KF_ALTDOWN ) == KF_ALTDOWN;

  BOOL isDown = !isReleased;

  WORD repeatCount = LOWORD(lParam);

  if (isExtendedKey)
    scanCode = MAKEWORD(scanCode, 0xE0);

  switch (vkCode)
  {
  case VK_SHIFT:
  case VK_CONTROL:
  case VK_MENU:
    vkCode = LOWORD(MapVirtualKeyA(scanCode, MAPVK_VSC_TO_VK_EX)); //remaps to L and R specific codes
  }

  Key key = key_from_scancode(scanCode);

  game_input->keys[key].isdown = isDown;

  if (isDown)
  {
    if (!is_ring_buffer_full(earray))
    {
      bump_ring_buffer_with(earray, event, e)
      {
        e->t = EVENT_KEYPRESS;
        e->k = key;
      }
    } else
    {
      UNIMPLEMENTED(log full ring buffer);
    }
  }

  if (isAltDown) // handle syskey events
    return DefWindowProc(hWnd, message, wParam, lParam);
  return 0;
}

LRESULT process_mouse_button(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, Input* game_input, ring_buffer* earray)
{
  // TODO: add event processing for mouse clicks.
  // some things to think about:
  //  need to get the coords of the click from the Win32 event, so that we process the clicks accurately
  //  how to tell what button was clicked, if several are held down at once?
  //  should perhaps get the button state of Shift and Control from the Win32 event?
  
  game_input->keys[KEY_MOUSEL ].isdown = (wParam & MK_LBUTTON ) == MK_LBUTTON;
  game_input->keys[KEY_MOUSER ].isdown = (wParam & MK_RBUTTON ) == MK_RBUTTON;
  game_input->keys[KEY_MOUSEM ].isdown = (wParam & MK_MBUTTON ) == MK_MBUTTON;
  game_input->keys[KEY_MOUSEX1].isdown = (wParam & MK_XBUTTON1) == MK_XBUTTON1;
  game_input->keys[KEY_MOUSEX2].isdown = (wParam & MK_XBUTTON2) == MK_XBUTTON2;

  return 0;
}

struct {
  Input* game_input;
  ring_buffer* earray;
  win32_pixel_buffer pixels;
  int resolution_x;
  int resolution_y;
} global_state;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  switch (message)
  {
  case WM_DESTROY:
    PostQuitMessage(0);
    return 0;

  case WM_PAINT:
  {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hWnd, &ps);
    EndPaint(hWnd, &ps);
    return 0;
  }

  case WM_SIZE:
    return 0;

  case WM_KEYDOWN:
  case WM_KEYUP:
  case WM_SYSKEYDOWN:
  case WM_SYSKEYUP:
    return process_keystroke(hWnd, message, wParam, lParam, global_state.game_input, global_state.earray);

  case WM_LBUTTONDOWN:
  case WM_LBUTTONUP:
  case WM_MBUTTONDOWN:
  case WM_MBUTTONUP:
  case WM_RBUTTONDOWN:
  case WM_RBUTTONUP:
  case WM_XBUTTONDOWN:
  case WM_XBUTTONUP:
    return process_mouse_button(hWnd, message, wParam, lParam, global_state.game_input, global_state.earray);

  case WM_MOUSEHWHEEL:
  case WM_MOUSEWHEEL:
  case WM_MOUSEMOVE:
    return 0;

  default:
    return DefWindowProc(hWnd, message, wParam, lParam);
  }
}

int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow)
{
  /* --- register window class and create window --- */
  HWND window;
  {
    WNDCLASSA window_class;
    window_class.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    window_class.lpfnWndProc = WndProc;
    window_class.cbClsExtra = 0;
    window_class.cbWndExtra = 0;
    window_class.hInstance = hInst;
    window_class.hIcon = NULL;
    window_class.hCursor = LoadCursor(NULL, IDC_ARROW);
    window_class.hbrBackground = NULL;
    window_class.lpszMenuName = NULL;
    window_class.lpszClassName = "creative window class name";

    ATOM window_class_id = RegisterClassA(&window_class);
    if (window_class_id == 0) report_error_exit("registering window class");

    global_state.resolution_x = 960;
    global_state.resolution_y = 540;

    RECT rc = {0,0,global_state.resolution_x,global_state.resolution_y};
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

    HWND hWnd = CreateWindowExA(
      0,
      (LPCSTR)window_class_id,
      "inspiring window",
      WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_OVERLAPPEDWINDOW,
      /* x */ CW_USEDEFAULT,
      /* y */ CW_USEDEFAULT,
      /* nWidth  */ rc.right - rc.left,
      /* nHeight */ rc.bottom - rc.top,
      NULL,
      NULL,
      hInst,
      NULL
    );
    if (hWnd == NULL) report_error_exit("creating window");

    window = hWnd;

    ShowWindow(hWnd, cmdshow);
    UpdateWindow(hWnd);
  }

  /* --- create & allocate pixel buffer --- */
  create_pixel_buffer(&global_state.pixels, global_state.resolution_x, global_state.resolution_y);

  /* --- allocate memory --- */
  game_memory memory = {0};
  ring_buffer event_buffer = {0};
  Input game_input = {0};
  {
    memory.size = 1024 * 1024 * 20;
    memory.buffer = VirtualAlloc(NULL, memory.size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (memory.buffer == NULL) report_error_exit("allocating memory\n");

    if (alloc_ring_buffer(&event_buffer, 64 * 1024) == false) report_error_exit("allocating ring buffer\n");

    global_state.earray = &event_buffer;
    global_state.game_input = &game_input;
  }

  /* --- setup loop timer --- */
  long long int last_time_tick = get_tick();
  long long int tick_frequency;
  long long int resolution_milliseconds;
  {
    LARGE_INTEGER tick_frequency_query;
    QueryPerformanceFrequency(&tick_frequency_query);
    tick_frequency = tick_frequency_query.QuadPart;

    TIMECAPS timecaps;
    timeGetDevCaps(&timecaps, sizeof(timecaps));
    timeBeginPeriod(timecaps.wPeriodMin);
    resolution_milliseconds = (long long int)timecaps.wPeriodMin;
  }
  long long int target_frame_time_microseconds = 1000000 / 60;
  long long int last_report_time = last_time_tick;
  long long int microseconds_spent_on_work_since_last_report = 0;
  long long int frames_since_last_report = 0;

  /* --- main loop --- */
  int retval = 0;
  bool running = true;
  while (running)
  {
    /* --- message pump --- */
    MSG message;
    while (PeekMessageA(&message, NULL, 0, 0, PM_REMOVE))
    {
      if (message.message == WM_QUIT)
      {
        running = false;
        retval = (int)message.wParam;
      }
      else
      {
        TranslateMessage(&message);
        DispatchMessageA(&message);
      }
    }

    /* --- get mouse position & input events --- */
    {
      POINT mousePos;
      if (GetCursorPos(&mousePos) && ScreenToClient(window, &mousePos))
      {
        game_input.mouse.x = mousePos.x;
        game_input.mouse.y = mousePos.y;
      }

      game_input.events = (event*)event_buffer.read;
      game_input.events_count = event_buffer.size_used / sizeof(event);

      game_input.window_resolution.x = global_state.resolution_x;
      game_input.window_resolution.y = global_state.resolution_y;
    }

    /* --- call the game code --- */
    {
      pixel_buffer pixels = {0};
      pixels.buffer = global_state.pixels.buffer;
      pixels.width = global_state.pixels.width;
      pixels.height = global_state.pixels.height;
      pixels.bytesPerPixel = global_state.pixels.bytesPerPixel;
      pixels.stride = global_state.pixels.stride;
      pixels.size = global_state.pixels.size;

      event* first_frame_event = game_input.events;

      game_main(&game_input, &pixels, &memory);

      event_buffer.size_used -= (int)((ULONG_PTR)game_input.events - (ULONG_PTR)first_frame_event);
      event_buffer.read = game_input.events;
    }

    /* --- draw the buffer to the screen --- */
    {
      HDC hdc = GetDC(window);
      flip_buffers(hdc, global_state.resolution_x, global_state.resolution_y, &global_state.pixels);
      ReleaseDC(window, hdc);
    }

    /* --- control loop timing --- */
    {
      long long int current_time_tick = get_tick();

      long long int microseconds_spent = (current_time_tick - last_time_tick) * 1000000 / tick_frequency;
      long long int frames_missed = microseconds_spent / target_frame_time_microseconds;
      microseconds_spent_on_work_since_last_report += microseconds_spent;
      frames_since_last_report++;

      DWORD milliseconds_to_sleep = (DWORD)((target_frame_time_microseconds - microseconds_spent % target_frame_time_microseconds) / 1000 / resolution_milliseconds * resolution_milliseconds);
      Sleep(milliseconds_to_sleep);
      long long int target_frame_end_tick = last_time_tick + (frames_missed + 1) * target_frame_time_microseconds * tick_frequency / 1000000;
      do {
        current_time_tick = get_tick();
      } while (current_time_tick < target_frame_end_tick);
      
      game_input.dt = (float)(current_time_tick - last_time_tick) / (float)tick_frequency;
      last_time_tick = current_time_tick;

      long long int microseconds_since_report = (current_time_tick - last_report_time) * 1000000 / tick_frequency;
      if (microseconds_since_report > 1000000)
      {
        long long int average_microseconds_per_frame = microseconds_spent_on_work_since_last_report / frames_since_last_report;
        float fps = (float)frames_since_last_report / (float)microseconds_since_report * 1000000.0f;

        char buffer[256];
        snprintf(buffer, 256, "mus/frame = %lld;   fps = %lld\n", average_microseconds_per_frame, frames_since_last_report);
        OutputDebugStringA(buffer);

        last_report_time = current_time_tick;
        microseconds_spent_on_work_since_last_report = 0;
        frames_since_last_report = 0;
      }

      if (frames_missed > 0)
      {
        char buffer[256];
        snprintf(buffer, 256, "missed %lld frame(s)\n", frames_missed);
        OutputDebugStringA(buffer);
      }
    }

  }

  /* --- tidy up --- */
  timeEndPeriod((UINT)(resolution_milliseconds));
  
  return retval;
}
