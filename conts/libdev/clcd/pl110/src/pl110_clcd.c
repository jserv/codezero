

#include <pl110_clcd.h>

#define		read(a)		*((volatile unsigned int *)(a))
#define		write(v, a)	(*((volatile unsigned int *)(a)) = v)
#define		setbit(bit, a)	write(read(a) | bit, a)
#define		clrbit(bit, a)  write(read(a) & ~bit, a)

/*
  * Default panel, we will use this for now
  * Seems like qemu has support for this
  */
static struct pl110_clcd_panel vga = {
	.mode = {
		.name		= "VGA",
		.refresh	= 60,
		.xres		= 640,
		.yres		= 480,
		.pixclock	= 39721,
		.left_margin	= 40,
		.right_margin	= 24,
		.upper_margin	= 32,
		.lower_margin	= 11,
		.hsync_len	= 96,
		.vsync_len	= 2,
		.sync		= 0,
		.vmode		= SCAN_VMODE_NONINTERLACED,
	},
	.width		= -1,
	.height		= -1,
	.tim2		= PL110_TIM2_BCD | PL110_TIM2_IPC,
	.cntl		= PL110_CNTL_LCDTFT | PL110_CNTL_LCDVCOMP(1),
	.bpp		= 16,
};

static void pl110_clcd_set_uppanel_fb(unsigned int clcd_base, 
										   unsigned int fb_base)
{
	write(fb_base, (clcd_base + PL110_CLCD_UPBASE));
}

#if 0
static void pl110_clcd_set_lwrpanel_fb(unsigned int clcd_base, unsigned int fb_base)
{
	write(fb_base,  (clcd_base +PL110_CLCD_LPBASE));
}

static unsigned int pl110_clcd_get_uppanel_fb(unsigned int clcd_base)
{
	return read((clcd_base +PL110_CLCD_UPBASE));
}

static unsigned int pl110_clcd_get_lwrpanel_fb(unsigned int clcd_base)
{
	return read((clcd_base +PL110_CLCD_LPBASE));
}
#endif

void pl110_initialise(struct pl110_clcd *clcd, char *buf)
{
	clcd->panel = &vga;
	clcd->frame_buffer = buf;

	pl110_clcd_set_uppanel_fb(clcd->virt_base, (unsigned int)(buf));

}

