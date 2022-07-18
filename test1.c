#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/mman.h>
int pagesize;
#define handle_error(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)

char *buffer;

static void
handler(int sig, siginfo_t *si, void *unused)
{
    printf("Got SIGSEGV at address: 0x%lx\n",
            (long) si->si_addr);
   if (mprotect(si->si_addr, pagesize,
                PROT_WRITE) == -1)
        handle_error("mprotect");

    //exit(EXIT_FAILURE);
}

int
main(int argc, char *argv[])
{
    char *p;
    pagesize = sysconf(_SC_PAGE_SIZE);
    struct sigaction sa;

   sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = handler;
    if (sigaction(SIGSEGV, &sa, NULL) == -1)
        handle_error("sigaction");

   
    if (pagesize == -1)
        handle_error("sysconf");

   /* Allocate a buffer aligned on a page boundary;
       initial protection is PROT_READ | PROT_WRITE */

   buffer = memalign(pagesize, 100 * pagesize);
    if (buffer == NULL)
        handle_error("memalign");

   printf("Start of region:        0x%lx\n", (long) buffer);

   if (mprotect(buffer , pagesize *100,
                PROT_READ) == -1)
        handle_error("mprotect");

    int i=0;
   for (p = buffer ;  ; )
   {    
        *(p++) = 'a';
        if (((int)p % pagesize) == 0 )
        printf("%d)0x%lx\n",i++, (long) p);

   }
        

   printf("Loop completed\n");     /* Should never happen */
    exit(EXIT_SUCCESS);
}