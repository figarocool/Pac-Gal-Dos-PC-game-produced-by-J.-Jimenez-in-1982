# DOS / DOSBox build

PAC-GAL has a native DJGPP build that runs in DOS and DOSBox. The game code is
the C recreation in this repository. The package does not include the historic
1982 executable. The recreation retains the historical layout and effects;
the current sources describe them through drawing instructions and musical
scores. The October 2026 update rebuilds the DOS package with that representation.

## Requirements

- DJGPP (i586-pc-msdosdjgpp GCC)
- SDL3 `main` source, whose DOS backend is still newer than the current stable
  release. Build it as a static DJGPP library using its
  `build-scripts/i586-pc-msdosdjgpp.cmake` toolchain file.
- A DPMI host at runtime. DOSBox 0.74-3 in particular needs one, such as
  CWSDPMI; obtain and install it separately from the official DJGPP distribution.
  It is deliberately not bundled in this project archive.
- DOSBox configured with VGA/VESA and Sound Blaster emulation for graphics/audio.

Build SDL3 from its source checkout:

```sh
cmake -S SDL -B build-sdl3-dos \
  -DCMAKE_TOOLCHAIN_FILE=SDL/build-scripts/i586-pc-msdosdjgpp.cmake \
  -DCMAKE_BUILD_TYPE=Release -DSDL_SHARED=OFF -DSDL_STATIC=ON \
  -DSDL_TESTS=OFF -DSDL_EXAMPLES=OFF -DSDL_TEST_LIBRARY=OFF
cmake --build build-sdl3-dos -j4
cmake --install build-sdl3-dos --prefix "$PWD/sdl3-dos-install"
```

Source the DJGPP `setenv` script first, then point the build at SDL3:

```sh
export SDL3_DOS_INCLUDE="$PWD/sdl3-dos-install/include"
export SDL3_DOS_STATIC_LIB="$PWD/build-sdl3-dos/libSDL3.a"
make dos
```

The result is `dist/PACGAL.EXE`. Copy it and a separately installed DPMI host
into a DOSBox-mounted folder. At the DOS prompt run the DPMI host, then:

```text
PACGAL.EXE
```

`make dos` only needs the DJGPP compiler and an already-built SDL3 library; it
does not download or bundle third-party software.
