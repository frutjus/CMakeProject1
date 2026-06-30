#pragma once

#define DEBUG

#define ARRAY_COUNT(arr) (sizeof(arr) / sizeof(arr[0]))

#ifdef DEBUG
  #define ASSERT(cond) if (!(cond)) { *(char*)0 = 0; }
#else
  #define ASSERT(cond)
#endif

#define INTERNAL static
#define GLOBAL static

#define UNIMPLEMENTED(any) ASSERT(0)

typedef enum {
  false,
  true,
} bool;

#ifdef DEBUG
  #if defined(_MSC_VER)
    // For MSVC (Visual Studio)
    #define DEBUG_BREAK() __debugbreak()
  #elif defined(__GNUC__) || defined(__clang__)
    // For GCC / Clang on Linux/macOS
    #if defined(__x86_64__) || defined(__i386__)
      #define DEBUG_BREAK() __asm__ volatile("int $3")
    #elif defined(__arm__) || defined(__aarch64__)
      #define DEBUG_BREAK() __asm__ volatile(".inst 0xd4200000") // BRK #0
    #else
      #include <signal.h>
      #define DEBUG_BREAK() raise(SIGTRAP) // Fallback for other architectures
    #endif
  #else
    #include <signal.h>
    #define DEBUG_BREAK() raise(SIGTRAP) // General fallback
  #endif
#else
  #define DEBUG_BREAK()
#endif

