#include <arch/pl011_uart.h>

struct pl011_uart uart;

void platform_init(void);

void platform_init(void)
{
	pl011_initialise(&uart);
}

/* Initialises the uart class data structures, and the device. 
 * Terminal-like operation is assumed for default settings. 
 */
int pl011_initialise(struct pl011_uart * uart)
{
	
	uart->frame_errors   = 0;
	uart->parity_errors  = 0;
	uart->break_errors   = 0;
	uart->overrun_errors = 0;
	
	/* Initialise data register for 8 bit data read/writes */
	pl011_set_word_width(8);
	
	/* Fifos are disabled because by default it is assumed the port 
	 * will be used as a user terminal, and in that case the typed 
	 * characters will only show up when fifos are flushed, rather than
	 * when each character is typed. We avoid this by not using fifos.
	 */
	pl011_disable_fifos();	
	
	/* Set default baud rate of 38400 */
	pl011_set_baudrate(38400, 24000000);
	
	/* Set default settings of 1 stop bit, no parity, no hw flow ctrl */
	pl011_set_stopbits(1);
	pl011_parity_disable();

	/* Disable all irqs */
	pl011_set_irq_mask(0x3FF);
	
	/* Enable rx, tx, and uart chip */
	pl011_tx_enable();
	pl011_rx_enable();
	pl011_uart_enable();
	
	return 0;
}

