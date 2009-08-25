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

struct initdata;
void read_bootdesc(struct initdata *initdata);

#endif /* __BOOTDESC_H__ */
