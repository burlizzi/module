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
    
    struct timespec time1, time2,time3;
    int temp;
    clock_gettime(CLOCK_MONOTONIC, &time1);
    char xx=0;
    if (argc==2 && strcmp(argv[1],"-s")==0)
    {
        while(1)
        {
            if(address[1]!=address[2])
                address[2]=address[1];
        }
    }


    address[1]++;
    while(1)
    {
     //   
        //if (xx!=address[2])        printf ("%x,%x\n",address[1],address[2]);  
        //xx=address[2];
        clock_gettime(CLOCK_MONOTONIC, &time3);
        if(address[1]==address[2] || diff(time1,time3).tv_sec>0)
        {
            clock_gettime(CLOCK_MONOTONIC, &time2);
            if (address[1]==address[2])
                fprintf (log,"ping round: %f ms\n", (float)(diff(time1,time2).tv_nsec)/1000000+(float)(diff(time1,time2).tv_sec)*1000);
            else
                fprintf (log,"ping failed: %f ms\n", (float)(diff(time1,time2).tv_nsec)/1000000+(float)(diff(time1,time2).tv_sec)*1000);
            fflush(log);
            usleep(10000);
            
            //fprintf (log,"x\n");  
            fflush(log);
            //fprintf (log,"y\n");  
            fflush(log);
            address[1]++;
            clock_gettime(CLOCK_MONOTONIC, &time1);
        }
        //usleep(10);

    }
    return 0;


    printf ("Changed message: %p %s\n", address, address);
    fprintf (log,"1\n");
    fflush(log);
    sleep(1);
   // for (i = 0; i < 100000000   ; i++)
    {
        printf ("Changed message: %p %s\n", address, address);
        sleep(1);
    //    memcpy (address , "user", 6);

    }

    fprintf (log,"a\n");
    fflush(log);
    memcpy (address , S("AAAAAAAAAAAAAAAAAA"));
    sleep(1);
    fprintf (log,"b\n");
    fflush(log);
    memcpy (address +10, S("bbbbbbbbbbbbbbb"));
    sleep(1);
    fprintf (log,"c\n");
    fflush(log);
    memcpy (address+20 , S("ccccccccccccccc"));
    sleep(1);
//    mlock(address,PAGE_SIZE);
    printf ("Changed message: %s\n", address);
    fprintf (log,"2\n");
    fflush(log);
    memcpy (address , S("ciao"));
    fprintf (log,"3\n");
    fflush(log);
    //sleep(1);
    //sleep(1);
    memcpy (address + PAGE_SIZE*45-10, S("Hello from *user* this is file:"));
    //memcpy (address + 11, "*mio**", 6);
    sleep(1);

    memcpy (address + PAGE_SIZE*45-10, S("Hello from *again* this is file:"));
    printf ("Changed message: %s\n", address);
    //sleep(5);
    //sleep(1);
    printf ("Changed message: %s\n", address+PAGE_SIZE*45-10);
    //sleep(1);
    munmap(address,PAGE_SIZE*400);
    close (configfd);
    //sleep(1);
  
    return 0;
}