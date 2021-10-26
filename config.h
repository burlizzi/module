#include <linux/moduleparam.h>
#define xstr(a) str(a)
#define str(a) #a

#ifndef DEVICE_NAME
#define DEVICE_NAME vrfm
#endif

#ifndef MODULE_NAME
#define MODULE_NAME rfm2g0
#endif
extern bool debug;
#define LOG if (unlikely(debug)) printk
