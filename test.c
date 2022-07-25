#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h> // close function

#include <limits.h>

#define S(x) x,sizeof(x)

#define PAGE_SIZE     4096
#define PAGE_SHIFT    12


int main (int argc, char **argv)
{

    size_t i;
    
    //asm volatile("int3;");
    //raise(SIGTRAP);
 char hostname[HOST_NAME_MAX + 1];
  gethostname(hostname, HOST_NAME_MAX + 1);


    int configfd;
    char *address = NULL;
    
    

    configfd = open ("/dev/rfm2g0", O_RDWR);
    if (configfd < 0)
    {
        perror ("Open call failed");
        return -1;
    }

    address = mmap (NULL, PAGE_SIZE*400, PROT_READ | PROT_WRITE, MAP_SHARED, configfd, 0);
    
    if (address == MAP_FAILED)
    {
        perror ("mmap operation failed");
        return -1;
    }

    FILE* log=fopen("/dev/stdout","w");
    //FILE* log=fopen("/dev/kmsg","w");

    //memcpy (address + 11, "*user*", 6);
    //printf ("Initial message: %s\n", address);
    //sleep(1);
    fprintf (log,"0\n");
    fflush(log);
    memcpy (address , "user", 6);
    sleep(1);
    fprintf (log,"1\n");
    fflush(log);
    memcpy (address , "use1", 6);
    sleep(1);
    fprintf (log,"2\n");
    fflush(log);
    memcpy (address , "use2", 6);

    
    
    munmap(address,PAGE_SIZE*400);
    close (configfd);
    //sleep(1);
  
    return 0;
}