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



struct timespec diff(struct timespec start,struct  timespec end)
{
    struct timespec temp;
    if ((end.tv_nsec-start.tv_nsec)<0) {
        temp.tv_sec = end.tv_sec-start.tv_sec-1;
        temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
    } else {
        temp.tv_sec = end.tv_sec-start.tv_sec;
        temp.tv_nsec = end.tv_nsec-start.tv_nsec;
    }
    return temp;
}

int main (int argc, char **argv)
{
    //asm volatile("int3;");
    //raise(SIGTRAP);


    int configfd;
    volatile char *address = NULL;
    int i;
    

    configfd = open (argc>1?argv[1]:"/dev/rfm2g0", O_RDWR);
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
    printf ("Changed message: %s\n", address+PAGE_SIZE*200-10);


    //memcpy (address + 11, "*user*", 6);
    //printf ("Initial message: %s\n", address);
    //sleep(1);
    
    struct timespec time1, time2,time3;
    int temp;
    clock_gettime(CLOCK_MONOTONIC, &time1);
    char xx=0;


  
    for (size_t i = 0; i < 100000000; i++)
    {
        //printf(".");
        address[rand()%134728]='0'+i;
        //usleep(1000000);
    }
    return 0;

    //sleep(5);
    //sleep(1);
    munmap(address,PAGE_SIZE*400);
    //close (configfd);
    //sleep(1);
 /* */
    return 0;
}