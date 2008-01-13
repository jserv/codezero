/* Automatically generated, don't edit */
/* Generated on: bahadir-desktop */
/* At: Sun, 13 Jan 2008 12:03:34 +0000 */
/* Linux version 2.6.22-14-generic (buildd@terranova) (gcc version 4.1.3 20070929 (prerelease) (Ubuntu 4.1.2-16ubuntu2)) #1 SMP Tue Dec 18 08:02:57 UTC 2007 */

/* L4 Project Kernel Configurator */

/* Main architecture */
#undef  ARCH_TEST
#define ARCH_ARM 1


/* ARM Architecture Configuration */

/* ARM Processor Type */
#undef  ARM_CPU_ARM920T
#define ARM_CPU_ARM926 1
#undef  ARM_CPU_ARM925


/* ARM Architecture Family */
#define ARM_SUBARCH_V5 1
#undef  ARM_SUBARCH_V6


/* ARM Platform Type */
#undef  ARM_PLATFORM_EB
#undef  ARM_PLATFORM_AB926
#define ARM_PLATFORM_PB926 1


/* Platform Drivers */
#define DRIVER_UART_PL011 1
#define DRIVER_TIMER_SP804 1
#define DRIVER_IRQCTRL_PL190 1


#undef  DUMMY

/* That's all, folks! */
/* Symbols derived from this file by SConstruct\derive_symbols() */
#define __ARCH__		arm
#define __PLATFORM__		pb926
#define __SUBARCH__		v5

