
#ifndef __TESTS_H__
#define __TESTS_H__

//#define DEBUG

#ifdef DEBUG
#define dgb_printf printf
#else
#define dbg_printf(fmt,...)
#endif

#endif
