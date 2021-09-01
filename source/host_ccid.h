



#define USB_HOST_CCID_CLASS_CODE       0x0BU
#define USB_HOST_CCID_SUBCLASS_CODE    0


#define CCID_MSG_NotifySlotChange 0x50

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

