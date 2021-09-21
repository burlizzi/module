#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>

#define PAGE_SIZE     4096

int main (int argc, char **argv)
{

    int configfd;
    char *address = NULL;

    configfd = open ("/dev/rfm2g0", O_RDWR);
    if (configfd < 0)
    {
        perror ("Open call failed");
        return -1;
    }

    address = mmap (NULL, PAGE_SIZE*200, PROT_READ | PROT_WRITE, MAP_SHARED, configfd, 0);
    if (address == MAP_FAILED)
    {
        perror ("mmap operation failed");
        return -1;
    }

    //memcpy (address + 11, "*user*", 6);
    printf ("Initial message: %s\n", address);
    //sleep(1);
    memcpy (address + 11, "*user*", 6);
    sleep(1);
    memcpy (address + PAGE_SIZE-10, "Hello from *user* this is file:", 126);
    //memcpy (address + 11, "*mio**", 6);
    sleep(1);
    printf ("Changed message: %s\n", address);
    sleep(1);
    printf ("Changed message: %s\n", address+PAGE_SIZE-10);
    //sleep(1);
    close (configfd);
    //sleep(1);
  
    return 0;
}