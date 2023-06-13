#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <sys/mman.h>
#include <unistd.h> // close function
#define S(x) x,sizeof(x)

#define PAGE_SIZE     4096
#define PAGE_SHIFT    12


int main (int argc, char **argv)
{
    //asm volatile("int3;");
    //raise(SIGTRAP);


    int configfd;
    volatile char *address = NULL;
    int i;
    

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

    char counter=0;
    while(1)
    {
        for (size_t i = 0; i < 4096; i+=2)
        {
            address[i]++;
        }
        for (size_t i = 0; i < 4094; i+=2)
        {
            if (address[i]!=address[i+2])
                fprintf (log,"check failed: %d-%d at %d",address[i],address[i+2],i);
        }

        usleep(100);
    }
    return 0;


}
