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
    Coordinates offset = { 60, 60 };
    Coordinates dimensions = { 200, 100 };
    uchar border_color = 0x20;
    uchar fill_color = 0x36;
    int initial_bios_mode = get_bios_mode();

    /* enter BIOS mode 13 hex */
    set_bios_mode(0x13);

    /* initialize the graphics context */
    if (!(init_context(&context)))
    {
        set_bios_mode(initial_bios_mode);
        printf("Could not initialize off-screen buffer.\n");
        return 1;
    }

    /* render a rectangle */
    draw_rectangle(&context, offset, dimensions, border_color, fill_color);
    update_buffer(&context);
    system("PAUSE");

    /* free resources */
    free_context(&context);

    /* return to the previous mode */
    set_bios_mode(initial_bios_mode);
    return 0;
}
