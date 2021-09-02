



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
#include "mem_dump.h"
#include "host_ccid.h"
 


static void driver_CCID_bulk_out(uint8_t * buf, uint32_t len);
static void driver_CCID_bulk_in(uint8_t * buf, uint32_t len);
static void driver_CCID_interrupt_in(uint8_t * buf, uint32_t len);

typedef enum
{
    kInit,
    kCheckCardInserted,
    kICC_power_on,
    kICC_power_on_status,
    kXfrBlock,
    kXfrBlock_status,
    kCheckCardRemove,
    kWait,
    kFail,
    kIdle
}CCID_state_t;

static bool i_card_insert_detect(uint8_t bmSlotICCState);
static uint8_t i_get_command_status(uint8_t status);



#define APP_STATUS_INIT 0
#define NO_ACTION 0xffffffff
int app_status = APP_STATUS_INIT;
int flag_test = 0;
int flag_ccid_ready_for_communication = 0;
CCID_state_t ccid_state = kInit;

#define CCID_RX_LEN 64
#define CCID_TX_LEN 64

uint8_t buf_ccid_tx[CCID_TX_LEN];
uint8_t buf_ccid_rx[CCID_RX_LEN] = {0};

void USB_CCID_BULK_OUT_Callback(void *param, uint8_t *data, uint32_t dataLength, usb_status_t status)
{
    usb_echo("USB_CCID_BULK_OUT_Callback dataLength = %d\r\n", dataLength);
    mem_dump_8(data, dataLength);
    if(ccid_state == kICC_power_on)
    {
        usb_echo("ICC power on command sent \r\n\r\n\r\n");
        driver_CCID_bulk_in(buf_ccid_rx, CCID_RX_LEN);
        ccid_state = kICC_power_on_status;
    }
    else if(ccid_state == kXfrBlock)
    {
        usb_echo("k_XfrBlock command sent \r\n\r\n\r\n");
        driver_CCID_bulk_in(buf_ccid_rx, CCID_RX_LEN);
        ccid_state = kXfrBlock_status;
    }
}
void USB_CCID_BULK_IN_Callback(void *param, uint8_t *data, uint32_t dataLength, usb_status_t status)
{
    usb_echo("USB_CCID_BULK_IN_Callback dataLength = %d\r\n", dataLength);
    mem_dump_8(data, dataLength);
    if(ccid_state == kICC_power_on_status)
    {
        usb_echo("k_ICC_power_on_status \r\n");
        if(i_get_command_status(((CCID_msg_RDR_to_PC_DataBlock*)data)->msg_common.bStatus) == CCID_BULK_IN_STATUS_OK)
        {
            usb_echo("status ok.\r\n\r\n\r\n");
            buf_ccid_tx[CCID_MSG_OFFSET_ABDATA + 0] = 0x00;
            buf_ccid_tx[CCID_MSG_OFFSET_ABDATA + 1] = 0xB0;
            buf_ccid_tx[CCID_MSG_OFFSET_ABDATA + 2] = 0x90;
            buf_ccid_tx[CCID_MSG_OFFSET_ABDATA + 3] = 0x00;
            buf_ccid_tx[CCID_MSG_OFFSET_ABDATA + 4] = 0x10;
            CCID_bulk_out_msg_XfrBlock(buf_ccid_tx, 
                                       5, // dwLength
                                       0, // bSlot
                                       9, // bSeq
                                       0, // bBWI
                                       0  // wLevelParameter
                                       );
            ccid_state = kXfrBlock;
        }
        else
        {
            usb_echo("status fail.\r\n");
            ccid_state = kIdle;
        }        
    }
    else if(ccid_state == kXfrBlock_status)
    {
        usb_echo("k_XfrBlock_status \r\n");
        if(i_get_command_status(((CCID_msg_RDR_to_PC_DataBlock*)data)->msg_common.bStatus) == CCID_BULK_IN_STATUS_OK)
        {
            usb_echo("status ok.\r\n\r\n\r\n");
            driver_CCID_interrupt_in(buf_ccid_rx, CCID_RX_LEN);
            ccid_state = kCheckCardRemove;
        }
        else
        {
            usb_echo("status fail.\r\n");
            ccid_state = kIdle;
        }
    }
}

void USB_CCID_INTERRUPT_IN_Callback(void *param, uint8_t *data, uint32_t dataLength, usb_status_t status)
{
    usb_echo("USB_CCID_INTERRUPT_IN_Callback dataLength = %d\r\n", dataLength);
    mem_dump_8(data, dataLength);

    if(ccid_state == kCheckCardInserted)
    {
        if((dataLength == 2) && (data[0] == CCID_MSG_NotifySlotChange))
        {
            if(i_card_insert_detect(data[1]))
            {
                usb_echo("card inserted, power on.\r\n\r\n\r\n");
                ccid_state = kICC_power_on;
                CCID_bulk_out_msg_IccPowerOn(buf_ccid_tx, 0, 8, CCID_ICCPOWERON_3p0);
            }
            else
            {
                usb_echo("state machine fail. func = %s, line = %d\r\n", __func__, __LINE__);
                ccid_state = kIdle;
            }
        }
        else
        {
            usb_echo("kCheckCardInserted fail.\r\n");
            ccid_state = kIdle;
        }
    }
    else if(ccid_state == kCheckCardRemove)
    {
        usb_echo("kCheckCardRemove\r\n");
        ccid_state = kIdle;
    }
    else
    {
        usb_echo("state machine fail. func = %s, line = %d\r\n", __func__, __LINE__);
        ccid_state = kIdle;
    }
}
 
void ccid_app_task(void)
{
    if(flag_ccid_ready_for_communication == 0)
    {
        return;
    }

    if(ccid_state == kInit)
    {
        usb_echo("\r\n\r\n\r\n\r\n");
        usb_echo("-----------------------\r\n");
        usb_echo("ccid sequence test\r\n");
        usb_echo("-----------------------\r\n");
        driver_CCID_interrupt_in(buf_ccid_rx, CCID_RX_LEN);
        ccid_state = kCheckCardInserted;
    }
}


void ccid_communication_ready(void)
{
    flag_ccid_ready_for_communication = 1;
}

static bool i_card_insert_detect(uint8_t bmSlotICCState)
{
    int i;
    bool r = false;
    for(i = 0; i<3; i++)
    {
        if(bmSlotICCState & 0x3)
        {
            usb_echo("slot %d card inserted.\r\n", i+1);
            r = true;
        }
        bmSlotICCState >>= 2;
    }

    return r;
}





static uint8_t i_get_command_status(uint8_t status)
{
    /*
        status: xx                 xxxx             xx
                bmCommandStatus    bmRFU            bmICCStatus
    */
    return ((status>>6) & 0x3);
}




static void driver_CCID_bulk_out(uint8_t * buf, uint32_t len)
{
    USB_HostCdcDataSend(g_cdc.classHandle, buf, len, USB_CCID_BULK_OUT_Callback, &g_cdc);
}

static void driver_CCID_bulk_in(uint8_t * buf, uint32_t len)
{
    USB_HostCdcDataRecv(g_cdc.classHandle, buf, len, USB_CCID_BULK_IN_Callback, &g_cdc);
}

static void driver_CCID_interrupt_in(uint8_t * buf, uint32_t len)
{
    USB_HostCdcInterruptRecv(g_cdc.classHandle, buf, len,USB_CCID_INTERRUPT_IN_Callback, &g_cdc);
}


static void CCID_bulk_out_msg_common(CCID_msg_common_bulk_out_t * msg,
                                 uint8_t  bMessageType,
                                 uint32_t dwLength,
                                 uint8_t  bSlot,
                                 uint8_t  bSeq,
                                 bool     clear)
{
    if(clear)
    {
        memset(msg, 0, CCID_MSG_LEN_MIN);
    }
    msg->bMessageType = bMessageType;
    msg->dwLength     = dwLength;
    msg->bSlot        = bSlot;
    msg->bSeq         = bSeq;
    driver_CCID_bulk_out((uint8_t *)msg, CCID_MSG_LEN_MIN + dwLength);
}

#define NOT_CLEAR_BUF false
#define CLEAR_BUF     true

/*
    Example:
    CCID_bulk_out_msg_IccPowerOn(buf, 0, 0, RDR_IccPowerOn_Auto);
*/
void CCID_bulk_out_msg_IccPowerOn(uint8_t * buf, uint8_t bSlot, uint8_t bSeq, uint8_t bPowerSelect)
{
    memset(buf, 0, CCID_MSG_LEN_MIN);
    ((CCID_msg_PC_to_RDR_IccPowerOn *)buf)->bPowerSelect = bPowerSelect;
    CCID_bulk_out_msg_common((CCID_msg_common_bulk_out_t *)buf, 0x62, 0, bSlot, bSeq, NOT_CLEAR_BUF);
}

void CCID_bulk_out_msg_IccPowerOff(uint8_t * buf, uint8_t bSlot, uint8_t bSeq)
{
    CCID_bulk_out_msg_common((CCID_msg_common_bulk_out_t *)buf, 0x63, 0, bSlot, bSeq, CLEAR_BUF);
}

void CCID_bulk_out_msg_GetSlotStatus(uint8_t * buf, uint8_t bSlot, uint8_t bSeq)
{
    CCID_bulk_out_msg_common((CCID_msg_common_bulk_out_t *)buf, 0x65, 0, bSlot, bSeq, CLEAR_BUF);
}

/*
    Caller fill data, and leave 10 bytes header, then call this API
*/
void CCID_bulk_out_msg_XfrBlock(uint8_t * buf, uint32_t dwLength, uint8_t bSlot, uint8_t bSeq, uint8_t bBWI, uint16_t wLevelParameter)
{
    memset(buf, 0, CCID_MSG_LEN_MIN);
    ((CCID_msg_PC_to_RDR_XfrBlock *)buf)->bBWI            = bBWI;
    ((CCID_msg_PC_to_RDR_XfrBlock *)buf)->wLevelParameter = wLevelParameter;
    CCID_bulk_out_msg_common((CCID_msg_common_bulk_out_t *)buf, 0x6F, dwLength, bSlot, bSeq, NOT_CLEAR_BUF);
}

void CCID_bulk_out_msg_GetParameters(uint8_t * buf, uint8_t bSlot, uint8_t bSeq)
{
    CCID_bulk_out_msg_common((CCID_msg_common_bulk_out_t *)buf, 0x6C, 0, bSlot, bSeq, CLEAR_BUF);
}

void CCID_bulk_out_msg_ResetParameters(uint8_t * buf, uint8_t bSlot, uint8_t bSeq)
{
    CCID_bulk_out_msg_common((CCID_msg_common_bulk_out_t *)buf, 0x6D, 0, bSlot, bSeq, CLEAR_BUF);
}

/*
    Caller fill data, and leave 10 bytes header, then call this API
*/
void CCID_bulk_out_msg_SetParameters(uint8_t * buf, uint32_t dwLength, uint8_t bSlot, uint8_t bSeq, uint8_t bProtocolNum)
{
    memset(buf, 0, CCID_MSG_LEN_MIN);
    ((CCID_msg_PC_to_RDR_SetParameters *)buf)->bProtocolNum = bProtocolNum;
    CCID_bulk_out_msg_common((CCID_msg_common_bulk_out_t *)buf, 0x61, dwLength, bSlot, bSeq, NOT_CLEAR_BUF);
}
 
/*
    Caller fill data, and leave 10 bytes header, then call this API.
*/
void CCID_bulk_out_msg_Escape(uint8_t * buf, uint32_t dwLength, uint8_t bSlot, uint8_t bSeq, uint8_t bProtocolNum)
{
    CCID_bulk_out_msg_common((CCID_msg_common_bulk_out_t *)buf, 0x6B, dwLength, bSlot, bSeq, NOT_CLEAR_BUF);
}

void CCID_bulk_out_msg_IccClock(uint8_t * buf, uint8_t bSlot, uint8_t bSeq, uint8_t bClockCommand)
{
    memset(buf, 0, CCID_MSG_LEN_MIN);
   ((CCID_msg_PC_to_RDR_IccClock *)buf)->bClockCommand = bClockCommand;
    CCID_bulk_out_msg_common((CCID_msg_common_bulk_out_t *)buf, 0x6E, 0, bSlot, bSeq, NOT_CLEAR_BUF);
}

void CCID_bulk_out_msg_T0APDU(uint8_t * buf, uint8_t bSlot, uint8_t bSeq, 
                              uint8_t bmChanges, uint8_t bClassGetResponse, uint8_t bClassEnvelope)
{
    memset(buf, 0, CCID_MSG_LEN_MIN);
   ((CCID_msg_PC_to_RDR_T0APDU *)buf)->bmChanges         = bmChanges;
   ((CCID_msg_PC_to_RDR_T0APDU *)buf)->bClassGetResponse = bClassGetResponse;
   ((CCID_msg_PC_to_RDR_T0APDU *)buf)->bClassEnvelope    = bClassEnvelope;
    CCID_bulk_out_msg_common((CCID_msg_common_bulk_out_t *)buf, 0x6A, 0, bSlot, bSeq, NOT_CLEAR_BUF);
}

/*
    Caller fill data, and leave 10 bytes header, then call this API
*/
void CCID_bulk_out_msg_Secure(uint8_t * buf, uint32_t dwLength, uint8_t bSlot, uint8_t bSeq, uint8_t bBWI, uint16_t wLevelParameter)
{
    memset(buf, 0, CCID_MSG_LEN_MIN);
    ((CCID_msg_PC_to_RDR_Secure *)buf)->bBWI            = bBWI;
    ((CCID_msg_PC_to_RDR_Secure *)buf)->wLevelParameter = wLevelParameter;
    CCID_bulk_out_msg_common((CCID_msg_common_bulk_out_t *)buf, 0x69, dwLength, bSlot, bSeq, NOT_CLEAR_BUF);
}

void CCID_bulk_out_msg_Mechanical(uint8_t * buf, uint8_t bSlot, uint8_t bSeq, uint8_t bFunction)
{
    memset(buf, 0, CCID_MSG_LEN_MIN);
    ((CCID_msg_PC_to_RDR_Mechanical *)buf)->bFunction = bFunction;
    CCID_bulk_out_msg_common((CCID_msg_common_bulk_out_t *)buf, 0x71, 0, bSlot, bSeq, NOT_CLEAR_BUF);
}

void CCID_bulk_out_msg_Abort(uint8_t * buf, uint8_t bSlot, uint8_t bSeq)
{
    CCID_bulk_out_msg_common((CCID_msg_common_bulk_out_t *)buf, 0x72, 0, bSlot, bSeq, CLEAR_BUF);
}

void CCID_bulk_out_msg_SetDataRateAndClockFrequency(
    uint8_t * buf, uint8_t bSlot, uint8_t bSeq, uint32_t dwClockFrequency, uint8_t dwDataRate)
{
    ((CCID_msg_PC_to_RDR_SetDataRateAndClockFrequency*)buf)->dwClockFrequency = dwClockFrequency;
    ((CCID_msg_PC_to_RDR_SetDataRateAndClockFrequency*)buf)->dwDataRate       = dwDataRate;
    CCID_bulk_out_msg_common((CCID_msg_common_bulk_out_t *)buf, 0x73, 8, bSlot, bSeq, CLEAR_BUF);
}
























