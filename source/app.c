/*
 * Copyright (c) 2015 - 2016, Freescale Semiconductor, Inc.
 * Copyright 2016 - 2017,2019 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

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
#if (defined(FSL_FEATURE_SOC_SYSMPU_COUNT) && (FSL_FEATURE_SOC_SYSMPU_COUNT > 0U))
#include "fsl_sysmpu.h"
#endif /* FSL_FEATURE_SOC_SYSMPU_COUNT */
#include "app.h"
#include "fsl_component_serial_manager.h"
#if ((!USB_HOST_CONFIG_KHCI) && (!USB_HOST_CONFIG_EHCI) && (!USB_HOST_CONFIG_OHCI) && (!USB_HOST_CONFIG_IP3516HS))
#error Please enable USB_HOST_CONFIG_KHCI, USB_HOST_CONFIG_EHCI, USB_HOST_CONFIG_OHCI, or USB_HOST_CONFIG_IP3516HS in file usb_host_config.
#endif

#include "host_ccid.h"

#include "usb_phy.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/
/*******************************************************************************
 * Prototypes
 ******************************************************************************/
extern void USB_HostClockInit(void);
extern void USB_HostIsrEnable(void);
extern void USB_HostTaskFn(void *param);
void BOARD_InitHardware(void);
extern void UART_UserRxCallback(void *callbackParam,
                                serial_manager_callback_message_t *message,
                                serial_manager_status_t status);
extern void UART_UserTxCallback(void *callbackParam,
                                serial_manager_callback_message_t *message,
                                serial_manager_status_t status);
/*******************************************************************************
 * Variables
 ******************************************************************************/
/* Allocate the memory for the heap. */
#if defined(configAPPLICATION_ALLOCATED_HEAP) && (configAPPLICATION_ALLOCATED_HEAP)
USB_DMA_NONINIT_DATA_ALIGN(USB_DATA_ALIGN_SIZE) uint8_t ucHeap[configTOTAL_HEAP_SIZE];
#endif
usb_host_handle g_hostHandle;
volatile uint8_t g_AttachFlag;
USB_RAM_ADDRESS_ALIGNMENT(4) static uint8_t s_serialWriteHandleBuffer[SERIAL_MANAGER_WRITE_HANDLE_SIZE];
USB_RAM_ADDRESS_ALIGNMENT(4) static uint8_t s_serialReadHandleBuffer[SERIAL_MANAGER_READ_HANDLE_SIZE];
serial_write_handle_t g_UartTxHandle;
serial_write_handle_t g_UartRxHandle;

extern char usbRecvUart[USB_HOST_CDC_UART_RX_MAX_LEN];

/*******************************************************************************
 * Code
 ******************************************************************************/

void USB_OTG1_IRQHandler(void)
{
    USB_HostEhciIsrFunction(g_hostHandle);
}

void USB_HostClockInit(void)
{
    usb_phy_config_struct_t phyConfig = {
        BOARD_USB_PHY_D_CAL,
        BOARD_USB_PHY_TXCAL45DP,
        BOARD_USB_PHY_TXCAL45DM,
    };

    CLOCK_EnableUsbhs0PhyPllClock(kCLOCK_Usbphy480M, 480000000U);
    CLOCK_EnableUsbhs0Clock(kCLOCK_Usb480M, 480000000U);
    USB_EhciPhyInit(CONTROLLER_ID, BOARD_XTAL0_CLK_HZ, &phyConfig);
}

void USB_HostIsrEnable(void)
{
    uint8_t irqNumber;

    uint8_t usbHOSTEhciIrq[] = USBHS_IRQS;
    irqNumber                = usbHOSTEhciIrq[CONTROLLER_ID - kUSB_ControllerEhci0];
/* USB_HOST_CONFIG_EHCI */

/* Install isr, set priority, and enable IRQ. */
#if defined(__GIC_PRIO_BITS)
    GIC_SetPriority((IRQn_Type)irqNumber, USB_HOST_INTERRUPT_PRIORITY);
#else
    NVIC_SetPriority((IRQn_Type)irqNumber, USB_HOST_INTERRUPT_PRIORITY);
#endif
    EnableIRQ((IRQn_Type)irqNumber);
}

void USB_HostTaskFn(void *param)
{
    USB_HostEhciTaskFunction(param);
}
/*!
 * @brief USB isr function.
 */

/*!
 * @brief host callback function.
 *
 * device attach/detach callback function.
 *
 * @param deviceHandle           device handle.
 * @param configurationHandle attached device's configuration descriptor information.
 * @param event_code           callback event code, please reference to enumeration host_event_t.
 *
 * @retval kStatus_USB_Success              The host is initialized successfully.
 * @retval kStatus_USB_NotSupported         The application don't support the configuration.
 */
usb_status_t USB_HostEvent(usb_device_handle deviceHandle,
                           usb_host_configuration_handle configurationHandle,
                           uint32_t event_code)
{
    usb_status_t status;
    status = kStatus_USB_Success;

    switch (event_code & 0x0000FFFFU)
    {
        case kUSB_HostEventAttach:
            status = USB_HostCdcEvent(deviceHandle, configurationHandle, event_code);
            break;
        case kUSB_HostEventNotSupported:
            usb_echo("device not supported.\r\n");
            break;

        case kUSB_HostEventEnumerationDone:
            status = USB_HostCdcEvent(deviceHandle, configurationHandle, event_code);
            break;

        case kUSB_HostEventDetach:
            status = USB_HostCdcEvent(deviceHandle, configurationHandle, event_code);
            break;

        case kUSB_HostEventEnumerationFail:
            usb_echo("enumeration failed\r\n");
            break;

        default:
            break;
    }
    return status;
}

/*!
 * @brief app initialization.
 */
void APP_init(void)
{
    status_t status = (status_t)kStatus_SerialManager_Error;
    g_UartTxHandle  = (serial_write_handle_t)&s_serialWriteHandleBuffer[0];
    g_UartRxHandle  = (serial_read_handle_t)&s_serialReadHandleBuffer[0];
    status          = (status_t)SerialManager_OpenWriteHandle(g_serialHandle, g_UartTxHandle);
    assert(kStatus_SerialManager_Success == status);
    (void)SerialManager_InstallTxCallback(g_UartTxHandle, UART_UserTxCallback, &g_UartTxHandle);

    status = (status_t)SerialManager_OpenReadHandle(g_serialHandle, g_UartRxHandle);
    assert(kStatus_SerialManager_Success == status);
    (void)SerialManager_InstallRxCallback(g_UartRxHandle, UART_UserRxCallback, &g_UartRxHandle);

    SerialManager_ReadNonBlocking(g_UartRxHandle, (uint8_t *)&usbRecvUart[0], USB_HOST_CDC_UART_RX_MAX_LEN);

    g_AttachFlag = 0;

    USB_HostCdcInitBuffer();

    USB_HostClockInit();

#if ((defined FSL_FEATURE_SOC_SYSMPU_COUNT) && (FSL_FEATURE_SOC_SYSMPU_COUNT))
    SYSMPU_Enable(SYSMPU, 0);
#endif /* FSL_FEATURE_SOC_SYSMPU_COUNT */

    status = USB_HostInit(CONTROLLER_ID, &g_hostHandle, USB_HostEvent);
    if (status != kStatus_USB_Success)
    {
        usb_echo("host init error\r\n");
        return;
    }
    USB_HostIsrEnable();
}

void usb_host_task(void *hostHandle)
{
    while (1)
    {
        USB_HostTaskFn(hostHandle);
    }
}

extern void ccid_app_task(void);
void app_task(void *param)
{
    APP_init();
    if (xTaskCreate(usb_host_task, "usb host task", 2000L / sizeof(portSTACK_TYPE), g_hostHandle, 4, NULL) != pdPASS)
    {
        usb_echo("create host task error\r\n");
    }

    while (1)
    { 
        ccid_app_task();
        USB_HostCdcTask(&g_cdc);
    }
}

void ccid_tick_task(void *param)
{
    #define DELAY_MS 1000
    TickType_t xDelay = DELAY_MS / portTICK_PERIOD_MS;

    while(1)
    {
         // usb_echo("CCID tick. \r\n");
         vTaskDelay(xDelay);
    }
}

int main(void)
{
    BOARD_ConfigMPU();
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitDebugConsole();
    
    usb_echo("CCID host running. \r\n");

    if (xTaskCreate(app_task, "app task", 2000L / sizeof(portSTACK_TYPE), NULL, 3, NULL) != pdPASS)
    {
        usb_echo("create app task error\r\n");
    }
    
    if (xTaskCreate(ccid_tick_task, "app ccid_tick_task", 2000L / sizeof(portSTACK_TYPE), NULL, 3, NULL) != pdPASS)
    {
        usb_echo("create ccid_tick_task task error\r\n");
    }
    vTaskStartScheduler();
    while (1)
    {
        ;
    }
}
