#include <dos.h>

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
	int bios_mode = get_bios_mode();

	/* enter mode 13h */
	set_bios_mode(0x13);
	system("PAUSE");

	/* return to the previous mode */
	set_bios_mode(bios_mode);
	system("PAUSE");

	return 0;
}
