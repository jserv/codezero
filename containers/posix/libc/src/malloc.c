
/*
 * K&R Malloc 
 *
 * System specifc code should implement `more_core'
 */
#include "k_r_malloc.h"
#include <stddef.h> /* For NULL */
#include <stdlib.h>
#include <string.h> /* For memcpy */

#ifdef THREAD_SAFE
#include <mutex/mutex.h>
extern struct mutex malloc_mutex;
struct mutex malloc_mutex;

#define MALLOC_LOCK mutex_count_lock(&malloc_mutex)
#define MALLOC_UNLOCK mutex_count_unlock(&malloc_mutex)
#else
#define MALLOC_LOCK
#define MALLOC_UNLOCK
#endif

static Header   base;		/* empty list to get started */
Header  *_kr_malloc_freep = NULL;	/* start of free list */
#define freep _kr_malloc_freep

#ifdef CONFIG_MALLOC_INSTRUMENT
size_t __malloc_instrumented_allocated;
#endif

#ifdef CONFIG_MALLOC_DEBUG_INTERNAL
#include <stdio.h>
#include <assert.h>
int __malloc_check(void);
void __malloc_dump(void);
#endif

/*
 * malloc: general-purpose storage allocator 
 */
void *
malloc(size_t nbytes)
{
	Header *p, *prevp;
	unsigned nunits;
	nunits = (nbytes + sizeof(Header) - 1) / sizeof(Header) + 1;

	MALLOC_LOCK;
	if ((prevp = freep) == NULL) {	/* no free list yet */
		base.s.ptr = freep = prevp = &base;
		base.s.size = 0;
	}
	for (p = prevp->s.ptr;; prevp = p, p = p->s.ptr) {
		if (p->s.size >= nunits) {	/* big enough */
			if (p->s.size == nunits)	/* exactly */
				prevp->s.ptr = p->s.ptr;
			else {	/* allocate tail end */
				p->s.size -= nunits;
				p += p->s.size;
				p->s.size = nunits;
			}
			freep = prevp;
#ifdef CONFIG_MALLOC_DEBUG
			{
				/* Write bit pattern over data */
				char *x = (char *) (p + 1);
				int i;
				for (i = 0; i < nbytes; i++)
					x[i] = 0xd0;
			}
#endif

#ifdef CONFIG_MALLOC_INSTRUMENT
			__malloc_instrumented_allocated += nunits;
#endif
#ifdef CONFIG_MALLOC_DEBUG_INTERNAL
			if (__malloc_check() != 0) {
				printf("malloc %d %p\n", nbytes, (void *) (p + 1));
				__malloc_dump();
				assert(__malloc_check() == 0);
			}
#endif
			MALLOC_UNLOCK;
			return (void *) (p + 1);
		}
		if (p == freep) {	/* wrapped around free list */
			if ((p = (Header *) morecore(nunits)) == NULL) {
				MALLOC_UNLOCK;
				return NULL;	/* none left */
			} else {

			}
		}
	}
	MALLOC_UNLOCK;
}

/*
 * free: put block ap in free list 
 */
void
free(void *ap)
{
	Header *bp, *p;

	if (ap == NULL)
		return;

	MALLOC_LOCK;
	bp = (Header *) ap - 1;	/* point to block header */
	for (p = freep; !(bp > p && bp < p->s.ptr); p = p->s.ptr)
		if (p >= p->s.ptr && (bp > p || bp < p->s.ptr))
			break;	/* freed block at start or end of arena */

#ifdef CONFIG_MALLOC_INSTRUMENT
	__malloc_instrumented_allocated -= bp->s.size;
#endif

	if (bp + bp->s.size == p->s.ptr) {	/* join to upper nbr */
		bp->s.size += p->s.ptr->s.size;
		bp->s.ptr = p->s.ptr->s.ptr;
	} else {
		bp->s.ptr = p->s.ptr;
	}

	if (p + p->s.size == bp) {	/* join to lower nbr */
		p->s.size += bp->s.size;
		p->s.ptr = bp->s.ptr;
	} else {
		p->s.ptr = bp;
	}

	freep = p;

#ifdef CONFIG_MALLOC_DEBUG_INTERNAL
	if (__malloc_check() != 0) {
		printf("free %p\n", ap);
		__malloc_dump();
		assert(__malloc_check() == 0);
	}
#endif
	MALLOC_UNLOCK;
}

#ifdef CONFIG_MALLOC_DEBUG_INTERNAL

int
__malloc_check(void)
{
	Header *p, *prevp;
	if ((prevp = freep) == NULL) {	/* no free list yet */
		return 0;
	}
	for (p = prevp->s.ptr;; prevp = p, p = p->s.ptr) {
		if ((void*) p == NULL) {
			return 1;
		}
		/* Free bits should be in order */
		if (p > p->s.ptr && p != &base) {
			return 1;
		}
		if ((uintptr_t) p + (p->s.size * sizeof(Header)) > (uintptr_t) p->s.ptr && p != &base) {
			return 1;
		}
		/* shouldn't have zero sized free bits */
		if (p->s.size == 0 && p != &base) {
			return 1;
		}
		if (p == freep) {	/* wrapped around free list */
			break;
		}
	}
	return 0;
}

void
__malloc_dump(void)
{
	Header *p, *prevp;
	if ((prevp = freep) == NULL) {	/* no free list yet */
		return;
	}
	printf("Malloc dump\n");
	for (p = prevp->s.ptr;; prevp = p, p = p->s.ptr) {
		if (p > p->s.ptr && p != &base) {
			printf("* ");
		}
		if (p->s.size == 0 && p != &base) {
			printf("# ");
		}
		if ((uintptr_t) p + (p->s.size * sizeof(Header)) > (uintptr_t) p->s.ptr && p != &base) {
			printf("$ ");
		}
		if (p == &base) {
			printf(" p: <base>\n");
		} else {
			printf(" p: %p (%d) -> %p\n", p, p->s.size, p->s.ptr);
		}
		assert(p != NULL);
		if (p == freep) {	/* wrapped around free list */
			return;
		}
	}
}
#endif
