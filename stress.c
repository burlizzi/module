#include "rfm2g_api.h"
#include "stdio.h"
#include <unistd.h>
#include <string.h>

RfmBoardName( RFM2G_UINT8 bid )
{
    static char *name;

    switch( bid )
    {
        case 0x65: name = "VMIPCI-5565"; break;
        default:   name = "UNKNOWN"; break;
    }

    return( name );

}   /* End of RfmBoardName() */
int main(int argc, char**argv)
{
    RFM2G_STATUS    status;
    RFM2GHANDLE		Handle;
    size_t i;
    char* string=argc>1?argv[1]:"/dev/rfm2g0";
    status = RFM2gOpen( string, &Handle);   
    if( status != RFM2G_SUCCESS )
    {
        printf( "Unable to open \"%s\".\n", string );
        printf( "Error: %d.\n\n",  status);
        printf( "Error: %s.\n\n",  RFM2gErrorMsg(status));
        return(-1);
    }

    printf( "    %d.  %s  (%s, Node %d)\n", 0, string,
        RfmBoardName( Handle->Config.BoardId ),
        Handle->Config.NodeId);

    volatile char* mem;
    RFM2gUserMemory(Handle,&mem,0,1);
    for (i = 0; i < 10; i++)
    {
        RFM2gWrite(Handle,0,"ciao",4);
        usleep(10000);
    }
    for (i = 0; i < 100000; i++)
    {
        strcpy(mem,"ciao");
        usleep(10);
    }
    

    RFM2gClose( &Handle );

}