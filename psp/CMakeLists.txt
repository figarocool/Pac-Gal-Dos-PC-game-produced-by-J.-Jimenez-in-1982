cmake_minimum_required(VERSION 3.10)
project(pacgal_psp C)

add_executable(pacgal_psp
  ../src/main.c
  ../src/game.c
  ../src/audio.c
  ../src/speaker.c
  ../src/timing.c
  ../src/presentation.c
  ../src/i18n.c
  platform.c
)
target_compile_definitions(pacgal_psp PRIVATE PSP=1)
target_compile_options(pacgal_psp PRIVATE -O2 -std=c11 -Wall -Wextra -Wpedantic)
find_package(SDL2 REQUIRED)
target_include_directories(pacgal_psp PRIVATE ${SDL2_INCLUDE_DIRS})
target_link_libraries(pacgal_psp PRIVATE ${SDL2_LIBRARIES} psputility)

create_pbp_file(
  TARGET pacgal_psp
  TITLE "PAC-GAL 1982"
  VERSION "01.01"
  ICON_PATH "${CMAKE_CURRENT_SOURCE_DIR}/icon0.png"
  PREVIEW_PATH "${CMAKE_CURRENT_SOURCE_DIR}/preview.png"
  MEMSIZE 1
)
