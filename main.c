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
    Rectangle rect2 = { { 50, 50 }, { 150, 77 }, 0x0E, (uchar)NULL };
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

    /* render rectangles */
    draw_rectangle(&context, rect1);
    draw_rectangle(&context, rect2);
    update_buffer(&context);
    system("PAUSE");

    /* free resources */
    free_context(&context);

    /* return to the previous mode */
    set_bios_mode(initial_bios_mode);
    return 0;
}
