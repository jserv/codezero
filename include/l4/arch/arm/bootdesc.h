#ifndef __BOOTDESC_H__
#define __BOOTDESC_H__

/* Supervisor task at load time. */
struct svc_image {
	char name[16];
	unsigned int phys_start;
	unsigned int phys_end;
} __attribute__((__packed__));

/* Supervisor task descriptor at load time */
struct bootdesc {
	int desc_size;
	int total_images;
	struct svc_image images[];
} __attribute__((__packed__));

#if defined (__KERNEL__)
extern struct bootdesc *bootdesc;

void read_bootdesc(void);
void copy_bootdesc(void);
#endif

#endif /* __BOOTDESC_H__ */
