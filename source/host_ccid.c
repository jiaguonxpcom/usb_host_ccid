



#include "usb_host_config.h"
#include "usb_host.h"
#include "fsl_device_registers.h"
#include "usb_host_cdc.h"
#include "fsl_debug_console.h"
#include "host_cdc.h"
#include "fsl_common.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "board.h"
 




#define APP_STATUS_INIT 0
#define NO_ACTION 0xffffffff
int app_status = APP_STATUS_INIT;
int flag_test = 0;
int flag_ccid_ready_for_communication = 0;

void USB_CCID_BULK_OUT_Callback(void *param, uint8_t *data, uint32_t dataLength, usb_status_t status)
{
    usb_echo("USB_CCID_BULK_OUT_Callback \r\n");
    flag_test = 2;
}
void USB_CCID_INTERRUPT_IN_Callback(void *param, uint8_t *data, uint32_t dataLength, usb_status_t status)
{
    usb_echo("USB_CCID_INTERRUPT_IN_Callback \r\n");
    flag_test = 4;
}
void USB_CCID_BULK_IN_Callback(void *param, uint8_t *data, uint32_t dataLength, usb_status_t status)
{
    usb_echo("USB_CCID_BULK_IN_Callback \r\n");
    flag_test = 6;
}

void ccid_app_task(void)
{
    uint8_t buf[8] = {0};
    
    if(flag_ccid_ready_for_communication == 0)
    {
        return;
    }
    
    if(flag_test == NO_ACTION)
    {
        return;
    }
    
    if(flag_test == 0)
    {
        usb_echo("ccid_ready_for_communicatio \r\n");
        USB_HostCdcDataSend(g_cdc.classHandle, "12345", 5, USB_CCID_BULK_OUT_Callback, &g_cdc);
    }
    else if(flag_test == 2)
    {
        USB_HostCdcInterruptRecv(g_cdc.classHandle, buf, 8,USB_CCID_INTERRUPT_IN_Callback, &g_cdc);
    }
    else if(flag_test == 4)
    {
        USB_HostCdcDataRecv(g_cdc.classHandle, buf, 8,USB_CCID_BULK_IN_Callback, &g_cdc);
    }

    flag_test = NO_ACTION;
}


void ccid_communication_ready(void)
{
    flag_ccid_ready_for_communication = 1;
}



