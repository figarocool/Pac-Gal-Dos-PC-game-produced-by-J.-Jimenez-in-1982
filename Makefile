CC ?= cc
CFLAGS ?= -O2 -std=c11 -Wall -Wextra -Wpedantic
CPPFLAGS += $(shell pkg-config --cflags sdl2)
LDLIBS += $(shell pkg-config --libs sdl2) -lm

all: pac-gal
pac-gal: src/speaker.c src/speaker.h src/build.h src/presentation.c src/presentation.h src/audio.c src/audio.h src/main.c src/game.c src/game.h src/timing.c src/timing.h src/maze.h src/font.h src/i18n.c src/i18n.h
	$(CC) $(CPPFLAGS) $(CFLAGS) src/main.c src/game.c src/timing.c src/audio.c src/speaker.c src/presentation.c src/i18n.c -o $@.tmp $(LDLIBS)
	mv $@.tmp $@
test: pac-gal audio-test speaker-test pc-speaker-test
	./pac-gal --self-test
	SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ./pac-gal --start --seed 1982 --frames 3 --screenshot /tmp/pac-gal-smoke.bmp
differential: pac-gal
	$(CC) -Isrc $(CFLAGS) tests/differential.c src/game.c -o /tmp/pacgal-differential -lm
	/tmp/pacgal-differential
deep-test:
	python3 tools/deep_test.py
oracle:
	python3 tools/dos_oracle.py
	PACGAL_GHOSTS=1 python3 tools/dos_oracle.py --moves
vita:
	cmake -S vita -B build-vita -DVITASDK=/usr/local/vitasdk -DCMAKE_BUILD_TYPE=Release
	cmake --build build-vita -j 4
	mkdir -p dist
	cp build-vita/pac-gal.vpk dist/PAC-GAL-PSVita.vpk
	python3 tools/check_vpk.py
psp:
	PSPDEV=/usr/local/pspdev /usr/local/pspdev/bin/psp-cmake -S psp -B build-psp -DCMAKE_BUILD_TYPE=Release
	cmake --build build-psp -j 4
	mkdir -p dist
	cp build-psp/EBOOT.PBP dist/PAC-GAL-PSP-EBOOT.PBP
clean:
	$(RM) pac-gal
audio-test:
	$(CC) -Isrc -Itests $(CFLAGS) tests/audio.c src/audio.c -o /tmp/pacgal-audio-test -lm
	/tmp/pacgal-audio-test
.PHONY: vita psp deep-test audio-test all test clean differential oracle

speaker-test:
	$(CC) -Isrc $(CFLAGS) tests/speaker.c src/speaker.c src/audio.c -o /tmp/pacgal-speaker-test -lm
	/tmp/pacgal-speaker-test
.PHONY: speaker-test

pc-speaker-test:
	$(CC) -Isrc $(CFLAGS) tests/pc-speaker.c src/speaker.c src/audio.c -o /tmp/pacgal-pc-speaker-test -lm
	/tmp/pacgal-pc-speaker-test
.PHONY: pc-speaker-test
