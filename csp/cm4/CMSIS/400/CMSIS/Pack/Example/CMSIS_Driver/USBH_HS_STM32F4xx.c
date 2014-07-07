/* -----------------------------------------------------------------------------
 * Copyright (c) 2013 - 2014 ARM Ltd.
 *
 * This software is provided 'as-is', without any express or implied warranty. 
 * In no event will the authors be held liable for any damages arising from 
 * the use of this software. Permission is granted to anyone to use this 
 * software for any purpose, including commercial applications, and to alter 
 * it and redistribute it freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not 
 *    claim that you wrote the original software. If you use this software in
 *    a product, an acknowledgment in the product documentation would be 
 *    appreciated but is not required. 
 * 
 * 2. Altered source versions must be plainly marked as such, and must not be 
 *    misrepresented as being the original software. 
 * 
 * 3. This notice may not be removed or altered from any source distribution.
 *   
 *
 * $Date:        13. February 2014
 * $Revision:    V2.00
 *  
 * Driver:       Driver_USBH1
 * Configured:   via RTE_Device.h configuration file 
 * Project:      USB High/Full-Speed Host Driver for ST STM32F4xx
 * ---------------------------------------------------------------------- 
 * Use the following configuration settings in the middleware component
 * to connect to this driver.
 * 
 *   Configuration Setting                Value
 *   ---------------------                -----
 *   Connect to hardware via Driver_USBH# = 1
 *   USB Host controller interface        = Custom
 * -------------------------------------------------------------------- */

/* History:
 *  Version 2.00
 *    Initial release for USB Host Driver API v2.0
 */

#include <stdint.h>
#include <string.h>
#include "cmsis_os.h"
#include "stm32f4xx.h"

#include "OTG_HS_STM32F4xx.h"

#include "Driver_USBH.h"

#include "RTE_Device.h"
#include "RTE_Components.h"

extern uint8_t otg_hs_role;

extern bool OTG_HS_PinsConfigure   (uint8_t pins_mask);
extern bool OTG_HS_PinsUnconfigure (uint8_t pins_mask);
extern bool OTG_HS_PinVbusOnOff    (bool state);

#define OTG                         OTG_HS

/* USBH Driver ****************************************************************/

#define ARM_USBH_DRIVER_VERSION ARM_DRIVER_VERSION_MAJOR_MINOR(2,0)

/* Driver Version */
static const ARM_DRIVER_VERSION usbh_driver_version = { ARM_USBH_API_VERSION, ARM_USBH_DRIVER_VERSION };

/* Driver Capabilities */
static const ARM_USBH_CAPABILITIES usbh_driver_capabilities = {
  0x0001, /* Root HUB available Ports Mask   */
  false,  /* Automatic SPLIT packet handling */
  true,   /* Signal Connect event            */
  true,   /* Signal Disconnect event         */
  true    /* Signal Overcurrent event        */
};


#define OTG_MAX_CH                 16

static uint32_t *OTG_DFIFO[OTG_MAX_CH] = { OTG_HS_DFIFO0,  OTG_HS_DFIFO1,  
                                           OTG_HS_DFIFO2,  OTG_HS_DFIFO3,  
                                           OTG_HS_DFIFO4,  OTG_HS_DFIFO5,  
                                           OTG_HS_DFIFO6,  OTG_HS_DFIFO7,  
                                           OTG_HS_DFIFO8,  OTG_HS_DFIFO9,  
                                           OTG_HS_DFIFO10, OTG_HS_DFIFO11, 
                                           OTG_HS_DFIFO12, OTG_HS_DFIFO13, 
                                           OTG_HS_DFIFO14, OTG_HS_DFIFO15  
                                         };

static ARM_USBH_SignalPortEvent_t       signal_port_event;
static ARM_USBH_SignalEndpointEvent_t   signal_endpoint_event;

typedef struct _endpoint_info_t {
  uint8_t   type;
  uint8_t   speed;
  uint16_t  max_packet_size;
  uint16_t  interval_reload;
} endpoint_info_t;

static endpoint_info_t endpoint_info[OTG_MAX_CH];

typedef struct _transfer_info_t {
  uint32_t  packet;
  uint8_t  *data;
  uint32_t  num;
  uint32_t  transferred;
  uint16_t  interval;
  struct {
    uint8_t active      :  1;
    uint8_t in_progress :  1;
  } status;
  uint8_t   event;
} transfer_info_t;

static transfer_info_t transfer_info[OTG_MAX_CH];

static bool port_reset = false;


/* USBH Channel Functions ------------*/

/**
  \fn          uint32_t USBH_HW_CH_GetIndexFromAddress (OTG_HS_HC *ptr_ch)
  \brief       Get the Index of Channel from it's Address.
  \param[in]   ptr_ch   Pointer to the Channel
  \return      Index of the Channel
*/
__INLINE static uint32_t USBH_HW_CH_GetIndexFromAddress (OTG_HS_HC *ptr_ch) {
  return (ptr_ch - (OTG_HS_HC *)(&(OTG->HCCHAR0)));
}

/**
  \fn          OTG_HS_HC *USBH_HW_CH_GetAddressFromIndex (uint32_t index)
  \brief       Get the Channel Address from it's Index.
  \param[in]   index    Index of the Channel
  \return      Address of the Channel
*/
__INLINE static OTG_HS_HC *USBH_HW_CH_GetAddressFromIndex (uint32_t index) {
  return ((OTG_HS_HC *)(&(OTG->HCCHAR0)) + index);
}

/**
  \fn          void *USBH_HW_CH_FindFree (void)
  \brief       Find a free Channel.
  \return      Pointer to the first free Channel (0 = no free Channel is available)
*/
__INLINE static void *USBH_HW_CH_FindFree (void) {
  OTG_HS_HC *ptr_ch;
  uint32_t   i;

  ptr_ch = (OTG_HS_HC *)(&(OTG->HCCHAR0));

  for (i = 0; i < OTG_MAX_CH; i++) {
    if (!(ptr_ch->HCCHAR & 0x3FFFFFFF)) return ptr_ch;
    ptr_ch++;
  }

  return 0;
}

/**
  \fn          bool USBH_HW_CH_Disable (OTG_HS_HC *ptr_ch)
  \brief       Disable the Channel.
  \param[in]   ptr_ch   Pointer to the Channel
  \return      true = success, false = fail
*/
__INLINE static bool USBH_HW_CH_Disable (OTG_HS_HC *ptr_ch) {
  int i;

  if (!ptr_ch) return false;

  ptr_ch->HCTSIZ &= ~OTG_HS_HCISIZx_DOPING;
  if (ptr_ch->HCCHAR & OTG_HS_HCCHARx_CHENA) {
    ptr_ch->HCINTMSK = 0;
    if (ptr_ch->HCINT & OTG_HS_HCINTx_NAK) {
      ptr_ch->HCINT  =  0x7BB;
      return true;
    }
    ptr_ch->HCINT  =  0x7BB;
    ptr_ch->HCCHAR =  ptr_ch->HCCHAR | OTG_HS_HCCHARx_CHENA | OTG_HS_HCCHARx_CHDIS;
    for (i =0 ; i < 1000; i++) {
      if (ptr_ch->HCINT & OTG_HS_HCINTx_CHH) {
        ptr_ch->HCINT = 0x7BB;
        return true;
      }
    }
    return false;
  }

  return true;
}

/**
  \fn          bool USBH_HW_CH_TransferEnqueue (OTG_HS_HC *ptr_ch, 
                                                uint32_t   packet, 
                                                uint8_t   *data,   
                                                uint32_t   num)
  \brief       Enqueue the Transfer on a Channel.
  \param[in]   ptr_ch   Pointer to the Channel
  \param[in]   packet   Packet information
  \param[in]   data     Pointer to buffer with data to send or for data to receive
  \param[in]   num      Number of data bytes to transfer
  \return      true = success, false = fail
*/
static bool USBH_HW_CH_TransferEnqueue (OTG_HS_HC *ptr_ch, uint32_t packet, uint8_t *data, uint32_t num) {
  uint32_t  hcchar;
  uint32_t  hctsiz;
  uint32_t  hcintmsk;
  uint32_t  mpsiz;
  uint32_t  ch_idx;
  uint32_t *ptr_src;
  uint32_t *ptr_dest;
  uint32_t  cnt;

  if (!ptr_ch)                          return false;
  if (!data && num)                     return false;
  if (!(OTG->HPRT & OTG_HS_HPRT_PCSTS)) return false;

  hcchar   = ptr_ch->HCCHAR;                      /* Read channel characterist*/
  hctsiz   = ptr_ch->HCTSIZ;                      /* Read channel size info   */
  hcintmsk = 0;
  cnt      = 0;

  /* Prepare transfer                                                         */
                                                  /* Prepare HCCHAR register  */
  hcchar &=        OTG_HS_HCCHARx_ODDFRM   |      /* Keep ODDFRM              */
                   OTG_HS_HCCHARx_DAD_MSK  |      /* Keep DAD                 */
                   OTG_HS_HCCHARx_MC_MSK   |      /* Keep MC                  */
                   OTG_HS_HCCHARx_EPTYP_MSK|      /* Keep EPTYP               */
                   OTG_HS_HCCHARx_LSDEV    |      /* Keep LSDEV               */
                   OTG_HS_HCCHARx_EPNUM_MSK|      /* Keep EPNUM               */
                   OTG_HS_HCCHARx_MPSIZ_MSK;      /* Keep MPSIZ               */
  switch (packet & ARM_USBH_PACKET_TOKEN_Msk) {
    case ARM_USBH_PACKET_IN:
      hcchar   |=  OTG_HS_HCCHARx_EPDIR;
      hcintmsk  =  OTG_HS_HCINTMSKx_DTERRM | 
                   OTG_HS_HCINTMSKx_BBERRM | 
                   OTG_HS_HCINTMSKx_TXERRM | 
                   OTG_HS_HCINTMSKx_ACKM   | 
                   OTG_HS_HCINTMSKx_NAKM   | 
                   OTG_HS_HCINTMSKx_STALLM | 
                   OTG_HS_HCINTMSKx_XFRCM  ;
    break;
    case ARM_USBH_PACKET_OUT:
      hcchar   &= ~OTG_HS_HCCHARx_EPDIR;
      hcintmsk  =  OTG_HS_HCINTMSKx_TXERRM | 
                   OTG_HS_HCINTMSKx_NYET   | 
                   OTG_HS_HCINTMSKx_ACKM   | 
                   OTG_HS_HCINTMSKx_NAKM   | 
                   OTG_HS_HCINTMSKx_STALLM | 
                   OTG_HS_HCINTMSKx_XFRCM  ;
      cnt       = (num + 3) / 4;
    break;
    case ARM_USBH_PACKET_SETUP:
      hcchar   &= ~OTG_HS_HCCHARx_EPDIR;
      hcintmsk  =  OTG_HS_HCINTMSKx_TXERRM | 
                   OTG_HS_HCINTMSKx_XFRCM  ;
      hctsiz   &= ~OTG_HS_HCTSIZx_DPID_MSK;
      hctsiz   |=  OTG_HS_HCTSIZx_DPID_MDATA;
      cnt       = (num + 3) / 4;
    break;
    case ARM_USBH_PACKET_PING:
      hcchar   &= ~OTG_HS_HCCHARx_EPDIR;
      hcintmsk  =  OTG_HS_HCINTMSKx_TXERRM | 
                   OTG_HS_HCINTMSKx_ACKM   | 
                   OTG_HS_HCINTMSKx_NAKM   | 
                   OTG_HS_HCINTMSKx_STALLM | 
                   OTG_HS_HCINTMSKx_XFRCM  ;
  }
  hcchar       &= ~OTG_HS_HCCHARx_CHDIS;
  hcchar       |=  OTG_HS_HCCHARx_CHENA;

                                                  /* Prepare HCTSIZ register  */
  hctsiz       &=  OTG_HS_HCTSIZx_DPID_MSK;       /* Keep DPID                */
  switch (packet & ARM_USBH_PACKET_DATA_Msk) {
    case ARM_USBH_PACKET_DATA0:
      hctsiz   &= ~OTG_HS_HCTSIZx_DPID_MSK;
      hctsiz   |=  OTG_HS_HCTSIZx_DPID_DATA0;
      break;
    case ARM_USBH_PACKET_DATA1:
      hctsiz   &= ~OTG_HS_HCTSIZx_DPID_MSK;
      hctsiz   |=  OTG_HS_HCTSIZx_DPID_DATA1;
      break;
    default:
      break;
  }
  if (packet & ARM_USBH_PACKET_PING) {            /* If OUT pckt DOPING       */
    hctsiz     |=  OTG_HS_HCISIZx_DOPING;
  }

  mpsiz = hcchar & 0x7FF;                         /* Maximum packet size      */
  if (num) {                                      /* Normal packet            */
    hctsiz |= ((num+mpsiz-1) / mpsiz) << 19;      /* Prepare PKTCNT field     */
    hctsiz |= ( num                 ) <<  0;      /* Prepare XFRSIZ field     */
  } else {                                        /* Zero length packet       */
    hctsiz |= ( 1                   ) << 19;      /* Prepare PKTCNT field     */
    hctsiz |= ( 0                   ) <<  0;      /* Prepare XFRSIZ field     */
  }

  ch_idx   = USBH_HW_CH_GetIndexFromAddress (ptr_ch);
  ptr_src  = (uint32_t *)(data);
  ptr_dest = OTG_DFIFO[ch_idx];

  ptr_ch->HCINTMSK = hcintmsk;                    /* Enable channel interrupts*/
  ptr_ch->HCTSIZ   = hctsiz;                      /* Write ch transfer size   */
  ptr_ch->HCCHAR   = hcchar;                      /* Write ch characteristics */

  while (cnt--)                                   /* Lad data                 */
    *ptr_dest = *ptr_src++;

  return true;
}

/* USBH Module Functions -------------*/

/**
  \fn          ARM_DRIVER_VERSION USBH_HW_GetVersion (void)
  \brief       Get driver version.
  \return      ARM_DRIVER_VERSION
*/
static ARM_DRIVER_VERSION USBH_HW_GetVersion (void) { return usbh_driver_version; }

/**
  \fn          ARM_USBH_CAPABILITIES USBH_HW_GetCapabilities (void)
  \brief       Get driver capabilities.
  \return      ARM_USBH_CAPABILITIES
*/
static ARM_USBH_CAPABILITIES USBH_HW_GetCapabilities (void) { return usbh_driver_capabilities; }

/**
  \fn          int32_t USBH_HW_Initialize (ARM_USBH_SignalPortEvent_t     cb_port_event,
                                           ARM_USBH_SignalEndpointEvent_t cb_endpoint_event)
  \brief       Initialize USB Host Interface.
  \param[in]   cb_port_event      Pointer to ARM_USBH_SignalPortEvent
  \param[in]   cb_endpoint_event  Pointer to ARM_USBH_SignalEndpointEvent
  \return      execution status
*/
static int32_t USBH_HW_Initialize (ARM_USBH_SignalPortEvent_t cb_port_event, ARM_USBH_SignalEndpointEvent_t cb_endpoint_event) {
  int32_t tout;

  signal_port_event      = cb_port_event;
  signal_endpoint_event  = cb_endpoint_event;

  port_reset             = false;

  memset(endpoint_info, 0, sizeof(endpoint_info));
  memset(transfer_info, 0, sizeof(transfer_info));

  if (OTG_HS_PinsConfigure (ARM_USB_PIN_DP | ARM_USB_PIN_DM | ARM_USB_PIN_OC | ARM_USB_PIN_VBUS) == false) return ARM_DRIVER_ERROR;

  RCC->AHB1ENR   |=  RCC_AHB1ENR_OTGHSEN;           /* OTG HS clock enable    */
  RCC->AHB1RSTR  |=  RCC_AHB1ENR_OTGHSEN;           /* Reset OTG HS clock     */
  osDelay(1);                                       /* Wait 1 ms              */
  RCC->AHB1RSTR  &= ~RCC_AHB1ENR_OTGHSEN;

#if (RTE_USB_OTG_HS_PHY)
  /* External ULPI PHY */
  RCC->AHB1ENR   |=  RCC_AHB1ENR_OTGHSULPIEN;       /* OTG HS ULPI clock en   */
  OTG->GUSBCFG   &= ~OTG_HS_GUSBCFG_PHSEL;          /* High-spd trnscvr       */
  OTG->GUSBCFG   |=  OTG_HS_GUSBCFG_PTCI      |     /* Ind. pass through      */
                     OTG_HS_GUSBCFG_PCCI      |     /* Ind. complement        */
                     OTG_HS_GUSBCFG_ULPIEVBUSI|     /* ULPI ext Vbus ind      */
                     OTG_HS_GUSBCFG_ULPIEVBUSD;     /* ULPI ext Vbus drv      */
#else
  /* Embedded PHY */
  OTG->GUSBCFG   |=  OTG_HS_GUSBCFG_PHSEL  |        /* Full-speed transceiver */
                     OTG_HS_GUSBCFG_PHYLPCS;        /* 48 MHz external clock  */
  OTG->GCCFG     &= ~OTG_HS_GCCFG_VBUSBSEN;         /* Disable VBUS sens of B */
  OTG->GCCFG     &= ~OTG_HS_GCCFG_VBUSASEN;         /* Disable VBUS sens of A */
  OTG->GCCFG     |=  OTG_HS_GCCFG_NOVBUSSENS;       /* No VBUS sensing        */
#endif

  OTG->GAHBCFG   &= ~OTG_HS_GAHBCFG_GINT;           /* Disable interrupts     */

  /* Wait until AHB Master state machine is in the idle condition             */
  for (tout = 1000; tout >= 0; tout--) {            /* Wait max 1 second      */
    if (OTG->GRSTCTL & OTG_HS_GRSTCTL_AHBIDL) break;
    if (!tout) return ARM_DRIVER_ERROR;
    osDelay (1);
  }
  OTG->GRSTCTL |=  OTG_HS_GRSTCTL_CSRST;            /* Core soft reset        */
  for (tout = 1000; tout >= 0; tout--) {            /* Wait max 1 second      */
    if (!(OTG->GRSTCTL & OTG_HS_GRSTCTL_CSRST)) break;
    if (!tout) return ARM_DRIVER_ERROR;
    osDelay (1);
  }
  osDelay (20);

  if (!(OTG->GUSBCFG & OTG_HS_GUSBCFG_FHMOD)) {
    OTG->GUSBCFG |=  OTG_HS_GUSBCFG_FHMOD;          /* Force host mode        */
    osDelay (50);
  }

  /* Core initialization                                                      */
  OTG->GRXFSIZ   = (512/4) +                        /* RxFIFO depth is 512 by */
                   (  8/4) +                        /* 8 bytes for Int EP     */
                        2  ;                        /* 2 Packet info & status */
  OTG->GNPTXFSIZ = ((512/4)<<16) | ((512/4)  +4);   /* Non-peri TxFIFO        */
  OTG->HPTXFSIZ  = ((240/4)<<16) | ((512/4)*2+4);   /* Peri TxFIFO mem        */

  OTG->GINTMSK  |=  OTG_HS_GINTMSK_DISCINT|         /* En disconn int         */
                    OTG_HS_GINTMSK_HCIM   |         /* En host ch int         */
                    OTG_HS_GINTMSK_PRTIM  |         /* En host prt int        */
                    OTG_HS_GINTMSK_RXFLVLM|         /* Enable RXFIFO int      */
                    OTG_HS_GINTMSK_SOFM   ;         /* Enable SOF int         */
  OTG->HAINTMSK  =  0x0000FFFF;                     /* En all ch ints         */
  OTG->GINTSTS   =  0xFFFFFFFF;                     /* Clear interrupts       */

  NVIC_SetPriority (OTG_HS_IRQn, 0);                /* OTG int highest prio   */

  return ARM_DRIVER_OK;
}

/**
  \fn          int32_t USBH_HW_Uninitialize (void)
  \brief       De-initialize USB Host Interface.
  \return      execution status
*/
static int32_t USBH_HW_Uninitialize (void) {

  RCC->AHB1RSTR  |=  RCC_AHB1ENR_OTGHSEN;           /* Reset OTG HS clock     */
  osDelay(1);                                       /* Wait 1 ms              */
  RCC->AHB1RSTR  &= ~RCC_AHB1ENR_OTGHSEN;
  RCC->AHB1ENR   &= ~RCC_AHB1ENR_OTGHSEN;           /* OTG HS clock disable   */

  if (OTG_HS_PinsUnconfigure (ARM_USB_PIN_DP | ARM_USB_PIN_DM | ARM_USB_PIN_OC | ARM_USB_PIN_VBUS) == false) return ARM_DRIVER_ERROR;
#if (!RTE_USB_OTG_HS_PHY)
  RCC->AHB1ENR  &= ~RCC_AHB1ENR_OTGHSULPIEN;        /* OTG HS ULPI clock dis  */
#endif

  return ARM_DRIVER_OK;
}

/**
  \fn          int32_t USBH_HW_PowerControl (ARM_POWER_STATE state)
  \brief       Control USB Host Interface Power.
  \param[in]   state    Power state
  \return      execution status
*/
static int32_t USBH_HW_PowerControl (ARM_POWER_STATE state) { 

  switch (state) {
    case ARM_POWER_OFF:
      NVIC_DisableIRQ   (OTG_HS_IRQn);              /* Disable OTG interrupt  */
      OTG->GAHBCFG  &= ~OTG_HS_GAHBCFG_GINT;        /* Disable interrupts     */
      OTG->GCCFG    &= ~OTG_HS_GCCFG_PWRDWN;        /* Enable power down      */
      otg_hs_role    =  ARM_USB_ROLE_NONE;
      break;

    case ARM_POWER_LOW:
      return ARM_DRIVER_ERROR_UNSUPPORTED;

    case ARM_POWER_FULL:
      otg_hs_role    =  ARM_USB_ROLE_HOST;
      OTG->GCCFG    |=  OTG_HS_GCCFG_PWRDWN;        /* Disable power down     */
      NVIC_EnableIRQ   (OTG_HS_IRQn);               /* Enable OTG interrupt   */
      OTG->GAHBCFG  |=  OTG_HS_GAHBCFG_GINT;        /* Enable interrupts      */
      break;

    default:
      return ARM_DRIVER_ERROR;
  }

  return ARM_DRIVER_OK;
}

/**
  \fn          int32_t USBH_HW_PortVbusOnOff (uint8_t port, bool vbus)
  \brief       Root HUB Port VBUS on/off.
  \param[in]   port  Root HUB Port Number
  \param[in]   vbus
                - false: VBUS off
                - true:  VBUS on
  \return      execution status
*/
static int32_t USBH_HW_PortVbusOnOff (uint8_t port, bool vbus) {

  if (port) return ARM_DRIVER_ERROR;

  if (vbus) {                                       /* VBUS power on          */
    OTG->GAHBCFG &= ~OTG_HS_GAHBCFG_GINT;           /* Disable interrupts     */
    OTG->HPRT    |=  OTG_HS_HPRT_PPWR;              /* Port power on          */
    if (OTG_HS_PinVbusOnOff (true ) == false) return ARM_DRIVER_ERROR;
    osDelay(200);                                   /* Allow VBUS to stabilize*/
    OTG->HAINT    =  0x0000FFFF;                    /* Clear port interrupts  */
    OTG->GINTSTS  =  0xFFFFFFFF;                    /* Clear interrupts       */
    OTG->GAHBCFG |=  OTG_HS_GAHBCFG_GINT;           /* Disable interrupts     */
  } else {                                          /* VBUS power off         */
    if (OTG_HS_PinVbusOnOff (false) == false) return ARM_DRIVER_ERROR;
    OTG->HPRT    &= ~OTG_HS_HPRT_PPWR;              /* Port power off         */
  }

  return ARM_DRIVER_OK;
}

/**
  \fn          int32_t USBH_HW_PortReset (uint8_t port)
  \brief       Do Root HUB Port Reset.
  \param[in]   port     Root HUB Port Number
  \return      execution status
*/
static int32_t USBH_HW_PortReset (uint8_t port) {
  uint32_t hprt;
  uint32_t hcfg;

  if (port)                             return ARM_DRIVER_ERROR;
  if (!(OTG->HPRT & OTG_HS_HPRT_PCSTS)) return ARM_DRIVER_ERROR;

  hcfg = OTG->HCFG;
  hprt = OTG->HPRT;
  switch ((hprt >> 17) & 3) {
    case 0:                             /* High-speed detected                */
    case 1:                             /* Full-speed detected                */
      if (OTG->HFIR != 48000) OTG->HFIR = 48000;
      if ((hcfg & 3) != 1) {
        OTG->HCFG = (hcfg & ~OTG_HS_HCFG_FSLSPCS(3)) | OTG_HS_HCFG_FSLSPCS(1);
      }
      break;
    case 2:                             /* Low-speed detected                 */
      if (OTG->HFIR != 6000) OTG->HFIR = 6000;
      if ((hcfg & 3) != 2) {
        OTG->HCFG = (hcfg & ~OTG_HS_HCFG_FSLSPCS(3)) | OTG_HS_HCFG_FSLSPCS(2);
      }
      break;
    case 3:
      break;
  }

  if (!(OTG->HPRT & OTG_HS_HPRT_PCSTS)) return ARM_DRIVER_ERROR;

  port_reset = true;
  hprt  = OTG->HPRT;
  hprt &= ~OTG_HS_HPRT_PENA;            /* Disable port                       */
  OTG->HPRT = hprt;
  osDelay (10);
  hprt |=  OTG_HS_HPRT_PRST;            /* Port reset                         */
  OTG->HPRT = hprt;
  osDelay (50);
  hprt &= ~OTG_HS_HPRT_PRST;            /* Clear port reset                   */
  OTG->HPRT = hprt;
  osDelay (100);
  port_reset = false;

  return ARM_DRIVER_OK;
}

/**
  \fn          int32_t USBH_HW_PortSuspend (uint8_t port)
  \brief       Suspend Root HUB Port (stop generating SOFs).
  \param[in]   port     Root HUB Port Number
  \return      execution status
*/
static int32_t USBH_HW_PortSuspend (uint8_t port) { 

  if (port) return ARM_DRIVER_ERROR;

  OTG->HPRT |=  OTG_HS_HPRT_PSUSP;      /* Port suspend                       */

  return ARM_DRIVER_OK;
}

/**
  \fn          int32_t USBH_HW_PortResume (uint8_t port)
  \brief       Resume Root HUB Port (start generating SOFs).
  \param[in]   port     Root HUB Port Number
  \return      execution status
*/
static int32_t USBH_HW_PortResume (uint8_t port) { 

  if (port) return ARM_DRIVER_ERROR;

  OTG->HPRT |=  OTG_HS_HPRT_PRES;       /* Port resume                        */

  return ARM_DRIVER_OK;
}

/**
  \fn          ARM_USBH_PORT_STATE USBH_HW_PortGetState (uint8_t port)
  \brief       Get current Root HUB Port State.
  \param[in]   port  Root HUB Port Number
  \return      Port State ARM_USBH_PORT_STATE
*/
static ARM_USBH_PORT_STATE USBH_HW_PortGetState (uint8_t port) { 
  ARM_USBH_PORT_STATE port_state = { 0 };
  uint32_t hprt;

  if (port) return port_state;

  hprt = OTG->HPRT;

  port_state.connected = (hprt & OTG_HS_HPRT_PCSTS) != 0;
  switch ((hprt & OTG_HS_HPRT_PSPD_MSK) >> OTG_HS_HPRT_PSPD_POS) {
    case 0:                             /* High speed                         */
     port_state.speed = ARM_USB_SPEED_HIGH;
     break;
    case 1:                             /* Full speed                         */
     port_state.speed = ARM_USB_SPEED_FULL;
     break;
    case 2:                             /* Low speed                          */
     port_state.speed = ARM_USB_SPEED_LOW;
     break;
    default:
     break;
  }

  return port_state; 
}

/**
  \fn          int32_t USBH_HW_EndpointTransferAbort (ARM_USBH_EP_HANDLE ep_hndl)
  \brief       Abort current USB Endpoint transfer.
  \param[in]   ep_hndl  Endpoint Handle
  \return      execution status
*/
static int32_t USBH_HW_EndpointTransferAbort (ARM_USBH_EP_HANDLE ep_hndl) {
  uint32_t ch_idx;

  if (!ep_hndl) return ARM_DRIVER_ERROR;

  ch_idx = USBH_HW_CH_GetIndexFromAddress ((OTG_HS_HC *)ep_hndl);

  if (transfer_info[ch_idx].status.active) {
    transfer_info[ch_idx].status.active = 0;
    if (!USBH_HW_CH_Disable((OTG_HS_HC *)(ep_hndl))) return ARM_DRIVER_ERROR;
  }

  return ARM_DRIVER_OK;
}

/**
  \fn          int32_t USBH_HW_EndpointModify (ARM_USBH_EP_HANDLE ep_hndl,
                                               uint8_t            dev_addr,
                                               uint8_t            dev_speed,
                                               uint8_t            hub_addr,
                                               uint8_t            hub_port,
                                               uint16_t           ep_max_packet_size)
  \brief       Modify Endpoint in System.
  \param[in]   ep_hndl    Endpoint Handle
  \param[in]   dev_addr   Device Address
  \param[in]   dev_speed  Device Speed
  \param[in]   hub_addr   Hub Address
  \param[in]   hub_port   Hub Port
  \param[in]   ep_max_packet_size Endpoint Maximum Packet Size
  \return      execution status
*/
static int32_t USBH_HW_EndpointModify (ARM_USBH_EP_HANDLE ep_hndl, uint8_t dev_addr, uint8_t dev_speed, uint8_t hub_addr, uint8_t hub_port, uint16_t ep_max_packet_size) { 
  OTG_HS_HC *ptr_ch;
  uint32_t   ch_idx;
  uint32_t   hcchar;

  if (!ep_hndl) return ARM_DRIVER_ERROR;

  ptr_ch = (OTG_HS_HC *)(ep_hndl);
  ch_idx = USBH_HW_CH_GetIndexFromAddress (ptr_ch);

  if (USBH_HW_EndpointTransferAbort (ep_hndl) != ARM_DRIVER_OK) return ARM_DRIVER_ERROR;

  /* Fill in all fields of Endpoint Descriptor                                */
  hcchar  = ptr_ch->HCCHAR;
  hcchar &= (~OTG_HS_HCCHARx_MPSIZ_MSK) &   /* Clear maximum packet size field*/
            (~OTG_HS_HCCHARx_LSDEV    ) &   /* Clear device speed bit         */
            (~OTG_HS_HCCHARx_DAD_MSK  ) ;   /* Clear device address field     */
  hcchar |=   OTG_HS_HCCHARx_MPSIZ   (ep_max_packet_size)              | 
            ( OTG_HS_HCCHARx_LSDEV * (dev_speed == ARM_USB_SPEED_LOW)) | 
            ( OTG_HS_HCCHARx_DAD     (dev_addr))                       ;
  ptr_ch->HCCHAR = hcchar;              /* Update modified fields             */

  endpoint_info[ch_idx].speed           = dev_speed;
  endpoint_info[ch_idx].max_packet_size = ep_max_packet_size;

  return ARM_DRIVER_OK;
}

/**
  \fn          USBH_HW_EP_HANDLE USBH_HW_EndpointCreate (uint8_t  dev_addr,
                                                         uint8_t  dev_speed,
                                                         uint8_t  hub_addr,
                                                         uint8_t  hub_port,
                                                         uint8_t  ep_addr,
                                                         uint8_t  ep_type,
                                                         uint16_t ep_max_packet_size,
                                                         uint8_t  ep_interval)
  \brief       Create Endpoint in System.
  \param[in]   dev_addr   Device Address
  \param[in]   dev_speed  Device Speed
  \param[in]   hub_addr   Hub Address
  \param[in]   hub_port   Hub Port
  \param[in]   ep_addr    Endpoint Address
                - ep_addr.0..3: Address
                - ep_addr.7:    Direction
  \param[in]   ep_type    Endpoint Type
  \param[in]   ep_max_packet_size Endpoint Maximum Packet Size
  \param[in]   ep_interval        Endpoint Polling Interval
  \return      Endpoint Handle ARM_USBH_EP_HANDLE
*/
static ARM_USBH_EP_HANDLE USBH_HW_EndpointCreate (uint8_t dev_addr, uint8_t dev_speed, uint8_t hub_addr, uint8_t hub_port, uint8_t ep_addr, uint8_t ep_type, uint16_t ep_max_packet_size, uint8_t  ep_interval) {
  OTG_HS_HC *ptr_ch;
  uint32_t   ch_idx;

  ptr_ch = (OTG_HS_HC *)(USBH_HW_CH_FindFree ());             /* Find free Ch */
  if (!ptr_ch) return NULL;                                   /* If no free   */

  ch_idx = USBH_HW_CH_GetIndexFromAddress (ptr_ch);

  /* Fill in all fields of Endpoint Descriptor                                */
  ptr_ch->HCCHAR = OTG_HS_HCCHARx_MPSIZ   (ep_max_packet_size)             | 
                   OTG_HS_HCCHARx_EPNUM   (ep_addr)                        | 
                   OTG_HS_HCCHARx_EPDIR * (!((ep_addr >> 7) & 0x0001))     | 
                   OTG_HS_HCCHARx_LSDEV * (dev_speed == ARM_USB_SPEED_LOW) | 
                   OTG_HS_HCCHARx_EPTYP   (ep_type)                        | 
                   OTG_HS_HCCHARx_DAD     (dev_addr);

  endpoint_info[ch_idx].speed           = dev_speed;
  endpoint_info[ch_idx].max_packet_size = ep_max_packet_size;
  endpoint_info[ch_idx].type            = ep_type;
  switch (ep_type) {
    case ARM_USB_ENDPOINT_CONTROL:
    case ARM_USB_ENDPOINT_BULK:
      break;
    case ARM_USB_ENDPOINT_ISOCHRONOUS:
    case ARM_USB_ENDPOINT_INTERRUPT:
      if (dev_speed == ARM_USB_SPEED_HIGH) {
        if ((ep_interval > 0) && (ep_interval <= 16)) {
          endpoint_info[ch_idx].interval_reload = 1 << (ep_interval - 1);
        }
      } else if ((dev_speed == ARM_USB_SPEED_FULL) || (dev_speed == ARM_USB_SPEED_LOW)) {
        if (ep_interval > 0) {
          endpoint_info[ch_idx].interval_reload = ep_interval;
        }
      }
      ptr_ch->HCCHAR |= OTG_HS_HCCHARx_MC((((ep_max_packet_size >> 11) + 1) & 3));
      break;
  }

  return ((ARM_USBH_EP_HANDLE)ptr_ch);
}

/**
  \fn          int32_t USBH_HW_EndpointDelete (ARM_USBH_EP_HANDLE ep_hndl)
  \brief       Delete Endpoint from System.
  \param[in]   ep_hndl  Endpoint Handle
  \return      execution status
*/
static int32_t USBH_HW_EndpointDelete (ARM_USBH_EP_HANDLE ep_hndl) {
  OTG_HS_HC *ptr_ch;

  if (!ep_hndl) return ARM_DRIVER_ERROR;

  if (USBH_HW_EndpointTransferAbort (ep_hndl) != ARM_DRIVER_OK) return ARM_DRIVER_ERROR;

  ptr_ch           = (OTG_HS_HC *)(ep_hndl);
  ptr_ch->HCCHAR   = 0;
  ptr_ch->HCINT    = 0;
  ptr_ch->HCINTMSK = 0;
  ptr_ch->HCTSIZ   = 0;

  memset(&endpoint_info[USBH_HW_CH_GetIndexFromAddress (ptr_ch)], 0, sizeof(endpoint_info));

  return ARM_DRIVER_OK;
}

/**
  \fn          int32_t USBH_HW_EndpointReset (ARM_USBH_EP_HANDLE ep_hndl)
  \brief       Reset Endpoint.
  \param[in]   ep_hndl  Endpoint Handle
  \return      execution status
*/
static int32_t USBH_HW_EndpointReset (ARM_USBH_EP_HANDLE ep_hndl) {
  OTG_HS_HC *ptr_ch;

  if (!ep_hndl) return ARM_DRIVER_ERROR;

  if (USBH_HW_EndpointTransferAbort (ep_hndl) != ARM_DRIVER_OK) return ARM_DRIVER_ERROR;

  ptr_ch           = (OTG_HS_HC *)(ep_hndl);
  ptr_ch->HCINT    = 0;
  ptr_ch->HCINTMSK = 0;
  ptr_ch->HCTSIZ   = 0;

  return ARM_DRIVER_OK;
}

/**
  \fn          int32_t USBH_HW_EndpointTransfer (ARM_USBH_EP_HANDLE ep_hndl,
                                                 uint32_t packet,
                                                 uint8_t *data,
                                                 uint32_t num)
  \brief       Transfer packets through USB Endpoint.
  \param[in]   ep_hndl  Endpoint Handle
  \param[in]   packet   Packet information
  \param[in]   data     Pointer to buffer with data to send or for data to receive
  \param[in]   num      Number of data bytes to transfer
  \return      execution status
*/
static int32_t USBH_HW_EndpointTransfer (ARM_USBH_EP_HANDLE ep_hndl, uint32_t packet, uint8_t *data, uint32_t num) {
  uint32_t ch_idx;

  if (!ep_hndl)                         return ARM_DRIVER_ERROR;
  if (!(OTG->HPRT & OTG_HS_HPRT_PCSTS)) return ARM_DRIVER_ERROR;

  ch_idx = USBH_HW_CH_GetIndexFromAddress ((OTG_HS_HC *)(ep_hndl));

  memset(&transfer_info[ch_idx], 0, sizeof(transfer_info_t));

  transfer_info[ch_idx].packet             = packet;
  transfer_info[ch_idx].data               = data;
  transfer_info[ch_idx].num                = num;
  transfer_info[ch_idx].transferred        = 0;
  transfer_info[ch_idx].interval           = (endpoint_info[ch_idx].interval_reload != 0);
  transfer_info[ch_idx].event              = 0;
  transfer_info[ch_idx].status.in_progress = 1;
  transfer_info[ch_idx].status.active      = 1;

  USBH_HW_CH_TransferEnqueue ((OTG_HS_HC *)ep_hndl, packet, data, num);

  return ARM_DRIVER_OK;
}

/**
  \fn          uint32_t USBH_HW_EndpointTransferGetResult (ARM_USBH_EP_HANDLE ep_hndl)
  \brief       Get result of USB Endpoint transfer.
  \param[in]   ep_hndl  Endpoint Handle
  \return      number of successfully transfered data bytes
*/
static uint32_t USBH_HW_EndpointTransferGetResult (ARM_USBH_EP_HANDLE ep_hndl) {

  if (!ep_hndl) return 0;

  return (transfer_info[USBH_HW_CH_GetIndexFromAddress((OTG_HS_HC *)ep_hndl)].transferred);
}

/**
  \fn          uint16_t USBH_HW_GetFrameNumber (void)
  \brief       Get current USB Frame Number.
  \return      Frame Number
*/
static uint16_t USBH_HW_GetFrameNumber (void) {

  return ((OTG->HFNUM >> 3) & 0x7FF);
}

/**
  \fn          void USBH_HS_IRQ (uint32_t gintsts)
  \brief       USB Host Interrupt Routine (IRQ).
*/
void USBH_HS_IRQ (uint32_t gintsts) {
  OTG_HS_HC *ptr_ch;
  uint8_t   *ptr_data_8;
  uint32_t  *ptr_data_32;
  uint32_t  *dfifo;
  uint32_t   hprt, haint, hcint, pktcnt, xfrsiz, mpsiz, hcchar, hcchar_upd;
  uint32_t   grxsts, bcnt, ch, dat, len, len_rest;
  uint8_t    signal;


  if (gintsts & OTG_HS_GINTSTS_HPRTINT) {         /* If host port interrupt   */
    hprt = OTG->HPRT;
    OTG->HPRT = hprt & (~OTG_HS_HPRT_PENA);       /* Leave PENA bit           */
    if (hprt  & OTG_HS_HPRT_PCDET) {              /* Port connect detected    */
      if (!port_reset) {                          /* If port not under reset  */
        signal_port_event(0, ARM_USBH_EVENT_CONNECT);
      }
    }
    if (hprt & OTG_HS_HPRT_PENCHNG) {             /* If port enable changed   */
      if (hprt & OTG_HS_HPRT_PENA) {              /* If device connected      */
        if (port_reset) {
          port_reset = false;
          signal_port_event(0, ARM_USBH_EVENT_RESET);
        }
      } 
    }
  }
  if (gintsts & OTG_HS_GINTSTS_DISCINT) {         /* If device disconnected   */
    OTG->GINTSTS = OTG_HS_GINTSTS_DISCINT;        /* Clear disconnect int     */
    if (!port_reset) {                            /* Ignore discon under reset*/
      for (ch = 0; ch < OTG_MAX_CH; ch++) {
        if (transfer_info[ch].status.active) {
          transfer_info[ch].status.active = 0;
        }
      }
      signal_port_event(0, ARM_USBH_EVENT_DISCONNECT);
    }
  }
                                                  /* Handle reception int     */
  if (gintsts & OTG_HS_GINTSTS_RXFLVL) {          /* If RXFIFO non-empty int  */
    OTG->GINTMSK &= ~OTG_HS_GINTMSK_RXFLVLM;
    grxsts = OTG->GRXSTSR;
    if (((grxsts >> 17) & 0x0F) == 0x02){         /* If PKTSTS = 0x02         */
      grxsts     = (OTG->GRXSTSP);
      ch         = (grxsts >> 0) & 0x00F;
      bcnt       = (grxsts >> 4) & 0x7FF;
      dfifo      = OTG_DFIFO[ch];
      ptr_data_32= (uint32_t *)(transfer_info[ch].data + transfer_info[ch].transferred);
      len        = bcnt / 4;                      /* Received number of 32-bit*/
      len_rest   = bcnt & 3;                      /* Number of bytes left     */
      while (len--) {
        *ptr_data_32++ = *dfifo;
      }
      ptr_data_8 = (uint8_t *)ptr_data_32;
      if (len_rest) {
        dat      = *dfifo;
        while (len_rest--) {
          *ptr_data_8++ = dat;
          dat  >>= 8;
        }
      }
      transfer_info[ch].transferred += bcnt;
    } else {                                      /* If PKTSTS != 0x02        */
      grxsts     = OTG->GRXSTSP;
    }
    OTG->GINTMSK |= OTG_HS_GINTMSK_RXFLVLM;
  }
                                                  /* Handle host ctrl int     */
  if (gintsts & OTG_HS_GINTSTS_HCINT) {           /* If host channel interrupt*/
    haint = OTG->HAINT;
    for (ch = 0; ch < OTG_MAX_CH; ch++) {
      if (!haint) break;
      if (haint & (1 << ch)) {                    /* If channels interrupt act*/
        haint     &= ~(1 << ch);
        signal     =   0;
        ptr_ch     =  (OTG_HS_HC *)(&OTG->HCCHAR0) + ch;
        hcint      =   ptr_ch->HCINT & ptr_ch->HCINTMSK;
        hcchar     =   ptr_ch->HCCHAR;
        hcchar_upd =   0;
        if (hcint & OTG_HS_HCINTx_NYET) {         /* If NYET event, XFRC will follow */
          ptr_ch->HCTSIZ &= ~OTG_HS_HCISIZx_DOPING;
          pktcnt = (ptr_ch->HCTSIZ >> 19) & 0x3FF;
          xfrsiz = (ptr_ch->HCTSIZ >>  0) & 0x7FFFF;
          mpsiz  = (ptr_ch->HCCHAR >>  0) & 0x7FF;
          if (xfrsiz > mpsiz)
            transfer_info[ch].transferred = (((transfer_info[ch].num + mpsiz - 1) / mpsiz) - pktcnt) * mpsiz;
          else 
            transfer_info[ch].transferred = xfrsiz;
          if (transfer_info[ch].num == transfer_info[ch].transferred) 
            transfer_info[ch].event = ARM_USBH_EVENT_HANDSHAKE_NYET;
        }
        if (hcint & OTG_HS_HCINTx_XFRC) {         /* If data transfer finished*/
          if (!transfer_info[ch].transferred)
            transfer_info[ch].transferred = transfer_info[ch].num;
          if (transfer_info[ch].event != ARM_USBH_EVENT_HANDSHAKE_NYET)
            transfer_info[ch].event = ARM_USBH_EVENT_TRANSFER_COMPLETE;
          goto halt_ch;
        } else if (hcint & OTG_HS_HCINTx_STALL) { /* If STALL event           */
          transfer_info[ch].event = ARM_USBH_EVENT_HANDSHAKE_STALL;
        } else if ((hcint & OTG_HS_HCINTx_NAK)   ||         /* If NAK received*/
                   (hcint & OTG_HS_HCINTx_TXERR) ||         /* If TXERR rece  */
                   (hcint & OTG_HS_HCINTx_BBERR) ||         /* If BBERR rece  */
                   (hcint & OTG_HS_HCINTx_DTERR)) {         /* If DTERR rece  */
                                                  /* Update transfer info     */
          if (hcint & OTG_HS_HCINTx_NAK) {
            /* On NAK, NAK is not returned to middle layer but transfer is      
               restarted from driver for remaining data                       */
            if (ptr_ch->HCCHAR & (1 << 15)) {               /* If endpoint IN */
              if (endpoint_info[ch].type == ARM_USB_ENDPOINT_INTERRUPT) {
                transfer_info[ch].status.in_progress = 0;
                goto halt_ch;
              } else {
                hcchar_upd |= hcchar | OTG_HS_HCCHARx_CHENA;
              }
            } else {                                        /* If endpoint OUT*/
              if (transfer_info[ch].packet == ARM_USBH_PACKET_PING) {
                hcchar_upd |= hcchar | OTG_HS_HCCHARx_CHENA;
              } else {
                pktcnt = (ptr_ch->HCTSIZ >> 19) & 0x3FF;
                mpsiz  = (ptr_ch->HCCHAR >>  0) & 0x7FF;
                transfer_info[ch].transferred = (((transfer_info[ch].num + mpsiz - 1) / mpsiz) - pktcnt) * mpsiz;
                goto halt_ch;
              }
            }
          } else {
            transfer_info[ch].event = ARM_USBH_EVENT_BUS_ERROR;
            goto halt_ch;
          }
        } else if (hcint & OTG_HS_HCINTx_CHH) {   /* If channel halted        */
                                                  /* Transfer is done here    */
          ptr_ch->HCINTMSK = 0;                   /* Mask all interrupts      */
          hcint = 0x7BB;                          /* Clear all interrupts     */
          transfer_info[ch].status.in_progress = 0;
          if (transfer_info[ch].event) {
            transfer_info[ch].status.active = 0;
            signal = 1;
          }
        } else if (hcint & OTG_HS_HCINTx_ACK) {             /* If ACK received*/
          /* On ACK, ACK is not an event that can be returned so when channel 
             is halted it will be signaled to middle layer if transfer is     
             completed otherwise transfer will be restarted for remaining     
             data                                                             */
          if (ptr_ch->HCCHAR & (1 << 15)) {                 /* If endpoint IN */
            if ((transfer_info[ch].num != transfer_info[ch].transferred) &&   /* If all data was not transferred  */
                (transfer_info[ch].transferred != 0)                     &&   /* If zero-length packet was not received */
               ((transfer_info[ch].transferred % endpoint_info[ch].max_packet_size) == 0)) {  /* If short packet was not received */
              hcchar_upd |= hcchar | OTG_HS_HCCHARx_CHENA;
            }
          } else if (transfer_info[ch].packet == ARM_USBH_PACKET_PING) {
            ptr_ch->HCTSIZ &= ~OTG_HS_HCISIZx_DOPING;
            transfer_info[ch].event = ARM_USBH_EVENT_TRANSFER_COMPLETE;
            goto halt_ch;
          }
        } else {
halt_ch:                                          /* Halt the channel         */
          ptr_ch->HCINTMSK = OTG_HS_HCINTx_CHH;
          hcchar_upd |= hcchar | OTG_HS_HCCHARx_CHENA | OTG_HS_HCCHARx_CHDIS;
        }
        ptr_ch->HCINT = hcint;
        if (signal)     signal_endpoint_event((ARM_USBH_EP_HANDLE)ptr_ch, transfer_info[ch].event);
        if (hcchar_upd) ptr_ch->HCCHAR = hcchar_upd;
      }
      ptr_ch++;
    }
  }

  /* Restart remainding transfers that were not completed due to NAK or ACK   */
  if (gintsts & OTG_HS_GINTSTS_SOF) {             /* If start of frame int    */
    OTG->GINTSTS = OTG_HS_GINTSTS_SOF;            /* Clear SOF interrupt      */
    for (ch = 0; ch < OTG_MAX_CH; ch++) {
      if (transfer_info[ch].status.active && !transfer_info[ch].status.in_progress) {
        if (endpoint_info[ch].type == ARM_USB_ENDPOINT_INTERRUPT) {
          if (transfer_info[ch].interval) {
            if (--transfer_info[ch].interval == 0) 
              transfer_info[ch].interval = endpoint_info[ch].interval_reload;
            else 
              continue;
          }
        }
        transfer_info[ch].status.in_progress = 1;
        USBH_HW_CH_TransferEnqueue (USBH_HW_CH_GetAddressFromIndex (ch), transfer_info[ch].packet, transfer_info[ch].data + transfer_info[ch].transferred, transfer_info[ch].num - transfer_info[ch].transferred);
      }
    }
  }
}

ARM_DRIVER_USBH Driver_USBH1 = {
  USBH_HW_GetVersion, 
  USBH_HW_GetCapabilities, 
  USBH_HW_Initialize, 
  USBH_HW_Uninitialize, 
  USBH_HW_PowerControl, 
  USBH_HW_PortVbusOnOff,
  USBH_HW_PortReset, 
  USBH_HW_PortSuspend, 
  USBH_HW_PortResume, 
  USBH_HW_PortGetState, 
  USBH_HW_EndpointCreate, 
  USBH_HW_EndpointModify, 
  USBH_HW_EndpointDelete, 
  USBH_HW_EndpointReset, 
  USBH_HW_EndpointTransfer, 
  USBH_HW_EndpointTransferGetResult, 
  USBH_HW_EndpointTransferAbort, 
  USBH_HW_GetFrameNumber
};
