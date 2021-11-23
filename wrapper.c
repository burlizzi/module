#include <dlfcn.h>
#include <stdio.h>

#include <gnu/lib-names.h>



#define WRAP(x) int x(char a,char b,char c,char d,char e,char f,char g,char h,char i){\
static int (*orig_##x)(char a,char b,char c,char d,char e,char f,char g,char h,char i) = NULL;\
if (orig_##x == NULL) orig_##x = (int (*)(char,char,char,char,char,char,char,char,char)) dlsym(handle, ""#x);\
printf("%2x %2x %2x %2x %2x %2x %2x",a,b,c,d,e,f,g,h,i);int ret=orig_##x(a,b,c,d,e,f,g,h,i);printf("%2x\n",ret);}

void* handle = NULL;

void init()
{
    handle=dlopen("test.so", RTLD_GLOBAL| RTLD_NOW);
}



WRAP(test)
WRAP(test1)
WRAP(test2)
WRAP(test3)
WRAP(test4)


