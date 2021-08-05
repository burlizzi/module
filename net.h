#define ETH_ALEN 6

#define MAX_BUFFER 1024


int sendpacket (const char* data, unsigned int count);
void net_shutdown(void);
int net_init(void);
