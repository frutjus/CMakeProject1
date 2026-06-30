cmake_minimum_required (VERSION 3.8)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project("CMakeProject1")

add_executable(CMakeProject1)

target_sources(CMakeProject1 PRIVATE "src/game.c" "src/graphics.c" "src/datatypes.c"  "src/allocator.c" "src/conway.c" "src/rts.c")

if(
  (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC") OR
  (CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
)
  target_sources(CMakeProject1 PRIVATE "src/win32.c")

  target_compile_options(CMakeProject1 PRIVATE /W3)

  target_link_options(CMakeProject1 PRIVATE /SUBSYSTEM:WINDOWS)
  
  target_link_libraries(CMakeProject1 PRIVATE mincore winmm opengl32 glu32)

elseif(
  (CMAKE_CXX_COMPILER_ID STREQUAL "GNU") OR
  (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
)

  target_compile_options(CMakeProject1 -Wall)

endif()
