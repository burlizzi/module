#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h> // close function
#define S(x) x,sizeof(x)

#define PAGE_SIZE     4096
#define PAGE_SHIFT    12


int main (int argc, char **argv)
{

    int configfd;
    char *address = NULL;
    int i;
    

    configfd = open ("/dev/rfm2g0", O_RDWR);
    if (configfd < 0)
    {
        perror ("Open call failed");
        return -1;
    }

    address = mmap (NULL, 165536, PROT_READ | PROT_WRITE, MAP_SHARED, configfd, 0);
    if (address == MAP_FAILED)
    {
        perror ("mmap operation failed");
        return -1;
    }

    FILE* log=fopen("/dev/kmsg","w");

    //memcpy (address + 11, "*user*", 6);
    //printf ("Initial message: %s\n", address);
    //sleep(1);
    fprintf (log,"0\n");
    fflush(log);
    printf ("Changed message: %s\n", address);
    fprintf (log,"1\n");
    fflush(log);
//    for (i = 0; i < 100000000   ; i++)
    {
        memcpy (address , "user", 6);
    }
    
    
    //sleep(1);
    fprintf (log,"2\n");
    fflush(log);
    memcpy (address , "pippo", 6);
    fprintf (log,"3\n");
    fflush(log);
    //sleep(1);
    //sleep(1);
    //memcpy (address + 65435-10, S("Hello from *user* this is file:"));
    //memcpy (address + 11, "*mio**", 6);
    //sleep(1);
    printf ("Changed message: %s\n", address);
    //sleep(1);
    //printf ("Changed message: %s\n", address+65435-10);
    //sleep(1);
    close (configfd);
    //sleep(1);
  
    return 0;
}