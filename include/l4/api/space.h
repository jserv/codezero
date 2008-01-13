#ifndef __API_SPACE_H__
#define __API_SPACE_H__


enum space_control_opcode {
	SPCCTRL_SHM = 0
};

#if 0
struct shm_kdata {
	l4id_t creator;
	unsigned long npages;
	unsigned long server_pfn;
	unsigned long client_pfn;
};
#endif

#endif /* __API_SPACE_H__ */
