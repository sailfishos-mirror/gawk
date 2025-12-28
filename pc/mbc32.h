/* Converting char32_t to multibyte and back for MinGW and MSVCRT.

   The idea is to provide our own implementation for when the
   console's codepage is 65001 (UTF-8), and otherwise to delegate to
   the Windows' own mbrtowc etc.  This means that characters beyond
   the BMP will only be supported when the console uses UTF-8.  The
   Windows implementations of "wide-character" functions are
   ill-equipped to support characters beyond the BMP, because the
   wchar_t is a 16-bit type, and most/all functions in the C runtime
   that accept wchar_t arguments don't support codepoints beyond the
   BMP, which require two wchar_t values to represent them.  */

#ifndef MBC32_H
#define MBC32_H

/* MinGW64 build linked against UCRT has somne of this in its library,
   via uchar.h.  */
#ifdef _UCRT

#include <uchar.h>

#else	/* !_UCRT */

/* The standard headers whose functions and typedefs we redirect below
   need to be included before the redirection.  */
#include <stdint.h>
#include <wchar.h>

typedef uint_least32_t char32_t;

/* Replacement for mbstate_t to support characters beyond the BMP.  */
struct mb32state
{
  char32_t ch; /* character being constructed */
  short total; /* total number of bytes in UTF-8 sequence */
  short rem;   /* remaining number of bytes */
};

typedef struct mb32state mb32state_t;

/* When using MSVCRT, we redirect mbstate_t in Gawk to the union below.  */
typedef union
{
  mbstate_t mbs16;
  mb32state_t mbs32;
} mbstate32_t;

#define mbstate_t mbstate32_t

extern int mingw_mbsinit (const mbstate_t *);

#define mbsinit(ps)          mingw_mbsinit(ps)

#endif	/* !_UCRT */

extern size_t mingw_mbrtoc32 (char32_t *__restrict__, const char *__restrict__,
			      size_t, mbstate_t *__restrict__);
extern size_t mbrlenc32 (const char *__restrict__, size_t n,
			 mbstate_t *__restrict__);
extern size_t mingw_c32rtomb (char *__restrict__, char32_t,
			      mbstate_t *__restrict__);

/* Our mbrtoc32 supports only UTF-8, so we need to redirect calls to
   our version which delegates to mbrtowc when in non-UTF-8 locale.  */
#define mbrtoc32(pc,s,n,ps)  mingw_mbrtoc32(pc,s,n,ps)

/* Likewise with c32rtomb.  */
#define c32rtomb(s,c,ps)     mingw_c32rtomb(s,c,ps)

/* Gawk calls mbrlen, so we need to redirect that to our own version
   which supports characters beyond the BMP.  */
#define mbrlen(s,n,ps)       mbrlenc32(s,n,ps)

#endif	/* MBC32_H */
/* Converting char32_t to multibyte and back for MinGW and MSVCRT.

   The idea is to provide our own implementation for when the
   console's codepage is 65001 (UTF-8), and otherwise to delegate to
   the Windows' own mbrtowc etc.  This means that characters beyond
   the BMP will only be supported when the console uses UTF-8.  The
   Windows implementations of "wide-character" functions are
   ill-equipped to support characters beyond the BMP, because the
   wchar_t is a 16-bit type, and most/all functions in the C runtime
   that accept wchar_t arguments don't support codepoints beyond the
   BMP, which require two wchar_t values to represent them.  */

#ifndef MBC32_H
#define MBC32_H

/* MinGW64 build linked against UCRT has somne of this in its library,
   via uchar.h.  */
#ifdef _UCRT

#include <uchar.h>

#else	/* !_UCRT */

/* The standard headers whose functions and typedefs we redirect below
   need to be included before the redirection.  */
#include <stdint.h>
#include <wchar.h>

typedef uint_least32_t char32_t;

/* Replacement for mbstate_t to support characters beyond the BMP.  */
struct mb32state
{
  char32_t ch; /* character being constructed */
  short total; /* total number of bytes in UTF-8 sequence */
  short rem;   /* remaining number of bytes */
};

typedef struct mb32state mb32state_t;

/* When using MSVCRT, we redirect mbstate_t in Gawk to the union below.  */
typedef union
{
  mbstate_t mbs16;
  mb32state_t mbs32;
} mbstate32_t;

#define mbstate_t mbstate32_t

extern int mingw_mbsinit (const mbstate_t *);

#define mbsinit(ps)          mingw_mbsinit(ps)

#endif	/* !_UCRT */

extern size_t mingw_mbrtoc32 (char32_t *__restrict__, const char *__restrict__,
			      size_t, mbstate_t *__restrict__);
extern size_t mbrlenc32 (const char *__restrict__, size_t n,
			 mbstate_t *__restrict__);
extern size_t mingw_c32rtomb (char *__restrict__, char32_t,
			      mbstate_t *__restrict__);

/* Our mbrtoc32 supports only UTF-8, so we need to redirect calls to
   our version which delegates to mbrtowc when in non-UTF-8 locale.  */
#define mbrtoc32(pc,s,n,ps)  mingw_mbrtoc32(pc,s,n,ps)

/* Likewise with c32rtomb.  */
#define c32rtomb(s,c,ps)     mingw_c32rtomb(s,c,ps)

/* Gawk calls mbrlen, so we need to redirect that to our own version
   which supports characters beyond the BMP.  */
#define mbrlen(s,n,ps)       mbrlenc32(s,n,ps)

#endif	/* MBC32_H */
