#ifndef GRAPHICS_H
#define GRAPHICS_H

#ifdef __WATCOMC__
#include <malloc.h>
#else
#include <alloc.h>
#endif

#include <conio.h>
#include <dos.h>
#include <mem.h>
#include "common.h"

/* Input status port, to check rendering status. */
#define INPUT_STATUS 0x3DA

/* Represents a set of coordinates in 2D space. */
typedef struct Coordinates
{
    int x;
    int y;
} Coordinates;

typedef struct GraphicsContext
{
    Coordinates screen_size;
    uchar far *screen;
    uchar far *off_screen;
} GraphicsContext;

typedef struct Rectangle
{
    Coordinates offset;
    Coordinates dimensions;
    uchar border_color;
    uchar fill_color;
} Rectangle;

int init_context(GraphicsContext *context);
void free_context(GraphicsContext *context);
void update_buffer(GraphicsContext *context);
void draw_rectangle(GraphicsContext *context, Rectangle rectangle);

#endif /* GRAPHICS_H */
