#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
//linux libs
//#include <bluetooth/bluetooth.h>
//#include <bluetooth/hci.h>
//#include <bluetooth/hci_lib.h>
#define MAX_RESP 255
// defined mac address here, needs further configs
#define MAC_ADDR 19
int scan()
{
    //inquiry_info **inq_info = NULL;
    int max_responces = MAX_RESP;
    int num_responces;
    int dev_id, sock, len, flags;
    int i;
    char address[MAC_ADDR] = { 0 };
    char name[248] = { 0 };

}
