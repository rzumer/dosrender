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

    if (rectangle.offset.x > context->screen_size.x || rectangle.offset.y > context->screen_size.y)
    {
        return;
    }

    /* offset the pointer to the video memory */
    buffer = (uchar *)(context->off_screen + rectangle.offset.y * context->screen_size.x + rectangle.offset.x + underflow.x);

    for (y = -1 * MIN(rectangle.offset.y, 0); y < rectangle.dimensions.y - overflow.y; y++)
    {
        /* draw a full scanline of either the border or the fill color, depending on the current line */
        line_color =
            y == 0 || y == (rectangle.dimensions.y - 1) ?
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