#include <stdio.h>
#include <stdlib.h>
#include "graphics.h"

int get_bios_mode(void)
{
    union REGS in, out;

    in.h.ah = 0xf;
    int86(0x10, &in, &out);
    return out.h.al;
}

void set_bios_mode(int mode)
{
    union REGS in, out;

    in.h.ah = 0x0;
    in.h.al = mode;
    int86(0x10, &in, &out);
}

int main(void) {
    GraphicsContext context = { { 0, 0 }, NULL, NULL };
    Coordinates rect1_coords[4] = { { 10, 50 }, { 140, 90 }, { 140, 110 }, { 10, 150 } };
    Coordinates rect2_coords[4] = { { 310, 50 }, { 180, 90 }, { 180, 110 }, { 310, 150 } };
    Coordinates triangle_coords[3] = { { 160, 100 }, { 100, 170 }, { 220, 170 } };
    Polygon rect1_polygon = { NULL, 4, 0x33, 0x33, MATRIX_3X3_IDENTITY };
    Polygon rect2_polygon = { NULL, 4, 0x33, 0x33, MATRIX_3X3_IDENTITY };
    Polygon triangle_polygon = { NULL, 3, 0x28, 14, MATRIX_3X3_IDENTITY };
    int r;

    int initial_bios_mode = get_bios_mode();

    rect1_polygon.vertices = &rect1_coords;
    rect2_polygon.vertices = &rect2_coords;
    triangle_polygon.vertices = &triangle_coords;

    /* enter BIOS mode 13 hex */
    set_bios_mode(0x13);

    /* initialize the graphics context */
    if (!(init_context(&context)))
    {
        set_bios_mode(initial_bios_mode);
        printf("Could not initialize off-screen buffer.\n");
        return 1;
    }

    /* transform shapes */
    triangle_polygon = scale_polygon(triangle_polygon, 0.5, 0.5);
    triangle_polygon = rotate_polygon(triangle_polygon, 75.0, AXIS_X);

    /* render graphics */
    draw_polygon(&context, rect1_polygon);
    draw_polygon(&context, rect2_polygon);
    draw_polygon(&context, clone_polygon(triangle_polygon));

    update_buffer(&context);

    for (r = 0; r < 2; r++)
    {
        triangle_polygon.border_color = 255;
        triangle_polygon.fill_color = 255;
        draw_polygon(&context, clone_polygon(triangle_polygon));
        sleep(1);
        update_buffer(&context);
        triangle_polygon.border_color = 0x28;
        triangle_polygon.fill_color = 14;
        //triangle_polygon = rotate_polygon(triangle_polygon, 30.0, AXIS_Z);
        draw_polygon(&context, clone_polygon(triangle_polygon));
        sleep(1);
        update_buffer(&context);
    }

    system("PAUSE");

    /* free resources */
    free_context(&context);

    /* return to the previous mode */
    set_bios_mode(initial_bios_mode);
    return 0;
}
