



#define USB_HOST_CCID_CLASS_CODE       0x0BU
#define USB_HOST_CCID_SUBCLASS_CODE    0


#define CCID_MSG_NotifySlotChange 0x50


#define CCID_MSG_LEN_MIN       10
#define CCID_MSG_OFFSET_ABDATA 10

#define CCID_ICCPOWERON_Auto 0
#define CCID_ICCPOWERON_5p0  1
#define CCID_ICCPOWERON_3p0  2
#define CCID_ICCPOWERON_1p8  3

#define CCID_SET_PARA_T0 0
#define CCID_SET_PARA_T1 1

#define CCID_FUNCTION_ACCEPT  1
#define CCID_FUNCTION_EJECT   2
#define CCID_FUNCTION_CAPTURE 3
#define CCID_FUNCTION_LOCK    4
#define CCID_FUNCTION_UNLOCK  5

#define CCID_BULK_IN_STATUS_OK             0
#define CCID_BULK_IN_STATUS_FAILED         1
#define CCID_BULK_IN_STATUS_TIME_EXTENSION 2


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
} CCID_msg_common_bulk_out_t;

typedef struct
{
    CCID_msg_common_bulk_out_t msg_common;
    uint8_t                    bPowerSelect;
    uint8_t                    abRFU[2];
} CCID_msg_PC_to_RDR_IccPowerOn;

typedef struct
{
    CCID_msg_common_bulk_out_t msg_common;
    uint8_t                    bBWI;
    uint16_t                   wLevelParameter;
} CCID_msg_PC_to_RDR_XfrBlock;

typedef struct
{
    CCID_msg_common_bulk_out_t msg_common;
    uint8_t                    bProtocolNum;
} CCID_msg_PC_to_RDR_SetParameters;

typedef struct
{
    CCID_msg_common_bulk_out_t msg_common;
    uint8_t                    bClockCommand;
} CCID_msg_PC_to_RDR_IccClock;

typedef struct
{
    CCID_msg_common_bulk_out_t msg_common;
    uint8_t                    bmChanges;
    uint8_t                    bClassGetResponse;
    uint8_t                    bClassEnvelope;
} CCID_msg_PC_to_RDR_T0APDU;

typedef struct
{
    CCID_msg_common_bulk_out_t msg_common;
    uint8_t                    bBWI;
    uint16_t                   wLevelParameter;
} CCID_msg_PC_to_RDR_Secure;


typedef struct
{
    CCID_msg_common_bulk_out_t msg_common;
    uint8_t                    bFunction;
} CCID_msg_PC_to_RDR_Mechanical;

typedef struct
{
    CCID_msg_common_bulk_out_t msg_common;
    uint8_t                    abRFU[3];
    uint32_t                   dwClockFrequency;
    uint32_t                   dwDataRate;
} CCID_msg_PC_to_RDR_SetDataRateAndClockFrequency;


typedef struct 
{
    uint8_t   bMessageType;
    uint32_t  dwLength;
    uint8_t   bSlot;
    uint8_t   bSeq;
    uint8_t   bStatus;
    uint8_t   bError;    
} CCID_msg_common_bulk_in_t;


typedef struct
{
    CCID_msg_common_bulk_in_t msg_common;
    uint8_t                   bChainParameter;
} CCID_msg_RDR_to_PC_DataBlock;

typedef struct
{
    CCID_msg_common_bulk_in_t msg_common;
    uint8_t                   bClockStatus;
} CCID_msg_RDR_to_PC_SlotStatus;

typedef struct
{
    CCID_msg_common_bulk_in_t msg_common;
    uint8_t                   bProtocolNum;
} CCID_msg_RDR_to_PC_Parameters;

typedef struct
{
    CCID_msg_common_bulk_in_t msg_common;
    uint8_t                   bRFU;
} CCID_msg_RDR_to_PC_Escape;

typedef struct
{
    CCID_msg_common_bulk_in_t msg_common;
    uint8_t                   bRFU;
    uint32_t                  dwClockFrequency;
    uint32_t                  dwDataRate;
} CCID_msg_RDR_to_DataRateAndClockFrequency;

typedef struct
{
    uint8_t bMessageType;
    uint8_t bmSlotICCState;
} CCID_msg_RDR_to_PC_NotifySlotChange;

typedef struct
{
    uint8_t bMessageType;
    uint8_t bSlot;
    uint8_t bSeq;
    uint8_t bHardwareErrorCode;
} CCID_msg_RDR_to_PC_HardwareError;

#pragma pack(pop)




extern void CCID_app_task(void);
extern void CCID_communication_ready(void);

extern void CCID_bulk_out_msg_IccPowerOn(uint8_t * buf, uint8_t bSlot, uint8_t bSeq, uint8_t bPowerSelect);
extern void CCID_bulk_out_msg_IccPowerOff(uint8_t * buf, uint8_t bSlot, uint8_t bSeq);
extern void CCID_bulk_out_msg_GetSlotStatus(uint8_t * buf, uint8_t bSlot, uint8_t bSeq);
extern void CCID_bulk_out_msg_XfrBlock(uint8_t * buf, uint32_t dwLength, uint8_t bSlot, uint8_t bSeq, uint8_t bBWI, uint16_t wLevelParameter);
extern void CCID_bulk_out_msg_GetParameters(uint8_t * buf, uint8_t bSlot, uint8_t bSeq);
extern void CCID_bulk_out_msg_ResetParameters(uint8_t * buf, uint8_t bSlot, uint8_t bSeq);
extern void CCID_bulk_out_msg_SetParameters(uint8_t * buf, uint32_t dwLength, uint8_t bSlot, uint8_t bSeq, uint8_t bProtocolNum);
extern void CCID_bulk_out_msg_Escape(uint8_t * buf, uint32_t dwLength, uint8_t bSlot, uint8_t bSeq, uint8_t bProtocolNum);
extern void CCID_bulk_out_msg_IccClock(uint8_t * buf, uint8_t bSlot, uint8_t bSeq, uint8_t bClockCommand);
extern void CCID_bulk_out_msg_T0APDU(uint8_t * buf, uint8_t bSlot, uint8_t bSeq, 
                                     uint8_t bmChanges, uint8_t bClassGetResponse, uint8_t bClassEnvelope);
extern void CCID_bulk_out_msg_Secure(uint8_t * buf, uint32_t dwLength, uint8_t bSlot, uint8_t bSeq, uint8_t bBWI, uint16_t wLevelParameter);
extern void CCID_bulk_out_msg_Mechanical(uint8_t * buf, uint8_t bSlot, uint8_t bSeq, uint8_t bFunction);
extern void CCID_bulk_out_msg_Abort(uint8_t * buf, uint8_t bSlot, uint8_t bSeq);
extern void CCID_bulk_out_msg_SetDataRateAndClockFrequency(
    uint8_t * buf, uint8_t bSlot, uint8_t bSeq, uint32_t dwClockFrequency, uint8_t dwDataRate);

