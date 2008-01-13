/*
 * Lightweight and simple RPC primitives.
 *
 * Copyright (c) 2007 Bahadir Balban
 *
 * Some methodology is needed for efficient and functional message passing
 * among processes, particulary when shared memory or same address space
 * messaging is available. The rpc primitives here attempt to fill this gap.
 *
 * The idea is to generate as little bloat as possible. To that end we don't
 * need encryption, marshalling, type tracking and discovery. Also Very minor
 * boilerplated code is produced from C macros rather than a notorious rpc tool.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* What default value arg0 is given for rpc calls */
#define ARG0				8
#define PAGE_SIZE			0x1000
char pagebuf[PAGE_SIZE];

struct ipc_shmem {
	unsigned long vaddr;
};

struct tlmsg {
	struct ipc_shmem shmem;
	unsigned long arg[3];
	unsigned long ret;
	unsigned long method_num;
};

typedef int (*remote_call_t)(struct tlmsg *m);

int square(int i)
{
	printf("%s: Entering...\n", __FUNCTION__);
	return i*i;
}

/* Hand coded wrapper for non-complex argument types. */
int rpc_square(struct tlmsg *m)
{
	printf("%s: Entering...\n", __FUNCTION__);
	return square(m->arg[0]);
}

struct complex_struct {
	int item;
	int item2;
};

/* Use this to declare an RPC wrapper for your function. Your function
 * must return a primitive type (e.g, int, float, long etc.) and can
 * have a ref to a single argument of complex type (e.g. a struct). Apart
 * from these limitations it is essentially a regular C function. Note
 * that one can pass a lot of data back and forth using a single struct.
 * The wrappers are very lightweight, and thanks to many possibilities of
 * shared memory (e.g. same address-space, shared pages) data need not be
 * copied when passed back and forth. */
#define DECLARE_RPC_BYREF(ret, func, type0)				\
static inline ret rpc_byref_##func(struct tlmsg *m)			\
{	/* Find struct address */					\
	unsigned long multiword_struct = m->arg[0] +			\
	m->shmem.vaddr; /* Data passed by a shared entity */		\
	return func((type0 *)multiword_struct); /* Call real function */\
}

/* Same as above, but passing a structure by value instead of reference.
 * This is much slower due to lots of copying involved. It is not
 * recommended but included for completion. */
#define DECLARE_RPC_BYVAL(ret, func, type0)				\
static inline ret rpc_byval_##func(struct tlmsg *m)			\
{	/* Find struct address */					\
	unsigned long multiword_struct = m->arg[0] +			\
       	m->shmem.vaddr; /* Data passed by a shared entity */		\
	/* Call real function, by value */				\
	return func((type0)(*(type0 *)multiword_struct)); 		\
}
/* Use these directly to declare the function, *and* its RPC wrapper. */
#define RPC_FUNC_BYREF(ret, func, type0)				\
ret func(type0 *);							\
DECLARE_RPC_BYREF(ret, func, type0)					\
ret func(type0 *arg0)

#define RPC_FUNC_BYVAL(ret, func, type0)				\
ret func(type0);							\
DECLARE_RPC_BYVAL(ret, func, type0)					\
ret func(type0 arg0)

RPC_FUNC_BYVAL(int, complex_byval, struct complex_struct)
{
	printf("%s: Entering...\n", __FUNCTION__);
	arg0.item++;
	return 0;
}

RPC_FUNC_BYREF(int, complex_byref, struct complex_struct)
{
	printf("%s: Entering...\n", __FUNCTION__);
	arg0->item++;
	return 0;
}

#define RPC_NAME(func, by)		rpc_##by##_##func
remote_call_t remote_call_array[] = {
	rpc_square,
	RPC_NAME(complex_byval, byval),
	RPC_NAME(complex_byref, byref),
};

struct tlmsg *getmsg(int x)
{
	struct tlmsg *m = (struct tlmsg *)malloc(sizeof(struct tlmsg));
	m->method_num = x;
	m->arg[0] = ARG0;
	m->shmem.vaddr = (unsigned long)&pagebuf;
	return m;
}

void putmsg(struct tlmsg *m)
{
	free((void *)m);
}

void check_rpc_success()
{
	struct complex_struct *cs = (struct complex_struct *)(pagebuf + ARG0);

	printf("complex struct at offset 0x%x. cs->item: %d, expected: 1.\n", ARG0, cs->item);
}

int main(void)
{
	struct tlmsg *m[3];
	unsigned int ret;
	remote_call_t call[3];
	memset((void *)pagebuf, 0, PAGE_SIZE);
	for (int i = 0; i < 3; i++) {
		m[i] = getmsg(i);
		printf("Calling remote function %d according to incoming msg.\n", i);
		call[i] = remote_call_array[m[i]->method_num];
		ret = call[i](m[i]); /* i.e. call rpc function number i, with
				      * message number i as argument */
		printf("Call returned %d\n", ret);
		m[i]->ret = ret;
		putmsg(m[i]);
	}
	check_rpc_success();
}



