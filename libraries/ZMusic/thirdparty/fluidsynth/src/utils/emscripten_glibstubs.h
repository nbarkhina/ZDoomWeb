#ifndef _EMSCRIPTEN_GLIBSTUBS_H
#define _EMSCRIPTEN_GLIBSTUBS_H

/*
 * Portable, single-threaded glib replacement for the Emscripten/WebAssembly
 * build of FluidSynth.
 *
 * Real glib is not available for wasm, and the win32_glibstubs are tied to the
 * Win32 API (SRWLOCK / CRITICAL_SECTION / CONDITION_VARIABLE / Interlocked /
 * _beginthreadex). This header provides the same surface using portable,
 * single-threaded primitives.
 *
 * The GZDoom web build is single-threaded (no pthreads / SharedArrayBuffer) and
 * ZMusic drives FluidSynth synchronously (fluid_synth_write_float in
 * ComputeOutput) with synth.cpu-cores = 1 and ENABLE_MIXER_THREADS undefined,
 * so no worker threads are ever spawned and all locking is a no-op. The thread
 * helpers exist only so fluid_sys.c links; they are not exercised at runtime.
 *
 * Like win32_glibstubs.h, this forces FluidSynth onto the pre-2.32 ("old")
 * glib thread API by evaluating GLIB_CHECK_VERSION to 0.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <time.h>
#include <sys/stat.h>

#if defined(__has_include)
#  if __has_include(<alloca.h>)
#    include <alloca.h>
#  endif
#endif

/* Miscellaneous stubs */
#define GLIB_CHECK_VERSION(x, y, z) 0 /* Evaluate to 0 to use the "old" thread API */
#define GLIB_MAJOR_VERSION 2
#define GLIB_MINOR_VERSION 29

typedef struct
{
    int code;
    const char *message;
} GError;
typedef void *gpointer;
typedef const void *gconstpointer;

/* Booleans and basic glib integer types — normally supplied by <glib.h> (or
   <windows.h> on the Win32 stub path via TRUE/FALSE). */
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

typedef int gboolean;
typedef char gchar;
typedef unsigned char guchar;
typedef int gint;
typedef unsigned int guint;
typedef short gshort;
typedef unsigned short gushort;
typedef long glong;
typedef unsigned long gulong;
typedef int8_t gint8;
typedef uint8_t guint8;
typedef int16_t gint16;
typedef uint16_t guint16;
typedef int32_t gint32;
typedef uint32_t guint32;
typedef int64_t gint64;
typedef uint64_t guint64;
typedef float gfloat;
typedef double gdouble;
typedef size_t gsize;

#define g_new(s, c) FLUID_ARRAY(s, c)
#define g_free(p) FLUID_FREE(p)
#define g_strfreev FLUID_FREE
#define g_newa(_type, _len) (_type *)alloca(sizeof(_type) * (_len))
#define g_assert(a) assert(a)
#define G_LIKELY(expr) (expr)
#define G_UNLIKELY(expr) (expr)

#define g_vsnprintf(b, c, f, a) vsnprintf(b, c, f, a)
#define g_snprintf(b, c, f, ...) snprintf(b, c, f, __VA_ARGS__)

#define g_return_val_if_fail(expr, val) if (expr) {} else { return val; }
#define g_clear_error(err) do {} while (0)

#define G_FILE_TEST_EXISTS 1
#define G_FILE_TEST_IS_REGULAR 2

#define g_file_test fluid_g_file_test
#define g_shell_parse_argv fluid_g_shell_parse_argv

/* File status — fluid_sys.h maps fluid_stat()/fluid_stat_buf_t onto g_stat()/
   struct stat on the POSIX branch (which the wasm build takes). */
#define g_stat(_path, _buf) stat((_path), (_buf))

static inline int fluid_g_file_test(const char *path, int flags)
{
    struct stat st;
    if (path == NULL || stat(path, &st) != 0)
        return 0;
    if (flags & G_FILE_TEST_EXISTS)
        return 1;
    if (flags & G_FILE_TEST_IS_REGULAR)
        return S_ISREG(st.st_mode) ? 1 : 0;
    return 0;
}

/* Not exercised by ZMusic's synchronous render path; provided for completeness. */
static inline int fluid_g_shell_parse_argv(const char *command_line, int *argcp,
                                           char ***argvp, void *dummy)
{
    (void)command_line;
    (void)dummy;
    if (argcp) *argcp = 0;
    if (argvp) *argvp = NULL;
    return 0;
}

#define g_get_monotonic_time fluid_g_get_monotonic_time
static inline double fluid_g_get_monotonic_time(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1000000.0 + (double)ts.tv_nsec / 1000.0;
}

/* Byte ordering (wasm is little-endian) */
#ifdef __BYTE_ORDER__
#define G_BYTE_ORDER __BYTE_ORDER__
#define G_BIG_ENDIAN __ORDER_BIG_ENDIAN__
#else
/* If __BYTE_ORDER__ isn't defined, assume little endian */
#define G_BYTE_ORDER 1234
#define G_BIG_ENDIAN 4321
#endif

#if G_BYTE_ORDER == G_BIG_ENDIAN
#define GINT16_FROM_LE(x) (int16_t)(((uint16_t)(x) >> 8) | ((uint16_t)(x) << 8))
#define GINT32_FROM_LE(x) (int32_t)((FLUID_LE16TOH(x) << 16) | (FLUID16_LE16TOH(x >> 16)))
#else
#define GINT32_FROM_LE(x) (x)
#define GINT16_FROM_LE(x) (x)
#endif

/* Thread support — single-threaded: no worker threads are ever created. */
#define g_thread_supported() 1
#define g_thread_init(_) do {} while (0)
#define g_usleep(usecs) do { (void)(usecs); } while (0)

typedef gpointer (*GThreadFunc)(void *data);
typedef struct
{
    GThreadFunc func;
    void *data;
} GThread;

#define g_thread_self() ((GThread *)NULL)
#define g_thread_create fluid_g_thread_create
#define g_thread_join fluid_g_thread_join

/* No threading on the single-threaded web build. fluid_sys.c compiles these in
   (new_fluid_thread), but ZMusic never spawns a thread, so a failing stub is
   safe: callers treat a NULL thread as a creation failure. */
static inline GThread *fluid_g_thread_create(GThreadFunc func, void *data,
                                             int joinable, GError **error)
{
    (void)func;
    (void)data;
    (void)joinable;
    if (error != NULL) *error = NULL;
    return NULL;
}

static inline void fluid_g_thread_join(GThread *thread)
{
    (void)thread;
}

/* Regular mutex — no-op (single-threaded) */
typedef int GStaticMutex;
#define G_STATIC_MUTEX_INIT 0
#define g_static_mutex_init(_m) do { *(_m) = 0; } while (0)
#define g_static_mutex_free(_m) do {} while (0)
#define g_static_mutex_lock(_m) do { (void)(_m); } while (0)
#define g_static_mutex_unlock(_m) do { (void)(_m); } while (0)

/* Recursive lock capable mutex — no-op */
typedef int GStaticRecMutex;
#define g_static_rec_mutex_init(_m) do { *(_m) = 0; } while (0)
#define g_static_rec_mutex_free(_m) do {} while (0)
#define g_static_rec_mutex_lock(_m) do { (void)(_m); } while (0)
#define g_static_rec_mutex_unlock(_m) do { (void)(_m); } while (0)

/* Dynamically allocated mutex suitable for fluid_cond_t use — no-op */
typedef int GMutex;
#define g_mutex_free(m) do { if (m != NULL) g_free(m); } while (0)
#define g_mutex_lock(m) do { (void)(m); } while (0)
#define g_mutex_unlock(m) do { (void)(m); } while (0)

static inline GMutex *g_mutex_new(void)
{
    GMutex *mutex = g_new(GMutex, 1);
    if (mutex != NULL) *mutex = 0;
    return mutex;
}

/* Thread condition signaling — no-op (no thread ever waits) */
typedef int GCond;
#define g_cond_free(cond) do { if (cond != NULL) g_free(cond); } while (0)
#define g_cond_signal(cond) do { (void)(cond); } while (0)
#define g_cond_broadcast(cond) do { (void)(cond); } while (0)
#define g_cond_wait(cond, mutex) do { (void)(cond); (void)(mutex); } while (0)

static inline GCond *g_cond_new(void)
{
    GCond *cond = g_new(GCond, 1);
    if (cond != NULL) *cond = 0;
    return cond;
}

/* Thread private data — a plain pointer in a single-threaded world */
typedef void *GStaticPrivate;
#define g_static_private_init(_priv) do { *(_priv) = NULL; } while (0)
#define g_static_private_get(_priv) (*(_priv))
#define g_static_private_set(_priv, _data, _dtor) do { *(_priv) = (_data); } while (0)
#define g_static_private_free(_priv) do {} while (0)

/* Atomic operations — single-threaded ⇒ plain operations are correct */
#define g_atomic_int_inc(_pi) (++(*(_pi)))
#define g_atomic_int_get(_pi) (*(_pi))
#define g_atomic_int_set(_pi, _val) do { *(_pi) = (_val); } while (0)
#define g_atomic_int_dec_and_test(_pi) (--(*(_pi)) == 0)
#define g_atomic_int_compare_and_exchange(_pi, _old, _new) \
    (*(_pi) == (_old) ? (*(_pi) = (_new), 1) : 0)

static inline int fluid_g_atomic_int_exchange_and_add(volatile int *p, int add)
{
    int old = *p;
    *p += add;
    return old;
}
#define g_atomic_int_exchange_and_add(_pi, _add) \
    fluid_g_atomic_int_exchange_and_add((volatile int *)(_pi), (_add))

#define g_atomic_pointer_get(_pp) (*(_pp))
#define g_atomic_pointer_set(_pp, _val) do { *(_pp) = (_val); } while (0)
#define g_atomic_pointer_compare_and_exchange(_pp, _old, _new) \
    (*(_pp) == (_old) ? (*(_pp) = (_new), 1) : 0)

#endif /* _EMSCRIPTEN_GLIBSTUBS_H */
