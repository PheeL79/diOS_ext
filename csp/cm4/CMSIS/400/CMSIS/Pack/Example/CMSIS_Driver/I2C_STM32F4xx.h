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
 * Project:      I2C Driver definitions for ST STM32F4xx
 * -------------------------------------------------------------------- */

#ifndef __I2C_STM32F4XX_H
#define __I2C_STM32F4XX_H

#include <stdint.h>
#include <stdbool.h>
#include "stm32f4xx.h"

#include "Driver_I2C.h"


/* Current driver status flag definition */
#define I2C_INIT            (1 << 0)          // I2C initialized
#define I2C_POWER           (1 << 1)          // I2C powered on
#define I2C_SETUP           (1 << 2)          // I2C configured

/* Transfer status flags definitions */
#define XFER_CTRL_GEN_STOP  (1 << 0)          // Generate stop condition
#define XFER_CTRL_RSTART    (1 << 1)          // Generate repeated start and readdress
#define XFER_CTRL_ADDR_DONE (1 << 2)          // Addressing done


/* DMA Information definitions */
typedef const struct _I2C_DMA {
  DMA_Stream_TypeDef   *rx_stream;            // Receive stream register interface
  DMA_Stream_TypeDef   *tx_stream;            // Transmit stream register interface
  IRQn_Type             rx_irq_num;           // Receive stream IRQ number
  IRQn_Type             tx_irq_num;           // Transmit stream IRQ number
  uint8_t               rx_channel;           // Receive channel number
  uint8_t               tx_channel;           // Transmit channel number
  uint8_t               rx_priority;          // Receive stream priority
  uint8_t               tx_priority;          // Transmit stream priority
} I2C_DMA;


/* I2C Input/Output Configuration */
typedef const struct _I2C_IO {
  GPIO_TypeDef         *scl_port;             // SCL IO Port
  GPIO_TypeDef         *sda_port;             // SDA IO Port
  uint8_t               scl_pin;              // SCL IO Pin
  uint8_t               sda_pin;              // SDA IO Pin
} I2C_IO;


/* I2C Transfer Information (Run-Time) */
typedef struct _I2C_TRANSFER_INFO {
  uint32_t              num;                  // Number of data to transfer
  uint32_t              cnt;                  // Data transfer counter
  uint8_t              *data;                 // Data pointer
  uint16_t              addr;                 // Device address
  uint8_t               ctrl;                 // Transfer control flags
} I2C_TRANSFER_INFO;


/* I2C Information (Run-Time) */
typedef struct _I2C_INFO {
  ARM_I2C_SignalEvent_t cb_event;             // Event Callback
  ARM_I2C_STATUS        status;               // Status flags
  I2C_TRANSFER_INFO     xfer;                 // Transfer information
  uint8_t               init;                 // Init counter
  uint8_t               flags;                // Current I2C state flags
  uint16_t volatile     error;                // Error status flags
} I2C_INFO;


/* I2C Resources definition */
typedef struct {
        I2C_TypeDef    *reg;                  // I2C peripheral register interface
        I2C_IO          io;                   // I2C Input/Output pins
        IRQn_Type       ev_irq_num;           // I2C Event IRQ Number
        IRQn_Type       er_irq_num;           // I2C Error IRQ Number
        uint32_t        periph_clock_mask;    // Peripheral clock enable bitmask
        uint32_t        periph_clock;         // Peripheral clock
        I2C_DMA        *dma;                  // I2C DMA Configuration
        I2C_INFO       *info;                 // Run-Time information
} const I2C_RESOURCES;

#endif /* __I2C_STM32F4XX_H */
