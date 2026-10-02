cmake_minimum_required(VERSION 3.16)
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(NOT DEFINED VITASDK)
    set(VITASDK "/usr/local/vitasdk")
  endif()
  set(CMAKE_TOOLCHAIN_FILE "${VITASDK}/share/vita.toolchain.cmake" CACHE PATH "Vita toolchain")
endif()
project(pacgal_vita C)
include("${VITASDK}/share/vita.cmake" REQUIRED)
add_executable(pacgal_vita ../src/main.c ../src/game.c ../src/audio.c ../src/speaker.c ../src/timing.c ../src/presentation.c ../src/i18n.c platform.c)
target_compile_definitions(pacgal_vita PRIVATE VITA=1)
target_compile_options(pacgal_vita PRIVATE -O2 -std=c11 -Wall -Wextra -Wpedantic)
target_link_libraries(pacgal_vita SDL2
 SceGxm_stub SceDisplay_stub SceCtrl_stub SceAppMgr_stub SceAppUtil_stub
 SceAudio_stub SceAudioIn_stub SceSysmodule_stub SceIofilemgr_stub
 SceCommonDialog_stub SceTouch_stub SceHid_stub SceMotion_stub
 ScePower_stub SceProcessmgr_stub m)
vita_create_self(eboot.bin pacgal_vita SAFE)
vita_create_vpk(pac-gal.vpk "PACG19820" eboot.bin
 VERSION "01.16" NAME "PAC-GAL 1982"
 FILE sce_sys/icon0.png sce_sys/icon0.png
 FILE sce_sys/livearea/contents/bg.png sce_sys/livearea/contents/bg.png
 FILE sce_sys/livearea/contents/startup.png sce_sys/livearea/contents/startup.png
 FILE sce_sys/livearea/contents/template.xml sce_sys/livearea/contents/template.xml
 FILE ../LICENSE LICENSE FILE ../NOTICE NOTICE)
