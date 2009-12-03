
#ifndef __PL110_CLCD_H__
#define __PL110_CLCD_H__

/* Register offsets */
#define PL110_CLCD_TIMING0	0x000 /* Horizontal Axis Panel Control*/
#define PL110_CLCD_TIMING1	0x004 /* Vertical Axis Panel Control */
#define PL110_CLCD_TIMING2	0x008 /* Clock and Polarity Signal Control*/
#define PL110_CLCD_TIMING3	0x00c /* Line End Control */
#define PL110_CLCD_UPBASE	0x010 /* Upper Panel Frame Base Address*/
#define PL110_CLCD_LPBASE	0x014 /* Lower Panel Frame Base Address */
#define PL110_CLCD_IMSC		0x018 /* Interrupt Mast Set Clear */
#define PL110_CLCD_CONTROL	0x01c /* CLCD Control */
#define PL110_CLCD_RIS		0x020 /* Raw Interrupt Status */
#define PL110_CLCD_MIS		0x024 /* Masked Interrupt Status */
#define PL110_CLCD_ICR		0x028 /* Interrupt Clear */
#define PL110_CLCD_UPCURR	0x02c /* Upper Panel Current Address Value */
#define PL110_CLCD_LPCURR	0x030 /* Lower Panel Current Address Value */
//#define PL110_LCD_PALETTE
#define PL110_CLCD_PERIPHID0	0xfe0 /* Peripheral Identification */
#define PL110_CLCD_PERIPHID1	0xfe4 /* Peripheral Identification */
#define PL110_CLCD_PERIPHID2	0xfe8 /* Peripheral Identification */
#define PL110_CLCD_PERIPHID3	0xfec /* Peripheral Identification */
#define PL110_CLCD_PCELLID0	0xff0 /* Peripheral Identification */
#define PL110_CLCD_PCELLID1	0xff4 /* PrimeCell Identification */
#define PL110_CLCD_PCELLID2	0xff8 /* PrimeCell Identification */
#define PL110_CLCD_PCELLID3	0xffc /* PrimeCell Identification */

/* Scan mode */
#define SCAN_VMODE_NONINTERLACED  0	/* non interlaced */
#define SCAN_VMODE_INTERLACED	1	/* interlaced	*/
#define SCAN_VMODE_DOUBLE		2	/* double scan */
#define SCAN_VMODE_ODD_FLD_FIRST	4	/* interlaced: top line first */
#define SCAN_VMODE_MASK		255

/* Control Register Bits */
#define PL110_CNTL_LCDEN		(1 << 0)
#define PL110_CNTL_LCDBPP1		(0 << 1)
#define PL110_CNTL_LCDBPP2		(1 << 1)
#define PL110_CNTL_LCDBPP4		(2 << 1)
#define PL110_CNTL_LCDBPP8		(3 << 1)
#define PL110_CNTL_LCDBPP16		(4 << 1)
#define PL110_CNTL_LCDBPP16_565	(6 << 1)
#define PL110_CNTL_LCDBPP24		(5 << 1)
#define PL110_CNTL_LCDBW		(1 << 4)
#define PL110_CNTL_LCDTFT		(1 << 5)
#define PL110_CNTL_LCDMONO8		(1 << 6)
#define PL110_CNTL_LCDDUAL		(1 << 7)
#define PL110_CNTL_BGR		(1 << 8)
#define PL110_CNTL_BEBO		(1 << 9)
#define PL110_CNTL_BEPO		(1 << 10)
#define PL110_CNTL_LCDPWR		(1 << 11)
#define PL110_CNTL_LCDVCOMP(x)	((x) << 12)
#define PL110_CNTL_LDMAFIFOTIME	(1 << 15)
#define PL110_CNTL_WATERMARK		(1 << 16)

#define PL110_TIM2_CLKSEL		(1 << 5)
#define PL110_TIM2_IVS		(1 << 11)
#define PL110_TIM2_IHS		(1 << 12)
#define PL110_TIM2_IPC		(1 << 13)
#define PL110_TIM2_IOE		(1 << 14)
#define PL110_TIM2_BCD		(1 << 26)



struct videomode {
	const char *name;	/* optional */
	unsigned int refresh;		/* optional */
	unsigned int xres;
	unsigned int yres;
	unsigned int pixclock;
	unsigned int left_margin;
	unsigned int right_margin;
	unsigned int upper_margin;
	unsigned int lower_margin;
	unsigned int hsync_len;
	unsigned int vsync_len;
	unsigned int sync;
	unsigned int vmode;
	unsigned int flag;
};

struct pl110_clcd_panel {
	struct videomode	mode;
	signed short		width;	/* width in mm */
	signed short		height;	/* height in mm */
	unsigned int			tim2;
	unsigned int			tim3;
	unsigned int			cntl;
	unsigned int		bpp:8,
				fixedtimings:1,
				grayscale:1;
	unsigned int		connector;
};

struct pl110_clcd {
	unsigned int virt_base;
	struct pl110_clcd_panel *panel;	
	char *frame_buffer;
};

void pl110_initialise(struct pl110_clcd *clcd, char *buf);

#endif /* __PL110_CLCD_H__ */
