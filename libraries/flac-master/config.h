/* FLAC config.h — generated for MSVC x64 (little-endian), Ogg enabled. */
#ifndef FLAC_CONFIG_H
#define FLAC_CONFIG_H

#define CPU_IS_BIG_ENDIAN 0
#define CPU_IS_LITTLE_ENDIAN 1
#define ENABLE_64_BIT_WORDS 1
#ifdef __EMSCRIPTEN__
#define HAVE_FSEEKO 1
#endif

#define OGG_FOUND 1
#define FLAC__HAS_OGG OGG_FOUND

#ifdef __EMSCRIPTEN__
#define FLAC__HAS_X86INTRIN 0
#else
#define FLAC__HAS_X86INTRIN 1
#endif
#define FLAC__HAS_NEONINTRIN 0
#define FLAC__HAS_A64NEONINTRIN 0

#define HAVE_LROUND 1
#define HAVE_STDINT_H 1
#define HAVE_STDLIB_H 1
#define HAVE_STRING_H 1
#define HAVE_MEMORY_H 1
#define HAVE_INTTYPES_H 1

#define SIZEOF_OFF_T 4
#define SIZEOF_VOIDP 8

#define PACKAGE_VERSION "1.4.3"
#define PACKAGE_NAME "flac"
#define PACKAGE_STRING "flac 1.4.3"
#define PACKAGE_TARNAME "flac"
#define PACKAGE_BUGREPORT ""
#define PACKAGE_URL ""

#endif
