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
#include "matrix.h"

/* Input status port, to check rendering status. */
#define INPUT_STATUS 0x3DA

typedef enum Axis
{
    AXIS_X,
    AXIS_Y,
    AXIS_Z
} Axis;

/* Represents a set of coordinates in 2D space. */
typedef struct Coordinates
{
    coord_t x;
    coord_t y;
    coord_t z;
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
    uchar border_color;
    uchar fill_color;
    Matrix3x3 transformation;
} Polygon;

Polygon clone_polygon(Polygon polygon);
Coordinates get_polygon_centroid(Polygon *polygon);

int init_context(GraphicsContext *context);
void free_context(GraphicsContext *context);
void update_buffer(GraphicsContext *context);

Coordinates apply_transformation(Coordinates vertex, Coordinates origin, Matrix3x3 transformation);
void draw_point(GraphicsContext *context, Point point);
void draw_line(GraphicsContext *context, Line line);
void draw_rectangle(GraphicsContext *context, Rectangle rectangle);
void draw_polygon(GraphicsContext *context, Polygon polygon);

Coordinates scale_vertex(Coordinates vertex, Coordinates origin, double scale_x, double scale_y);
Line scale_line(Line line, double scale_x, double scale_y);
Rectangle scale_rectangle(Rectangle rectangle, double scale_x, double scale_y);
Polygon scale_polygon(Polygon polygon, double scale_x, double scale_y);

Coordinates rotate_vertex(Coordinates vertex, Coordinates origin, double angle);
Line rotate_line(Line line, double angle);
Polygon rotate_polygon(Polygon polygon, double angle, Axis axis);

Coordinates shear_vertex(Coordinates vertex, Coordinates origin, double shear_x, double shear_y);
Line shear_line(Line line, double shear_x, double shear_y);
Polygon shear_polygon(Polygon polygon, double shear_x, double shear_y);

#endif /* GRAPHICS_H */
