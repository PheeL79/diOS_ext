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
 * Driver:       Driver_I2C1, Driver_I2C2, Driver_I2C3
 * Configured:   via RTE_Device.h configuration file 
 * Project:      I2C Driver for ST STM32F4xx
 * ---------------------------------------------------------------------- 
 * Use the following configuration settings in the middleware component
 * to connect to this driver.
 * 
 *   Configuration Setting               Value     I2C Interface
 *   ---------------------               -----     -------------
 *   Connect to hardware via Driver_I2C# = 1       use I2C1
 *   Connect to hardware via Driver_I2C# = 2       use I2C2
 *   Connect to hardware via Driver_I2C# = 3       use I2C3
 * -------------------------------------------------------------------- */

/* History:
 *  Version 2.00
 *    Updated to the CMSIS Driver API V2.00
 *  Version 1.02
 *    Bugfix (corrected I2C register access)
 *  Version 1.01
 *    Based on API V1.10 (namespace prefix ARM_ added)
 *  Version 1.00
 *    Initial release
 */ 

#include <string.h>

#include "cmsis_os.h"
#include "stm32f4xx.h"

#include "I2C_STM32F4xx.h"
#include "DMA_STM32F4xx.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_rcc.h"

#include "Driver_I2C.h"

#include "RTE_Device.h"
#include "RTE_Components.h"

#define ARM_I2C_DRV_VERSION ARM_DRIVER_VERSION_MAJOR_MINOR(2,00)    /* driver version */

#if ((defined(RTE_Drivers_I2C1) || \
      defined(RTE_Drivers_I2C2) || \
      defined(RTE_Drivers_I2C3))   \
     && !RTE_I2C1                  \
     && !RTE_I2C2                  \
     && !RTE_I2C3)
#error "I2C not configured in RTE_Device.h!"
#endif

#if (RTE_I2C1)
#if (  (RTE_I2C1_RX_DMA          != 0)                                     && \
     ( (RTE_I2C1_RX_DMA_NUMBER   != 1)                                     || \
      ((RTE_I2C1_RX_DMA_STREAM   != 0) && (RTE_I2C1_RX_DMA_STREAM  != 5))  || \
       (RTE_I2C1_RX_DMA_CHANNEL  != 1)                                     || \
      ((RTE_I2C1_RX_DMA_PRIORITY  < 0) || (RTE_I2C1_RX_DMA_PRIORITY > 3))))
#error "I2C1 Rx DMA configuration in RTE_Device.h is invalid!"
#endif
#if (  (RTE_I2C1_TX_DMA          != 0)                                     && \
     ( (RTE_I2C1_TX_DMA_NUMBER   != 1)                                     || \
      ((RTE_I2C1_TX_DMA_STREAM   != 6) && (RTE_I2C1_TX_DMA_STREAM  != 7))  || \
       (RTE_I2C1_TX_DMA_CHANNEL  != 1)                                     || \
      ((RTE_I2C1_TX_DMA_PRIORITY  < 0) || (RTE_I2C1_TX_DMA_PRIORITY > 3))))
#error "I2C1 Tx DMA configuration in RTE_Device.h is invalid!"
#endif
#endif

#if (RTE_I2C2)
#if (  (RTE_I2C2_RX_DMA          != 0)                                     && \
     ( (RTE_I2C2_RX_DMA_NUMBER   != 1)                                     || \
      ((RTE_I2C2_RX_DMA_STREAM   != 2) && (RTE_I2C2_RX_DMA_STREAM  != 2))  || \
       (RTE_I2C2_RX_DMA_CHANNEL  != 7)                                     || \
      ((RTE_I2C2_RX_DMA_PRIORITY  < 0) || (RTE_I2C2_RX_DMA_PRIORITY > 3))))
#error "I2C2 Rx DMA configuration in RTE_Device.h is invalid!"
#endif
#if (  (RTE_I2C2_TX_DMA          != 0)                                     && \
     ( (RTE_I2C2_TX_DMA_NUMBER   != 1)                                     || \
       (RTE_I2C2_TX_DMA_STREAM   != 7)                                     || \
       (RTE_I2C2_TX_DMA_CHANNEL  != 7)                                     || \
      ((RTE_I2C2_TX_DMA_PRIORITY  < 0) || (RTE_I2C2_TX_DMA_PRIORITY > 3))))
#error "I2C2 Tx DMA configuration in RTE_Device.h is invalid!"
#endif
#endif

#if (RTE_I2C3)
#if (  (RTE_I2C3_RX_DMA          != 0)                                     && \
     ( (RTE_I2C3_RX_DMA_NUMBER   != 1)                                     || \
       (RTE_I2C3_RX_DMA_STREAM   != 2)                                     || \
       (RTE_I2C3_RX_DMA_CHANNEL  != 3)                                     || \
      ((RTE_I2C3_RX_DMA_PRIORITY  < 0) || (RTE_I2C3_RX_DMA_PRIORITY > 3))))
#error "I2C3 Rx DMA configuration in RTE_Device.h is invalid!"
#endif
#if (  (RTE_I2C3_TX_DMA          != 0)                                     && \
     ( (RTE_I2C3_TX_DMA_NUMBER   != 1)                                     || \
       (RTE_I2C3_TX_DMA_STREAM   != 4)                                     || \
       (RTE_I2C3_TX_DMA_CHANNEL  != 3)                                     || \
      ((RTE_I2C3_TX_DMA_PRIORITY  < 0) || (RTE_I2C3_TX_DMA_PRIORITY > 3))))
#error "I2C3 Tx DMA configuration in RTE_Device.h is invalid!"
#endif
#endif


#define GPIO_AF_I2C             GPIO_AF_I2C1        /* same for all I2C */

#define I2C_BUSY_TIMEOUT            25  /* I2C bus busy wait timeout in us        */
#define I2C_BUS_CLEAR_CLOCK_PERIOD  10  /* I2C bus clock period in us             */


/* Driver Version */
static const ARM_DRIVER_VERSION DriverVersion = {
  ARM_I2C_API_VERSION,
  ARM_I2C_DRV_VERSION
};

/* Driver Capabilities */
static const ARM_I2C_CAPABILITIES DriverCapabilities = { 0 };


#if (RTE_I2C1)

/* I2C1 DMA */
#if (RTE_I2C1_RX_DMA && RTE_I2C1_TX_DMA)
static const I2C_DMA I2C1_DMA = {
  DMAx_STREAMy(RTE_I2C1_RX_DMA_NUMBER, RTE_I2C1_RX_DMA_STREAM),
  DMAx_STREAMy(RTE_I2C1_TX_DMA_NUMBER, RTE_I2C1_TX_DMA_STREAM),
  DMAx_STREAMy_IRQn(RTE_I2C1_RX_DMA_NUMBER, RTE_I2C1_RX_DMA_STREAM),
  DMAx_STREAMy_IRQn(RTE_I2C1_TX_DMA_NUMBER, RTE_I2C1_TX_DMA_STREAM),
  RTE_I2C1_RX_DMA_CHANNEL,
  RTE_I2C1_TX_DMA_CHANNEL,
  RTE_I2C1_RX_DMA_PRIORITY,
  RTE_I2C1_TX_DMA_PRIORITY
};
#endif

/* I2C1 Information (Run-Time) */
static I2C_INFO I2C1_Info;

/* I2C1 Resources */
static I2C_RESOURCES I2C1_Resources = {
  I2C1,
  {
    RTE_I2C1_SCL_PORT,
    RTE_I2C1_SDA_PORT,
    RTE_I2C1_SCL_BIT,
    RTE_I2C1_SDA_BIT
  },
  I2C1_EV_IRQn,
  I2C1_ER_IRQn,
  RCC_APB1Periph_I2C1,
  0,
#if (RTE_I2C1_RX_DMA && RTE_I2C1_TX_DMA)
  &I2C1_DMA,
#else
  NULL,
#endif
  &I2C1_Info
};

#endif /* RTE_I2C1 */


#if (RTE_I2C2)

/* I2C2 DMA */
#if (RTE_I2C2_RX_DMA && RTE_I2C2_TX_DMA)
static const I2C_DMA I2C2_DMA = {
  DMAx_STREAMy(RTE_I2C2_RX_DMA_NUMBER, RTE_I2C2_RX_DMA_STREAM),
  DMAx_STREAMy(RTE_I2C2_TX_DMA_NUMBER, RTE_I2C2_TX_DMA_STREAM),
  DMAx_STREAMy_IRQn(RTE_I2C2_RX_DMA_NUMBER, RTE_I2C2_RX_DMA_STREAM),
  DMAx_STREAMy_IRQn(RTE_I2C2_TX_DMA_NUMBER, RTE_I2C2_TX_DMA_STREAM),
  RTE_I2C2_RX_DMA_CHANNEL,
  RTE_I2C2_TX_DMA_CHANNEL,
  RTE_I2C2_RX_DMA_PRIORITY,
  RTE_I2C2_TX_DMA_PRIORITY
};
#endif

/* I2C2 Information (Run-Time) */
static I2C_INFO I2C2_Info;

/* I2C2 Resources */
static I2C_RESOURCES I2C2_Resources = {
  I2C2,
  {
    RTE_I2C2_SCL_PORT,
    RTE_I2C2_SDA_PORT,
    RTE_I2C2_SCL_BIT,
    RTE_I2C2_SDA_BIT
  },
  I2C2_EV_IRQn,
  I2C2_ER_IRQn,
  RCC_APB1Periph_I2C2,
  0,
#if (RTE_I2C2_RX_DMA && RTE_I2C2_TX_DMA)
  &I2C2_DMA,
#else
  NULL,
#endif
  &I2C2_Info
};

#endif /* RTE_I2C2 */


#if (RTE_I2C3)

/* I2C3 DMA */
#if (RTE_I2C3_RX_DMA && RTE_I2C3_TX_DMA)
static const I2C_DMA I2C3_DMA = {
  DMAx_STREAMy(RTE_I2C3_RX_DMA_NUMBER, RTE_I2C3_RX_DMA_STREAM),
  DMAx_STREAMy(RTE_I2C3_TX_DMA_NUMBER, RTE_I2C3_TX_DMA_STREAM),
  DMAx_STREAMy_IRQn(RTE_I2C3_RX_DMA_NUMBER, RTE_I2C3_RX_DMA_STREAM),
  DMAx_STREAMy_IRQn(RTE_I2C3_TX_DMA_NUMBER, RTE_I2C3_TX_DMA_STREAM),
  RTE_I2C3_RX_DMA_CHANNEL,
  RTE_I2C3_TX_DMA_CHANNEL,
  RTE_I2C3_RX_DMA_PRIORITY,
  RTE_I2C3_TX_DMA_PRIORITY
};
#endif

/* I2C3 Information (Run-Time) */
static I2C_INFO I2C3_Info;

/* I2C3 Resources */
static I2C_RESOURCES I2C3_Resources = {
  I2C3,
  {
    RTE_I2C3_SCL_PORT,
    RTE_I2C3_SDA_PORT,
    RTE_I2C3_SCL_BIT,
    RTE_I2C3_SDA_BIT
  },
  I2C3_EV_IRQn,
  I2C3_ER_IRQn,
  RCC_APB1Periph_I2C3,
  0,
#if (RTE_I2C3_RX_DMA && RTE_I2C3_TX_DMA)
  &I2C3_DMA,
#else
  NULL,
#endif
  &I2C3_Info
};

#endif /* RTE_I2C3 */


/**
  \fn          uint32_t RCC_GPIOMask (GPIO_TypeDef *port)
  \brief       Determine AHB1 clock enable bit mask for given GPIO port
  \return      \ref RCC_AHB1_Peripherals bit mask for GPIO ports
*/
static uint32_t RCC_GPIOMask (GPIO_TypeDef *GPIOx) {
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
  \fn          ARM_DRV_VERSION I2C_GetVersion (void)
  \brief       Get driver version.
  \return      \ref ARM_DRV_VERSION
*/
static ARM_DRIVER_VERSION I2CX_GetVersion (void) {
  return DriverVersion;
}


/**
  \fn          ARM_I2C_CAPABILITIES I2C_GetCapabilities (void)
  \brief       Get driver capabilities.
  \return      \ref ARM_I2C_CAPABILITIES
*/
static ARM_I2C_CAPABILITIES I2CX_GetCapabilities (void) {
  return DriverCapabilities;
}


/**
  \fn          int32_t I2C_Initialize (ARM_I2C_SignalEvent_t cb_event, I2C_RESOURCES *i2c)
  \brief       Initialize I2C Interface.
  \param[in]   cb_event  Pointer to \ref ARM_I2C_SignalEvent
  \param[in]   i2c   Pointer to I2C resources
  \return      \ref ARM_I2C_STATUS
*/
static int32_t I2C_Initialize (ARM_I2C_SignalEvent_t cb_event, I2C_RESOURCES *i2c) {
  GPIO_InitTypeDef io_cfg;

  if (i2c->info->init++) {
    /* Already initialized */
    return ARM_DRIVER_OK;
  }

  /* Reset Run-Time information structure */
  memset (i2c->info, 0x00, sizeof (I2C_INFO));

  /* Setup I2C pin configuration */
  io_cfg.GPIO_Mode  = GPIO_Mode_AF;
  io_cfg.GPIO_Speed = GPIO_Medium_Speed;
  io_cfg.GPIO_OType = GPIO_OType_OD;
  io_cfg.GPIO_PuPd  = GPIO_PuPd_NOPULL;

  /* Configure SCL Pin */
  RCC_AHB1PeriphClockCmd (RCC_GPIOMask(i2c->io.scl_port), ENABLE);
  GPIO_PinAFConfig (i2c->io.scl_port, i2c->io.scl_pin, GPIO_AF_I2C);
  io_cfg.GPIO_Pin = 1 << i2c->io.scl_pin;
  GPIO_Init (i2c->io.scl_port, &io_cfg);

  /* Configure SDA Pin */
  RCC_AHB1PeriphClockCmd (RCC_GPIOMask(i2c->io.sda_port), ENABLE);
  GPIO_PinAFConfig (i2c->io.sda_port, i2c->io.sda_pin, GPIO_AF_I2C);
  io_cfg.GPIO_Pin = 1 << i2c->io.sda_pin;
  GPIO_Init (i2c->io.sda_port, &io_cfg);

  /* Clear and Enable I2C IRQ */
  NVIC_ClearPendingIRQ(i2c->ev_irq_num);
  NVIC_ClearPendingIRQ(i2c->er_irq_num);
  NVIC_EnableIRQ(i2c->ev_irq_num);
  NVIC_EnableIRQ(i2c->er_irq_num);
  
  if (i2c->dma) {
    /* Enable DMA clock in RCC */
    RCC_AHB1PeriphClockCmd (RCC_AHB1Periph_DMA1, ENABLE);

    /* Enable DMA IRQ in NVIC */
    NVIC_EnableIRQ (i2c->dma->rx_irq_num);
    NVIC_EnableIRQ (i2c->dma->tx_irq_num);
  }

  i2c->info->flags = I2C_INIT;
  return ARM_DRIVER_OK;
}

#if (RTE_I2C1)
static int32_t I2C1_Initialize (ARM_I2C_SignalEvent_t cb_event) {
  return I2C_Initialize(cb_event, &I2C1_Resources);
}
#endif

#if (RTE_I2C2)
static int32_t I2C2_Initialize (ARM_I2C_SignalEvent_t cb_event) {
  return I2C_Initialize(cb_event, &I2C2_Resources);
}
#endif

#if (RTE_I2C3)
static int32_t I2C3_Initialize (ARM_I2C_SignalEvent_t cb_event) {
  return I2C_Initialize(cb_event, &I2C3_Resources);
}
#endif


/**
  \fn          int32_t I2C_Uninitialize (I2C_RESOURCES *i2c)
  \brief       De-initialize I2C Interface.
  \param[in]   i2c  Pointer to I2C resources
  \return      \ref execution_status
*/
static int32_t I2C_Uninitialize (I2C_RESOURCES *i2c) {
  GPIO_InitTypeDef io_cfg;

  if (i2c->info->init) {
    if (--i2c->info->init) {
      return ARM_DRIVER_OK;
    }
  }

  /* Disable I2C IRQ */
  NVIC_DisableIRQ(i2c->ev_irq_num);
  NVIC_DisableIRQ(i2c->er_irq_num);

  if (i2c->dma) {
    /* Disable DMA stream IRQ in NVIC */
    NVIC_DisableIRQ (i2c->dma->rx_irq_num);
    NVIC_DisableIRQ (i2c->dma->tx_irq_num);
  }

  /* Disable I2C peripheral clock */
  RCC_APB1PeriphClockCmd (i2c->periph_clock_mask, DISABLE);

  /* Setup I2C pin configuration */
  io_cfg.GPIO_Mode  = GPIO_Mode_IN;
  io_cfg.GPIO_Speed = GPIO_Low_Speed;
  io_cfg.GPIO_OType = GPIO_OType_PP;
  io_cfg.GPIO_PuPd  = GPIO_PuPd_NOPULL;

  /* Unconfigure SCL Pin */
  GPIO_PinAFConfig (i2c->io.scl_port, i2c->io.scl_pin, 0x00);
  io_cfg.GPIO_Pin = 1 << i2c->io.scl_pin;
  GPIO_Init (i2c->io.scl_port, &io_cfg);

  /* Unconfigure SDA Pin */
  GPIO_PinAFConfig (i2c->io.sda_port, i2c->io.sda_pin, 0x00);
  io_cfg.GPIO_Pin = 1 << i2c->io.sda_pin;
  GPIO_Init (i2c->io.sda_port, &io_cfg);

  i2c->info->flags = 0;

  return ARM_DRIVER_OK;
}

#if (RTE_I2C1)
static int32_t I2C1_Uninitialize (void) {
  return I2C_Uninitialize(&I2C1_Resources);
}
#endif

#if (RTE_I2C2)
static int32_t I2C2_Uninitialize (void) {
  return I2C_Uninitialize(&I2C2_Resources);
}
#endif

#if (RTE_I2C3)
static int32_t I2C3_Uninitialize (void) {
  return I2C_Uninitialize(&I2C3_Resources);
}
#endif


/**
  \fn          int32_t ARM_I2C_PowerControl (ARM_POWER_STATE state, I2C_RESOURCES *i2c)
  \brief       Control I2C Interface Power.
  \param[in]   state  Power state
  \param[in]   i2c  Pointer to I2C resources
  \return      \ref execution_status
*/
static int32_t I2C_PowerControl (ARM_POWER_STATE state, I2C_RESOURCES *i2c) {
  if (!(i2c->info->flags & I2C_INIT)) {
    /* Driver not initialized */
    return (ARM_DRIVER_ERROR);
  }

  switch (state) {
    case ARM_POWER_OFF:
      if (i2c->info->flags & I2C_POWER) {
        i2c->info->flags &= ~(I2C_POWER | I2C_SETUP);

        /* Disable I2C */
        I2C_Cmd (i2c->reg, DISABLE);

        /* Disable I2C Clock */
        RCC_APB1PeriphClockCmd (i2c->periph_clock_mask, DISABLE);

        /* Disable DMA Streams */
        if (i2c->dma) {
          DMA_DeInit (i2c->dma->rx_stream);
          DMA_DeInit (i2c->dma->tx_stream);
        }
      }
      break;

    case ARM_POWER_LOW:
      return ARM_DRIVER_ERROR_UNSUPPORTED;

    case ARM_POWER_FULL:
      /* Enable I2C clock */
      RCC_APB1PeriphClockCmd (i2c->periph_clock_mask, ENABLE);

      /* Reset I2C peripheral */
      RCC_APB1PeriphResetCmd (i2c->periph_clock_mask, ENABLE);
      RCC_APB1PeriphResetCmd (i2c->periph_clock_mask, DISABLE);

      /* Enable error interrupts */
      I2C_ITConfig (i2c->reg, I2C_IT_ERR, ENABLE);

      /* Enable clock stretching */
      I2C_StretchClockCmd (i2c->reg, ENABLE);

      /* Enable I2C */
      I2C_Cmd (i2c->reg, ENABLE);

      /* Ready for operation */
      i2c->info->flags |= I2C_POWER;
      break;
  }

  return ARM_DRIVER_OK;
}

#if (RTE_I2C1)
static int32_t I2C1_PowerControl (ARM_POWER_STATE state) {
  return I2C_PowerControl(state, &I2C1_Resources);
}
#endif

#if (RTE_I2C2)
static int32_t I2C2_PowerControl (ARM_POWER_STATE state) {
  return I2C_PowerControl(state, &I2C2_Resources);
}
#endif

#if (RTE_I2C3)
static int32_t I2C3_PowerControl (ARM_POWER_STATE state) {
  return I2C_PowerControl(state, &I2C3_Resources);
}
#endif


/**
  \fn          int32_t I2C_MasterTransmit (uint32_t       addr,
                                           const uint8_t *data,
                                           uint32_t       num,
                                           bool           xfer_pending,
                                           I2C_RESOURCES *i2c)
  \brief       Start transmitting data as I2C Master.
  \param[in]   addr          Slave address (7-bit or 10-bit)
  \param[in]   data          Pointer to buffer with data to send to I2C Slave
  \param[in]   num           Number of data bytes to send
  \param[in]   xfer_pending  Transfer operation is pending - Stop condition will not be generated
  \param[in]   i2c           Pointer to I2C resources
  \return      \ref execution_status
*/
static int32_t I2C_MasterTransmit (uint32_t       addr,
                                   const uint8_t *data,
                                   uint32_t       num,
                                   bool           xfer_pending,
                                   I2C_RESOURCES *i2c) {
  DMA_InitTypeDef dma_cfg;

  if (i2c->info->status.busy) {
    return (ARM_DRIVER_ERROR_BUSY);
  }

  i2c->info->status.busy      = 1;
  i2c->info->status.mode      = 1;
  i2c->info->status.direction = 0;

  i2c->info->xfer.num  = num;
  i2c->info->xfer.cnt  = 0;
  i2c->info->xfer.data = (uint8_t *)data;
  i2c->info->xfer.addr = addr;
  i2c->info->xfer.ctrl = (xfer_pending) ? (0) : (XFER_CTRL_GEN_STOP);

  if (i2c->dma) {
    /* Configure DMA transmit stream */
    dma_cfg.DMA_Channel            = i2c->dma->tx_channel << DMA_CHANNEL_POS;
    dma_cfg.DMA_PeripheralBaseAddr = (uint32_t)&(i2c->reg->DR);
    dma_cfg.DMA_Memory0BaseAddr    = (uint32_t)data;
    dma_cfg.DMA_DIR                = DMA_DIR_MemoryToPeripheral;
    dma_cfg.DMA_BufferSize         = num;
    dma_cfg.DMA_PeripheralInc      = DMA_PeripheralInc_Disable;
    dma_cfg.DMA_MemoryInc          = DMA_MemoryInc_Enable;
    dma_cfg.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    dma_cfg.DMA_MemoryDataSize     = DMA_MemoryDataSize_Byte;
    dma_cfg.DMA_Mode               = DMA_Mode_Normal;
    dma_cfg.DMA_Priority           = i2c->dma->tx_priority << DMA_PRIORITY_POS;
    dma_cfg.DMA_FIFOMode           = DMA_FIFOMode_Disable;
    dma_cfg.DMA_FIFOThreshold      = DMA_FIFOThreshold_Full;
    dma_cfg.DMA_MemoryBurst        = DMA_MemoryBurst_Single;
    dma_cfg.DMA_PeripheralBurst    = DMA_PeripheralBurst_Single;

    /* Configure and enable stream and it's interrupts */
    DMA_Init     (i2c->dma->tx_stream, &dma_cfg);
    DMA_ITConfig (i2c->dma->tx_stream, DMA_IT_TC, ENABLE);
    DMA_Cmd      (i2c->dma->tx_stream, ENABLE);

    /* Configure I2C Interrupts and enable DMA trigger */
    I2C_ITConfig (i2c->reg, I2C_IT_BUF, DISABLE);
    I2C_DMACmd   (i2c->reg, ENABLE);
  }
  else {
    /* Interrupt mode data transfer */
    I2C_DMACmd (i2c->reg, DISABLE);
    I2C_ITConfig (i2c->reg, I2C_IT_BUF, ENABLE);
  }

  /* Generate start */
  I2C_GenerateSTART (i2c->reg, ENABLE);

  I2C_ITConfig (i2c->reg, I2C_IT_EVT, ENABLE);
  return ARM_DRIVER_OK;
}

#if (RTE_I2C1)
static int32_t I2C1_MasterTransmit (uint32_t addr, const uint8_t *data, uint32_t num, bool xfer_pending) {
  return I2C_MasterTransmit(addr, data, num, xfer_pending, &I2C1_Resources);
}
#endif

#if (RTE_I2C2)
static int32_t I2C2_MasterTransmit (uint32_t addr, const uint8_t *data, uint32_t num, bool xfer_pending) {
  return I2C_MasterTransmit(addr, data, num, xfer_pending, &I2C2_Resources);
}
#endif

#if (RTE_I2C3)
static int32_t I2C3_MasterTransmit (uint32_t addr, const uint8_t *data, uint32_t num, bool xfer_pending) {
  return I2C_MasterTransmit(addr, data, num, xfer_pending, &I2C3_Resources);
}
#endif


/**
  \fn          int32_t I2C_MasterReceive (uint32_t       addr,
                                          uint8_t       *data,
                                          uint32_t       num,
                                          bool           xfer_pending,
                                          I2C_RESOURCES *i2c)
  \brief       Start receiving data as I2C Master.
  \param[in]   addr          Slave address (7-bit or 10-bit)
  \param[out]  data          Pointer to buffer for data to receive from I2C Slave
  \param[in]   num           Number of data bytes to receive
  \param[in]   xfer_pending  Transfer operation is pending - Stop condition will not be generated
  \param[in]   i2c           Pointer to I2C resources
  \return      \ref execution_status
*/
static int32_t I2C_MasterReceive (uint32_t       addr,
                                  uint8_t       *data,
                                  uint32_t       num,
                                  bool           xfer_pending,
                                  I2C_RESOURCES *i2c) {
  DMA_InitTypeDef dma_cfg;

  if (i2c->info->status.busy) {
    return (ARM_DRIVER_ERROR_BUSY);
  }

  i2c->info->status.busy      = 1;
  i2c->info->status.mode      = 1;
  i2c->info->status.direction = 1;

  i2c->info->xfer.num  = num;
  i2c->info->xfer.cnt  = 0;
  i2c->info->xfer.data = data;
  i2c->info->xfer.addr = addr;
  i2c->info->xfer.ctrl = (xfer_pending) ? (0) : (XFER_CTRL_GEN_STOP);

  if (i2c->dma) {
    /* Configure DMA receive stream */
    dma_cfg.DMA_Channel            = i2c->dma->rx_channel << DMA_CHANNEL_POS;
    dma_cfg.DMA_PeripheralBaseAddr = (uint32_t)&(i2c->reg->DR);
    dma_cfg.DMA_Memory0BaseAddr    = (uint32_t)data;
    dma_cfg.DMA_DIR                = DMA_DIR_PeripheralToMemory;
    dma_cfg.DMA_BufferSize         = num;
    dma_cfg.DMA_PeripheralInc      = DMA_PeripheralInc_Disable;
    dma_cfg.DMA_MemoryInc          = DMA_MemoryInc_Enable;
    dma_cfg.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    dma_cfg.DMA_MemoryDataSize     = DMA_MemoryDataSize_Byte;
    dma_cfg.DMA_Mode               = DMA_Mode_Normal;
    dma_cfg.DMA_Priority           = i2c->dma->rx_priority << DMA_PRIORITY_POS;
    dma_cfg.DMA_FIFOMode           = DMA_FIFOMode_Disable;
    dma_cfg.DMA_FIFOThreshold      = DMA_FIFOThreshold_Full;
    dma_cfg.DMA_MemoryBurst        = DMA_MemoryBurst_Single;
    dma_cfg.DMA_PeripheralBurst    = DMA_PeripheralBurst_Single;

    /* Configure and enable stream and it's interrupts */
    DMA_Init     (i2c->dma->rx_stream, &dma_cfg);
    DMA_ITConfig (i2c->dma->rx_stream, DMA_IT_TC, ENABLE);
    DMA_Cmd      (i2c->dma->rx_stream, ENABLE);

    /* Configure I2C Interrupts and enable DMA trigger */
    I2C_ITConfig           (i2c->reg, I2C_IT_BUF, DISABLE);
    I2C_DMALastTransferCmd (i2c->reg, ENABLE);
    I2C_DMACmd             (i2c->reg, ENABLE);
  }
  else {
    I2C_DMACmd             (i2c->reg, DISABLE);
    I2C_ITConfig           (i2c->reg, I2C_IT_BUF, ENABLE);
  }

  /* Enable acknowlede generation and generate start */
  I2C_AcknowledgeConfig (i2c->reg, ENABLE);
  I2C_GenerateSTART     (i2c->reg, ENABLE);

  I2C_ITConfig (i2c->reg, I2C_IT_EVT, ENABLE);
  return ARM_DRIVER_OK;
}

#if (RTE_I2C1)
static int32_t I2C1_MasterReceive (uint32_t addr, uint8_t *data, uint32_t num, bool xfer_pending) {
  return I2C_MasterReceive(addr, data, num, xfer_pending, &I2C1_Resources);
}
#endif

#if (RTE_I2C2)
static int32_t I2C2_MasterReceive (uint32_t addr, uint8_t *data, uint32_t num, bool xfer_pending) {
  return I2C_MasterReceive(addr, data, num, xfer_pending, &I2C2_Resources);
}
#endif

#if (RTE_I2C3)
static int32_t I2C3_MasterReceive (uint32_t addr, uint8_t *data, uint32_t num, bool xfer_pending) {
  return I2C_MasterReceive(addr, data, num, xfer_pending, &I2C3_Resources);
}
#endif


/**
  \fn          int32_t I2C_SlaveTransmit (const uint8_t *data, uint32_t num, I2C_RESOURCES *i2c)
  \brief       Start transmitting data as I2C Slave.
  \param[in]   data          Pointer to buffer with data to send to I2C Master
  \param[in]   num           Number of data bytes to send
  \param[in]   i2c           Pointer to I2C resources
  \return      \ref execution_status
*/
static int32_t I2C_SlaveTransmit (const uint8_t *data, uint32_t num, I2C_RESOURCES *i2c) {
  DMA_InitTypeDef dma_cfg;

  if (i2c->info->status.busy) {
    return (ARM_DRIVER_ERROR_BUSY);
  }

  i2c->info->xfer.addr = 0;
  i2c->info->xfer.data = (uint8_t *)data;
  i2c->info->xfer.num  = num;
  i2c->info->xfer.ctrl = 0;

  if (i2c->dma) {
    /* Configure DMA receive stream */
    dma_cfg.DMA_Channel            = i2c->dma->tx_channel << DMA_CHANNEL_POS;
    dma_cfg.DMA_PeripheralBaseAddr = (uint32_t)&(i2c->reg->DR);
    dma_cfg.DMA_Memory0BaseAddr    = (uint32_t)data;
    dma_cfg.DMA_DIR                = DMA_DIR_MemoryToPeripheral;
    dma_cfg.DMA_BufferSize         = num;
    dma_cfg.DMA_PeripheralInc      = DMA_PeripheralInc_Disable;
    dma_cfg.DMA_MemoryInc          = DMA_MemoryInc_Enable;
    dma_cfg.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    dma_cfg.DMA_MemoryDataSize     = DMA_MemoryDataSize_Byte;
    dma_cfg.DMA_Mode               = DMA_Mode_Normal;
    dma_cfg.DMA_Priority           = i2c->dma->tx_priority << DMA_PRIORITY_POS;
    dma_cfg.DMA_FIFOMode           = DMA_FIFOMode_Disable;
    dma_cfg.DMA_FIFOThreshold      = DMA_FIFOThreshold_Full;
    dma_cfg.DMA_MemoryBurst        = DMA_MemoryBurst_Single;
    dma_cfg.DMA_PeripheralBurst    = DMA_PeripheralBurst_Single;

    /* Configure and enable stream and it's interrupts */
    DMA_Init     (i2c->dma->tx_stream, &dma_cfg);
    DMA_ITConfig (i2c->dma->tx_stream, DMA_IT_TC, ENABLE);
    DMA_Cmd      (i2c->dma->tx_stream, ENABLE);

    /* Configure I2C Interrupts and enable DMA trigger */
    I2C_ITConfig (i2c->reg, I2C_IT_BUF, DISABLE);
    I2C_DMACmd   (i2c->reg, ENABLE);
  }
  else {
    /* Interrupt mode data transfer */
    I2C_DMACmd (i2c->reg, DISABLE);
    I2C_ITConfig (i2c->reg, I2C_IT_BUF, ENABLE);
  }

  I2C_ITConfig (i2c->reg, I2C_IT_EVT, ENABLE);

  return ARM_DRIVER_OK;
}

#if (RTE_I2C1)
static int32_t I2C1_SlaveTransmit (const uint8_t *data, uint32_t num) {
  return I2C_SlaveTransmit(data, num, &I2C1_Resources);
}
#endif

#if (RTE_I2C2)
static int32_t I2C2_SlaveTransmit (const uint8_t *data, uint32_t num) {
  return I2C_SlaveTransmit(data, num, &I2C2_Resources);
}
#endif

#if (RTE_I2C3)
static int32_t I2C3_SlaveTransmit (const uint8_t *data, uint32_t num) {
  return I2C_SlaveTransmit(data, num, &I2C3_Resources);
}
#endif


/**
  \fn          int32_t I2C_SlaveReceive (uint8_t *data, uint32_t num, I2C_RESOURCES *i2c)
  \brief       Start receiving data as I2C Slave.
  \param[out]  data          Pointer to buffer for data to receive from I2C Master
  \param[in]   num           Number of data bytes to receive
  \param[in]   i2c           Pointer to I2C resources
  \return      \ref execution_status
*/
static int32_t I2C_SlaveReceive (uint8_t *data, uint32_t num, I2C_RESOURCES *i2c) {
  DMA_InitTypeDef dma_cfg;

  if (i2c->info->status.busy) {
    return (ARM_DRIVER_ERROR_BUSY);
  }

  i2c->info->xfer.addr = 0;
  i2c->info->xfer.data = data;
  i2c->info->xfer.num  = num;
  i2c->info->xfer.ctrl = 0;

  if (i2c->dma) {
    /* Disable DMA stream and trigger */
    DMA_Cmd (i2c->dma->rx_stream, DISABLE);
    I2C_DMACmd (i2c->reg, DISABLE);

    /* Configure DMA receive stream */
    dma_cfg.DMA_Channel            = i2c->dma->rx_channel << DMA_CHANNEL_POS;
    dma_cfg.DMA_PeripheralBaseAddr = (uint32_t)&(i2c->reg->DR);
    dma_cfg.DMA_Memory0BaseAddr    = (uint32_t)data;
    dma_cfg.DMA_DIR                = DMA_DIR_PeripheralToMemory;
    dma_cfg.DMA_BufferSize         = num;
    dma_cfg.DMA_PeripheralInc      = DMA_PeripheralInc_Disable;
    dma_cfg.DMA_MemoryInc          = DMA_MemoryInc_Enable;
    dma_cfg.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    dma_cfg.DMA_MemoryDataSize     = DMA_MemoryDataSize_Byte;
    dma_cfg.DMA_Mode               = DMA_Mode_Normal;
    dma_cfg.DMA_Priority           = i2c->dma->rx_priority << DMA_PRIORITY_POS;
    dma_cfg.DMA_FIFOMode           = DMA_FIFOMode_Disable;
    dma_cfg.DMA_FIFOThreshold      = DMA_FIFOThreshold_Full;
    dma_cfg.DMA_MemoryBurst        = DMA_MemoryBurst_Single;
    dma_cfg.DMA_PeripheralBurst    = DMA_PeripheralBurst_Single;

    /* Configure and enable stream and it's interrupts */
    DMA_Init     (i2c->dma->rx_stream, &dma_cfg);
    DMA_ITConfig (i2c->dma->rx_stream, DMA_IT_TC, ENABLE);
    DMA_Cmd      (i2c->dma->rx_stream, ENABLE);

    /* Configure I2C Interrupts and enable DMA trigger */
    I2C_ITConfig           (i2c->reg, I2C_IT_BUF, DISABLE);
    I2C_DMALastTransferCmd (i2c->reg, ENABLE);
    I2C_DMACmd             (i2c->reg, ENABLE);
  }
  else {
    I2C_DMACmd             (i2c->reg, DISABLE);
    I2C_ITConfig           (i2c->reg, I2C_IT_BUF, ENABLE);
  }

  /* Enable acknowlede generation and event interrupts */
  I2C_AcknowledgeConfig (i2c->reg, ENABLE);

  I2C_ITConfig (i2c->reg, I2C_IT_EVT, ENABLE);
  return ARM_DRIVER_OK;
}

#if (RTE_I2C1)
static int32_t I2C1_SlaveReceive (uint8_t *data, uint32_t num) {
  return I2C_SlaveReceive(data, num, &I2C1_Resources);
}
#endif

#if (RTE_I2C2)
static int32_t I2C2_SlaveReceive (uint8_t *data, uint32_t num) {
  return I2C_SlaveReceive(data, num, &I2C2_Resources);
}
#endif

#if (RTE_I2C3)
static int32_t I2C3_SlaveReceive (uint8_t *data, uint32_t num) {
  return I2C_SlaveReceive(data, num, &I2C3_Resources);
}
#endif


/**
  \fn          int32_t I2C_Control (uint32_t control, uint32_t arg, I2C_RESOURCES *i2c)
  \brief       Control I2C Interface.
  \param[in]   control  operation
  \param[in]   arg      argument of operation (optional)
  \param[in]   i2c      pointer to I2C resources
  \return      \ref execution_status
*/
static int32_t I2C_Control (uint32_t control, uint32_t arg, I2C_RESOURCES *i2c) {
  GPIO_InitTypeDef io_cfg;
  uint32_t i;
  uint32_t tick;
  uint32_t state;
  uint16_t ccr;
  uint16_t trise;

  if (!(i2c->info->flags & I2C_POWER)) {
    /* I2C not powered */
    return ARM_DRIVER_ERROR;
  }

  switch (control) {
    case ARM_I2C_OWN_ADDRESS:
      i2c->reg->OAR1 = (arg & 0x03FF) |
                       (1 << 14)      |
                       (arg & ARM_I2C_ADDRESS_10BIT) ? (1 << 15) : (0);
      break;

    case ARM_I2C_BUS_SPEED:
      switch (arg) {
        case ARM_I2C_BUS_SPEED_STANDARD:
          /* Clock = 100kHz,  Rise Time = 1000ns */
          if (RTE_PCLK1 > 45000000) return ARM_DRIVER_ERROR_UNSUPPORTED;
          if (RTE_PCLK1 <  2000000) return ARM_DRIVER_ERROR_UNSUPPORTED;
          ccr   =  RTE_PCLK1 / 100000 / 2;
          trise = (RTE_PCLK1 / 1000000) + 1;
          break;
        case ARM_I2C_BUS_SPEED_FAST:
          /* Clock = 400kHz,  Rise Time = 300ns */
          if (RTE_PCLK1 > 45000000) return ARM_DRIVER_ERROR_UNSUPPORTED;
          if (RTE_PCLK1 <  4000000) return ARM_DRIVER_ERROR_UNSUPPORTED;
          if ((RTE_PCLK1 >= 10000000) && ((RTE_PCLK1 % 10000000) == 0)) {
            ccr = I2C_CCR_FS | I2C_CCR_DUTY | (RTE_PCLK1 / 400000 / 25);
          } else {
            ccr = I2C_CCR_FS |                (RTE_PCLK1 / 400000 / 3);
          }
          trise = (RTE_PCLK1 / 333333) + 1;
          break;
        default:
          return ARM_DRIVER_ERROR_UNSUPPORTED;
      }

      i2c->reg->CR1   &= ~I2C_CR1_PE;           /* Disable I2C peripheral */
      i2c->reg->CR2   &= ~I2C_CR2_FREQ;
      i2c->reg->CR2   |=  RTE_PCLK1 / 1000000;
      i2c->reg->CCR    =  ccr;
      i2c->reg->TRISE  =  trise;
      i2c->reg->CR1   |=  I2C_CR1_PE;           /* Enable I2C peripheral */
      break;

    case ARM_I2C_BUS_CLEAR:
      /* Configure SCl and SDA pins as GPIO pin */
      GPIO_PinAFConfig (i2c->io.scl_port, i2c->io.scl_pin, 0x00);
      GPIO_PinAFConfig (i2c->io.sda_port, i2c->io.sda_pin, 0x00);

      io_cfg.GPIO_Mode  = GPIO_Mode_OUT;
      io_cfg.GPIO_Speed = GPIO_Medium_Speed;
      io_cfg.GPIO_OType = GPIO_OType_OD;
      io_cfg.GPIO_PuPd  = GPIO_PuPd_NOPULL;

      io_cfg.GPIO_Pin = 1 << i2c->io.scl_pin;
      GPIO_Init (i2c->io.scl_port, &io_cfg);

      io_cfg.GPIO_Pin = 1 << i2c->io.sda_pin;
      GPIO_Init (i2c->io.sda_port, &io_cfg);
      
      /* Pull SCL and SDA high */
      GPIO_WriteBit (i2c->io.scl_port, i2c->io.scl_pin, Bit_SET);
      GPIO_WriteBit (i2c->io.sda_port, i2c->io.sda_pin, Bit_SET);
      tick = osKernelSysTick();
      while ((osKernelSysTick() - tick) < osKernelSysTickMicroSec (I2C_BUS_CLEAR_CLOCK_PERIOD));
      
      for (i = 0; i < 9; i++) {
        if (GPIO_ReadInputDataBit (i2c->io.sda_port, i2c->io.sda_pin)) {
          /* Break if slave released SDA line */
          break;
        }
        /* Clock high */
        GPIO_WriteBit (i2c->io.scl_port, i2c->io.scl_pin, Bit_SET);
        tick = osKernelSysTick();
        while ((osKernelSysTick() - tick) < osKernelSysTickMicroSec (I2C_BUS_CLEAR_CLOCK_PERIOD/2));

        /* Clock low */
        GPIO_WriteBit (i2c->io.scl_port, i2c->io.scl_pin, Bit_RESET);
        tick = osKernelSysTick();
        while ((osKernelSysTick() - tick) < osKernelSysTickMicroSec (I2C_BUS_CLEAR_CLOCK_PERIOD/2));
      }
      
      /* Check SDA state */
      state = GPIO_ReadInputDataBit (i2c->io.sda_port, i2c->io.sda_pin);

      /* Configure SDA and SCL pins as I2C peripheral pins */
      io_cfg.GPIO_Mode  = GPIO_Mode_AF;
      io_cfg.GPIO_Speed = GPIO_Medium_Speed;
      io_cfg.GPIO_OType = GPIO_OType_OD;
      io_cfg.GPIO_PuPd  = GPIO_PuPd_NOPULL;

      io_cfg.GPIO_Pin = 1 << i2c->io.scl_pin;
      GPIO_Init (i2c->io.scl_port, &io_cfg);

      io_cfg.GPIO_Pin = 1 << i2c->io.sda_pin;
      GPIO_Init (i2c->io.sda_port, &io_cfg);

      GPIO_PinAFConfig (i2c->io.scl_port, i2c->io.scl_pin, GPIO_AF_I2C);
      GPIO_PinAFConfig (i2c->io.sda_port, i2c->io.sda_pin, GPIO_AF_I2C);

      return (state) ? ARM_DRIVER_OK : ARM_DRIVER_ERROR;

    case ARM_I2C_ABORT_TRANSFER:
      /* Disable DMA requests and I2C interrupts */
      i2c->reg->CR2 &= ~(I2C_CR2_DMAEN | I2C_CR2_ITBUFEN | I2C_CR2_ITEVTEN);

      if (i2c->dma) {
        /* Disable DMA Streams */
        DMA_Cmd (i2c->dma->rx_stream, DISABLE);
        DMA_Cmd (i2c->dma->tx_stream, DISABLE);
      }

      /* Generate stop */
      /* Master generates stop after current byte transfer */
      /* Slave releases SCL and SDA after the current byte transfer */
      I2C_GenerateSTOP (i2c->reg, ENABLE);

      i2c->info->xfer.num  = 0;
      i2c->info->xfer.cnt  = 0;
      i2c->info->xfer.data = NULL;
      i2c->info->xfer.addr = 0;
      i2c->info->xfer.ctrl = 0;

      i2c->info->status.busy             = 0;
      i2c->info->status.mode             = 0;
      i2c->info->status.direction        = 0;
      i2c->info->status.general_call     = 0;
      i2c->info->status.address_nack     = 0;
      i2c->info->status.arbitration_lost = 0;
      i2c->info->status.bus_error        = 0;

      /* Disable and reenable peripheral to clear some flags */
      I2C_Cmd (i2c->reg, DISABLE);
      I2C_Cmd (i2c->reg, ENABLE);
      break;
  }
  return ARM_DRIVER_OK;
}

#if (RTE_I2C1)
static int32_t I2C1_Control (uint32_t control, uint32_t arg) {
  return I2C_Control(control, arg, &I2C1_Resources);
}
#endif

#if (RTE_I2C2)
static int32_t I2C2_Control (uint32_t control, uint32_t arg) {
  return I2C_Control(control, arg, &I2C2_Resources);
}
#endif

#if (RTE_I2C3)
static int32_t I2C3_Control (uint32_t control, uint32_t arg) {
  return I2C_Control(control, arg, &I2C3_Resources);
}
#endif


/**
  \fn          ARM_I2C_STATUS I2C_GetStatus (I2C_RESOURCES *i2c)
  \brief       Get I2C status.
  \param[in]   i2c      pointer to I2C resources
  \return      I2C status \ref ARM_I2C_STATUS
*/
static ARM_I2C_STATUS I2C_GetStatus (I2C_RESOURCES *i2c) {
  return (i2c->info->status);
}

#if (RTE_I2C1)
static ARM_I2C_STATUS I2C1_GetStatus (void) {
  return I2C_GetStatus(&I2C1_Resources);
}
#endif

#if (RTE_I2C2)
static ARM_I2C_STATUS I2C2_GetStatus (void) {
  return I2C_GetStatus(&I2C2_Resources);
}
#endif

#if (RTE_I2C3)
static ARM_I2C_STATUS I2C3_GetStatus (void) {
  return I2C_GetStatus(&I2C3_Resources);
}
#endif

/**
  \fn          void I2C_EV_IRQHandler (I2C_RESOURCES *i2c)
  \brief       I2C Event Interrupt handler.
  \param[in]   i2c  Pointer to I2C resources
*/
static void I2C_EV_IRQHandler (I2C_RESOURCES *i2c) {
  I2C_TRANSFER_INFO *tr = &i2c->info->xfer;
  uint8_t  data;
  uint32_t sr1, sr2;
  uint32_t rw;
  uint32_t event;

  sr1 = i2c->reg->SR1;

  if (sr1 & I2C_SR1_SB) {
    /* (EV5): start bit generated, send address */

    if (tr->addr & ARM_I2C_ADDRESS_10BIT) {
      /* 10-bit addressing mode */
      data = 0xF0 | ((tr->addr >> 7) & 0x06);
    }
    else {
      /* 7-bit addressing mode */
      data = ((tr->addr & 0x7F) << 1) | i2c->info->status.direction;
    }
    I2C_SendData (i2c->reg, data);
  }
  else if (sr1 & I2C_SR1_ADD10) {
    /* (EV9): 10-bit address header sent, send device address LSB */
    I2C_SendData (i2c->reg, tr->addr & 0xFF);
    
    if (i2c->info->status.direction) {
      /* Master receiver generates repeated start in 10-bit addressing mode */
      tr->ctrl |= XFER_CTRL_RSTART;
    }
  }
  else if (sr1 & I2C_SR1_ADDR) {
    /* (EV6): addressing complete */

    if (i2c->info->status.mode) {
      /* Master mode */

      if (i2c->info->status.direction) {
        /* Master receiver */
        if (tr->num == 1) {
          I2C_AcknowledgeConfig (i2c->reg, DISABLE);
        }

        /* Clear ADDR flag */
        i2c->reg->SR1;
        i2c->reg->SR2;

        if (tr->ctrl & XFER_CTRL_RSTART) {
          tr->ctrl &= ~XFER_CTRL_RSTART;
          /* Generate repeated start */
          I2C_GenerateSTART (i2c->reg, ENABLE);
        }
        else {
          if (tr->num == 1) {
            I2C_GenerateSTOP (i2c->reg, ENABLE);
          }
          else {
            if (tr->num == 2) {
              I2C_AcknowledgeConfig (i2c->reg, DISABLE);
              I2C_NACKPositionConfig (i2c->reg, I2C_NACKPosition_Next);
            }
          }
        }
      }
      else {
        /* Master transmitter */

        /* Clear ADDR flag */
        i2c->reg->SR1;
        i2c->reg->SR2;
      }
    }
    else {
      /* Slave mode */
      sr2 = i2c->reg->SR2;    //ADDR flag cleared

      rw = (sr2 & I2C_SR2_TRA) != 0;

      if ((sr2 & I2C_SR2_GENCALL) || (tr->data == NULL) || (i2c->info->status.direction != rw)) {
        if (rw) {
          /* Slave receiver */
          event = ARM_I2C_EVENT_SLAVE_RECEIVE;
        }
        else {
          /* Slave transmitter */
          event = ARM_I2C_EVENT_SLAVE_TRANSMIT;
        }

        if (sr2 & I2C_SR2_GENCALL) {
          event |= ARM_I2C_EVENT_GENERAL_CALL;

          i2c->info->status.general_call = 1;
        }

        if (i2c->info->cb_event) {
          i2c->info->cb_event (event, 0);
        }
      }
    }
  }
  else if (sr1 & I2C_SR1_STOPF) {
    /* STOP condition after ACK detected by slave */
    if (tr->cnt == tr->num) {
      if (i2c->info->cb_event) {
        i2c->info->cb_event (ARM_I2C_EVENT_SLAVE_DONE, tr->cnt);
      }
    }
  }
  else if (sr1 & I2C_SR1_TXE) {
    if (sr1 & I2C_SR1_BTF) {
      if (tr->cnt == tr->num) {
        if (tr->ctrl & XFER_CTRL_GEN_STOP) {
          I2C_GenerateSTOP (i2c->reg, ENABLE);
          while (i2c->reg->SR2 & I2C_SR2_BUSY);
        }
        I2C_ITConfig(i2c->reg, I2C_IT_EVT | I2C_IT_BUF, DISABLE);

        i2c->info->status.busy      = 0;
        i2c->info->status.mode      = 0;
        i2c->info->status.direction = 0;

        if (i2c->info->cb_event) {
          i2c->info->cb_event (ARM_I2C_EVENT_MASTER_DONE, tr->cnt);
        }
      }
    }
    else if (tr->cnt < tr->num) {
      I2C_SendData (i2c->reg, tr->data[tr->cnt++]);
    }
  }
  else if ((sr1 & I2C_SR1_RXNE) && !(sr1 & I2C_SR1_BTF)) {
    tr->data[tr->cnt++] = I2C_ReceiveData (i2c->reg);

    if (tr->cnt == (tr->num - 3)) {
      I2C_ITConfig(i2c->reg, I2C_IT_BUF, DISABLE);
    }
    else if (tr->cnt == tr->num) {
      I2C_ITConfig(i2c->reg, I2C_IT_EVT | I2C_IT_BUF, DISABLE);
      while (i2c->reg->SR2 & I2C_SR2_BUSY);
      i2c->info->status.busy      = 0;
    }
  }
  else if ((sr1 & I2C_SR1_RXNE) && (sr1 & I2C_SR1_BTF)) {
    if (tr->cnt == (tr->num - 3)) {
      /* Three bytes remaining */
      I2C_AcknowledgeConfig (i2c->reg, DISABLE);
      /* Read data N-2 */
      tr->data[tr->cnt++] = I2C_ReceiveData (i2c->reg);
    }
    else if (tr->cnt == (tr->num - 2)) {
      /* Two bytes remaining */
      if (tr->ctrl & XFER_CTRL_GEN_STOP) {
        I2C_GenerateSTOP (i2c->reg, ENABLE);
      }
      I2C_ITConfig (i2c->reg, I2C_IT_EVT, DISABLE);

      /* Read data N-1 and N */
      tr->data[tr->cnt++] = I2C_ReceiveData (i2c->reg);
      tr->data[tr->cnt++] = I2C_ReceiveData (i2c->reg);

      i2c->info->status.busy      = 0;
      i2c->info->status.mode      = 0;
      i2c->info->status.direction = 0;
      if (tr->ctrl & XFER_CTRL_GEN_STOP) {
        while (i2c->reg->SR2 & I2C_SR2_BUSY);
      }
    }
    else {
      tr->data[tr->cnt++] = I2C_ReceiveData (i2c->reg);
    }
  }
}

#if (RTE_I2C1)
void I2C1_EV_IRQHandler (void) {
  I2C_EV_IRQHandler(&I2C1_Resources);
}
#endif

#if (RTE_I2C2)
void I2C2_EV_IRQHandler (void) {
  I2C_EV_IRQHandler(&I2C2_Resources);
}
#endif

#if (RTE_I2C3)
void I2C3_EV_IRQHandler (void) {
  I2C_EV_IRQHandler(&I2C3_Resources);
}
#endif


/**
  \fn          void I2C_ER_IRQHandler (I2C_RESOURCES *i2c)
  \brief       I2C Error Interrupt handler.
  \param[in]   i2c  Pointer to I2C resources
*/
static void I2C_ER_IRQHandler (I2C_RESOURCES *i2c) {
  uint32_t sr1 = i2c->reg->SR1;
  uint16_t err = 0;

  if (sr1 & I2C_SR1_SMBALERT) {
    /* SMBus alert */
    err |= I2C_SR1_SMBALERT;
  }
  if (sr1 & I2C_SR1_TIMEOUT) {
    /* Timeout - SCL remained LOW for 25ms */
    err |= I2C_SR1_TIMEOUT;
  }
  if (sr1 & I2C_SR1_PECERR) {
    /* PEC Error in reception */
    err |= I2C_SR1_PECERR;
  }
  if (sr1 & I2C_SR1_OVR) {
    /* Overrun/Underrun */
    err |= I2C_SR1_OVR;
  }
  if (sr1 & I2C_SR1_AF) {
    /* Acknowledge failure */
    err |= I2C_SR1_AF;

    if (i2c->info->cb_event) {
      if (i2c->info->status.mode) {
        /* Master mode */
        if (!(i2c->info->xfer.ctrl & XFER_CTRL_ADDR_DONE)) {
          /* Addressing not done */
          i2c->info->cb_event (ARM_I2C_EVENT_ADDRESS_NACK, 0);
        }
      }
      else {
        /* Slave transmitter */
        if (i2c->info->xfer.cnt == i2c->info->xfer.num) {
          /* Regular NACK, master ended the transfer */
          i2c->info->cb_event (ARM_I2C_EVENT_SLAVE_DONE, i2c->info->xfer.cnt);
        }
      }
    }
  }
  if (sr1 & I2C_SR1_ARLO) {
    /* Arbitration lost */
    err |= I2C_SR1_ARLO;

    /* Switch to slave mode */
    i2c->info->status.mode             = 0;
    i2c->info->status.direction        = 0;
    i2c->info->status.arbitration_lost = 1;

    if (i2c->info->cb_event) {
      i2c->info->cb_event (ARM_I2C_EVENT_ARBITRATION_LOST, 0);
    }
  }
  if (sr1 & I2C_SR1_BERR) {
    /* Bus error - misplaced start/stop */
    err |= I2C_SR1_BERR;

    i2c->info->status.bus_error = 1;

    if (i2c->info->cb_event) {
      i2c->info->cb_event (ARM_I2C_EVENT_BUS_ERROR, 0);
    }
  }
  /* Clear error flags */
  i2c->reg->SR1 &= ~err;
}

#if (RTE_I2C1)
void I2C1_ER_IRQHandler (void) {
  I2C_ER_IRQHandler(&I2C1_Resources);
}
#endif

#if (RTE_I2C2)
void I2C2_ER_IRQHandler (void) {
  I2C_ER_IRQHandler(&I2C2_Resources);
}
#endif

#if (RTE_I2C3)
void I2C3_ER_IRQHandler (void) {
  I2C_ER_IRQHandler(&I2C3_Resources);
}
#endif


#if ((RTE_I2C1 && RTE_I2C1_TX_DMA) || \
     (RTE_I2C2 && RTE_I2C2_TX_DMA) || \
     (RTE_I2C3 && RTE_I2C3_TX_DMA))
/**
  \fn          void I2C_DMA_TxEvent (uint32_t event, I2C_RESOURCES *i2c)
  \brief       I2C DMA Transmitt Event handler
  \param[in]   i2c  Pointer to I2C resources
*/
static void I2C_DMA_TxEvent (uint32_t event, I2C_RESOURCES *i2c) {
  i2c->reg->CR2 &= ~I2C_CR2_DMAEN;

  /* BTF event will be generated after EOT */
  while (!(i2c->reg->SR1 & I2C_SR1_BTF));
  
  if (i2c->info->xfer.ctrl & XFER_CTRL_GEN_STOP) {
    I2C_GenerateSTOP (i2c->reg, ENABLE);
  }
  else {
    I2C_ITConfig (i2c->reg, I2C_IT_EVT, DISABLE);
  }

  i2c->info->status.busy      = 0;
  i2c->info->status.mode      = 0;
  i2c->info->status.direction = 0;
}
#endif


#if ((RTE_I2C1 && RTE_I2C1_RX_DMA) || \
     (RTE_I2C2 && RTE_I2C2_RX_DMA) || \
     (RTE_I2C3 && RTE_I2C3_RX_DMA))
/**
  \fn          void I2C_DMA_RxEvent (uint32_t event, I2C_RESOURCES *i2c)
  \brief       I2C DMA Receive Event handler
  \param[in]   i2c  Pointer to I2C resources
*/
static void I2C_DMA_RxEvent (uint32_t event, I2C_RESOURCES *i2c) {
  i2c->reg->CR2 &= ~I2C_CR2_DMAEN;

  I2C_ITConfig (i2c->reg, I2C_IT_EVT, DISABLE);

  if (i2c->info->xfer.num != 1) {
    if (i2c->info->xfer.ctrl & XFER_CTRL_GEN_STOP) {
      I2C_GenerateSTOP (i2c->reg, ENABLE);
    }
  }

  i2c->info->status.busy      = 0;
  i2c->info->status.mode      = 0;
  i2c->info->status.direction = 0;
}
#endif


/* DMA Stream Event */
#if (RTE_I2C1)
#if (RTE_I2C1_TX_DMA)
void DMAx_STREAMy_EVENT (RTE_I2C1_TX_DMA_NUMBER, RTE_I2C1_TX_DMA_STREAM) (uint32_t event) {
  I2C_DMA_TxEvent (event, &I2C1_Resources);

}
#endif
#if (RTE_I2C1_RX_DMA)
void DMAx_STREAMy_EVENT (RTE_I2C1_RX_DMA_NUMBER, RTE_I2C1_RX_DMA_STREAM) (uint32_t event) {
  I2C_DMA_RxEvent (event, &I2C1_Resources);
}
#endif
#endif

#if (RTE_I2C2)
#if (RTE_I2C2_TX_DMA)
void DMAx_STREAMy_EVENT (RTE_I2C2_TX_DMA_NUMBER, RTE_I2C2_TX_DMA_STREAM) (uint32_t event) {
  I2C_DMA_TxEvent (event, &I2C2_Resources);
}
#endif
#if (RTE_I2C2_RX_DMA)
void DMAx_STREAMy_EVENT (RTE_I2C2_RX_DMA_NUMBER, RTE_I2C2_RX_DMA_STREAM) (uint32_t event) {
  I2C_DMA_RxEvent (event, &I2C2_Resources);
}
#endif
#endif

#if (RTE_I2C3)
#if (RTE_I2C3_TX_DMA)
void DMAx_STREAMy_EVENT (RTE_I2C3_TX_DMA_NUMBER, RTE_I2C3_TX_DMA_STREAM) (uint32_t event) {
  I2C_DMA_TxEvent (event, &I2C3_Resources);
}
#endif
#if (RTE_I2C3_RX_DMA)
void DMAx_STREAMy_EVENT (RTE_I2C3_RX_DMA_NUMBER, RTE_I2C3_RX_DMA_STREAM) (uint32_t event) {
  I2C_DMA_RxEvent (event, &I2C3_Resources);
}
#endif
#endif


/* I2C1 Driver Control Block */
#if (RTE_I2C1)
ARM_DRIVER_I2C Driver_I2C1 = {
  I2CX_GetVersion,
  I2CX_GetCapabilities,
  I2C1_Initialize,
  I2C1_Uninitialize,
  I2C1_PowerControl,
  I2C1_MasterTransmit,
  I2C1_MasterReceive,
  I2C1_SlaveTransmit,
  I2C1_SlaveReceive,
  I2C1_Control,
  I2C1_GetStatus
};
#endif

/* I2C2 Driver Control Block */
#if (RTE_I2C2)
ARM_DRIVER_I2C Driver_I2C2 = {
  I2CX_GetVersion,
  I2CX_GetCapabilities,
  I2C2_Initialize,
  I2C2_Uninitialize,
  I2C2_PowerControl,
  I2C2_MasterTransmit,
  I2C2_MasterReceive,
  I2C2_SlaveTransmit,
  I2C2_SlaveReceive,
  I2C2_Control,
  I2C2_GetStatus
};
#endif

/* I2C3 Driver Control Block */
#if (RTE_I2C3)
ARM_DRIVER_I2C Driver_I2C3 = {
  I2CX_GetVersion,
  I2CX_GetCapabilities,
  I2C3_Initialize,
  I2C3_Uninitialize,
  I2C3_PowerControl,
  I2C3_MasterTransmit,
  I2C3_MasterReceive,
  I2C3_SlaveTransmit,
  I2C3_SlaveReceive,
  I2C3_Control,
  I2C3_GetStatus
};
#endif
