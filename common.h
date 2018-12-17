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
