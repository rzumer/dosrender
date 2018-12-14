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

/* Draws a rectangle on the screen. */
void draw_rectangle(GraphicsContext *context, Coordinates offset, Coordinates dimensions, uchar color, int filled)
{
    Coordinates overflow;
    uchar *buffer;
    int y;
        
    overflow.x = MAX((offset.x + dimensions.x - context->screen_size.x), 0);
    overflow.y = MAX((offset.y + dimensions.y - context->screen_size.y), 0);

    /* offset the pointer to the video memory */
    buffer = (uchar *)(context->off_screen + offset.y * context->screen_size.x + offset.x);

    for (y = 0; y < dimensions.y - overflow.y; y++)
    {
        if (filled || y == 0 || y == (dimensions.y - overflow.y - 1))
        {
            /* draw a full horizontal line */
            _fmemset((void *)(buffer + y * context->screen_size.x), color, dimensions.x - overflow.x);
        }
        else
        {
            /* draw vertical borders only */
            *(buffer + y * context->screen_size.x) = color;
            *(buffer + y * context->screen_size.x + dimensions.x - overflow.x) = color;
        }
    }
}
