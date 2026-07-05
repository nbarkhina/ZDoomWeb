## GZDoom -> Emscripten (WebAssembly) build
## Run from the web/ directory:  make -j$(nproc)
## (build.sh / start_emc.sh wrap this; SKILL pattern from the Bsnes project.)

MAKEFLAGS += --no-builtin-rules
ROOT   := .
OBJDIR := obj
OUT    := gzdoom.js
DISTDIR := dist

CC  := emcc
CXX := em++

include sources.mk
include zmusic.mk

# ---- helpers -------------------------------------------------------------
# map a repo-root-relative source path to its object file under obj/
objs = $(patsubst %,$(OBJDIR)/%.o,$(1))

STUB_SRC := stubs.cpp

ENGINE_PCH_OBJS   := $(call objs,$(ENGINE_CPP_PCH))
ENGINE_OTHER_OBJS := $(call objs,$(ENGINE_CPP_OTHER))
ENGINE_C_OBJS     := $(call objs,$(ENGINE_C))
STUB_OBJS         := $(call objs,$(STUB_SRC))
BZIP2_OBJS        := $(call objs,$(LIB_BZIP2))
LZMA_OBJS         := $(call objs,$(LIB_LZMA))
MINIZ_OBJS        := $(call objs,$(LIB_MINIZ))
WEBP_OBJS         := $(call objs,$(LIB_WEBP))
OGG_OBJS          := $(call objs,$(LIB_OGG))
VORBIS_OBJS       := $(call objs,$(LIB_VORBIS))
FLAC_OBJS         := $(call objs,$(LIB_FLAC))
SNDFILE_OBJS      := $(call objs,$(LIB_SNDFILE))
MPG123_OBJS       := $(call objs,$(LIB_MPG123))
ZWIDGET_OBJS      := $(call objs,$(LIB_ZWIDGET))

# ZMusic: one object group per backend (each needs its own include precedence)
ZMUSIC_OBJS := $(foreach g,$(ZM_GROUPS),$(call objs,$(ZM_$(g)_SRCS)))

ALL_OBJS := $(ENGINE_PCH_OBJS) $(ENGINE_OTHER_OBJS) $(ENGINE_C_OBJS) $(STUB_OBJS) \
            $(BZIP2_OBJS) $(LZMA_OBJS) $(MINIZ_OBJS) $(WEBP_OBJS) $(ZWIDGET_OBJS) \
            $(OGG_OBJS) $(VORBIS_OBJS) $(FLAC_OBJS) $(SNDFILE_OBJS) $(MPG123_OBJS) \
            $(ZMUSIC_OBJS)

# ---- flags ---------------------------------------------------------------
OPT := -O2
# wasm-opt (Binaryen) segfaults running its -O2 passes on this module, so the
# post-link optimizer is kept at -O1. Code is still compiled at -O2 above, which
# is where the runtime win comes from; -O1 here only lightens whole-module opt.
LINK_OPT := -O1
# NOTE: -sUSE_SDL=2 is intentionally NOT global: it injects emscripten's SDL2
# include dir (which ships its own <miniz.h>) ahead of ours, shadowing GZDoom's
# bundled miniz. We add it only to the few files that actually include SDL.
COMMON := $(OPT) -g0 -w -fexceptions -fno-strict-aliasing

SDL_FLAG := -sUSE_SDL=2

# Engine files that include SDL headers (must get -sUSE_SDL=2):
SDL_PLATFORM_SRCS := \
	src/common/platform/posix/sdl/hardware.cpp \
	src/common/platform/posix/sdl/i_gui.cpp \
	src/common/platform/posix/sdl/i_input.cpp \
	src/common/platform/posix/sdl/i_joystick.cpp \
	src/common/platform/posix/sdl/i_main.cpp \
	src/common/platform/posix/sdl/i_system.cpp \
	src/common/platform/posix/sdl/sdlglvideo.cpp
SDL_PLATFORM_OBJS := $(call objs,$(SDL_PLATFORM_SRCS))

# Engine include directories (mirrors src/CMakeLists.txt include_directories)
ENGINE_INC := \
	-I$(ROOT)/src \
	-I$(ROOT)/src/common/audio/sound \
	-I$(ROOT)/src/common/audio/music \
	-I$(ROOT)/src/common/2d \
	-I$(ROOT)/src/common/cutscenes \
	-I$(ROOT)/src/common/thirdparty/libsmackerdec/include \
	-I$(ROOT)/src/common/thirdparty \
	-I$(ROOT)/src/common/thirdparty/stb \
	-I$(ROOT)/src/common/thirdparty/utf8proc \
	-I$(ROOT)/src/common/textures \
	-I$(ROOT)/src/common/textures/formats \
	-I$(ROOT)/src/common/textures/hires \
	-I$(ROOT)/src/common/models \
	-I$(ROOT)/src/common/filesystem/include \
	-I$(ROOT)/src/common/utility \
	-I$(ROOT)/src/common/console \
	-I$(ROOT)/src/common/engine \
	-I$(ROOT)/src/common/menu \
	-I$(ROOT)/src/common/statusbar \
	-I$(ROOT)/src/common/fonts \
	-I$(ROOT)/src/common/objects \
	-I$(ROOT)/src/common/startscreen \
	-I$(ROOT)/src/common/widgets \
	-I$(ROOT)/src/common/rendering \
	-I$(ROOT)/src/common/rendering/hwrenderer/data \
	-I$(ROOT)/src/common/rendering/gl_load \
	-I$(ROOT)/src/common/rendering/gl \
	-I$(ROOT)/src/common/rendering/gles \
	-I$(ROOT)/src/common/rendering/gles/glad/include \
	-I$(ROOT)/src/common/scripting/vm \
	-I$(ROOT)/src/common/scripting/jit \
	-I$(ROOT)/src/common/scripting/core \
	-I$(ROOT)/src/common/scripting/interface \
	-I$(ROOT)/src/common/scripting/frontend \
	-I$(ROOT)/src/common/scripting/backend \
	-I$(ROOT)/src/g_statusbar \
	-I$(ROOT)/src/console \
	-I$(ROOT)/src/playsim \
	-I$(ROOT)/src/playsim/bots \
	-I$(ROOT)/src/playsim/mapthinkers \
	-I$(ROOT)/src/gamedata \
	-I$(ROOT)/src/gamedata/textures \
	-I$(ROOT)/src/gamedata/fonts \
	-I$(ROOT)/src/rendering \
	-I$(ROOT)/src/rendering/hwrenderer \
	-I$(ROOT)/src/rendering/2d \
	-I$(ROOT)/src/r_data \
	-I$(ROOT)/src/sound \
	-I$(ROOT)/src/menu \
	-I$(ROOT)/src/sound/backend \
	-I$(ROOT)/src/gamedata/xlat \
	-I$(ROOT)/src/utility \
	-I$(ROOT)/src/utility/nodebuilder \
	-I$(ROOT)/src/scripting \
	-I$(ROOT)/src/scripting/zscript \
	-I$(ROOT)/src/launcher \
	-I$(ROOT)/src/common/platform/posix \
	-I$(ROOT)/src/common/platform/posix/sdl \
	-I$(ROOT)/generated \
	-I$(ROOT)/libraries/ZMusic/include \
	-I$(ROOT)/libraries/ZWidget/include \
	-I$(ROOT)/libraries/webp/include \
	-I$(ROOT)/libraries/range_map/include \
	-I$(ROOT)/libraries/lzma/C \
	-I$(ROOT)/libraries/bzip2 \
	-I$(ROOT)/libraries/ZMusic/thirdparty/miniz \
	-I$(ROOT)/src/common/audio/sound/thirdparty

ENGINE_DEF := -DHAVE_GLES2 -DZMUSIC_STATIC -DNO_GTK -DNO_SSE \
	-DNO_SEND_STATS -D__forceinline=inline -DENGINE_NAME=\"GZDoom\" -D__EMSCRIPTEN__ \
	-Dstricmp=strcasecmp -Dstrnicmp=strncasecmp

ENGINE_FLAGS := $(ENGINE_INC) $(ENGINE_DEF)

# ---- per-group flag assignments (target-specific) ------------------------
# C++ standard is applied per-language in the .cpp rule (so ZMusic's 232 C files
# don't get -std=c++17). Default is c++17; ZWidget overrides to c++20.
$(ENGINE_PCH_OBJS):   GROUPFLAGS := $(ENGINE_FLAGS) -include $(ROOT)/src/g_pch.h
$(ENGINE_OTHER_OBJS): GROUPFLAGS := $(ENGINE_FLAGS)
$(ENGINE_C_OBJS):     GROUPFLAGS := $(ENGINE_FLAGS)
$(STUB_OBJS):         GROUPFLAGS :=
$(BZIP2_OBJS):        GROUPFLAGS := -DBZ_NO_STDIO
$(LZMA_OBJS):         GROUPFLAGS := -DZ7_PPMD_SUPPORT -I$(ROOT)/libraries/lzma/C
$(MINIZ_OBJS):        GROUPFLAGS := -DMINIZ_NO_STDIO -I$(ROOT)/libraries/ZMusic/thirdparty/miniz
$(WEBP_OBJS):         GROUPFLAGS := -I$(ROOT)/libraries/webp -I$(ROOT)/libraries/webp/src
$(OGG_OBJS):          GROUPFLAGS := -I$(ROOT)/libraries/ogg-main/include
$(VORBIS_OBJS):       GROUPFLAGS := -I$(ROOT)/libraries/vorbis-main/include -I$(ROOT)/libraries/vorbis-main/lib -I$(ROOT)/libraries/ogg-main/include
$(FLAC_OBJS):         GROUPFLAGS := -DHAVE_CONFIG_H -DFLAC__NO_DLL -I$(ROOT)/libraries/flac-master -I$(ROOT)/libraries/flac-master/include -I$(ROOT)/libraries/flac-master/src/libFLAC/include -I$(ROOT)/libraries/ogg-main/include
$(SNDFILE_OBJS):      GROUPFLAGS := -DHAVE_CONFIG_H -I$(ROOT)/libraries/libsnd/src -I$(ROOT)/libraries/libsnd/include -I$(ROOT)/libraries/ogg-main/include -I$(ROOT)/libraries/vorbis-main/include -I$(ROOT)/libraries/flac-master/include
$(MPG123_OBJS):       GROUPFLAGS := -I$(ROOT)/libraries/mpg123/generated -I$(ROOT)/libraries/mpg123/src -I$(ROOT)/libraries/mpg123/src/include -I$(ROOT)/libraries/mpg123/src/libmpg123
$(ZWIDGET_OBJS):      GROUPFLAGS := -I$(ROOT)/libraries/ZWidget/include -I$(ROOT)/libraries/ZWidget/include/zwidget -I$(ROOT)/libraries/ZWidget/src $(SDL_FLAG)
$(ZWIDGET_OBJS):      CXXSTD := -std=c++20

# SDL-using engine files: same as ENGINE_OTHER but with -sUSE_SDL=2 (listed last so it wins)
$(SDL_PLATFORM_OBJS): GROUPFLAGS := $(ENGINE_FLAGS) $(SDL_FLAG)

# ZMusic: one rule per backend group, each with its own include precedence.
define ZM_GROUP_RULE
$$(call objs,$$(ZM_$(1)_SRCS)): GROUPFLAGS := $$(ZM_$(1)_INC) $$(ZM_GLOBAL_INC) $$(ZM_DEFS)
endef
$(foreach g,$(ZM_GROUPS),$(eval $(call ZM_GROUP_RULE,$(g))))


# ---- link flags ----------------------------------------------------------
PK3DIR := $(ROOT)/pk3

# Only the engine resource pk3s are baked into the .data file. Game IWADs/PWADs
# (doom2.wad, mods, …) are NOT preloaded — script.js fetches the selected WAD
# over HTTP and FS.writeFile()s it into MEMFS right before callMain(), so the
# game data can be swapped without rebuilding/redownloading the whole .data.
#
# The SoundFont IS baked in: FluidSynth (the default MIDI synth, snd_mididevice
# = -5) loads $PROGDIR/soundfonts/gzdoom.sf2 (progdir = "/" on web), matching the
# Windows build's sampled-instrument music instead of OPL/ADL FM synthesis.
PRELOAD := \
	--preload-file $(PK3DIR)/gzdoom.pk3@/gzdoom.pk3 \
	--preload-file $(PK3DIR)/game_support.pk3@/game_support.pk3 \
	--preload-file $(PK3DIR)/game_widescreen_gfx.pk3@/game_widescreen_gfx.pk3 \
	--preload-file $(PK3DIR)/brightmaps.pk3@/brightmaps.pk3 \
	--preload-file $(PK3DIR)/lights.pk3@/lights.pk3 \
	--preload-file $(ROOT)/soundfont/gzdoom.sf2@/soundfonts/gzdoom.sf2

LINKFLAGS := \
	$(LINK_OPT) -fexceptions \
	-sUSE_SDL=2 \
	-sFULL_ES3=1 \
	-sMIN_WEBGL_VERSION=2 \
	-sMAX_WEBGL_VERSION=2 \
	-sALLOW_MEMORY_GROWTH=1 \
	-sINITIAL_MEMORY=268435456 \
	-sSTACK_SIZE=8388608 \
	-sASSERTIONS=0 \
	-sEXIT_RUNTIME=0 \
	-sINVOKE_RUN=0 \
	-sNO_DISABLE_EXCEPTION_CATCHING \
	-sEXPORTED_RUNTIME_METHODS=callMain,FS,ccall,cwrap,UTF8ToString \
	-lopenal \
	-lidbfs.js

# ---- targets -------------------------------------------------------------
all: $(OUT)

.PHONY: sdl2_port
sdl2_port:
	@embuilder build sdl2

$(ALL_OBJS): | sdl2_port

$(OUT): $(ALL_OBJS)
	$(CXX) -o $@ $(ALL_OBJS) $(COMMON) $(LINKFLAGS) $(PRELOAD)
	@echo "Copying web build ($(OUT), gzdoom.wasm, gzdoom.data) -> $(DISTDIR)/"
	@mkdir -p $(DISTDIR)
	cp $(OUT) gzdoom.wasm gzdoom.data $(DISTDIR)/

# C++ rule  (CXXSTD defaults to c++17, overridden per-group e.g. ZWidget=c++20)
CXXSTD := -std=c++17
$(OBJDIR)/%.cpp.o: $(ROOT)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(COMMON) $(CXXSTD) $(GROUPFLAGS) -MMD -MP -MF $(@:.o=.d) -c $< -o $@

# C rule  (no C++ standard flag)
$(OBJDIR)/%.c.o: $(ROOT)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(COMMON) $(GROUPFLAGS) -MMD -MP -MF $(@:.o=.d) -c $< -o $@

clean:
	rm -rf $(OBJDIR) $(OUT)
	rm -f $(DISTDIR)/gzdoom.js $(DISTDIR)/gzdoom.wasm $(DISTDIR)/gzdoom.data

.PHONY: all clean

-include $(ALL_OBJS:.o=.d)
