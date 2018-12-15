#include <stdio.h>
#include <stdlib.h>
#include "graphics.h"

int get_bios_mode(void)
{
    union REGS registers;

    registers.h.ah = 0xf;
    int86(0x10, &registers, &registers);
    return registers.h.al;
}

void set_bios_mode(int mode)
{
    union REGS registers;

    registers.h.ah = 0x0;
    registers.h.al = mode;
    int86(0x10, &registers, &registers);
}

int main(void) {
    GraphicsContext context = { { 0, 0 }, NULL, NULL };
    Rectangle rect1 = { { 60, 60 }, { 200, 100 }, 0x20, 0x36 };
    Coordinates rect1_coords[4] = { { 60, 60 }, { 260, 60 }, { 260, 160 }, { 60, 160 } };
    Coordinates triangle_coords[3] = { { 70, 50 }, { 10, 120 }, { 150, 120 } };
    Polygon rect1_polygon = { NULL, 4, 9 };
    Polygon triangle = { NULL, 3, 0x28 };
    Rectangle rect2 = { { -50, -50 }, { 150, 97 }, 0x0E, (uchar)NULL };
    Line line = { { -30, -10 }, { 100, 120 }, 2 };
    Point point = { { 100, 100 }, 10 };
    int initial_bios_mode = get_bios_mode();

    rect1_polygon.vertices = &rect1_coords;
    triangle.vertices = &triangle_coords;

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
    rect1_polygon = scale_polygon(rect1_polygon, 1.24f, 1.0f);
    line = scale_line(line, 2.0f, 1.0f);
    rect1 = scale_rectangle(rect1, -1.0f, -1.0f);

    /* render graphics */
    draw_rectangle(&context, rect1);
    draw_polygon(&context, rect1_polygon );
    draw_line(&context, line);
    draw_polygon(&context, triangle);
    draw_point(&context, point);
    draw_rectangle(&context, rect2);
    update_buffer(&context);
    system("PAUSE");

    /* free resources */
    free_context(&context);

    /* return to the previous mode */
    set_bios_mode(initial_bios_mode);
    return 0;
}
