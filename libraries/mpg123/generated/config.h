/*
 * config.h — hand-authored build configuration for a static, from-source,
 * generic (no-assembly) libmpg123.
 *
 * This fork builds every dependency from source with no CMake and no prebuilt
 * binaries, so we replace mpg123's autotools/CMake-generated config.h with this
 * committed one. It targets a DECODER-ONLY libmpg123 (no CLI, no libout123
 * output modules, no networking, no terminal) and covers both toolchains:
 *
 *   - MSVC x64        (Windows, libraries/mpg123/mpg123.vcxproj)
 *   - Emscripten/clang (web build, zmusic.mk)
 *
 * Values were derived from ports/cmake/src/config.cmake.h.in for mpg123 1.34.0
 * (MPG123_API_VERSION 49). Only the "generic" C decoder is selected, so no
 * platform assembly (.S) files are compiled and this same header works on x86-64
 * and wasm alike.
 */
#ifndef MPG123_STATIC_CONFIG_H
#define MPG123_STATIC_CONFIG_H

/* ---- decoder selection --------------------------------------------------- *
 * OPT_GENERIC   : portable C synth/dct (no SIMD, no runtime CPU dispatch).
 * REAL_IS_FLOAT : floating-point sample math (both targets have an FPU).
 */
#define OPT_GENERIC
#define REAL_IS_FLOAT

/* Better 16-bit rounding + IEEE-754 float storage (x86-64 and wasm are both
   little-endian IEEE-754). New Huffman tables are faster on modern CPUs. */
#define ACCURATE_ROUNDING 1
#define IEEE_FLOAT 1
#define USE_NEW_HUFFTABLE 1

/* Gapless playback and the sample-seek index (WITH_SEEKTABLE=1000). */
#define GAPLESS 1
#define FRAME_INDEX 1
#define INDEX_SIZE 1000

/* Cosmetic identity, returned by mpg123_distversion()/feature reporting. */
#define PACKAGE_NAME "mpg123"
#define PACKAGE_VERSION "1.34.0"

/* ---- off_t / large-file support ------------------------------------------ *
 * lfs_wrap.c maps the off_t public API onto the internal int64_t API and holds
 * a compile-time assert that SIZEOF_OFF_T == sizeof(off_t). We do NOT define
 * LFS_LARGEFILE_64 because neither target needs a separate off64_t path:
 *   - MSVC:            off_t is 32-bit (long)   -> SIZEOF_OFF_T 4
 *   - Emscripten/musl: off_t is 64-bit          -> SIZEOF_OFF_T 8
 * The consumer (ZMusic) must NOT define _FILE_OFFSET_BITS, so the plain off_t
 * symbols line up with these sizes.
 */
#ifdef _WIN32
#  define SIZEOF_OFF_T 4
#else
#  define SIZEOF_OFF_T 8
#endif

/* ---- platform feature probes --------------------------------------------- */
#ifdef _WIN32
/* MSVC / Windows */
#  define HAVE_LIMITS_H 1
#  define HAVE_LOCALE_H 1
#  define HAVE_SIGNAL_H 1
#  define HAVE_SYS_STAT_H 1
#  define HAVE_SYS_TYPES_H 1
#  define HAVE_STRERROR 1
#  define HAVE_ATOLL 1
#  define HAVE__SETMODE 1
/* Windows has no <dirent.h>; take mpg123's Win32 (FindFirstFileW) path for the
   directory/patch helpers in compat.c and map the POSIX case-insensitive string
   compares onto the MSVC CRT names (mirrors config.cmake.h.in's WIN32 branch). */
#  define WANT_WIN32_UNICODE 1
#  define strcasecmp _stricmp
#  define strncasecmp _strnicmp
/* stdio descriptor numbers (compat.c binary-mode helper); mirrors the CMake
   MSVC branch of config.cmake.h.in. */
#  define STDIN_FILENO  (_fileno(stdin))
#  define STDOUT_FILENO (_fileno(stdout))
#  define STDERR_FILENO (_fileno(stderr))
#else
/* Emscripten / clang (musl libc, POSIX) */
#  define HAVE_LIMITS_H 1
#  define HAVE_LOCALE_H 1
#  define HAVE_SIGNAL_H 1
#  define HAVE_SYS_STAT_H 1
#  define HAVE_SYS_TYPES_H 1
#  define HAVE_SYS_TIME_H 1
#  define HAVE_SYS_SELECT_H 1
#  define HAVE_SYS_SIGNAL_H 1
#  define HAVE_UNISTD_H 1
#  define HAVE_DIRENT_H 1
#  define HAVE_STRINGS_H 1
#  define HAVE_STRERROR 1
#  define HAVE_STRERROR_L 1
#  define HAVE_STRTOK_R 1
#  define HAVE_ATOLL 1
#  define HAVE_USELOCALE 1
#  define CCALIGN 1
#endif

#endif /* MPG123_STATIC_CONFIG_H */
