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
 * Project:      SPI Driver definitions for ST STM32F4xx
 * -------------------------------------------------------------------- */

#ifndef __SPI_STM32F4XX_H
#define __SPI_STM32F4XX_H

#include <stdint.h>
#include <stdbool.h>
#include "stm32f4xx.h"

#include "Driver_SPI.h"


/* Current driver status flag definition */
#define SPI_INIT       (1 << 0)               // SPI initialized
#define SPI_POWER      (1 << 1)               // SPI powered on
#define SPI_SETUP      (1 << 2)               // SPI configured
#define SPI_DATA_LOST  (1 << 3)               // SPI data lost occurred
#define SPI_MODE_FAULT (1 << 4)               // SPI mode fault occurred


/* DMA Information definitions */
typedef const struct _SPI_DMA {
  DMA_Stream_TypeDef   *rx_stream;            // Receive stream register interface
  DMA_Stream_TypeDef   *tx_stream;            // Transmit stream register interface
  IRQn_Type             rx_irq_num;           // Receive stream IRQ number
  IRQn_Type             tx_irq_num;           // Transmit stream IRQ number
  uint8_t               rx_channel;           // Receive channel number
  uint8_t               tx_channel;           // Transmit channel number
  uint8_t               rx_priority;          // Receive stream priority
  uint8_t               tx_priority;          // Transmit stream priority
} SPI_DMA;


/* SPI Input/Output Configuration */
typedef const struct _SPI_IO {
  GPIO_TypeDef         *nss_port;             // NNS IO Port
  GPIO_TypeDef         *scl_port;             // SCL IO Port
  GPIO_TypeDef         *miso_port;            // MISO IO Port
  GPIO_TypeDef         *mosi_port;            // MOSI IO Port
  uint8_t               nss_pin;              // NSS IO Pin
  uint8_t               scl_pin;              // SCL IO Pin
  uint8_t               miso_pin;             // MISO IO Pin
  uint8_t               mosi_pin;             // MOSI IO Pin
  uint32_t              af;                   // Alternate function
} SPI_IO;


/* SPI Transfer Information (Run-Time) */
typedef struct _SPI_TRANSFER_INFO {
  uint32_t              num;                  // Total number of transfers
  uint8_t              *rx_buf;               // Pointer to in data buffer
  uint8_t              *tx_buf;               // Pointer to out data buffer
  uint32_t              rx_cnt;               // Number of data received
  uint32_t              tx_cnt;               // Number of data sent
  uint32_t              dump_val;             // Variable for dumping DMA data
  uint16_t              def_val;              // Default transfer value
} SPI_TRANSFER_INFO;


/* SPI Information (Run-Time) */
typedef struct _SPI_INFO {
  ARM_SPI_SignalEvent_t cb_event;             // Event Callback
  ARM_SPI_STATUS        status;               // Status flags
  SPI_TRANSFER_INFO     xfer;                 // Transfer information
  SPI_InitTypeDef       config;               // Current configuration
  uint8_t               flags;                // Current SPI state flags
} SPI_INFO;


/* SPI Resources definition */
typedef struct {
        SPI_TypeDef    *reg;                  // SPI peripheral register interface
        SPI_IO          io;                   // SPI Input/Output pins
        IRQn_Type       irq_num;              // SPI IRQ Number
        uint32_t        periph_bus;           // Peripheral bus
        uint32_t        periph_clock;         // Peripheral clock
        SPI_DMA        *dma;                  // SPI DMA Configuration
        SPI_INFO       *info;                 // Run-Time information
} const SPI_RESOURCES;

#endif /* __SPI_STM32F4XX_H */
