#ifndef GRAPHICS_H
#define GRAPHICS_H

#ifdef __WATCOMC__
#include <malloc.h>
#else
#include <alloc.h>
#endif

#include <conio.h>
#include <dos.h>
#include <math.h>
#include <mem.h>
#include <stdlib.h>
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

typedef struct Point
{
    Coordinates coordinates;
    uchar color;
} Point;

typedef struct Line
{
    Coordinates a;
    Coordinates b;
    uchar color;
} Line;

typedef struct Rectangle
{
    Coordinates offset;
    Coordinates dimensions;
    uchar border_color;
    uchar fill_color;
} Rectangle;

typedef struct Polygon
{
    Coordinates *vertices;
    int vertices_length;
    uchar color;
} Polygon;

Polygon clone_polygon(Polygon polygon);

int init_context(GraphicsContext *context);
void free_context(GraphicsContext *context);
void update_buffer(GraphicsContext *context);
void draw_point(GraphicsContext *context, Point point);
void draw_line(GraphicsContext *context, Line line);
void draw_rectangle(GraphicsContext *context, Rectangle rectangle);
void draw_polygon(GraphicsContext *context, Polygon polygon);

Coordinates scale_vertex(Coordinates vertex, Coordinates origin, float scale_x, float scale_y);
Line scale_line(Line line, float scale_x, float scale_y);
Rectangle scale_rectangle(Rectangle rectangle, float scale_x, float scale_y);
Polygon scale_polygon(Polygon polygon, float scale_x, float scale_y);
Polygon rotate_polygon(Polygon polygon, float angle);

#endif /* GRAPHICS_H */
