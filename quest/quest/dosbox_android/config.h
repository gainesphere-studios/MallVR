/* DOSBox-X config.h for Android arm64 (Quest), hand-written (no autotools on Windows).
   Based on vs/config.h + configure.ac's aarch64 answers. Kept small: no OpenGL output, no debugger, no MT32/FluidSynth/FreeType/PNG yet. */
#ifndef DOSBOX_X_ANDROID_CONFIG_H
#define DOSBOX_X_ANDROID_CONFIG_H

#define C_SDL2 1
#define C_TARGETCPU ARMV8LE
#define C_DYNREC 1
#undef  C_DYNAMIC_X86
#define C_FPU 1
#undef  C_FPU_X86
#define C_UNALIGNED_MEMORY 1
#define C_HAVE_MMAP 1
#define C_HAVE_MPROTECT 1
#define C_HAVE_POSIX_MEMALIGN 1
#define C_ATTRIBUTE_ALWAYS_INLINE 1
#define C_HAS_ATTRIBUTE 1
#define C_HAS_BUILTIN_EXPECT 1
#define C_LIBZ 1
#define C_LIBPNG 1
#define C_SSHOT 1
#undef  C_IPX
#define C_SDL_NET 0
#undef  C_SDL_NET
#define C_SURFACE_POSTRENDER_ASPECT 1
#define C_SURFACE_POSTRENDER_ASPECT_BATCH_SIZE 16
#define DIRENT_HAS_D_TYPE 1
#define DB_HAVE_CLOCK_GETTIME 1
#define C_XBRZ 1
#define ENVIRON_LINKED 1

#define HAVE_INTTYPES_H 1
#define HAVE_MEMORY_H 1
#define HAVE_NETINET_IN_H 1
#define HAVE_PWD_H 1
#define HAVE_STDINT_H 1
#define HAVE_STDLIB_H 1
#define HAVE_STRINGS_H 1
#define HAVE_STRING_H 1
#define HAVE_SYS_SOCKET_H 1
#define HAVE_SYS_STAT_H 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_UNISTD_H 1
#define STDC_HEADERS 1

#define SIZEOF_INT_P 8
#define SIZEOF_UNSIGNED_CHAR 1
#define SIZEOF_UNSIGNED_INT 4
#define SIZEOF_UNSIGNED_LONG 8
#define SIZEOF_UNSIGNED_LONG_LONG 8
#define SIZEOF_UNSIGNED_SHORT 2

#define INLINE inline __attribute__((always_inline))
#define DB_FASTCALL
#define GCC_ATTRIBUTE(x) __attribute__ ((x))
#define GCC_UNLIKELY(x) __builtin_expect((x),0)
#define GCC_LIKELY(x) __builtin_expect((x),1)

typedef double Real64;
typedef unsigned char Bit8u;    typedef signed char Bit8s;
typedef unsigned short Bit16u;  typedef signed short Bit16s;
typedef unsigned int Bit32u;    typedef signed int Bit32s;
typedef unsigned long Bit64u;   typedef signed long Bit64s;
typedef Bit64u Bitu;            typedef Bit64s Bits;

#define HAS_CDIRECTLPT 0

#include "config_package.h"
#endif
