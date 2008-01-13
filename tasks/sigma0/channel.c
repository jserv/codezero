
struct channel {
	int dir;	/* Direction */
	char *name;	/* Name */
	int cd;		/* Channel descriptor */
};


struct interface {
	struct channel chan[];
};


int main(int argc, char *argv[])
{
	void *buf = malloc(sizeof(struct channel)*10);
	struct interface *intf = buf;
}
