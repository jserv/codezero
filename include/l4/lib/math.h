#ifndef __LIB_MATH_H__
#define __LIB_MATH_H__

#define min(x, y)		(((x) < (y)) ? (x) : (y))
#define max(x, y)		(((x) > (y)) ? (x) : (y))

/* Tests if ranges a-b intersect with range c-d */
static inline int set_intersection(unsigned long a, unsigned long b,
				   unsigned long c, unsigned long d)
{
	/*
	 * Below is the complement (') of the intersection
	 * of 2 ranges, much simpler ;-)
	 */
	if (b <= c || a >= d)
		return 0;

	/* The rest is always intersecting */
	return 1;
}

#endif /* __LIB_MATH_H__ */
