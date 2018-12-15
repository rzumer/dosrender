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

    underflow.x = MAX(rectangle.offset.x * -1, 0);
    underflow.y = MAX(rectangle.offset.y * -1, 0);
    overflow.x = MAX((rectangle.offset.x + rectangle.dimensions.x - context->screen_size.x), 0);
    overflow.y = MAX((rectangle.offset.y + rectangle.dimensions.y - context->screen_size.y), 0);

    if (rectangle.offset.x >= context->screen_size.x || rectangle.offset.y >= context->screen_size.y)
    {
        return;
    }

    /* offset the pointer to the video memory */
    buffer = (uchar *)(context->off_screen + rectangle.offset.y * context->screen_size.x + rectangle.offset.x + underflow.x);

    for (y = -1 * MIN(rectangle.offset.y, 0); y < rectangle.dimensions.y - overflow.y; y++)
    {
        /* draw a full scanline of either the border or the fill color, depending on the current line */
        line_color =
            y == 0 || y == (rectangle.dimensions.y) ?
            rectangle.border_color : rectangle.fill_color;

        if (line_color)
        {
            /* draw a full horizontal line */
            _fmemset(
                (void *)(buffer + y * context->screen_size.x),
                line_color,
                rectangle.dimensions.x - overflow.x - underflow.x);
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
                *(buffer + y * context->screen_size.x + rectangle.dimensions.x - overflow.x - underflow.x) = rectangle.border_color;
            }
        }
    }
}

/* Draws an arbitrary polygon, with a given border color. */
void draw_polygon(GraphicsContext *context, Polygon polygon)
{
    Coordinates a, b; /* holds coordinates used to draw each line of the polygon */
    Line line; /* holds parameters used to draw each line of the polygon */
    int i; /* index iterating over vertices */

    if (polygon.vertices_length <= 0)
    {
        return;
    }

    for (i = 0; i < polygon.vertices_length; i++)
    {
        a = polygon.vertices[i];
        b = i == polygon.vertices_length - 1 ? polygon.vertices[0] : polygon.vertices[i + 1];
        line.a = a;
        line.b = b;
        line.color = polygon.color;

        draw_line(context, line);
    }
}
