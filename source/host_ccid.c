



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
    k_ICC_power_on,
    k_ICC_power_on_status,
    kWait,
    kFail,
    kIdle
}CCID_state_t;
static bool card_insert_detect(uint8_t bmSlotICCState);



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
    if(ccid_state == k_ICC_power_on)
    {
        usb_echo("ICC power on call back\r\n");
        driver_CCID_bulk_in(buf_ccid_rx, CCID_RX_LEN);
        ccid_state = k_ICC_power_on_status;
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
            if(card_insert_detect(data[1]))
            {
                usb_echo("card inserted, power on.\r\n");
                ccid_state = k_ICC_power_on;
                CCID_bulk_out_msg_IccPowerOn(buf_ccid_tx, 0, 8, CCID_ICCPOWERON_3p0);
            }
            else
            {
                usb_echo("state machine fail. func = %s, line = %d\r\n", __func__, __LINE__);
            }
        }
        else
        {
            usb_echo("kCheckCardInserted fail.\r\n");
            ccid_state = kFail;
        }
    }
    else
    {
        usb_echo("state machine fail. func = %s, line = %d\r\n", __func__, __LINE__);
    }
}
void USB_CCID_BULK_IN_Callback(void *param, uint8_t *data, uint32_t dataLength, usb_status_t status)
{
    usb_echo("USB_CCID_BULK_IN_Callback dataLength = %d\r\n", dataLength);
    mem_dump_8(data, dataLength);
    if(ccid_state == k_ICC_power_on_status)
    {
        usb_echo("bulk in - k_ICC_power_on_status \r\n");
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
        //USB_HostCdcInterruptRecv(g_cdc.classHandle, buf_rx, CCID_INTERRUPT_RX_LEN, USB_CCID_INTERRUPT_IN_Callback, &g_cdc);
        driver_CCID_interrupt_in(buf_ccid_rx, CCID_RX_LEN);
        ccid_state = kCheckCardInserted;
    }
    
#if 0
    if(flag_test == NO_ACTION)
    {
        return;
    }
    
    if(flag_test == 0)
    {
        usb_echo("\r\n\r\n\r\n\r\n");
        usb_echo("--------------------------- \r\n");
        usb_echo("ccid_ready_for_communicatio \r\n");
        
        USB_HostCdcInterruptRecv(g_cdc.classHandle, buf, 8,USB_CCID_INTERRUPT_IN_Callback, &g_cdc);
        // USB_HostCdcDataSend(g_cdc.classHandle, "12345", 5, USB_CCID_BULK_OUT_Callback, &g_cdc);
    }
    else if(flag_test == 2)
    {
        USB_HostCdcInterruptRecv(g_cdc.classHandle, buf, 8,USB_CCID_INTERRUPT_IN_Callback, &g_cdc);
    }
    else if(flag_test == 4)
    {
        USB_HostCdcDataRecv(g_cdc.classHandle, buf, 8,USB_CCID_bulk_out_Callback, &g_cdc);
    }

    flag_test = NO_ACTION;
#endif
}


void ccid_communication_ready(void)
{
    flag_ccid_ready_for_communication = 1;
}

static bool card_insert_detect(uint8_t bmSlotICCState)
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

#pragma pack(push,1)
typedef struct 
{
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint16_t bcdCCID;
    uint8_t  bMaxSlotIndex;
    uint8_t  bVoltageSupport;
    uint32_t dwProtocols;
    uint32_t dwDefaultClock;
    uint32_t dwMaximumClock;
    uint8_t  bNumClockSupported;
    uint32_t dwDataRate;
    uint32_t dwMaxDataRate;
    uint8_t  bNumDataRatesSupported;
    uint32_t dwMaxIFSD;
    uint32_t dwSynchProtocols;
    uint32_t dwMechanical;
    uint32_t dwFeatures;
    uint32_t dwMaxCCIDMessageLength;
    uint8_t  bClassGetResponse;
    uint8_t  bClassEnvelope;
    uint16_t wLcdLayout;
    uint8_t  bPinSupport;
    uint8_t  bMaxCCIDBusySlots;
} descriptor_smart_card_specific_device_t;
#pragma pack(pop)

#pragma pack(push,1)
typedef struct 
{
    uint8_t   bMessageType;
    uint32_t  dwLength;
    uint8_t   bSlot;
    uint8_t   bSeq;
} CCID_msg_common_t;

typedef struct
{
    CCID_msg_common_t msg_commen;
    uint8_t           bPowerSelect;
    uint8_t           abRFU[2];
} CCID_msg_PC_to_RDR_IccPowerOn;

typedef struct
{
    CCID_msg_common_t msg_commen;
    uint8_t           bBWI;
    uint16_t          wLevelParameter;
} CCID_msg_PC_to_RDR_XfrBlock;

typedef struct
{
    CCID_msg_common_t msg_commen;
    uint8_t           bProtocolNum;
} CCID_msg_PC_to_RDR_SetParameters;

typedef struct
{
    CCID_msg_common_t msg_commen;
    uint8_t           bClockCommand;
} CCID_msg_PC_to_RDR_IccClock;

typedef struct
{
    CCID_msg_common_t msg_commen;
    uint8_t           bmChanges;
    uint8_t           bClassGetResponse;
    uint8_t           bClassEnvelope;
} CCID_msg_PC_to_RDR_T0APDU;

typedef struct
{
    CCID_msg_common_t msg_commen;
    uint8_t           bBWI;
    uint16_t          wLevelParameter;
} CCID_msg_PC_to_RDR_Secure;


typedef struct
{
    CCID_msg_common_t msg_commen;
    uint8_t           bFunction;
} CCID_msg_PC_to_RDR_Mechanical;

typedef struct
{
    CCID_msg_common_t msg_commen;
    uint8_t           abRFU[3];
    uint32_t          dwClockFrequency;
    uint32_t          dwDataRate;
} CCID_msg_PC_to_RDR_SetDataRateAndClockFrequency;

typedef struct
{
    CCID_msg_common_t msg_commen;
    uint8_t           bStatus;
    uint8_t           bError;
    uint8_t           bChainParameter;
} CCID_msg_RDR_to_PC_DataBlock;

typedef struct
{
    CCID_msg_common_t msg_commen;
    uint8_t           bStatus;
    uint8_t           bError;
    uint8_t           bClockStatus;
} CCID_msg_RDR_to_PC_SlotStatus;

typedef struct
{
    CCID_msg_common_t msg_commen;
    uint8_t           bStatus;
    uint8_t           bError;
    uint8_t           bProtocolNum;
} CCID_msg_RDR_to_PC_Parameters;

typedef struct
{
    CCID_msg_common_t msg_commen;
    uint8_t           bStatus;
    uint8_t           bError;
    uint8_t           bRFU;
} CCID_msg_RDR_to_PC_Escape;

typedef struct
{
    CCID_msg_common_t msg_commen;
    uint8_t           bStatus;
    uint8_t           bError;
    uint8_t           bRFU;
    uint32_t          dwClockFrequency;
    uint32_t          dwDataRate;
} CCID_msg_RDR_to_DataRateAndClockFrequency;

typedef struct
{
    uint8_t           bMessageType;
    uint8_t           bmSlotICCState;
} CCID_msg_RDR_to_PC_NotifySlotChange;

typedef struct
{
    uint8_t           bMessageType;
    uint8_t           bSlot;
    uint8_t           bSeq;
    uint8_t           bHardwareErrorCode;
} CCID_msg_RDR_to_PC_HardwareError;

#pragma pack(pop)







#define CCID_MSG_LEN 10
char buf_msg_tx[CCID_MSG_LEN];

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


static void CCID_bulk_out_msg_common(CCID_msg_common_t * msg,
                                 uint8_t  bMessageType,
                                 uint32_t dwLength,
                                 uint8_t  bSlot,
                                 uint8_t  bSeq,
                                 bool     clear)
{
    if(clear)
    {
        memset(msg, 0, CCID_MSG_LEN);
    }
    msg->bMessageType = bMessageType;
    msg->dwLength     = dwLength;
    msg->bSlot        = bSlot;
    msg->bSeq         = bSeq;
    driver_CCID_bulk_out((uint8_t *)msg, CCID_MSG_LEN + dwLength);
}

#define NOT_CLEAR_BUF false
#define CLEAR_BUF     true

/*
    Example:
    CCID_bulk_out_msg_IccPowerOn(buf, 0, 0, RDR_IccPowerOn_Auto);
*/
void CCID_bulk_out_msg_IccPowerOn(uint8_t * buf, uint8_t bSlot, uint8_t bSeq, uint8_t bPowerSelect)
{
    memset(buf, 0, CCID_MSG_LEN);
    ((CCID_msg_PC_to_RDR_IccPowerOn *)buf)->bPowerSelect = bPowerSelect;
    CCID_bulk_out_msg_common((CCID_msg_common_t *)buf, 0x62, 0, bSlot, bSeq, NOT_CLEAR_BUF);
}

void CCID_bulk_out_msg_IccPowerOff(uint8_t * buf, uint8_t bSlot, uint8_t bSeq)
{
    CCID_bulk_out_msg_common((CCID_msg_common_t *)buf, 0x63, 0, bSlot, bSeq, CLEAR_BUF);
}

void CCID_bulk_out_msg_GetSlotStatus(uint8_t * buf, uint8_t bSlot, uint8_t bSeq)
{
    CCID_bulk_out_msg_common((CCID_msg_common_t *)buf, 0x65, 0, bSlot, bSeq, CLEAR_BUF);
}

/*
    Caller fill data, and leave 10 bytes header, then call this API
*/
void CCID_bulk_out_msg_XfrBlock(uint8_t * buf, uint32_t dwLength, uint8_t bSlot, uint8_t bSeq, uint8_t bBWI, uint16_t wLevelParameter)
{
    memset(buf_msg_tx, 0, CCID_MSG_LEN);
    ((CCID_msg_PC_to_RDR_XfrBlock *)buf)->bBWI            = bBWI;
    ((CCID_msg_PC_to_RDR_XfrBlock *)buf)->wLevelParameter = wLevelParameter;
    CCID_bulk_out_msg_common((CCID_msg_common_t *)buf, 0x6F, dwLength, bSlot, bSeq, NOT_CLEAR_BUF);
}

void CCID_bulk_out_msg_GetParameters(uint8_t * buf, uint8_t bSlot, uint8_t bSeq)
{
    CCID_bulk_out_msg_common((CCID_msg_common_t *)buf, 0x6C, 0, bSlot, bSeq, CLEAR_BUF);
}

void CCID_bulk_out_msg_ResetParameters(uint8_t * buf, uint8_t bSlot, uint8_t bSeq)
{
    CCID_bulk_out_msg_common((CCID_msg_common_t *)buf, 0x6D, 0, bSlot, bSeq, CLEAR_BUF);
}

/*
    Caller fill data, and leave 10 bytes header, then call this API
*/
void CCID_bulk_out_msg_SetParameters(uint8_t * buf, uint32_t dwLength, uint8_t bSlot, uint8_t bSeq, uint8_t bProtocolNum)
{
    memset(buf_msg_tx, 0, CCID_MSG_LEN);
    ((CCID_msg_PC_to_RDR_SetParameters *)buf)->bProtocolNum = bProtocolNum;
    CCID_bulk_out_msg_common((CCID_msg_common_t *)buf, 0x61, dwLength, bSlot, bSeq, NOT_CLEAR_BUF);
}
 
/*
    Caller fill data, and leave 10 bytes header, then call this API.
*/
void CCID_bulk_out_msg_Escape(uint8_t * buf, uint32_t dwLength, uint8_t bSlot, uint8_t bSeq, uint8_t bProtocolNum)
{
    CCID_bulk_out_msg_common((CCID_msg_common_t *)buf, 0x6B, dwLength, bSlot, bSeq, NOT_CLEAR_BUF);
}

void CCID_bulk_out_msg_IccClock(uint8_t * buf, uint8_t bSlot, uint8_t bSeq, uint8_t bClockCommand)
{
    memset(buf, 0, CCID_MSG_LEN);
   ((CCID_msg_PC_to_RDR_IccClock *)buf)->bClockCommand = bClockCommand;
    CCID_bulk_out_msg_common((CCID_msg_common_t *)buf, 0x6E, 0, bSlot, bSeq, NOT_CLEAR_BUF);
}

void CCID_bulk_out_msg_T0APDU(uint8_t * buf, uint8_t bSlot, uint8_t bSeq, 
                              uint8_t bmChanges, uint8_t bClassGetResponse, uint8_t bClassEnvelope)
{
    memset(buf, 0, CCID_MSG_LEN);
   ((CCID_msg_PC_to_RDR_T0APDU *)buf)->bmChanges         = bmChanges;
   ((CCID_msg_PC_to_RDR_T0APDU *)buf)->bClassGetResponse = bClassGetResponse;
   ((CCID_msg_PC_to_RDR_T0APDU *)buf)->bClassEnvelope    = bClassEnvelope;
    CCID_bulk_out_msg_common((CCID_msg_common_t *)buf, 0x6A, 0, bSlot, bSeq, NOT_CLEAR_BUF);
}

/*
    Caller fill data, and leave 10 bytes header, then call this API
*/
void CCID_bulk_out_msg_Secure(uint8_t * buf, uint32_t dwLength, uint8_t bSlot, uint8_t bSeq, uint8_t bBWI, uint16_t wLevelParameter)
{
    memset(buf_msg_tx, 0, CCID_MSG_LEN);
    ((CCID_msg_PC_to_RDR_Secure *)buf)->bBWI            = bBWI;
    ((CCID_msg_PC_to_RDR_Secure *)buf)->wLevelParameter = wLevelParameter;
    CCID_bulk_out_msg_common((CCID_msg_common_t *)buf, 0x69, dwLength, bSlot, bSeq, NOT_CLEAR_BUF);
}

void CCID_bulk_out_msg_Mechanical(uint8_t * buf, uint8_t bSlot, uint8_t bSeq, uint8_t bFunction)
{
    memset(buf, 0, CCID_MSG_LEN);
    ((CCID_msg_PC_to_RDR_Mechanical *)buf)->bFunction = bFunction;
    CCID_bulk_out_msg_common((CCID_msg_common_t *)buf, 0x71, 0, bSlot, bSeq, NOT_CLEAR_BUF);
}

void CCID_bulk_out_msg_Abort(uint8_t * buf, uint8_t bSlot, uint8_t bSeq)
{
    CCID_bulk_out_msg_common((CCID_msg_common_t *)buf, 0x72, 0, bSlot, bSeq, CLEAR_BUF);
}

void CCID_bulk_out_msg_SetDataRateAndClockFrequency(
    uint8_t * buf, uint8_t bSlot, uint8_t bSeq, uint32_t dwClockFrequency, uint8_t dwDataRate)
{
    ((CCID_msg_PC_to_RDR_SetDataRateAndClockFrequency*)buf)->dwClockFrequency = dwClockFrequency;
    ((CCID_msg_PC_to_RDR_SetDataRateAndClockFrequency*)buf)->dwDataRate       = dwDataRate;
    CCID_bulk_out_msg_common((CCID_msg_common_t *)buf, 0x73, 8, bSlot, bSeq, CLEAR_BUF);
}
























