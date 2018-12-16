#include "graphics.h"

int init_context(GraphicsContext *context)
{
    Coordinates screen_size = { 320, 200 }; /* Screen size in the target 13 hex BIOS mode */
    int buffer_size = screen_size.x * screen_size.y;

    context->screen_size = screen_size;
    context->off_screen = (uchar *)(farmalloc(buffer_size));

    if (context->off_screen)
    {
        context->screen = (uchar *)(MK_FP(0xA000, 0));
        _fmemset((void *)(context->off_screen), 0, buffer_size);
        return 1;
    }
    else
    {
        return 0;
    }
}

void free_context(GraphicsContext *context)
{
    /* free owned memory */
    farfree(context->off_screen);

    /* clear the screen content to avoid graphical bugs */
    _fmemset((void *)(context->screen), 0, context->screen_size.x * context->screen_size.y);
}

void update_buffer(GraphicsContext *context)
{
    /* wait a full vertical blank before copying */
    while (inportb(INPUT_STATUS) & 8);
    while (!(inportb(INPUT_STATUS) & 8));

    /* copy the off-screen buffer to the video memory */
    _fmemcpy((void *)(context->screen), (void *)(context->off_screen), context->screen_size.x * context->screen_size.y);
}

Polygon clone_polygon(Polygon polygon)
{
    Polygon cloned_polygon = polygon;
    size_t vertices_size = polygon.vertices_length * sizeof(*(polygon.vertices));
    cloned_polygon.vertices = malloc(vertices_size);
    memcpy(cloned_polygon.vertices, polygon.vertices, vertices_size);

    return cloned_polygon;
}

/* Draws a single point on the screen. */
void draw_point(GraphicsContext *context, Point point)
{
    uchar *buffer; /* points to the screen buffer */
    Coordinates p = point.coordinates;

    if (p.x < 0 || p.y < 0 || p.x >= context->screen_size.x || p.y >= context->screen_size.y)
    {
        return;
    }

    buffer = (uchar *)(context->off_screen + p.y * context->screen_size.x + p.y);
    *(buffer) = point.color;
}

/* Draws a straight line between two points, based on Bresenham's algorithm. */
void draw_line(GraphicsContext *context, Line line)
{
    uchar *buffer; /* points to the screen buffer */
    Coordinates delta; /* delta between the points */
    float delta_error; /* error delta per step */
    float error = 0.0f; /* current error */
    int x = line.a.x; /* horizontal index */
    int y = line.a.y; /* vertical index */
    int vertical; /* iterate over y instead of x, for lines with large slopes */
    int converged; /* used to determine when to break out of the draw loop */

    delta.x = line.b.x - line.a.x;
    delta.y = line.b.y - line.a.y;

    if ((line.a.x >= context->screen_size.x && line.b.x >= context->screen_size.x) ||
        (line.a.y >= context->screen_size.y && line.b.y >= context->screen_size.y))
    {
        return;
    }

    vertical = abs(delta.y) > abs(delta.x);
    delta_error = vertical ? fabs(delta.x / (float)delta.y) : fabs(delta.y / (float)delta.x);

    /* invert the line coordinates if needed */
    if (vertical ? line.b.y < line.a.y : line.b.x < line.a.x)
    {
        Line inverted_line;
        inverted_line.a = line.b;
        inverted_line.b = line.a;
        inverted_line.color = line.color;

        draw_line(context, inverted_line);
        return;
    }

    /* point the buffer to the video memory */
    buffer = (uchar *)(context->off_screen);

    do
    {
        converged = x >= line.b.x && y >= line.b.y;

        if (x >= 0 && y >= 0 && x < context->screen_size.x && y < context->screen_size.y)
        {
            *(buffer + y * context->screen_size.x + x) = line.color;
        }

        error += delta_error;

        if (error >= 0.5f)
        {
            if (vertical)
            {
                x += SIGN(delta.x);
            }
            else
            {
                y += SIGN(delta.y);
            }

            error -= 1.0f;
        }

        vertical ? y++ : x++;
    } while (!converged);
}

/* Draws a rectangle on the screen with arbitrary border and fill colors (0 is transparent). */
void draw_rectangle(GraphicsContext *context, Rectangle rectangle)
{
    Coordinates overflow, underflow; /* used to avoid drawing outside of the screen */
    uchar *buffer; /* points to the screen buffer */
    uchar line_color; /* holds the color (border or fill) used when drawing a horizontal line */
    int y; /* scanline index for the draw loop */
    const int border_size = 1;

    underflow.x = MAX(rectangle.offset.x * -1, 0);
    underflow.y = MAX(rectangle.offset.y * -1, 0);
    overflow.x = MAX((rectangle.offset.x + rectangle.dimensions.x - context->screen_size.x), 0);
    overflow.y = MAX((rectangle.offset.y + rectangle.dimensions.y - context->screen_size.y), 0);

    if (rectangle.offset.x >= context->screen_size.x ||
        rectangle.offset.y >= context->screen_size.y ||
        rectangle.dimensions.x <= 0 ||
        rectangle.dimensions.y <= 0)
    {
        return;
    }

    /* offset the pointer to the video memory */
    buffer = (uchar *)(context->off_screen + rectangle.offset.y * context->screen_size.x + rectangle.offset.x + underflow.x);

    for (y = -1 * MIN(rectangle.offset.y, 0); y < rectangle.dimensions.y - overflow.y; y++)
    {
        /* draw a full scanline of either the border or the fill color, depending on the current line */
        line_color =
            y == 0 || y == (rectangle.dimensions.y - border_size) ?
            rectangle.border_color : rectangle.fill_color;

        if (line_color)
        {
            /* draw a full horizontal line */
            _fmemset(
                (void *)(buffer + y * context->screen_size.x),
                line_color,
                rectangle.dimensions.x - overflow.x - underflow.x - border_size);
        }

        /* draw the border */
        if (rectangle.border_color)
        {
            /* draw vertical borders (two pixels per scanline) only */
            if (rectangle.offset.x >= 0 && rectangle.offset.x < context->screen_size.x)
            {
                *(buffer + y * context->screen_size.x) = rectangle.border_color;
            }

            if (rectangle.offset.x + rectangle.dimensions.x >= 0 &&
                rectangle.offset.x + rectangle.dimensions.x < context->screen_size.x)
            {
                *(buffer + y * context->screen_size.x +
                    rectangle.dimensions.x - overflow.x - underflow.x - border_size) =
                    rectangle.border_color;
            }
        }
    }
}

/* Draws an arbitrary polygon, with a given border color. */
void draw_polygon(GraphicsContext *context, Polygon polygon)
{
    Line line; /* holds parameters used to draw each line of the polygon */
    int v; /* index iterating over vertices */

    if (polygon.vertices_length < 3)
    {
        /* not a polygon */
        return;
    }

    for (v = 0; v < polygon.vertices_length; v++)
    {
        line.a = polygon.vertices[v];
        line.b = v == polygon.vertices_length - 1 ? polygon.vertices[0] : polygon.vertices[v + 1];
        line.color = polygon.color;

        draw_line(context, line);
    }
}

/* Scales a vertex around an origin point. */
Coordinates scale_vertex(Coordinates vertex, Coordinates origin, float scale_x, float scale_y)
{
    Coordinates relative_vertex, scaled_vertex;

    relative_vertex.x = vertex.x - origin.x;
    relative_vertex.y = vertex.y - origin.y;

    scaled_vertex.x = (int)(relative_vertex.x * scale_x + 0.5) + origin.x;
    scaled_vertex.y = (int)(relative_vertex.y * scale_y + 0.5) + origin.y;

    return scaled_vertex;
}

/* Scales a line around its origin. Negative scale factors allow mirroring. */
Line scale_line(Line line, float scale_x, float scale_y)
{
    Line scaled_line = line;
    scaled_line.b = scale_vertex(line.b, line.a, scale_x, scale_y);

    return scaled_line;
}

/* Scales a rectangle around its origin. Negative scale factors allow mirroring. */
Rectangle scale_rectangle(Rectangle rectangle, float scale_x, float scale_y)
{
    Rectangle scaled_rectangle;
    int offset; /* holds the offset when swapping values due to mirroring */

    scaled_rectangle.offset = rectangle.offset;
    scaled_rectangle.dimensions.x = rectangle.dimensions.x * scale_x;
    scaled_rectangle.dimensions.y = rectangle.dimensions.y * scale_y;
    scaled_rectangle.border_color = rectangle.border_color;
    scaled_rectangle.fill_color = rectangle.fill_color;

    /* handle mirroring */
    if (scale_x < 0)
    {
        offset = scaled_rectangle.offset.x;
        scaled_rectangle.offset.x += scaled_rectangle.dimensions.x;
        scaled_rectangle.dimensions.x = offset - scaled_rectangle.offset.x;
    }

    if (scale_y < 0)
    {
        offset = scaled_rectangle.offset.y;
        scaled_rectangle.offset.y += scaled_rectangle.dimensions.y;
        scaled_rectangle.dimensions.y = offset - scaled_rectangle.offset.y;
    }

    return scaled_rectangle;
}

/* Scales a polygon around its origin. Negative scale factors allow mirroring. */
Polygon scale_polygon(Polygon polygon, float scale_x, float scale_y)
{
    Coordinates origin = polygon.vertices[0];
    Polygon scaled_polygon = clone_polygon(polygon);
    int v; /* vertex index */

    scaled_polygon.vertices[0] = origin;

    for (v = 1; v < polygon.vertices_length; v++)
    {
        scaled_polygon.vertices[v] = scale_vertex(polygon.vertices[v], origin, scale_x, scale_y);
    }

    return scaled_polygon;
}

/* Rotates a vertex in the 2D plane around an origin point. */
Coordinates rotate_vertex(Coordinates vertex, Coordinates origin, float angle)
{
    Coordinates relative_vertex, rotated_vertex;
    float radians = angle * M_PI / 180.0f;

    relative_vertex.x = vertex.x - origin.x;
    relative_vertex.y = vertex.y - origin.y;

    rotated_vertex.x = (int)(relative_vertex.x * cos(radians) - relative_vertex.y * sin(radians) + 0.5) + origin.x;
    rotated_vertex.y = (int)(relative_vertex.y * cos(radians) + relative_vertex.x * sin(radians) + 0.5) + origin.y;

    return rotated_vertex;
}

/* Rotates a line around its origin. */
Line rotate_line(Line line, float angle)
{
    Line rotated_line = line;
    rotated_line.b = rotate_vertex(line.b, line.a, angle);

    return rotated_line;
}

/* Rotates a polygon around its origin. */
Polygon rotate_polygon(Polygon polygon, float angle)
{
    Coordinates origin = polygon.vertices[0];
    Polygon rotated_polygon = clone_polygon(polygon);
    int v; /* vertex index */

    rotated_polygon.vertices[0] = origin;

    for (v = 1; v < polygon.vertices_length; v++)
    {
        rotated_polygon.vertices[v] = rotate_vertex(polygon.vertices[v], origin, angle);
    }

    return rotated_polygon;
}

/* Shears a vertex around an origin point. */
Coordinates shear_vertex(Coordinates vertex, Coordinates origin, float shear_x, float shear_y)
{
    Coordinates relative_vertex, shorn_vertex;

    relative_vertex.x = vertex.x - origin.x;
    relative_vertex.y = vertex.y - origin.y;

    shorn_vertex.x = (int)(relative_vertex.x + shear_x * relative_vertex.y + 0.5) + origin.x;
    shorn_vertex.y = (int)(relative_vertex.y + shear_y * relative_vertex.x + 0.5) + origin.y;

    return shorn_vertex;
}

/* Shears a line around its origin. */
Line shear_line(Line line, float shear_x, float shear_y)
{
    Line shorn_line = line;
    shorn_line.b = shear_vertex(line.b, line.a, shear_x, shear_y);

    return shorn_line;
}

/* Shears a polygon around its origin. */
Polygon shear_polygon(Polygon polygon, float shear_x, float shear_y)
{
    Coordinates origin = polygon.vertices[0];
    Polygon shorn_polygon = clone_polygon(polygon);
    int v; /* vertex index */

    shorn_polygon.vertices[0] = origin;

    for (v = 1; v < polygon.vertices_length; v++)
    {
        shorn_polygon.vertices[v] = shear_vertex(polygon.vertices[v], origin, shear_x, shear_y);
    }

    return shorn_polygon;
}
