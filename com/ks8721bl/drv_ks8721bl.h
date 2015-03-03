/**************************************************************************//**
* @file    drv_ks8721bl.h
* @brief   KS8721BL driver.
* @author  A. Filyanov
******************************************************************************/
#ifndef _DRV_KS8721BL_H_
#define _DRV_KS8721BL_H_

//-----------------------------------------------------------------------------
enum {
    DRV_REQ_KS8721BL_UNDEF = DRV_REQ_ETH_LAST,
    DRV_REQ_KS8721BL_LINK_INT_CLEAR,
    DRV_REQ_KS8721BL_LINK_INT_SETUP,
    DRV_REQ_KS8721BL_LINK_STATUS_GET,
    DRV_REQ_KS8721BL_AUTO_NEG_RESTART,
    DRV_REQ_KS8721BL_PHY_ID_TEST,
    DRV_REQ_KS8721BL_PHY_RESET,
    DRV_REQ_KS8721BL_LAST
};

#endif //_DRV_KS8721BL_H_