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
 * $Date:        11. February 2014
 * $Revision:    V2.00
 *
 * Driver:       Driver_SPI1, Driver_SPI2, Driver_SPI3,
 *               Driver_SPI4, Driver_SPI5, Driver_SPI6
 * Configured:   via RTE_Device.h configuration file
 * Project:      SPI Driver for ST STM32F4xx
 * ----------------------------------------------------------------------
 * Use the following configuration settings in the middleware component
 * to connect to this driver.
 *
 *   Configuration Setting               Value     SPI Interface
 *   ---------------------               -----     -------------
 *   Connect to hardware via Driver_SPI# = 1       use SPI1
 *   Connect to hardware via Driver_SPI# = 2       use SPI2
 *   Connect to hardware via Driver_SPI# = 3       use SPI3
 *   Connect to hardware via Driver_SPI# = 4       use SPI4
 *   Connect to hardware via Driver_SPI# = 5       use SPI5
 *   Connect to hardware via Driver_SPI# = 6       use SPI6
 * -------------------------------------------------------------------- */

/* History:
 *  Version 2.00
 *    Updated to CMSIS Driver API V2.00
 *    Added SPI4 and SPI6
 *  Version 1.04
 *    Added SPI5
 *  Version 1.03
 *    Event send_data_event added to capabilities
 *    SPI IRQ handling corrected
 *  Version 1.02
 *    Based on API V1.10 (namespace prefix ARM_ added)
 *  Version 1.01
 *    Corrections for configuration without DMA
 *  Version 1.00
 *    Initial release
 */

#include "cmsis_os.h"
#include "stm32f4xx.h"

#include "stm32f4xx_gpio.h"
#include "SPI_STM32F4xx.h"
#include "DMA_STM32F4xx.h"

#include "Driver_SPI.h"

#include "RTE_Device.h"
#include "RTE_Components.h"

#define ARM_SPI_DRV_VERSION ARM_DRIVER_VERSION_MAJOR_MINOR(2,00)   /* driver version */

#if ((defined(RTE_Drivers_SPI1) || defined(RTE_Drivers_SPI2) || \
      defined(RTE_Drivers_SPI3) || defined(RTE_Drivers_SPI4) || \
      defined(RTE_Drivers_SPI5) || defined(RTE_Drivers_SPI6))&& \
      !RTE_SPI1 && !RTE_SPI2 && !RTE_SPI3 &&                    \
      !RTE_SPI4 && !RTE_SPI5 && !RTE_SPI6)
#error "SPI not configured in RTE_Device.h!"
#endif

#if (RTE_SPI1)
#if (  (RTE_SPI1_RX_DMA          != 0)                                     && \
     ( (RTE_SPI1_RX_DMA_NUMBER   != 2)                                     || \
      ((RTE_SPI1_RX_DMA_STREAM   != 0) && (RTE_SPI1_RX_DMA_STREAM  != 2))  || \
       (RTE_SPI1_RX_DMA_CHANNEL  != 3)                                     || \
      ((RTE_SPI1_RX_DMA_PRIORITY  < 0) || (RTE_SPI1_RX_DMA_PRIORITY > 3))))
#error "SPI1 Rx DMA configuration in RTE_Device.h is invalid!"
#endif
#if (  (RTE_SPI1_TX_DMA          != 0)                                     && \
     ( (RTE_SPI1_TX_DMA_NUMBER   != 2)                                     || \
      ((RTE_SPI1_TX_DMA_STREAM   != 3) && (RTE_SPI1_TX_DMA_STREAM  != 5))  || \
       (RTE_SPI1_TX_DMA_CHANNEL  != 3)                                     || \
      ((RTE_SPI1_TX_DMA_PRIORITY  < 0) || (RTE_SPI1_TX_DMA_PRIORITY > 3))))
#error "SPI1 Tx DMA configuration in RTE_Device.h is invalid!"
#endif
#endif

#if (RTE_SPI2)
#if (  (RTE_SPI2_RX_DMA          != 0)                                     && \
     ( (RTE_SPI2_RX_DMA_NUMBER   != 1)                                     || \
       (RTE_SPI2_RX_DMA_STREAM   != 3)                                     || \
       (RTE_SPI2_RX_DMA_CHANNEL  != 0)                                     || \
      ((RTE_SPI2_RX_DMA_PRIORITY  < 0) || (RTE_SPI2_RX_DMA_PRIORITY > 3))))
#error "SPI2 Rx DMA configuration in RTE_Device.h is invalid!"
#endif
#if (  (RTE_SPI2_TX_DMA          != 0)                                     && \
     ( (RTE_SPI2_TX_DMA_NUMBER   != 1)                                     || \
       (RTE_SPI2_TX_DMA_STREAM   != 4)                                     || \
       (RTE_SPI2_TX_DMA_CHANNEL  != 0)                                     || \
      ((RTE_SPI2_TX_DMA_PRIORITY  < 0) || (RTE_SPI2_TX_DMA_PRIORITY > 3))))
#error "SPI2 Tx DMA configuration in RTE_Device.h is invalid!"
#endif
#endif

#if (RTE_SPI3)
#if (  (RTE_SPI3_RX_DMA          != 0)                                     && \
     ( (RTE_SPI3_RX_DMA_NUMBER   != 1)                                     || \
      ((RTE_SPI3_RX_DMA_STREAM   != 0) && (RTE_SPI3_RX_DMA_STREAM  != 2))  || \
       (RTE_SPI3_RX_DMA_CHANNEL  != 0)                                     || \
      ((RTE_SPI3_RX_DMA_PRIORITY  < 0) || (RTE_SPI3_RX_DMA_PRIORITY > 3))))
#error "SPI3 Rx DMA configuration in RTE_Device.h is invalid!"
#endif
#if (  (RTE_SPI3_TX_DMA          != 0)                                     && \
     ( (RTE_SPI3_TX_DMA_NUMBER   != 1)                                     || \
      ((RTE_SPI3_TX_DMA_STREAM   != 5) && (RTE_SPI3_TX_DMA_STREAM  != 7))  || \
       (RTE_SPI3_TX_DMA_CHANNEL  != 0)                                     || \
      ((RTE_SPI3_TX_DMA_PRIORITY  < 0) || (RTE_SPI3_TX_DMA_PRIORITY > 3))))
#error "SPI3 Tx DMA configuration in RTE_Device.h is invalid!"
#endif
#endif

#if (RTE_SPI4)
#if !(defined (STM32F427_437xx) || defined (STM32F429_439xx))
#error "SPI4 is available only on STM32F427/437 and STM32F429/439 devices!"
#endif
#if (  (RTE_SPI4_RX_DMA          != 0)                                     && \
     ( (RTE_SPI4_RX_DMA_NUMBER   != 2)                                     || \
      ((RTE_SPI4_RX_DMA_STREAM   != 0) && (RTE_SPI4_RX_DMA_STREAM  != 3))  || \
      ((RTE_SPI4_RX_DMA_CHANNEL  != 4) && (RTE_SPI4_RX_DMA_CHANNEL != 5))  || \
      ((RTE_SPI4_RX_DMA_PRIORITY  < 0) || (RTE_SPI4_RX_DMA_PRIORITY > 3))))
#error "SPI4 Rx DMA configuration in RTE_Device.h is invalid!"
#endif
#if (  (RTE_SPI4_TX_DMA          != 0)                                     && \
     ( (RTE_SPI4_TX_DMA_NUMBER   != 2)                                     || \
      ((RTE_SPI4_TX_DMA_STREAM   != 1) && (RTE_SPI4_TX_DMA_STREAM  != 4))  || \
      ((RTE_SPI4_TX_DMA_CHANNEL  != 4) && (RTE_SPI4_TX_DMA_CHANNEL != 5))  || \
      ((RTE_SPI4_TX_DMA_PRIORITY  < 0) || (RTE_SPI4_TX_DMA_PRIORITY > 3))))
#error "SPI4 Tx DMA configuration in RTE_Device.h is invalid!"
#endif
#endif

#if (RTE_SPI5)
#if !(defined (STM32F427_437xx) || defined (STM32F429_439xx))
#error "SPI5 is available only on STM32F427/437 and STM32F429/439 devices!"
#endif
#if (  (RTE_SPI5_RX_DMA          != 0)                                     && \
     ( (RTE_SPI5_RX_DMA_NUMBER   != 2)                                     || \
      ((RTE_SPI5_RX_DMA_STREAM   != 3) && (RTE_SPI5_RX_DMA_STREAM  != 5))  || \
      ((RTE_SPI5_RX_DMA_CHANNEL  != 2) && (RTE_SPI5_RX_DMA_CHANNEL != 7))  || \
      ((RTE_SPI5_RX_DMA_PRIORITY  < 0) || (RTE_SPI5_RX_DMA_PRIORITY > 3))))
#error "SPI5 Rx DMA configuration in RTE_Device.h is invalid!"
#endif
#if (  (RTE_SPI5_TX_DMA          != 0)                                     && \
     ( (RTE_SPI5_TX_DMA_NUMBER   != 2)                                     || \
      ((RTE_SPI5_TX_DMA_STREAM   != 4) && (RTE_SPI5_TX_DMA_STREAM  != 6))  || \
      ((RTE_SPI5_TX_DMA_CHANNEL  != 2) && (RTE_SPI5_TX_DMA_CHANNEL != 7))  || \
      ((RTE_SPI5_TX_DMA_PRIORITY  < 0) || (RTE_SPI5_TX_DMA_PRIORITY > 3))))
#error "SPI5 Tx DMA configuration in RTE_Device.h is invalid!"
#endif
#endif

#if (RTE_SPI6)
#if !(defined (STM32F427_437xx) || defined (STM32F429_439xx))
#error "SPI6 is available only on STM32F427/437 and STM32F429/439 devices!"
#endif
#if (  (RTE_SPI6_RX_DMA          != 0)                                     && \
     ( (RTE_SPI6_RX_DMA_NUMBER   != 2)                                     || \
      ((RTE_SPI6_RX_DMA_STREAM   != 6)                                     || \
      ((RTE_SPI6_RX_DMA_CHANNEL  != 1)                                     || \
      ((RTE_SPI6_RX_DMA_PRIORITY  < 0) || (RTE_SPI6_RX_DMA_PRIORITY > 3))))
#error "SPI6 Rx DMA configuration in RTE_Device.h is invalid!"
#endif
#if (  (RTE_SPI6_TX_DMA          != 0)                                     && \
     ( (RTE_SPI6_TX_DMA_NUMBER   != 2)                                     || \
      ((RTE_SPI6_TX_DMA_STREAM   != 5)                                     || \
      ((RTE_SPI6_TX_DMA_CHANNEL  != 1)                                     || \
      ((RTE_SPI6_TX_DMA_PRIORITY  < 0) || (RTE_SPI6_TX_DMA_PRIORITY > 3))))
#error "SPI6 Tx DMA configuration in RTE_Device.h is invalid!"
#endif
#endif


/* Driver Version */
static const ARM_DRIVER_VERSION DriverVersion = {
  ARM_SPI_API_VERSION,
  ARM_SPI_DRV_VERSION
};

/* Driver Capabilities */
static const ARM_SPI_CAPABILITIES DriverCapabilities = {
  1,  /* Simplex Mode (Master and Slave) */
  1,  /* TI Synchronous Serial Interface */
  0,  /* Microwire Interface */
  1   /* Signal Mode Fault event: \ref ARM_SPI_EVENT_MODE_FAULT */
};


#if (RTE_SPI1)

/* SPI1 DMA */
#if (RTE_SPI1_RX_DMA && RTE_SPI1_TX_DMA)
static const SPI_DMA SPI1_DMA = {
  DMAx_STREAMy(RTE_SPI1_RX_DMA_NUMBER, RTE_SPI1_RX_DMA_STREAM),
  DMAx_STREAMy(RTE_SPI1_TX_DMA_NUMBER, RTE_SPI1_TX_DMA_STREAM),
  DMAx_STREAMy_IRQn(RTE_SPI1_RX_DMA_NUMBER, RTE_SPI1_RX_DMA_STREAM),
  DMAx_STREAMy_IRQn(RTE_SPI1_TX_DMA_NUMBER, RTE_SPI1_TX_DMA_STREAM),
  RTE_SPI1_RX_DMA_CHANNEL,
  RTE_SPI1_TX_DMA_CHANNEL,
  RTE_SPI1_RX_DMA_PRIORITY,
  RTE_SPI1_TX_DMA_PRIORITY
};
#endif

/* SPI1 Information (Run-Time) */
static SPI_INFO SPI1_Info;

/* SPI1 Resources */
static SPI_RESOURCES SPI1_Resources = {
  SPI1,
  {
  #if (RTE_SPI1_NSS_PIN)
    RTE_SPI1_NSS_PORT,
  #else
    NULL,
  #endif
    RTE_SPI1_SCL_PORT,
    RTE_SPI1_MISO_PORT,
    RTE_SPI1_MOSI_PORT,
    RTE_SPI1_NSS_BIT,
    RTE_SPI1_SCL_BIT,
    RTE_SPI1_MISO_BIT,
    RTE_SPI1_MOSI_BIT,
    GPIO_AF_SPI1,
  },
  SPI1_IRQn,
  RCC_APB2Periph_SPI1,
  RTE_PCLK2,
#if (RTE_SPI1_RX_DMA && RTE_SPI1_TX_DMA)
  &SPI1_DMA,
#else
  NULL,
#endif
  &SPI1_Info
};

#endif /* RTE_SPI1 */


#if (RTE_SPI2)

/* SPI 2 DMA */
#if (RTE_SPI2_RX_DMA && RTE_SPI2_TX_DMA)
static const SPI_DMA SPI2_DMA = {
  DMAx_STREAMy(RTE_SPI2_RX_DMA_NUMBER, RTE_SPI2_RX_DMA_STREAM),
  DMAx_STREAMy(RTE_SPI2_TX_DMA_NUMBER, RTE_SPI2_TX_DMA_STREAM),
  DMAx_STREAMy_IRQn(RTE_SPI2_RX_DMA_NUMBER, RTE_SPI2_RX_DMA_STREAM),
  DMAx_STREAMy_IRQn(RTE_SPI2_TX_DMA_NUMBER, RTE_SPI2_TX_DMA_STREAM),
  RTE_SPI2_RX_DMA_CHANNEL,
  RTE_SPI2_TX_DMA_CHANNEL,
  RTE_SPI2_RX_DMA_PRIORITY,
  RTE_SPI2_TX_DMA_PRIORITY
};
#endif

/* SPI2 Information (Run-Time) */
static SPI_INFO SPI2_Info;

/* SPI2 Resources */
static SPI_RESOURCES SPI2_Resources = {
  SPI2,
  {
  #if (RTE_SPI2_NSS_PIN)
    RTE_SPI2_NSS_PORT,
  #else
    NULL,
  #endif
    RTE_SPI2_SCL_PORT,
    RTE_SPI2_MISO_PORT,
    RTE_SPI2_MOSI_PORT,
    RTE_SPI2_NSS_BIT,
    RTE_SPI2_SCL_BIT,
    RTE_SPI2_MISO_BIT,
    RTE_SPI2_MOSI_BIT,
    GPIO_AF_SPI2,
  },
  SPI2_IRQn,
  RCC_APB1Periph_SPI2,
  RTE_PCLK1,
#if (RTE_SPI2_RX_DMA && RTE_SPI2_TX_DMA)
  &SPI2_DMA,
#else
  NULL,
#endif
  &SPI2_Info
};

#endif /* RTE_SPI2 */


#if (RTE_SPI3)

/* SPI DMA */
#if (RTE_SPI3_RX_DMA && RTE_SPI3_TX_DMA)
static const SPI_DMA SPI3_DMA = {
  DMAx_STREAMy(RTE_SPI3_RX_DMA_NUMBER, RTE_SPI3_RX_DMA_STREAM),
  DMAx_STREAMy(RTE_SPI3_TX_DMA_NUMBER, RTE_SPI3_TX_DMA_STREAM),
  DMAx_STREAMy_IRQn(RTE_SPI3_RX_DMA_NUMBER, RTE_SPI3_RX_DMA_STREAM),
  DMAx_STREAMy_IRQn(RTE_SPI3_TX_DMA_NUMBER, RTE_SPI3_TX_DMA_STREAM),
  RTE_SPI3_RX_DMA_CHANNEL,
  RTE_SPI3_TX_DMA_CHANNEL,
  RTE_SPI3_RX_DMA_PRIORITY,
  RTE_SPI3_TX_DMA_PRIORITY
};
#endif

/* SPI3 Information (Run-Time) */
static SPI_INFO SPI3_Info;

/* SPI3 Resources */
static SPI_RESOURCES SPI3_Resources = {
  SPI3,
  {
  #if (RTE_SPI3_NSS_PIN)
    RTE_SPI3_NSS_PORT,
  #else
    NULL,
  #endif
    RTE_SPI3_SCL_PORT,
    RTE_SPI3_MISO_PORT,
    RTE_SPI3_MOSI_PORT,
    RTE_SPI3_NSS_BIT,
    RTE_SPI3_SCL_BIT,
    RTE_SPI3_MISO_BIT,
    RTE_SPI3_MOSI_BIT,
    GPIO_AF_SPI3,
  },
  SPI3_IRQn,
  RCC_APB1Periph_SPI3,
  RTE_PCLK1,
#if (RTE_SPI3_RX_DMA && RTE_SPI3_TX_DMA)
  &SPI3_DMA,
#else
  NULL,
#endif
  &SPI3_Info
};

#endif /* RTE_SPI3 */


#if (RTE_SPI4)

/* SPI DMA */
#if (RTE_SPI4_RX_DMA && RTE_SPI4_TX_DMA)
static const SPI_DMA SPI4_DMA = {
  DMAx_STREAMy(RTE_SPI4_RX_DMA_NUMBER, RTE_SPI4_RX_DMA_STREAM),
  DMAx_STREAMy(RTE_SPI4_TX_DMA_NUMBER, RTE_SPI4_TX_DMA_STREAM),
  DMAx_STREAMy_IRQn(RTE_SPI4_RX_DMA_NUMBER, RTE_SPI4_RX_DMA_STREAM),
  DMAx_STREAMy_IRQn(RTE_SPI4_TX_DMA_NUMBER, RTE_SPI4_TX_DMA_STREAM),
  RTE_SPI4_RX_DMA_CHANNEL,
  RTE_SPI4_TX_DMA_CHANNEL,
  RTE_SPI4_RX_DMA_PRIORITY,
  RTE_SPI4_TX_DMA_PRIORITY
};
#endif

/* SPI4 Information (Run-Time) */
static SPI_INFO SPI4_Info;

/* SPI4 Resources */
static SPI_RESOURCES SPI4_Resources = {
  SPI4,
  {
  #if (RTE_SPI4_NSS_PIN)
    RTE_SPI4_NSS_PORT,
  #else
    NULL,
  #endif
    RTE_SPI4_SCL_PORT,
    RTE_SPI4_MISO_PORT,
    RTE_SPI4_MOSI_PORT,
    RTE_SPI4_NSS_BIT,
    RTE_SPI4_SCL_BIT,
    RTE_SPI4_MISO_BIT,
    RTE_SPI4_MOSI_BIT,
    GPIO_AF_SPI4,
  },
  SPI4_IRQn,
  RCC_APB2Periph_SPI4,
  RTE_PCLK1,
#if (RTE_SPI4_RX_DMA && RTE_SPI4_TX_DMA)
  &SPI4_DMA,
#else
  NULL,
#endif
  &SPI4_Info
};

#endif /* RTE_SPI4 */


#if (RTE_SPI5)

/* SPI DMA */
#if (RTE_SPI5_RX_DMA && RTE_SPI5_TX_DMA)
static const SPI_DMA SPI5_DMA = {
  DMAx_STREAMy(RTE_SPI5_RX_DMA_NUMBER, RTE_SPI5_RX_DMA_STREAM),
  DMAx_STREAMy(RTE_SPI5_TX_DMA_NUMBER, RTE_SPI5_TX_DMA_STREAM),
  DMAx_STREAMy_IRQn(RTE_SPI5_RX_DMA_NUMBER, RTE_SPI5_RX_DMA_STREAM),
  DMAx_STREAMy_IRQn(RTE_SPI5_TX_DMA_NUMBER, RTE_SPI5_TX_DMA_STREAM),
  RTE_SPI5_RX_DMA_CHANNEL,
  RTE_SPI5_TX_DMA_CHANNEL,
  RTE_SPI5_RX_DMA_PRIORITY,
  RTE_SPI5_TX_DMA_PRIORITY
};
#endif

/* SPI5 Information (Run-Time) */
static SPI_INFO SPI5_Info;

/* SPI5 Resources */
static SPI_RESOURCES SPI5_Resources = {
  SPI5,
  {
  #if (RTE_SPI5_NSS_PIN)
    RTE_SPI5_NSS_PORT,
  #else
    NULL,
  #endif
    RTE_SPI5_SCL_PORT,
    RTE_SPI5_MISO_PORT,
    RTE_SPI5_MOSI_PORT,
    RTE_SPI5_NSS_BIT,
    RTE_SPI5_SCL_BIT,
    RTE_SPI5_MISO_BIT,
    RTE_SPI5_MOSI_BIT,
    GPIO_AF_SPI5,
  },
  SPI5_IRQn,
  RCC_APB2Periph_SPI5,
  RTE_PCLK1,
#if (RTE_SPI5_RX_DMA && RTE_SPI5_TX_DMA)
  &SPI5_DMA,
#else
  NULL,
#endif
  &SPI5_Info
};

#endif /* RTE_SPI5 */


#if (RTE_SPI6)

/* SPI DMA */
#if (RTE_SPI6_RX_DMA && RTE_SPI6_TX_DMA)
static const SPI_DMA SPI6_DMA = {
  DMAx_STREAMy(RTE_SPI6_RX_DMA_NUMBER, RTE_SPI6_RX_DMA_STREAM),
  DMAx_STREAMy(RTE_SPI6_TX_DMA_NUMBER, RTE_SPI6_TX_DMA_STREAM),
  DMAx_STREAMy_IRQn(RTE_SPI6_RX_DMA_NUMBER, RTE_SPI6_RX_DMA_STREAM),
  DMAx_STREAMy_IRQn(RTE_SPI6_TX_DMA_NUMBER, RTE_SPI6_TX_DMA_STREAM),
  RTE_SPI6_RX_DMA_CHANNEL,
  RTE_SPI6_TX_DMA_CHANNEL,
  RTE_SPI6_RX_DMA_PRIORITY,
  RTE_SPI6_TX_DMA_PRIORITY
};
#endif

/* SPI6 Information (Run-Time) */
static SPI_INFO SPI6_Info;

/* SPI6 Resources */
static SPI_RESOURCES SPI6_Resources = {
  SPI6,
  {
  #if (RTE_SPI6_NSS_PIN)
    RTE_SPI6_NSS_PORT,
  #else
    NULL,
  #endif
    RTE_SPI6_SCL_PORT,
    RTE_SPI6_MISO_PORT,
    RTE_SPI6_MOSI_PORT,
    RTE_SPI6_NSS_BIT,
    RTE_SPI6_SCL_BIT,
    RTE_SPI6_MISO_BIT,
    RTE_SPI6_MOSI_BIT,
    GPIO_AF_SPI6,
  },
  SPI6_IRQn,
  RCC_APB2Periph_SPI6,
  RTE_PCLK1,
#if (RTE_SPI6_RX_DMA && RTE_SPI6_TX_DMA)
  &SPI6_DMA,
#else
  NULL,
#endif
  &SPI6_Info
};

#endif /* RTE_SPI6 */

/**
  \fn          uint32_t SPI_IORCCMask (GPIO_TypeDef *port)
  \brief       Determine AHB1 clock enable bit mask for given GPIO port
  \return      \ref RCC_AHB1_Peripherals bit mask for GPIO ports
*/
static uint32_t SPI_IORCCMask (GPIO_TypeDef *GPIOx) {
  uint32_t mask;

  if      (GPIOx == GPIOA) { mask = RCC_AHB1Periph_GPIOA; }
  else if (GPIOx == GPIOB) { mask = RCC_AHB1Periph_GPIOB; }
  else if (GPIOx == GPIOC) { mask = RCC_AHB1Periph_GPIOC; }
  else if (GPIOx == GPIOD) { mask = RCC_AHB1Periph_GPIOD; }
  else if (GPIOx == GPIOE) { mask = RCC_AHB1Periph_GPIOE; }
  else if (GPIOx == GPIOF) { mask = RCC_AHB1Periph_GPIOF; }
  else if (GPIOx == GPIOG) { mask = RCC_AHB1Periph_GPIOG; }
  else if (GPIOx == GPIOH) { mask = RCC_AHB1Periph_GPIOH; }
  else if (GPIOx == GPIOI) { mask = RCC_AHB1Periph_GPIOI; }
  else if (GPIOx == GPIOJ) { mask = RCC_AHB1Periph_GPIOJ; }
  else                     { mask = RCC_AHB1Periph_GPIOK; }
  return mask;
}


/**
  \fn          ARM_DRIVER_VERSION SPIX_GetVersion (void)
  \brief       Get SPI driver version.
  \return      \ref ARM_DRV_VERSION
*/
static ARM_DRIVER_VERSION SPIX_GetVersion (void) {
  return DriverVersion;
}


/**
  \fn          ARM_SPI_CAPABILITIES SPI_GetCapabilities (void)
  \brief       Get driver capabilities.
  \return      \ref ARM_SPI_CAPABILITIES
*/
static ARM_SPI_CAPABILITIES SPIX_GetCapabilities (void) {
  return DriverCapabilities;
}


/**
  \fn          int32_t SPI_Initialize (ARM_SPI_SignalEvent_t cb_event, SPI_RESOURCES *spi)
  \brief       Initialize SPI Interface.
  \param[in]   cb_event  Pointer to \ref ARM_SPI_SignalEvent
  \param[in]   spi       Pointer to SPI resources
  \return      \ref execution_status
*/
static int32_t SPI_Initialize (ARM_SPI_SignalEvent_t cb_event, SPI_RESOURCES *spi) {
  GPIO_InitTypeDef io_cfg;
  uint32_t arg;

  /* Initialize SPI Run-Time Resources */
  spi->info->cb_event = cb_event;
  spi->info->flags    = 0;

  spi->info->xfer.num     = 0;
  spi->info->xfer.rx_buf  = NULL;
  spi->info->xfer.tx_buf  = NULL;
  spi->info->xfer.rx_cnt  = 0;
  spi->info->xfer.tx_cnt  = 0;
  spi->info->xfer.def_val = 0;

  /* Configure SPI NSS pin (GPIO), Output set to high */
  if (spi->io.nss_port) {
    RCC_AHB1PeriphClockCmd (SPI_IORCCMask(spi->io.nss_port), ENABLE);
    GPIO_WriteBit (spi->io.nss_port, 1 << spi->io.nss_pin, Bit_SET);
    GPIO_PinAFConfig (spi->io.nss_port, spi->io.nss_pin, 0x00);

    io_cfg.GPIO_Pin   = 1 << spi->io.nss_pin;
    io_cfg.GPIO_Mode  = GPIO_Mode_OUT;
    io_cfg.GPIO_Speed = GPIO_High_Speed;
    io_cfg.GPIO_OType = GPIO_OType_PP;
    io_cfg.GPIO_PuPd  = GPIO_PuPd_UP;

    GPIO_Init (spi->io.nss_port, &io_cfg);
  }

  /* Configure SPI SCL pin */
  RCC_AHB1PeriphClockCmd (SPI_IORCCMask(spi->io.scl_port), ENABLE);
  GPIO_PinAFConfig (spi->io.scl_port, spi->io.scl_pin, spi->io.af);

  io_cfg.GPIO_Pin   = 1 << spi->io.scl_pin;
  io_cfg.GPIO_Mode  = GPIO_Mode_AF;
  io_cfg.GPIO_Speed = GPIO_High_Speed;
  io_cfg.GPIO_OType = GPIO_OType_PP;
  io_cfg.GPIO_PuPd  = GPIO_PuPd_UP;

  GPIO_Init (spi->io.scl_port, &io_cfg);

  /* Configure SPI MISO pin */
  RCC_AHB1PeriphClockCmd (SPI_IORCCMask(spi->io.miso_port), ENABLE);
  GPIO_PinAFConfig (spi->io.miso_port, spi->io.miso_pin, spi->io.af);

  io_cfg.GPIO_Pin   = 1 << spi->io.miso_pin;
  io_cfg.GPIO_Mode  = GPIO_Mode_AF;
  io_cfg.GPIO_Speed = GPIO_High_Speed;
  io_cfg.GPIO_OType = GPIO_OType_PP;
  io_cfg.GPIO_PuPd  = GPIO_PuPd_UP;

  GPIO_Init (spi->io.miso_port, &io_cfg);

  /* Configure SPI MOSI pin */
  RCC_AHB1PeriphClockCmd (SPI_IORCCMask(spi->io.mosi_port), ENABLE);
  GPIO_PinAFConfig (spi->io.mosi_port, spi->io.mosi_pin, spi->io.af);

  io_cfg.GPIO_Pin   = 1 << spi->io.mosi_pin;
  io_cfg.GPIO_Mode  = GPIO_Mode_AF;
  io_cfg.GPIO_Speed = GPIO_High_Speed;
  io_cfg.GPIO_OType = GPIO_OType_PP;
  io_cfg.GPIO_PuPd  = GPIO_PuPd_UP;

  GPIO_Init (spi->io.mosi_port, &io_cfg);

  /* Enable DMA clock */
  if (spi->dma) {
    if ((spi->reg == SPI2) || (spi->reg == SPI3)) {
      /* DMA1 used for SPI2 and SPI3 */
      arg = RCC_AHB1Periph_DMA1;
    }
    else {
      /* DMA2 used for SPI1, SPI4, SPI5 and SPI6 */
      arg = RCC_AHB1Periph_DMA2;
    }
    RCC_AHB1PeriphClockCmd (arg, ENABLE);
    NVIC_EnableIRQ (spi->dma->rx_irq_num);
    NVIC_EnableIRQ (spi->dma->tx_irq_num);
  }

  /* Enable SPI IRQ in NVIC */
  NVIC_EnableIRQ (spi->irq_num);

  spi->info->flags = SPI_INIT;

  return ARM_DRIVER_OK;
}

#if (RTE_SPI1)
static int32_t SPI1_Initialize (ARM_SPI_SignalEvent_t pSignalEvent) {
  return SPI_Initialize(pSignalEvent, &SPI1_Resources);
}
#endif

#if (RTE_SPI2)
static int32_t SPI2_Initialize (ARM_SPI_SignalEvent_t pSignalEvent) {
  return SPI_Initialize(pSignalEvent, &SPI2_Resources);
}
#endif

#if (RTE_SPI3)
static int32_t SPI3_Initialize (ARM_SPI_SignalEvent_t pSignalEvent) {
  return SPI_Initialize(pSignalEvent, &SPI3_Resources);
}
#endif

#if (RTE_SPI4)
static int32_t SPI4_Initialize (ARM_SPI_SignalEvent_t pSignalEvent) {
  return SPI_Initialize(pSignalEvent, &SPI4_Resources);
}
#endif

#if (RTE_SPI5)
static int32_t SPI5_Initialize (ARM_SPI_SignalEvent_t pSignalEvent) {
  return SPI_Initialize(pSignalEvent, &SPI5_Resources);
}
#endif

#if (RTE_SPI6)
static int32_t SPI6_Initialize (ARM_SPI_SignalEvent_t pSignalEvent) {
  return SPI_Initialize(pSignalEvent, &SPI6_Resources);
}
#endif


/**
  \fn          int32_t SPI_Uninitialize (SPI_RESOURCES *spi)
  \brief       De-initialize SPI Interface.
  \param[in]   spi  Pointer to SPI resources
  \return      \ref execution_status
*/
static int32_t SPI_Uninitialize (SPI_RESOURCES *spi) {
  GPIO_InitTypeDef io_cfg;

  /* Disable SPI IRQ in NVIC */
  NVIC_DisableIRQ (spi->irq_num);

  /* Disable SPI peripheral clock */
  if ((spi->reg == SPI2) || (spi->reg == SPI3)) {
    RCC_APB1PeriphClockCmd (spi->periph_bus, DISABLE);
  }
  else {
    RCC_APB2PeriphClockCmd (spi->periph_bus, DISABLE);
  }

  /* Uninitialize DMA Streams */
  if (spi->dma) {
    DMA_DeInit(spi->dma->rx_stream);
    DMA_DeInit(spi->dma->tx_stream);
  }

  /* Unconfigure SPI NSS pin (GPIO) */
  if (spi->io.nss_port) {
    GPIO_PinAFConfig (spi->io.nss_port, spi->io.nss_pin, 0x00);

    io_cfg.GPIO_Pin   = 1 << spi->io.nss_pin;
    io_cfg.GPIO_Mode  = GPIO_Mode_IN;
    io_cfg.GPIO_Speed = GPIO_Low_Speed;
    io_cfg.GPIO_OType = GPIO_OType_PP;
    io_cfg.GPIO_PuPd  = GPIO_PuPd_NOPULL;

    GPIO_Init (spi->io.nss_port, &io_cfg);
  }

  /* Unconfigure SPI SCL pin */
  GPIO_PinAFConfig (spi->io.scl_port, spi->io.scl_pin, 0x00);

  io_cfg.GPIO_Pin   = 1 << spi->io.scl_pin;
  io_cfg.GPIO_Mode  = GPIO_Mode_IN;
  io_cfg.GPIO_Speed = GPIO_Low_Speed;
  io_cfg.GPIO_OType = GPIO_OType_PP;
  io_cfg.GPIO_PuPd  = GPIO_PuPd_NOPULL;

  GPIO_Init (spi->io.scl_port, &io_cfg);

  /* Unconfigure SPI MISO pin */
  GPIO_PinAFConfig (spi->io.miso_port, spi->io.miso_pin, 0x00);

  io_cfg.GPIO_Pin   = 1 << spi->io.miso_pin;
  io_cfg.GPIO_Mode  = GPIO_Mode_IN;
  io_cfg.GPIO_Speed = GPIO_Low_Speed;
  io_cfg.GPIO_OType = GPIO_OType_PP;
  io_cfg.GPIO_PuPd  = GPIO_PuPd_NOPULL;

  GPIO_Init (spi->io.miso_port, &io_cfg);

  /* Unconfigure SPI MOSI pin */
  GPIO_PinAFConfig (spi->io.mosi_port, spi->io.mosi_pin, 0x00);

  io_cfg.GPIO_Pin   = 1 << spi->io.mosi_pin;
  io_cfg.GPIO_Mode  = GPIO_Mode_IN;
  io_cfg.GPIO_Speed = GPIO_Low_Speed;
  io_cfg.GPIO_OType = GPIO_OType_PP;
  io_cfg.GPIO_PuPd  = GPIO_PuPd_NOPULL;

  GPIO_Init (spi->io.mosi_port, &io_cfg);

  /* Reset SPI status */
  spi->info->flags = 0;

  return ARM_DRIVER_OK;
}

#if (RTE_SPI1)
static int32_t SPI1_Uninitialize (void) {
  return SPI_Uninitialize(&SPI1_Resources);
}
#endif

#if (RTE_SPI2)
static int32_t SPI2_Uninitialize (void) {
  return SPI_Uninitialize(&SPI2_Resources);
}
#endif

#if (RTE_SPI3)
static int32_t SPI3_Uninitialize (void) {
  return SPI_Uninitialize(&SPI3_Resources);
}
#endif

#if (RTE_SPI4)
static int32_t SPI4_Uninitialize (void) {
  return SPI_Uninitialize(&SPI4_Resources);
}
#endif

#if (RTE_SPI5)
static int32_t SPI5_Uninitialize (void) {
  return SPI_Uninitialize(&SPI5_Resources);
}
#endif

#if (RTE_SPI6)
static int32_t SPI6_Uninitialize (void) {
  return SPI_Uninitialize(&SPI6_Resources);
}
#endif


/**
  \fn          int32_t SPI_PowerControl (ARM_POWER_STATE state, SPI_RESOURCES *spi)
  \brief       Control SPI Interface Power.
  \param[in]   state  Power state
  \param[in]   spi    Pointer to SPI resources
  \return      \ref execution_status
*/
static int32_t SPI_PowerControl (ARM_POWER_STATE state, SPI_RESOURCES *spi) {
  if (!(spi->info->flags & SPI_INIT)) {
    /* Driver not initialized */
    return (ARM_DRIVER_ERROR);
  }

  switch (state) {
    case ARM_POWER_OFF:
      if (spi->info->flags & SPI_POWER) {
        spi->info->flags &= ~(SPI_POWER | SPI_SETUP);

        /* Disable SPI peripheral */
        SPI_Cmd (spi->reg, DISABLE);

        /* Disable SPI peripheral clock */
        if ((spi->reg == SPI2) || (spi->reg == SPI3)) {
          RCC_APB1PeriphClockCmd (spi->periph_bus, DISABLE);
        }
        else {
          RCC_APB2PeriphClockCmd (spi->periph_bus, DISABLE);
        }

        /* Disable DMA Streams */
        if (spi->dma) {
          DMA_DeInit(spi->dma->rx_stream);
          DMA_DeInit(spi->dma->tx_stream);
        }
      }
      break;

    case ARM_POWER_LOW:
      return ARM_DRIVER_ERROR_UNSUPPORTED;

    case ARM_POWER_FULL:
      if ((spi->reg == SPI2) || (spi->reg == SPI3)) {
        /* Reset SPI peripheral */
        RCC_APB1PeriphResetCmd (spi->periph_bus, ENABLE);
        RCC_APB1PeriphResetCmd (spi->periph_bus, DISABLE);
        /* Enable SPI peripheral clock */
        RCC_APB1PeriphClockCmd (spi->periph_bus, ENABLE);
      }
      else {
        /* Reset SPI peripheral */
        RCC_APB2PeriphResetCmd (spi->periph_bus, ENABLE);
        RCC_APB2PeriphResetCmd (spi->periph_bus, DISABLE);
        /* Enable SPI peripheral clock */
        RCC_APB2PeriphClockCmd (spi->periph_bus, ENABLE);
      }

      if (spi->dma) {
        /* Enable DMA Transfer complete and transfer error interrupts */
        DMA_ITConfig (spi->dma->rx_stream, DMA_IT_TC | DMA_IT_TE, ENABLE);
        DMA_ITConfig (spi->dma->tx_stream, DMA_IT_TC | DMA_IT_TE, ENABLE);
      }
      /* Enable error interrupt */
      SPI_I2S_ITConfig (spi->reg, SPI_I2S_IT_ERR, ENABLE);

      /* Ready for operation */
      spi->info->flags |= SPI_POWER;
      break;
  }

  return ARM_DRIVER_OK;
}

#if (RTE_SPI1)
static int32_t SPI1_PowerControl (ARM_POWER_STATE state) {
  return SPI_PowerControl(state, &SPI1_Resources);
}
#endif

#if (RTE_SPI2)
static int32_t SPI2_PowerControl (ARM_POWER_STATE state) {
  return SPI_PowerControl(state, &SPI2_Resources);
}
#endif

#if (RTE_SPI3)
static int32_t SPI3_PowerControl (ARM_POWER_STATE state) {
  return SPI_PowerControl(state, &SPI3_Resources);
}
#endif

#if (RTE_SPI4)
static int32_t SPI4_PowerControl (ARM_POWER_STATE state) {
  return SPI_PowerControl(state, &SPI4_Resources);
}
#endif

#if (RTE_SPI5)
static int32_t SPI5_PowerControl (ARM_POWER_STATE state) {
  return SPI_PowerControl(state, &SPI5_Resources);
}
#endif

#if (RTE_SPI6)
static int32_t SPI6_PowerControl (ARM_POWER_STATE state) {
  return SPI_PowerControl(state, &SPI6_Resources);
}
#endif

/**
  \fn          int32_t SPI_Send (const void *data, uint32_t num, SPI_RESOURCES *spi)
  \brief       Start sending data to SPI transmitter.
  \param[in]   data  Pointer to buffer with data to send to SPI transmitter
  \param[in]   num   Number of data items to send
  \param[in]   spi   Pointer to SPI resources
  \return      \ref execution_status
*/
static int32_t SPI_Send (const void *data, uint32_t num, SPI_RESOURCES *spi) {
  DMA_InitTypeDef dma_cfg;

  if ((data == NULL) || (num == 0)) return ARM_DRIVER_ERROR_PARAMETER;

  if (!(spi->info->flags & SPI_SETUP)) {
    /* SPI not configured (Init + Power + Control) */
    return ARM_DRIVER_ERROR;
  }

  if (spi->reg->SR & SPI_SR_BSY) {
    /* Communication ongoing */
    return ARM_DRIVER_ERROR_BUSY;
  }

  if (spi->reg->CR1 & SPI_CR1_BIDIMODE) {
    /* Simplex mode, select transmit-only mode */
    spi->reg->CR1 |= SPI_CR1_BIDIOE;
  }

  spi->info->xfer.rx_buf = NULL;
  spi->info->xfer.tx_buf = (uint8_t *)data;

  if (spi->dma) {
    /* Configure DMA receive stream */
    dma_cfg.DMA_Channel            = spi->dma->rx_channel << DMA_CHANNEL_POS;
    dma_cfg.DMA_PeripheralBaseAddr = (uint32_t)&spi->reg->DR;
    dma_cfg.DMA_Memory0BaseAddr    = (uint32_t)&spi->info->xfer.dump_val;
    dma_cfg.DMA_DIR                = DMA_DIR_PeripheralToMemory;
    dma_cfg.DMA_BufferSize         = num;
    dma_cfg.DMA_PeripheralInc      = DMA_PeripheralInc_Disable;
    dma_cfg.DMA_MemoryInc          = DMA_MemoryInc_Disable;
    dma_cfg.DMA_PeripheralDataSize = (spi->reg->CR1 & SPI_CR1_DFF) ? (DMA_PeripheralDataSize_HalfWord) : (DMA_PeripheralDataSize_Byte);
    dma_cfg.DMA_MemoryDataSize     = (spi->reg->CR1 & SPI_CR1_DFF) ? (DMA_MemoryDataSize_HalfWord) : (DMA_MemoryDataSize_Byte);
    dma_cfg.DMA_Mode               = DMA_Mode_Normal;
    dma_cfg.DMA_Priority           = spi->dma->rx_priority << DMA_PRIORITY_POS;
    dma_cfg.DMA_FIFOMode           = DMA_FIFOMode_Disable;
    dma_cfg.DMA_FIFOThreshold      = DMA_FIFOThreshold_Full;
    dma_cfg.DMA_MemoryBurst        = DMA_MemoryBurst_Single;
    dma_cfg.DMA_PeripheralBurst    = DMA_PeripheralBurst_Single;

    DMA_Init (spi->dma->rx_stream, &dma_cfg);

    /* Configure DMA transmit stream */
    dma_cfg.DMA_Channel           = spi->dma->tx_channel << DMA_CHANNEL_POS;
    dma_cfg.DMA_Memory0BaseAddr   = (uint32_t)data;
    dma_cfg.DMA_DIR               = DMA_DIR_MemoryToPeripheral;
    dma_cfg.DMA_MemoryInc         = DMA_MemoryInc_Enable;
    dma_cfg.DMA_Priority          = spi->dma->tx_priority << DMA_PRIORITY_POS;

    DMA_Init (spi->dma->tx_stream, &dma_cfg);

    /* Configure flow controller */
    DMA_FlowControllerConfig (spi->dma->rx_stream, DMA_FlowCtrl_Peripheral);
    DMA_FlowControllerConfig (spi->dma->tx_stream, DMA_FlowCtrl_Peripheral);

    /* Enable DMA streams */
    if (!(spi->reg->CR1 & SPI_CR1_BIDIMODE)) {
      /* Full duplex mode */
      DMA_Cmd (spi->dma->rx_stream, ENABLE);
      SPI_I2S_DMACmd (spi->reg, SPI_I2S_DMAReq_Rx, ENABLE);
    }
    DMA_Cmd (spi->dma->tx_stream, ENABLE);
    SPI_I2S_DMACmd (spi->reg, SPI_I2S_DMAReq_Tx, ENABLE);
  }
  else {
    spi->info->xfer.num    = num;
    spi->info->xfer.rx_cnt = 0;
    spi->info->xfer.tx_cnt = 0;

    if (!(spi->reg->CR1 & SPI_CR1_BIDIMODE)) {
      /* Full duplex mode */
      SPI_I2S_ITConfig (spi->reg, SPI_I2S_IT_RXNE, ENABLE);
    }
    SPI_I2S_ITConfig (spi->reg, SPI_I2S_IT_TXE, ENABLE);
  }

  return ARM_DRIVER_OK;
}

#if (RTE_SPI1)
static int32_t SPI1_Send (const void *buf, uint32_t num) {
  return SPI_Send(buf, num, &SPI1_Resources);
}
#endif

#if (RTE_SPI2)
static int32_t SPI2_Send (const void *buf, uint32_t num) {
  return SPI_Send(buf, num, &SPI2_Resources);
}
#endif

#if (RTE_SPI3)
static int32_t SPI3_Send (const void *buf, uint32_t num) {
  return SPI_Send(buf, num, &SPI3_Resources);
}
#endif

#if (RTE_SPI4)
static int32_t SPI4_Send (const void *buf, uint32_t num) {
  return SPI_Send(buf, num, &SPI4_Resources);
}
#endif

#if (RTE_SPI5)
static int32_t SPI5_Send (const void *buf, uint32_t num) {
  return SPI_Send(buf, num, &SPI5_Resources);
}
#endif

#if (RTE_SPI6)
static int32_t SPI6_Send (const void *buf, uint32_t num) {
  return SPI_Send(buf, num, &SPI6_Resources);
}
#endif


/**
  \fn          int32_t SPI_Receive (void *data, uint32_t num, SPI_RESOURCES *spi)
  \brief       Start receiving data from SPI receiver.
  \param[out]  data  Pointer to buffer for data to receive from SPI receiver
  \param[in]   num   Number of data items to receive
  \param[in]   spi   Pointer to SPI resources
  \return      \ref execution_status
*/
static int32_t SPI_Receive (void *data, uint32_t num, SPI_RESOURCES *spi) {
  DMA_InitTypeDef dma_cfg;

  if ((data == NULL) || (num == 0)) { return ARM_DRIVER_ERROR_PARAMETER; }

  if (!(spi->info->flags & SPI_SETUP)) {
    /* SPI not configured (Init + Power + Control) */
    return ARM_DRIVER_ERROR;
  }

  if (spi->reg->SR & SPI_SR_BSY) {
    /* Communication ongoing */
    return ARM_DRIVER_ERROR_BUSY;
  }

  if (spi->reg->CR1 & SPI_CR1_BIDIMODE) {
    /* Simplex mode, select receive-only mode */
    spi->reg->CR1 &= ~SPI_CR1_BIDIOE;
  }

  spi->info->xfer.rx_buf = (uint8_t *)data;
  spi->info->xfer.tx_buf = NULL;

  if (spi->dma) {
    /* Configure DMA receive stream */
    dma_cfg.DMA_Channel            = spi->dma->rx_channel << DMA_CHANNEL_POS;
    dma_cfg.DMA_PeripheralBaseAddr = (uint32_t)&spi->reg->DR;
    dma_cfg.DMA_Memory0BaseAddr    = (uint32_t)data;
    dma_cfg.DMA_DIR                = DMA_DIR_PeripheralToMemory;
    dma_cfg.DMA_BufferSize         = num;
    dma_cfg.DMA_PeripheralInc      = DMA_PeripheralInc_Disable;
    dma_cfg.DMA_MemoryInc          = DMA_MemoryInc_Enable;
    dma_cfg.DMA_PeripheralDataSize = (spi->reg->CR1 & SPI_CR1_DFF) ? (DMA_PeripheralDataSize_HalfWord) : (DMA_PeripheralDataSize_Byte);
    dma_cfg.DMA_MemoryDataSize     = (spi->reg->CR1 & SPI_CR1_DFF) ? (DMA_MemoryDataSize_HalfWord) : (DMA_MemoryDataSize_Byte);
    dma_cfg.DMA_Mode               = DMA_Mode_Normal;
    dma_cfg.DMA_Priority           = spi->dma->rx_priority << DMA_PRIORITY_POS;
    dma_cfg.DMA_FIFOMode           = DMA_FIFOMode_Disable;
    dma_cfg.DMA_FIFOThreshold      = DMA_FIFOThreshold_1QuarterFull;
    dma_cfg.DMA_MemoryBurst        = DMA_MemoryBurst_Single;
    dma_cfg.DMA_PeripheralBurst    = DMA_PeripheralBurst_Single;

    DMA_Init (spi->dma->rx_stream, &dma_cfg);

    /* Configure DMA transmit stream */
    dma_cfg.DMA_Channel           = spi->dma->tx_channel << DMA_CHANNEL_POS;
    dma_cfg.DMA_Memory0BaseAddr   = spi->info->xfer.def_val;
    dma_cfg.DMA_DIR               = DMA_DIR_MemoryToPeripheral;
    dma_cfg.DMA_MemoryInc         = DMA_MemoryInc_Disable;
    dma_cfg.DMA_Priority          = spi->dma->tx_priority << DMA_PRIORITY_POS;

    DMA_Init (spi->dma->tx_stream, &dma_cfg);

    /* Set data counter */
    DMA_SetCurrDataCounter (spi->dma->rx_stream, (uint16_t)num);
    DMA_SetCurrDataCounter (spi->dma->tx_stream, (uint16_t)num);

    /* Configure flow controller */
    DMA_FlowControllerConfig (spi->dma->rx_stream, DMA_FlowCtrl_Peripheral);
    DMA_FlowControllerConfig (spi->dma->tx_stream, DMA_FlowCtrl_Peripheral);

    /* Enable DMA streams */
    DMA_Cmd (spi->dma->rx_stream, ENABLE);
    SPI_I2S_DMACmd (spi->reg, SPI_I2S_DMAReq_Rx, ENABLE);
    if (!(spi->reg->CR1 & SPI_CR1_BIDIMODE)) {
      /* Full duplex mode */
      DMA_Cmd (spi->dma->tx_stream, ENABLE);
      SPI_I2S_DMACmd (spi->reg, SPI_I2S_DMAReq_Tx, ENABLE);
    }
  }
  else {
    spi->info->xfer.num    = num;
    spi->info->xfer.rx_cnt = 0;
    spi->info->xfer.tx_cnt = 0;

    SPI_I2S_ITConfig (spi->reg, SPI_I2S_IT_RXNE, ENABLE);
    if (!(spi->reg->CR1 & SPI_CR1_BIDIMODE)) {
      /* Full duplex mode */
      SPI_I2S_ITConfig (spi->reg, SPI_I2S_IT_TXE, ENABLE);
    }
  }

  return ARM_DRIVER_OK;
}

#if (RTE_SPI1)
static int32_t SPI1_Receive (void *buf, uint32_t num) {
  return SPI_Receive(buf, num, &SPI1_Resources);
}
#endif

#if (RTE_SPI2)
static int32_t SPI2_Receive (void *buf, uint32_t num) {
  return SPI_Receive(buf, num, &SPI2_Resources);
}
#endif

#if (RTE_SPI3)
static int32_t SPI3_Receive (void *buf, uint32_t num) {
  return SPI_Receive(buf, num, &SPI3_Resources);
}
#endif

#if (RTE_SPI4)
static int32_t SPI4_Receive (void *buf, uint32_t num) {
  return SPI_Receive(buf, num, &SPI4_Resources);
}
#endif

#if (RTE_SPI5)
static int32_t SPI5_Receive (void *buf, uint32_t num) {
  return SPI_Receive(buf, num, &SPI5_Resources);
}
#endif

#if (RTE_SPI6)
static int32_t SPI6_Receive (void *buf, uint32_t num) {
  return SPI_Receive(buf, num, &SPI6_Resources);
}
#endif


/**
  \fn          int32_t SPI_Transfer (const void *data_out, void *data_in, uint32_t num, SPI_RESOURCES *spi)
  \brief       Start sending/receiving data to/from SPI transmitter/receiver.
  \param[in]   data_out  Pointer to buffer with data to send to SPI transmitter
  \param[out]  data_in   Pointer to buffer for data to receive from SPI receiver
  \param[in]   num       Number of data items to transfer
  \param[in]   spi       Pointer to SPI resources
  \return      \ref execution_status
*/
static int32_t SPI_Transfer (const void *data_out, void *data_in, uint32_t num, SPI_RESOURCES *spi) {
  DMA_InitTypeDef dma_cfg;

  if ((data_out == NULL) || (data_in == NULL) || (num == 0)) { return ARM_DRIVER_ERROR_PARAMETER; }

  if (!(spi->info->flags & SPI_SETUP)) {
    /* SPI not configured (Init + Power + Control) */
    return ARM_DRIVER_ERROR;
  }

  if (spi->reg->CR1 & SPI_CR1_BIDIMODE) {
    /* Simplex mode */
    return ARM_SPI_ERROR_MODE;
  }

  if (spi->reg->SR & SPI_SR_BSY) {
    /* Communication ongoing */
    return ARM_DRIVER_ERROR_BUSY;
  }

  spi->info->xfer.rx_buf = (uint8_t *)data_in;
  spi->info->xfer.tx_buf = (uint8_t *)data_out;

  if (spi->dma) {
    /* Configure DMA receive stream */
    dma_cfg.DMA_Channel            = spi->dma->rx_channel << DMA_CHANNEL_POS;
    dma_cfg.DMA_PeripheralBaseAddr = (uint32_t)&spi->reg->DR;
    dma_cfg.DMA_Memory0BaseAddr    = (uint32_t)data_in;
    dma_cfg.DMA_DIR                = DMA_DIR_PeripheralToMemory;
    dma_cfg.DMA_BufferSize         = num;
    dma_cfg.DMA_PeripheralInc      = DMA_PeripheralInc_Disable;
    dma_cfg.DMA_MemoryInc          = DMA_MemoryInc_Enable;
    dma_cfg.DMA_PeripheralDataSize = (spi->reg->CR1 & SPI_CR1_DFF) ? (DMA_PeripheralDataSize_HalfWord) : (DMA_PeripheralDataSize_Byte);
    dma_cfg.DMA_MemoryDataSize     = (spi->reg->CR1 & SPI_CR1_DFF) ? (DMA_MemoryDataSize_HalfWord) : (DMA_MemoryDataSize_Byte);
    dma_cfg.DMA_Mode               = DMA_Mode_Normal;
    dma_cfg.DMA_Priority           = spi->dma->rx_priority << DMA_PRIORITY_POS;
    dma_cfg.DMA_FIFOMode           = DMA_FIFOMode_Disable;
    dma_cfg.DMA_FIFOThreshold      = DMA_FIFOThreshold_1QuarterFull;
    dma_cfg.DMA_MemoryBurst        = DMA_MemoryBurst_Single;
    dma_cfg.DMA_PeripheralBurst    = DMA_PeripheralBurst_Single;

    DMA_Init (spi->dma->rx_stream, &dma_cfg);

    /* Configure DMA transmit stream */
    dma_cfg.DMA_Channel           = spi->dma->tx_channel << DMA_CHANNEL_POS;
    dma_cfg.DMA_Memory0BaseAddr   = (uint32_t)data_out;
    dma_cfg.DMA_DIR               = DMA_DIR_MemoryToPeripheral;
    dma_cfg.DMA_MemoryInc         = DMA_MemoryInc_Enable;
    dma_cfg.DMA_Priority          = spi->dma->tx_priority << DMA_PRIORITY_POS;

    DMA_Init (spi->dma->tx_stream, &dma_cfg);

    /* Set data counter */
    DMA_SetCurrDataCounter (spi->dma->rx_stream, (uint16_t)num);
    DMA_SetCurrDataCounter (spi->dma->tx_stream, (uint16_t)num);

    /* Configure flow controller */
    DMA_FlowControllerConfig (spi->dma->rx_stream, DMA_FlowCtrl_Peripheral);
    DMA_FlowControllerConfig (spi->dma->tx_stream, DMA_FlowCtrl_Peripheral);

    /* Enable DMA streams */
    DMA_Cmd (spi->dma->rx_stream, ENABLE);
    SPI_I2S_DMACmd (spi->reg, SPI_I2S_DMAReq_Tx, ENABLE);

    DMA_Cmd (spi->dma->tx_stream, ENABLE);
    SPI_I2S_DMACmd (spi->reg, SPI_I2S_DMAReq_Rx, ENABLE);
  }
  else {
    spi->info->xfer.num    = num;
    spi->info->xfer.rx_cnt = 0;
    spi->info->xfer.tx_cnt = 0;

    SPI_I2S_ITConfig (spi->reg, SPI_I2S_IT_RXNE, ENABLE);
    SPI_I2S_ITConfig (spi->reg, SPI_I2S_IT_TXE, ENABLE);
  }

  return ARM_DRIVER_OK;
}

#if (RTE_SPI1)
static int32_t SPI1_Transfer (const void *data_out, void *data_in, uint32_t num) {
  return SPI_Transfer (data_out, data_in, num, &SPI1_Resources);
}
#endif

#if (RTE_SPI2)
static int32_t SPI2_Transfer (const void *data_out, void *data_in, uint32_t num) {
  return SPI_Transfer (data_out, data_in, num, &SPI2_Resources);
}
#endif

#if (RTE_SPI3)
static int32_t SPI3_Transfer (const void *data_out, void *data_in, uint32_t num) {
  return SPI_Transfer (data_out, data_in, num, &SPI3_Resources);
}
#endif

#if (RTE_SPI4)
static int32_t SPI4_Transfer (const void *data_out, void *data_in, uint32_t num) {
  return SPI_Transfer (data_out, data_in, num, &SPI4_Resources);
}
#endif

#if (RTE_SPI5)
static int32_t SPI5_Transfer (const void *data_out, void *data_in, uint32_t num) {
  return SPI_Transfer (data_out, data_in, num, &SPI5_Resources);
}
#endif

#if (RTE_SPI6)
static int32_t SPI6_Transfer (const void *data_out, void *data_in, uint32_t num) {
  return SPI_Transfer (data_out, data_in, num, &SPI6_Resources);
}
#endif


/**
  \fn          uint32_t SPI_GetDataCount (SPI_RESOURCES *spi)
  \brief       Get transferred data count.
  \param[in]   spi  Pointer to SPI resources
  \return      number of data items transferred
*/
static uint32_t SPI_GetDataCount (SPI_RESOURCES *spi) {
  uint32_t cnt;
  SPI_TRANSFER_INFO *tr = &spi->info->xfer;

  if (!(spi->info->flags & SPI_SETUP)) {
    /* SPI not configured (Init + Power + Control) */
    return 0;
  }

  if (tr->rx_buf == NULL) {
    /* Send operation */
    cnt = (spi->dma) ? (DMA_GetCurrDataCounter(spi->dma->tx_stream)) : (tr->tx_cnt);
  }
  else {
    /* Receive or Transfer operation */
    cnt = (spi->dma) ? (DMA_GetCurrDataCounter(spi->dma->rx_stream)) : (tr->rx_cnt);
  }

  return (cnt);
}

#if (RTE_SPI1)
static uint32_t SPI1_GetDataCount (void) {
  return SPI_GetDataCount(&SPI1_Resources);
}
#endif

#if (RTE_SPI2)
static uint32_t SPI2_GetDataCount (void) {
  return SPI_GetDataCount(&SPI2_Resources);
}
#endif

#if (RTE_SPI3)
static uint32_t SPI3_GetDataCount (void) {
  return SPI_GetDataCount(&SPI3_Resources);
}
#endif

#if (RTE_SPI4)
static uint32_t SPI4_GetDataCount (void) {
  return SPI_GetDataCount(&SPI4_Resources);
}
#endif

#if (RTE_SPI5)
static uint32_t SPI5_GetDataCount (void) {
  return SPI_GetDataCount(&SPI5_Resources);
}
#endif

#if (RTE_SPI6)
static uint32_t SPI6_GetDataCount (void) {
  return SPI_GetDataCount(&SPI6_Resources);
}
#endif


/**
  \fn          int32_t SPI_Control (uint32_t control, uint32_t arg, SPI_RESOURCES *spi)
  \brief       Control SPI Interface.
  \param[in]   control  operation
  \param[in]   arg      argument of operation (optional)
  \param[in]   spi      Pointer to SPI resources
  \return      \ref execution_status
*/
static int32_t SPI_Control (uint32_t control, uint32_t arg, SPI_RESOURCES *spi) {
  bool mode = false;
  uint32_t code, n;

  if (!(spi->info->flags & SPI_POWER)) {
    /* SPI not powered */
    return ARM_DRIVER_ERROR;
  }

  code = control & ARM_SPI_CONTROL_Msk;

  if (code == ARM_SPI_CONTROL_SS) {
    /* Slave Select Line Control */
    if ((spi->info->config.SPI_NSS == SPI_NSS_Soft) && (spi->io.nss_port)) {
      /* Slave select is software controled */
      GPIO_WriteBit (spi->io.nss_port,
                     1 << spi->io.nss_pin,
                    (arg == ARM_SPI_SS_ACTIVE) ? Bit_RESET : Bit_SET);
    }
    else return (ARM_DRIVER_ERROR);
  }
  else {
    switch (code) {
      case ARM_SPI_MODE_INACTIVE:
        /* Disable SPI */
        SPI_Cmd (spi->reg, DISABLE);
        break;
      case ARM_SPI_MODE_MASTER:
        spi->info->config.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
        spi->info->config.SPI_Mode      = SPI_Mode_Master;
        mode = true;
        break;
      case ARM_SPI_MODE_SLAVE:
        spi->info->config.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
        spi->info->config.SPI_Mode      = SPI_Mode_Slave;
        mode = true;
        break;
      case ARM_SPI_MODE_MASTER_SIMPLEX:
        spi->info->config.SPI_Direction = SPI_Direction_1Line_Tx;
        spi->info->config.SPI_Mode      = SPI_Mode_Master;
        mode = true;
        break;
      case ARM_SPI_MODE_SLAVE_SIMPLEX:
        spi->info->config.SPI_Direction = SPI_Direction_1Line_Rx;
        spi->info->config.SPI_Mode      = SPI_Mode_Slave;
        mode = true;
        break;

      case ARM_SPI_SET_BUS_SPEED:
        /* Set SPI Bus Speed */
        for (n = 1; n < 8; n++) {
          if (arg >= (spi->periph_clock >> n)) break;
        }
        spi->info->config.SPI_BaudRatePrescaler = (n - 1) << 3;
        /* Update prescaler manually */
        spi->reg->CR1 &= ~SPI_CR1_SPE;
        spi->reg->CR1 |= (n - 1) << 3;
        spi->reg->CR1 |=  SPI_CR1_SPE;
        break;

      case ARM_SPI_GET_BUS_SPEED:
        /* Return current bus speed */
        return (spi->periph_clock >> ((spi->reg->CR1 & SPI_CR1_BR) >> 3));

      case ARM_SPI_SET_DEFAULT_TX_VALUE:
        /* Set default transfer value */
          spi->info->xfer.def_val = (uint16_t)(arg & 0xFFFF);
        break;

      case ARM_SPI_ABORT_TRANSFER:
        /* Disable SPI peripheral */
        spi->reg->CR1 &= ~SPI_CR1_SPE;
        /* Disable interrupts and DMA requests */
        spi->reg->CR2 &= ~(SPI_CR2_TXEIE   | SPI_CR2_RXNEIE |
                           SPI_CR2_TXDMAEN | SPI_CR2_RXDMAEN);

        /* Disable DMA Streams */
        if (spi->dma) {
          DMA_DeInit(spi->dma->rx_stream);
          DMA_DeInit(spi->dma->tx_stream);
        }

        /* Enable SPI peripheral */
        spi->reg->CR1 |=  SPI_CR1_SPE;
        /* Update status */
        break;

      default:
        return ARM_DRIVER_ERROR_UNSUPPORTED;
    }

    if (mode) {
      /* Set SPI Bus Speed */
      for (n = 1; n < 8; n++) {
        if (arg >= (spi->periph_clock >> n)) break;
      }
      spi->info->config.SPI_BaudRatePrescaler = (n - 1) << 3;

      /* Configure Frame Format */
      code = control & ARM_SPI_FRAME_FORMAT_Msk;

      if (code == ARM_SPI_TI_SSI) {
        SPI_TIModeCmd (spi->reg, ENABLE);
      }
      else {
        SPI_TIModeCmd (spi->reg, DISABLE);

        switch (code) {
          case ARM_SPI_CPOL0_CPHA0:
            spi->info->config.SPI_CPOL = SPI_CPOL_Low;
            spi->info->config.SPI_CPHA = SPI_CPHA_1Edge;
            break;
          case ARM_SPI_CPOL0_CPHA1:
            spi->info->config.SPI_CPOL = SPI_CPOL_Low;
            spi->info->config.SPI_CPHA = SPI_CPHA_2Edge;
            break;
          case ARM_SPI_CPOL1_CPHA0:
            spi->info->config.SPI_CPOL = SPI_CPOL_High;
            spi->info->config.SPI_CPHA = SPI_CPHA_1Edge;
            break;
          case ARM_SPI_CPOL1_CPHA1:
            spi->info->config.SPI_CPOL = SPI_CPOL_High;
            spi->info->config.SPI_CPHA = SPI_CPHA_2Edge;
            break;

          default:
            return ARM_DRIVER_ERROR_UNSUPPORTED;
        }
      }

      /* Configure Number of Data Bits */
      n = control & ARM_SPI_DATA_BITS_Msk;

      if (n == ARM_SPI_DATA_BITS(8)) {
        spi->info->config.SPI_DataSize = SPI_DataSize_8b;
      }
      else if (n == ARM_SPI_DATA_BITS(16)) {
        spi->info->config.SPI_DataSize = SPI_DataSize_16b;
      }
      else {
        return ARM_DRIVER_ERROR_UNSUPPORTED;
      }

      /* Configure Bit Order */
      code = control & ARM_SPI_BIT_ORDER_Msk;

      if (code == ARM_SPI_MSB_LSB) {
        spi->info->config.SPI_FirstBit = SPI_FirstBit_MSB;
      }
      else /* if (code == ARM_SPI_LSB_MSB) */ {
        spi->info->config.SPI_FirstBit = SPI_FirstBit_LSB;
      }

      /* Configure Slave Select Mode */
      if (spi->info->config.SPI_Mode == SPI_Mode_Master) {
        /* SPI MASTER mode */
        code = control & ARM_SPI_SS_MASTER_MODE_Msk;

        if (code == ARM_SPI_SS_MASTER_UNUSED) {
          /* Slave select is unused */
          SPI_SSOutputCmd (spi->reg, DISABLE);
        }
        else if (code == ARM_SPI_SS_MASTER_SW) {
          /* Slave select is software controlled */
          spi->info->config.SPI_NSS = SPI_NSS_Soft;
        }
        else {
          /* Slave select is hardware controlled */
          spi->info->config.SPI_NSS = SPI_NSS_Hard;

          if (code & ARM_SPI_SS_MASTER_HW_OUTPUT) {
            SPI_SSOutputCmd (spi->reg, ENABLE);
          }
          else /* if (code & ARM_SPI_SS_MASTER_HW_INPUT) */ {
            SPI_SSOutputCmd (spi->reg, DISABLE);
          }
        }
      }
      else {
        /* SPI SLAVE mode */
        code = control & ARM_SPI_SS_SLAVE_MODE_Msk;

        if (code == ARM_SPI_SS_SLAVE_HW) {
          spi->info->config.SPI_NSS = SPI_NSS_Hard;
        }
        else /* if (code == ARM_SPI_SS_SLAVE_SW) */ {
          spi->info->config.SPI_NSS = SPI_NSS_Soft;
        }

        SPI_SSOutputCmd (spi->reg, DISABLE);
      }
      
      /* Apply configuration */
      SPI_Cmd (spi->reg, DISABLE);
      SPI_Init (spi->reg, &spi->info->config);
      SPI_Cmd (spi->reg, ENABLE);

      /* State: configured */
      spi->info->flags |= SPI_SETUP;
    }
  }
  return (ARM_DRIVER_OK);
}

#if (RTE_SPI1)
static int32_t SPI1_Control (uint32_t control, uint32_t arg) {
  return SPI_Control(control, arg, &SPI1_Resources);
}
#endif

#if (RTE_SPI2)
static int32_t SPI2_Control (uint32_t control, uint32_t arg) {
  return SPI_Control(control, arg, &SPI2_Resources);
}
#endif

#if (RTE_SPI3)
static int32_t SPI3_Control (uint32_t control, uint32_t arg) {
  return SPI_Control(control, arg, &SPI3_Resources);
}
#endif

#if (RTE_SPI4)
static int32_t SPI4_Control (uint32_t control, uint32_t arg) {
  return SPI_Control(control, arg, &SPI4_Resources);
}
#endif

#if (RTE_SPI5)
static int32_t SPI5_Control (uint32_t control, uint32_t arg) {
  return SPI_Control(control, arg, &SPI5_Resources);
}
#endif

#if (RTE_SPI6)
static int32_t SPI6_Control (uint32_t control, uint32_t arg) {
  return SPI_Control(control, arg, &SPI6_Resources);
}
#endif


/**
  \fn          ARM_SPI_STATUS SPI_GetStatus (SPI_RESOURCES *spi)
  \brief       Get SPI status.
  \param[in]   spi  Pointer to SPI resources
  \return      SPI status \ref ARM_SPI_STATUS
*/
static ARM_SPI_STATUS SPI_GetStatus (SPI_RESOURCES *spi) {
  ARM_SPI_STATUS s;

  spi->info->status.tx_rx_busy = (spi->reg->SR & SPI_SR_BSY) != 0;
  s = spi->info->status;

  spi->info->status.data_lost  = 0;
  spi->info->status.mode_fault = 0;

  return (s);
}

#if (RTE_SPI1)
static ARM_SPI_STATUS SPI1_GetStatus (void) {
  return SPI_GetStatus(&SPI1_Resources);
}
#endif

#if (RTE_SPI2)
static ARM_SPI_STATUS SPI2_GetStatus (void) {
  return SPI_GetStatus(&SPI2_Resources);
}
#endif

#if (RTE_SPI3)
static ARM_SPI_STATUS SPI3_GetStatus (void) {
  return SPI_GetStatus(&SPI3_Resources);
}
#endif

#if (RTE_SPI4)
static ARM_SPI_STATUS SPI4_GetStatus (void) {
  return SPI_GetStatus(&SPI4_Resources);
}
#endif

#if (RTE_SPI5)
static ARM_SPI_STATUS SPI5_GetStatus (void) {
  return SPI_GetStatus(&SPI5_Resources);
}
#endif

#if (RTE_SPI6)
static ARM_SPI_STATUS SPI6_GetStatus (void) {
  return SPI_GetStatus(&SPI6_Resources);
}
#endif


/**
  \fn          void SPI_IRQHandler (SPI_RESOURCES *spi)
  \brief       SPI Interrupt handler.
  \param[in]   spi  Pointer to SPI resources
*/
static void SPI_IRQHandler (SPI_RESOURCES *spi) {
  SPI_TRANSFER_INFO *tr = &spi->info->xfer;
  uint16_t data;

  if (SPI_I2S_GetITStatus (spi->reg, SPI_I2S_IT_OVR) == SET) {
    /* Overrun flag is set */
    spi->info->status.data_lost = 1;
    if (spi->info->cb_event) {
       (spi->info->cb_event)(ARM_SPI_EVENT_DATA_LOST);
    }
  }
  if (SPI_I2S_GetITStatus (spi->reg, SPI_IT_MODF) == SET) {
    /* Mode fault flag is set */
    spi->info->status.mode_fault = 1;
    if (spi->info->cb_event) {
       (spi->info->cb_event)(ARM_SPI_EVENT_MODE_FAULT);
    }
  }

  if (SPI_I2S_GetITStatus (spi->reg, SPI_I2S_IT_TXE) == SET) {
    /* Transmit Buffer Empty */

    if (tr->num) {
      if (tr->tx_buf) {
        /* Send buffered data */
        data = *(tr->tx_buf++);
        if (spi->reg->CR1 & SPI_CR1_DFF) {
          /* 16-bit data frame format */
          data |= *(tr->tx_buf++) << 8;
        }
      }
      else {
        /* Send default transfer value */
        data = tr->def_val;
      }

      SPI_I2S_SendData (spi->reg, data);

      tr->tx_cnt++;

      if (tr->tx_cnt == tr->num) {
        /* All data sent, disable Tx Buffer Empty Interrupt */
        SPI_I2S_ITConfig (spi->reg, SPI_I2S_IT_TXE, DISABLE);

        if (spi->reg->CR1 & SPI_CR1_BIDIMODE) {
          /* Simplex mode */
          if (spi->info->xfer.rx_buf == NULL) {
            /* Send completed */
            if (spi->info->cb_event) {
               (spi->info->cb_event)(ARM_SPI_EVENT_TRANSFER_COMPLETE);
            }
          }
        }
      }
    }
    else {
      /* Unexpected transfer, data lost */
      if (spi->info->cb_event) {
         (spi->info->cb_event)(ARM_SPI_EVENT_DATA_LOST);
      }
    }
  }

  if (SPI_I2S_GetITStatus (spi->reg, SPI_I2S_IT_RXNE) == SET) {
    /* Receive Buffer Not Empty */
    data = SPI_I2S_ReceiveData (spi->reg);

    if (tr->num) {
      if (tr->rx_buf) {
        /* Put data into buffer */
        *(tr->rx_buf++) = (uint8_t)data;
        if (spi->reg->CR1 & SPI_CR1_DFF) {
          *(tr->rx_buf++) = (uint8_t)(data >> 8);
        }
      }
      tr->rx_cnt++;

      if (tr->rx_cnt == tr->num) {
        /* All data received */
        tr->num = 0;
        /* Disable Rx Buffer Not Empty Interrupt */
        SPI_I2S_ITConfig (spi->reg, SPI_I2S_IT_RXNE, DISABLE);
        NVIC_ClearPendingIRQ (spi->irq_num);
        /* Release buffer to the application */
        if (spi->info->cb_event) {
           (spi->info->cb_event)(ARM_SPI_EVENT_TRANSFER_COMPLETE);
        }
      }
    }
    else {
      /* Unexpected transfer, data lost */
      if (spi->info->cb_event) {
         (spi->info->cb_event)(ARM_SPI_EVENT_DATA_LOST);
      }
    }
  }
}

#if (RTE_SPI1)
void SPI1_IRQHandler (void) {
  SPI_IRQHandler(&SPI1_Resources);
}
#endif

#if (RTE_SPI2)
void SPI2_IRQHandler (void) {
  SPI_IRQHandler(&SPI2_Resources);
}
#endif

#if (RTE_SPI3)
void SPI3_IRQHandler (void) {
  SPI_IRQHandler(&SPI3_Resources);
 }
#endif

#if (RTE_SPI4)
void SPI4_IRQHandler (void) {
  SPI_IRQHandler(&SPI4_Resources);
 }
#endif

#if (RTE_SPI5)
void SPI5_IRQHandler (void) {
  SPI_IRQHandler(&SPI5_Resources);
 }
#endif

#if (RTE_SPI6)
void SPI6_IRQHandler (void) {
  SPI_IRQHandler(&SPI6_Resources);
 }
#endif


/* DMA Callbacks */

#if ((RTE_SPI1 && RTE_SPI1_TX_DMA) || (RTE_SPI2 && RTE_SPI2_TX_DMA) || \
     (RTE_SPI3 && RTE_SPI3_TX_DMA) || (RTE_SPI4 && RTE_SPI4_TX_DMA) || \
     (RTE_SPI5 && RTE_SPI5_TX_DMA) || (RTE_SPI6 && RTE_SPI6_TX_DMA))
/**
  \fn          void SPI_DMA_TxComplete (SPI_RESOURCES *spi)
  \brief       DMA transmit complete callback.
  \param[in]   spi  Pointer to SPI resources
*/
static void SPI_DMA_TxComplete (SPI_RESOURCES *spi) {
  SPI_I2S_DMACmd (spi->reg, SPI_I2S_DMAReq_Tx, DISABLE);

  if (spi->reg->CR1 & SPI_CR1_BIDIMODE) {
    /* Simplex mode */
    if (spi->info->xfer.rx_buf == NULL) {
      /* Send completed */
      if (spi->info->cb_event) {
         (spi->info->cb_event)(ARM_SPI_EVENT_TRANSFER_COMPLETE);
      }
    }
  }
}
#endif

#if ((RTE_SPI1 && RTE_SPI1_RX_DMA) || (RTE_SPI2 && RTE_SPI2_RX_DMA) || \
     (RTE_SPI3 && RTE_SPI3_RX_DMA) || (RTE_SPI4 && RTE_SPI4_RX_DMA) || \
     (RTE_SPI5 && RTE_SPI5_RX_DMA) || (RTE_SPI6 && RTE_SPI6_RX_DMA))
/**
  \fn          void SPI_DMA_RxComplete (SPI_RESOURCES *spi)
  \brief       DMA receive complete callback.
  \param[in]   spi  Pointer to SPI resources
*/
static void SPI_DMA_RxComplete (SPI_RESOURCES *spi) {
  SPI_I2S_DMACmd (spi->reg, SPI_I2S_DMAReq_Rx, DISABLE);

  if (spi->info->cb_event) {
     (spi->info->cb_event)(ARM_SPI_EVENT_TRANSFER_COMPLETE);
  }
}
#endif


#if (RTE_SPI1 && RTE_SPI1_TX_DMA && RTE_SPI1_RX_DMA)
void DMAx_STREAMy_EVENT(RTE_SPI1_TX_DMA_NUMBER, RTE_SPI1_TX_DMA_STREAM) (uint32_t event) {
  SPI_DMA_TxComplete(&SPI1_Resources);
}
void DMAx_STREAMy_EVENT(RTE_SPI1_RX_DMA_NUMBER, RTE_SPI1_RX_DMA_STREAM) (uint32_t event) {
  SPI_DMA_RxComplete(&SPI1_Resources);
}
#endif

#if (RTE_SPI2 && RTE_SPI2_TX_DMA && RTE_SPI2_RX_DMA)
void DMAx_STREAMy_EVENT(RTE_SPI2_TX_DMA_NUMBER, RTE_SPI2_TX_DMA_STREAM) (uint32_t event) {
  SPI_DMA_TxComplete(&SPI2_Resources);
}
void DMAx_STREAMy_EVENT(RTE_SPI2_RX_DMA_NUMBER, RTE_SPI2_RX_DMA_STREAM) (uint32_t event) {
  SPI_DMA_RxComplete(&SPI2_Resources);
}
#endif

#if (RTE_SPI3 && RTE_SPI3_TX_DMA && RTE_SPI3_RX_DMA)
void DMAx_STREAMy_EVENT(RTE_SPI3_TX_DMA_NUMBER, RTE_SPI3_TX_DMA_STREAM) (uint32_t event) {
  SPI_DMA_TxComplete(&SPI3_Resources);
}
void DMAx_STREAMy_EVENT(RTE_SPI3_RX_DMA_NUMBER, RTE_SPI3_RX_DMA_STREAM) (uint32_t event) {
  SPI_DMA_RxComplete(&SPI3_Resources);
}
#endif

#if (RTE_SPI4 && RTE_SPI4_TX_DMA && RTE_SPI4_RX_DMA)
void DMAx_STREAMy_EVENT(RTE_SPI4_TX_DMA_NUMBER, RTE_SPI4_TX_DMA_STREAM) (uint32_t event) {
  SPI_DMA_TxComplete(&SPI4_Resources);
}
void DMAx_STREAMy_EVENT(RTE_SPI4_RX_DMA_NUMBER, RTE_SPI4_RX_DMA_STREAM) (uint32_t event) {
  SPI_DMA_RxComplete(&SPI4_Resources);
}
#endif

#if (RTE_SPI5 && RTE_SPI5_TX_DMA && RTE_SPI5_RX_DMA)
void DMAx_STREAMy_EVENT(RTE_SPI5_TX_DMA_NUMBER, RTE_SPI5_TX_DMA_STREAM) (uint32_t event) {
  SPI_DMA_TxComplete(&SPI5_Resources);
}
void DMAx_STREAMy_EVENT(RTE_SPI5_RX_DMA_NUMBER, RTE_SPI5_RX_DMA_STREAM) (uint32_t event) {
  SPI_DMA_RxComplete(&SPI5_Resources);
}
#endif

#if (RTE_SPI6 && RTE_SPI6_TX_DMA && RTE_SPI6_RX_DMA)
void DMAx_STREAMy_EVENT(RTE_SPI6_TX_DMA_NUMBER, RTE_SPI6_TX_DMA_STREAM) (uint32_t event) {
  SPI_DMA_TxComplete(&SPI6_Resources);
}
void DMAx_STREAMy_EVENT(RTE_SPI6_RX_DMA_NUMBER, RTE_SPI6_RX_DMA_STREAM) (uint32_t event) {
  SPI_DMA_RxComplete(&SPI6_Resources);
}
#endif


/* SPI1 Driver Control Block */
#if (RTE_SPI1)
ARM_DRIVER_SPI Driver_SPI1 = {
  SPIX_GetVersion,
  SPIX_GetCapabilities,
  SPI1_Initialize,
  SPI1_Uninitialize,
  SPI1_PowerControl,
  SPI1_Send,
  SPI1_Receive,
  SPI1_Transfer,
  SPI1_GetDataCount,
  SPI1_Control,
  SPI1_GetStatus
};
#endif

/* SPI2 Driver Control Block */
#if (RTE_SPI2)
ARM_DRIVER_SPI Driver_SPI2 = {
  SPIX_GetVersion,
  SPIX_GetCapabilities,
  SPI2_Initialize,
  SPI2_Uninitialize,
  SPI2_PowerControl,
  SPI2_Send,
  SPI2_Receive,
  SPI2_Transfer,
  SPI2_GetDataCount,
  SPI2_Control,
  SPI2_GetStatus
};
#endif

/* SPI3 Driver Control Block */
#if (RTE_SPI3)
ARM_DRIVER_SPI Driver_SPI3 = {
  SPIX_GetVersion,
  SPIX_GetCapabilities,
  SPI3_Initialize,
  SPI3_Uninitialize,
  SPI3_PowerControl,
  SPI3_Send,
  SPI3_Receive,
  SPI3_Transfer,
  SPI3_GetDataCount,
  SPI3_Control,
  SPI3_GetStatus
};
#endif

/* SPI4 Driver Control Block */
#if (RTE_SPI4)
ARM_DRIVER_SPI Driver_SPI4 = {
  SPIX_GetVersion,
  SPIX_GetCapabilities,
  SPI4_Initialize,
  SPI4_Uninitialize,
  SPI4_PowerControl,
  SPI4_Send,
  SPI4_Receive,
  SPI4_Transfer,
  SPI4_GetDataCount,
  SPI4_Control,
  SPI4_GetStatus
};
#endif

/* SPI5 Driver Control Block */
#if (RTE_SPI5)
ARM_DRIVER_SPI Driver_SPI5 = {
  SPIX_GetVersion,
  SPIX_GetCapabilities,
  SPI5_Initialize,
  SPI5_Uninitialize,
  SPI5_PowerControl,
  SPI5_Send,
  SPI5_Receive,
  SPI5_Transfer,
  SPI5_GetDataCount,
  SPI5_Control,
  SPI5_GetStatus
};
#endif

/* SPI6 Driver Control Block */
#if (RTE_SPI6)
ARM_DRIVER_SPI Driver_SPI6 = {
  SPIX_GetVersion,
  SPIX_GetCapabilities,
  SPI6_Initialize,
  SPI6_Uninitialize,
  SPI6_PowerControl,
  SPI6_Send,
  SPI6_Receive,
  SPI6_Transfer,
  SPI6_GetDataCount,
  SPI6_Control,
  SPI6_GetStatus
};
#endif
