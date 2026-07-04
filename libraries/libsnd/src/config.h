/* libsndfile config.h — MSVC x64, Ogg/Vorbis/FLAC enabled, MPEG/Opus disabled. */
#ifndef SF_CONFIG_H
#define SF_CONFIG_H

#define COMPILER_IS_GCC 0
#define CPU_IS_BIG_ENDIAN 0
#define CPU_IS_LITTLE_ENDIAN 1
#define ENABLE_EXPERIMENTAL_CODE 0
#define WORDS_BIGENDIAN 0

#ifdef _WIN32
#define OS_IS_WIN32 1
#define USE_WINDOWS_API 1
#define WIN32_TARGET_DLL 1
#define HAVE_IO_H 1
#define HAVE_DIRECT_H 1
#define HAVE_UNISTD_H 0
#define HAVE_SSIZE_T 0
#define HAVE_GETTIMEOFDAY 0
#else
#define OS_IS_WIN32 0
#define USE_WINDOWS_API 0
#define WIN32_TARGET_DLL 0
#define HAVE_UNISTD_H 1
#define HAVE_SSIZE_T 1
#define HAVE_GETTIMEOFDAY 1
#endif
#define OS_IS_OPENBSD 0

#define HAVE_EXTERNAL_XIPH_LIBS 1
#define HAVE_MPEG 0
#define HAVE_OPUS 0
#define HAVE_SPEEX 0
#define HAVE_SQLITE3 0
#define HAVE_ALSA_ASOUNDLIB_H 0
#define HAVE_SNDIO_H 0

#define HAVE_BYTESWAP_H 0
#define HAVE_CALLOC 1
#define HAVE_CEIL 1
#define HAVE_DECL_S_IRGRP 0
#define HAVE_DLFCN_H 0
#define HAVE_ENDIAN_H 0
#define HAVE_FLOOR 1
#define HAVE_FMOD 1
#define HAVE_FREE 1
#define HAVE_FSTAT 1
#define HAVE_FSTAT64 0
#define HAVE_FSYNC 0
#define HAVE_FTRUNCATE 0
#define HAVE_GETPAGESIZE 0
#define HAVE_INTTYPES_H 1
#define HAVE_LIBM 0
#define HAVE_LOCALE_H 1
#define HAVE_LRINT 1
#define HAVE_LRINTF 1
#define HAVE_LROUND 1
#define HAVE_LSEEK 1
#define HAVE_LSEEK64 0
#define HAVE_MALLOC 1
#define HAVE_MEMORY_H 1
#define HAVE_MMAP 0
#define HAVE_OPEN 1
#define HAVE_PIPE 0
#define HAVE_READ 1
#define HAVE_REALLOC 1
#define HAVE_SETLOCALE 1
#define HAVE_SNPRINTF 1
#define HAVE_STDINT_H 1
#define HAVE_STDLIB_H 1
#define HAVE_STRINGS_H 0
#define HAVE_STRING_H 1
#define HAVE_SYS_STAT_H 1
#ifdef _WIN32
#define HAVE_SYS_TIME_H 0
#else
#define HAVE_SYS_TIME_H 1
#endif
#define HAVE_SYS_TYPES_H 1
#define HAVE_SYS_WAIT_H 0
#ifdef _WIN32
#define HAVE_IMMINTRIN_H 1
#else
#define HAVE_IMMINTRIN_H 0
#endif
#define HAVE_STDBOOL_H 1
#define HAVE_VSNPRINTF 1
#define HAVE_WAITPID 0
#define HAVE_WRITE 1
#define OSX_DARWIN_VERSION 0
#define _MINIX 0

#if (HAVE_SSIZE_T == 0)
#define ssize_t intptr_t
#endif

#define PACKAGE "libsndfile"
#define PACKAGE_BUGREPORT ""
#define PACKAGE_NAME "libsndfile"
#define PACKAGE_STRING "libsndfile 1.2.2"
#define PACKAGE_TARNAME "libsndfile"
#define PACKAGE_URL ""
#define PACKAGE_VERSION "1.2.2"
#define VERSION "1.2.2"

#define SIZEOF_DOUBLE 8
#define SIZEOF_FLOAT 4
#define SIZEOF_INT 4
#define SIZEOF_INT64_T 8
#define SIZEOF_LOFF_T 8
#define SIZEOF_LONG 4
#define SIZEOF_LONG_LONG 8
#define SIZEOF_OFF64_T 8
#define SIZEOF_OFF_T 8
#define SIZEOF_SHORT 2
#define SIZEOF_SIZE_T 8
#define SIZEOF_SSIZE_T 8
#define SIZEOF_VOIDP 8
#define SIZEOF_WCHAR_T 2

#define INLINE_CODE
#define inline __inline

#endif
