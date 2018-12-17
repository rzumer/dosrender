#ifndef COMMON_H
#define COMMON_H

#ifdef __WATCOMC__
#define farfree _ffree
#define farmalloc _fmalloc
#define inportb inp
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define PRECISION_INTEGER 1
#if PRECISION_INTEGER
#define coord_t int
#define cabs abs
#define CROUND(x) ROUND((x))
#define CINT(x) (x)
#else
#define coord_t double
#define cabs fabs
#define CROUND(x) (x)
#define CINT(x) ROUND((x))
#endif

#define uchar unsigned char
#define uint unsigned int
#define ulong unsigned long

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define ROUND(x) (long)((x) + 0.5)
#define UROUND(x) (ulong)((x) + 0.5)
#define SIGN(x) ((x) > 0) - ((x) < 0)

#define TRUE 1
#define FALSE 0

#endif /* COMMON_H */
