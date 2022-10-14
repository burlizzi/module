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

#define RFM2G_MAGIC 0xeb
#include "rfm2g_types.h"


#define RFM2G_PRODUCT_STRING    "AB-RFM2G-DRV-LNX"
#define RFM2G_PRODUCT_OS        "LINUX"
#define RFM2G_PRODUCT_VERSION   "R01.00"


#define WHENDEBUG(x)
/* RFM2GCONFIG structure is used by the RFM2gGetConfig function.  */
typedef struct rfm2gPciConfig
{
    /* RFM PCI Memory Acc. Local, Runtime, and DMA Register Info */
    /* Base Address Register 0 */
    RFM2G_UINT32    rfm2gOrWindowSize;/* Size of PCI Memory window             */
    RFM2G_UINT32    rfm2gOrRegSize;   /* Actual Memory space size              */

    /* RFM Memory Acc. Control and Status Register Info */
    /* Base Address Register 2 */
    RFM2G_UINT32    rfm2gCsWindowSize;/* Size of PCI Memory window            */
    RFM2G_UINT32    rfm2gCsRegSize;   /* Actual Memory space size             */

    /* RFM board memory space info */
    /* Base Address Register 3 */
    RFM2G_UINT32    rfm2gWindowSize;/* Size of PCI Memory Window             */

    RFM2G_UINT32    devfn;          /* Encoded device and function numbers   */

    RFM2G_UINT16    type;           /* From Device ID register               */

    /* Interrupt Info */
    RFM2G_UINT8     interruptNumber;  /* Which interrupt ? */
    RFM2G_UINT8     bus;            /* PCI bus number                        */
    RFM2G_UINT8     function;       /* Function                              */
    RFM2G_UINT8     revision;       /* Revision of RFM board                 */

    /* Base Address Register 3 */
    RFM2G_UINT32    rfm2gBase;      /* Physical base address of board memory */

    /* Base Address Register 2 */
    RFM2G_UINT32    rfm2gCsBase;      /* Physical base addr of board registers*/

    /* Base Address Register 0 */
    RFM2G_UINT32    rfm2gOrBase;      /* Physical base addr of board registers */
} RFM2GPCICONFIG;



/*****************************************************************************/

/* RFM2GCONFIG structure is used by the RFM2gGetConfig function.  */

typedef struct RFM2GCONFIG_
{
    RFM2G_NODE     NodeId;             /* Node ID of board                   */
    RFM2G_UINT8    BoardId;            /* Board Type ID                      */
    RFM2G_UINT8    Unit;               /* Device unit number                 */
    RFM2G_UINT8    PlxRevision;        /* Revision of PLX chip               */
    RFM2G_UINT32   MemorySize;         /* Total Board Memory                 */
    char           Device[128];        /* Name of device file                */
    char           Name[32];/* Product ordering option "VMISFT-RFM2G-ABC-DEF"*/
    char           DriverVersion[16];  /* Release level of driver            */
    RFM2G_UINT8    BoardRevision;      /* Revision of RFM board              */
    RFM2G_UINT8    RevisionId;         /* PCI Revision ID                    */
    RFM2G_UINT32   Lcsr1;              /* Local Control Status Reg - copy    */
    RFM2GPCICONFIG PciConfig;          /* PCI Bus Specific Info              */
    RFM2G_UINT16   BuildId;     /* Build Id of the Revision of the RFM board */
} RFM2GCONFIG;


typedef struct rfm2gConfigLinux
{
    RFM2G_UINT16   Size;               /* Size of structure                   */
    RFM2G_NODE     NodeId;             /* Node ID of board                    */
    RFM2G_UINT8    BoardId;            /* Board Type ID                       */
    RFM2G_UINT8    Unit;               /* Device unit number                  */
    RFM2G_UINT8    PlxRevision;
    RFM2G_UINT8    ExtendedAddressing; /* Are we using >16M addressing        */
    RFM2G_NODE     MaxNodeId;          /* Max Node ID supported by this board */
    RFM2G_UINT32   Capabilities;       /* Combination of RFMCAPFLAGS values   */
    RFM2G_UINT32   RegisterSize;       /* Size of Register Area               */
    RFM2G_UINT32   MemorySize;         /* Total Board Memory                  */
    char           Device[128];        /* Name of device file                 */
    RFM2G_UINT8    *pOrRegisters;      /* Mapped address of OR Registers      */
    RFM2G_UINT8    *pCsRegisters;      /* Mapped address of CS Registers      */
    RFM2G_UINT8    *pBaseAddress;      /* Mapped base address of RFM board    */
    RFM2G_UINT32   UserMemorySize;     /* Total Bytes of Available Memory     */
    RFM2G_UINT32   IntrCount;          /* Number of times Interrupts have been
                                          enabled                             */
    void*          SpinLock;           /* Protects critical sections          */
    RFM2GPCICONFIG PCI;                /* PCI Bus Specific Info               */
    RFM2G_BOOL     dmaByteSwap;        /* DMA byte swap on?                   */
    RFM2G_BOOL     pioByteSwap;        /* PIO byte swap on?                   */
    RFM2G_UINT8    RevisionId;         /* Current board revision/model        */
    RFM2G_UINT16   BuildId;   /* Build Id of the Current board revision/model */
    RFM2G_UINT32   SlidingWindowOffset; /* Offset of current sliding window   */
}  RFM2GCONFIGLINUX;

typedef enum rfm2gRegSetType
{
   RFM2GCFGREGMEM,   /* BAR 0 mem - Local Config, Runtime and DMA Registers  */
   RFM2GCFGREGIO,    /* BAR 1 I/O - Local Config, Runtime and DMA Registers  */
   RFM2GCTRLREGMEM,  /* BAR 2 mem - RFM Control and Status Registers         */
   RFM2GMEM,         /* BAR 3 mem - reflective memory RAM                    */
   RFM2GRESERVED0REG,/* BAR 4 Reserved                                       */
   RFM2GRESERVED1REG /* BAR 5 Reserved                                       */
}  RFM2GREGSETTYPE;


typedef struct rfm2gLinuxRegInfo
{
	RFM2GREGSETTYPE regset;
	RFM2G_UINT32 Offset;
	RFM2G_UINT32 Width;
	RFM2G_UINT32 Value;

} RFM2GLINUXREGINFO;




typedef struct rfm2gTransfer  /* Used for reading and writing */
{
    RFM2G_UINT32    Offset;     /* Offset into Reflective Memory             */
    RFM2G_UINT32    Length;     /* Number of bytes to transfer               */
    void *          Buffer;     /* Address to transfer to/from               */
} RFM2GTRANSFER;


typedef struct rfm2gAtomic  /* Used for peeking and poking */
{
    RFM2G_UINT64    data;       /* Data read from or written to RFM          */
    RFM2G_UINT32    offset;     /* Offset into Reflective Memory             */
    RFM2G_UINT8     width;      /* 1=8 bits, 2=16 bits, 4=32 bits, 8=64 bits */
    RFM2G_UINT8     spare[3];   /* For alignment                             */

} RFM2GATOMIC;


/* Interrupt Events */

typedef enum rfm2gEventType
{
   RFM2GEVENT_RESET,       /* Reset Interrupt                                */
   RFM2GEVENT_INTR1,       /* Network Interrupt 1                            */
   RFM2GEVENT_INTR2,       /* Network Interrupt 2                            */
   RFM2GEVENT_INTR3,       /* Network Interrupt 3                            */
   RFM2GEVENT_INTR4,       /* Network Interrupt 4 (Init interrupt)           */
   RFM2GEVENT_BAD_DATA,    /* Bad Data                                       */
   RFM2GEVENT_RXFIFO_FULL, /* RX FIFO has been full one or more times.       */
   RFM2GEVENT_ROGUE_PKT,   /* the board detected and removed a rogue packet. */
   RFM2GEVENT_RXFIFO_AFULL,/* RX FIFO has been almost full one or more times.*/
   RFM2GEVENT_SYNC_LOSS,   /* Sync loss has occured one of more times.       */
   RFM2GEVENT_MEM_WRITE_INHIBITED, /* Write to local memory was attempted and inhibited */
   RFM2GEVENT_LOCAL_MEM_PARITY_ERR, /* Parity errors have been detected on local memory accesses */
   RFM2GEVENT_LAST         /* Number of Events                               */
}  RFM2GEVENTTYPE;


typedef struct rfm2gEventInfo
{
    RFM2G_UINT32   ExtendedInfo; /* Data passed with event                  */
    RFM2GEVENTTYPE Event;        /* Event type union                        */
    RFM2G_UINT32   Timeout;      /* timeout value to wait for event mSec    */
    RFM2G_NODE     NodeId;       /* Source Node                             */
    void *         pDrvSpec;     /* Driver specific                         */
} RFM2GEVENTINFO;



#define IOCTL_RFM2G_WAIT_FOR_EVENT    _IOWR(RFM2G_MAGIC, 23, RFM2GEVENTINFO)
#define IOCTL_RFM2G_GET_CONFIG               _IOWR(RFM2G_MAGIC, 30, struct RFM2GCONFIG_)
#define IOCTL_RFM2G_READ              _IOW(RFM2G_MAGIC,  73, RFM2G_ADDR)
#define IOCTL_RFM2G_WRITE             _IOW(RFM2G_MAGIC,  74, RFM2G_ADDR)
#define IOCTL_RFM2G_READ_REG              _IOR(RFM2G_MAGIC, 65, RFM2G_UINT32)

#define IOCTL_RFM2G_ATOMIC_PEEK       _IOWR(RFM2G_MAGIC, 10, struct rfm2gAtomic)
#define IOCTL_RFM2G_ATOMIC_POKE       _IOW(RFM2G_MAGIC,  11, struct rfm2gAtomic)
#define IOCTL_RFM2G_SET_SPECIAL_MMAP_OFFSET  _IOW(RFM2G_MAGIC, 77, RFM2G_UINT64)

#define  RFMOR_LAS1RR_RANGE_2M      0xFFE00000

