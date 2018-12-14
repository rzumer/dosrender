#ifndef COMMON_H
#define COMMON_H

#ifdef __WATCOMC__
#define farfree _ffree
#define farmalloc _fmalloc
#define inportb inp
#endif

#define uchar unsigned char
#define uint unsigned int

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

#define TRUE 1
#define FALSE 0

#endif /* COMMON_H */
