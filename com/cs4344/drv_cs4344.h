/**************************************************************************//**
* @file    drv_cs4344.h
* @brief   CS4344 driver.
* @author  A. Filyanov
******************************************************************************/
#ifndef _DRV_CS4344_H_
#define _DRV_CS4344_H_

#include "drv_audio.h"
#include "os_audio.h"

#if (OS_AUDIO_ENABLED)
//-----------------------------------------------------------------------------
#define CS4344_I2Sx                         SPI3

enum {
    DRV_REQ_CS4344_UNDEF = DRV_REQ_AUDIO_LAST,
    DRV_REQ_CS4344_LAST
};

typedef OS_AudioDeviceIoSetupArgs CS4344_DrvArgsInit;
typedef OS_AudioDeviceArgsOpen CS4344_DrvArgsOpen;

extern const OS_AudioDeviceCaps cs4344_caps;
#endif //(OS_AUDIO_ENABLED)

#endif //_DRV_CS4344_H_