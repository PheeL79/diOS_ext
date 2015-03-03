/**************************************************************************//**
* @file    drv_ks8721bl.c
* @brief   KS8721BL driver.
* @author  A. Filyanov
******************************************************************************/
#include "hal.h"
#include "com/ks8721bl/drv_ks8721bl.h"
#include "os_debug.h"
#include "os_power.h"
#include "os_time.h"

//-----------------------------------------------------------------------------
#define MDL_NAME            "drv_ks8721bl"

//@brief Register map
enum {
    REG_00H_BASIC_CTRL,
    REG_01H_BASIC_STATUS,
    REG_02H_PHY_ID1,
    REG_03H_PHY_ID2,
    REG_04H_AUTO_NEG_ADVERT,
    REG_05H_AUTO_NEG_LINK_PART_ABILITY,
    REG_06H_AUTO_NEG_EXPANSION,
    REG_07H_AUTO_NEG_NEXT_PAGE,
    REG_08H_LINK_PART_ABILITY_NEXT_PAGE,
    REG_15H_RXER_COUNTER                    = 0x15,
    REG_1BH_INT_CTRL_STATUS                 = 0x1B,
    REG_1FH_100BASE_TX_PHY_CTRL             = 0x1F
};

//@brief Registers bits
#define REG_00H_BIT_RESET                   15
#define REG_00H_BIT_SPEED_SELECT            13
#define REG_00H_BIT_AUTO_NEG_EN             12
#define REG_00H_BIT_AUTO_NEG_RST            9
#define REG_00H_BIT_DUPLEX_MODE             8

#define REG_01H_BIT_100BASE_TX_FULL_DUPLEX  14
#define REG_01H_BIT_100BASE_TX_HALF_DUPLEX  13
#define REG_01H_BIT_10BASE_T_FULL_DUPLEX    12
#define REG_01H_BIT_10BASE_T_HALF_DUPLEX    11
#define REG_01H_BIT_AUTO_NEG_COMPLETE       5
#define REG_01H_BIT_AUTO_NEG_ABILITY        3
#define REG_01H_BIT_LINK_STATUS             2

#define REG_1BH_BIT_LINK_DOWN_IE            10
#define REG_1BH_BIT_LINK_UP_IE              8

//------------------------------------------------------------------------------
static Status   KS8721BL_Init(void* args_p);
static Status   KS8721BL_DeInit(void* args_p);
static Status   KS8721BL_Open(void* args_p);
static Status   KS8721BL_Close(void* args_p);
static Status   KS8721BL_IoCtl(const U32 request_id, void* args_p);

//------------------------------------------------------------------------------
/*static*/ HAL_DriverItf drv_ks8721bl = {
    .Init   = KS8721BL_Init,
    .DeInit = KS8721BL_DeInit,
    .Open   = KS8721BL_Open,
    .Close  = KS8721BL_Close,
    .IoCtl  = KS8721BL_IoCtl
};

/******************************************************************************/
Status KS8721BL_Init(void* args_p)
{
Status s = S_UNDEF;
    s = drv_ks8721bl.IoCtl(DRV_REQ_KS8721BL_PHY_RESET, OS_NULL);
    return s;
}

/******************************************************************************/
Status KS8721BL_DeInit(void* args_p)
{
Status s = S_UNDEF;
    s = S_OK;
    return s;
}

/******************************************************************************/
Status KS8721BL_Open(void* args_p)
{
Status s = S_UNDEF;
    s = S_OK;
    return s;
}

/******************************************************************************/
Status KS8721BL_Close(void* args_p)
{
Status s = S_UNDEF;
    s = S_OK;
    return s;
}

/******************************************************************************/
Status KS8721BL_IoCtl(const U32 request_id, void* args_p)
{
extern ETH_HandleTypeDef eth0_hd;
Status s = S_UNDEF;
    switch (request_id) {
        case DRV_REQ_STD_POWER_SET: {
            HAL_StatusTypeDef hal_status = HAL_OK;
            switch (*(OS_PowerState*)args_p) {
                case PWR_ON:
                    s = S_OK;
                    break;
                case PWR_OFF:
                    s = S_OK;
                    break;
                default:
                    break;
            }
            if (HAL_OK != hal_status) { s = S_HARDWARE_FAULT; }
            }
            break;
        case DRV_REQ_KS8721BL_LINK_INT_CLEAR:
            {
            U32 reg_value = 0;
            s = S_OK;
            //Clear interrupt status by read.
            HAL_ETH_ReadPHYRegister(&eth0_hd, REG_1BH_INT_CTRL_STATUS, &reg_value);
            }
            break;
        case DRV_REQ_KS8721BL_LINK_INT_SETUP:
            {
            U32 reg_value = 0;
            s = S_OK;
            /**** Configure PHY to generate an interrupt when Eth Link state changes ****/
            /* Read Register Configuration */
            HAL_ETH_ReadPHYRegister(&eth0_hd, REG_1BH_INT_CTRL_STATUS, &reg_value);
            BIT_SET(reg_value, BIT(REG_1BH_BIT_LINK_UP_IE) | BIT(REG_1BH_BIT_LINK_DOWN_IE));
            /* Enable Interrupt on change of link status */
            HAL_ETH_WritePHYRegister(&eth0_hd, REG_1BH_INT_CTRL_STATUS, reg_value);
            }
            break;
        case DRV_REQ_KS8721BL_LINK_STATUS_GET:
            {
            U32 reg_value = 0;
            s = S_OK;
            //Get link status.
            HAL_ETH_ReadPHYRegister(&eth0_hd, REG_01H_BASIC_STATUS, &reg_value);
            *(Bool*)args_p = BIT_TEST(reg_value, BIT(REG_01H_BIT_LINK_STATUS));
            }
            break;
        case DRV_REQ_KS8721BL_AUTO_NEG_RESTART:
            {
            U32 reg_value = 0;
            s = S_OK;
            HAL_ETH_ReadPHYRegister(&eth0_hd, REG_01H_BASIC_STATUS, &reg_value);
            if (ETH_AUTONEGOTIATION_DISABLE != eth0_hd.Init.AutoNegotiation) {
                if (BIT_TEST(reg_value, BIT(REG_01H_BIT_AUTO_NEG_ABILITY))) {
                    HAL_IO U32 tick_start = 0;
                    HAL_ETH_ReadPHYRegister(&eth0_hd, REG_00H_BASIC_CTRL, &reg_value);
                    /* Enable and Restart Auto-Negotiation process */
                    BIT_SET(reg_value, BIT(REG_00H_BIT_AUTO_NEG_EN) | BIT(REG_00H_BIT_AUTO_NEG_RST));
                    HAL_ETH_WritePHYRegister(&eth0_hd, REG_00H_BASIC_CTRL, reg_value);
                    /* Get tick */
                    tick_start = HAL_GetTick();
                    /* Wait until the auto-negotiation will be completed */
                    do {
                        HAL_ETH_ReadPHYRegister(&eth0_hd, REG_01H_BASIC_STATUS, &reg_value);
                        /* Check for the Timeout ( 3s ) */
                        if ((HAL_GetTick() - tick_start) > OS_MS_TO_TICKS(3000)) {
                            /* In case of timeout */
                            goto error;
                        }
                    } while (!BIT_TEST(reg_value, BIT(REG_01H_BIT_AUTO_NEG_COMPLETE)));
                    /* Configure the MAC with the Duplex Mode fixed by the auto-negotiation process */
                    if (BIT_TEST(reg_value, BIT(REG_01H_BIT_100BASE_TX_FULL_DUPLEX) | BIT(REG_01H_BIT_10BASE_T_FULL_DUPLEX))) {
                        /* Set Ethernet duplex mode to Full-duplex following the auto-negotiation */
                        eth0_hd.Init.DuplexMode = ETH_MODE_FULLDUPLEX;
                    } else {
                        /* Set Ethernet duplex mode to Half-duplex following the auto-negotiation */
                        eth0_hd.Init.DuplexMode = ETH_MODE_HALFDUPLEX;
                    }
                    /* Configure the MAC with the speed fixed by the auto-negotiation process */
                    if (BIT_TEST(reg_value, BIT(REG_01H_BIT_100BASE_TX_FULL_DUPLEX) | BIT(REG_01H_BIT_100BASE_TX_HALF_DUPLEX))) {
                        /* Set Ethernet speed to 100M following the auto-negotiation */
                        eth0_hd.Init.Speed = ETH_SPEED_100M;
                    } else {
                        /* Set Ethernet speed to 10M following the auto-negotiation */
                        eth0_hd.Init.Speed = ETH_SPEED_10M;
                    }
                } else { /* AutoNegotiation Disable */
error:
                    /* Check parameters */
                    OS_ASSERT_VALUE(IS_ETH_SPEED(eth0_hd.Init.Speed));
                    OS_ASSERT_VALUE(IS_ETH_DUPLEX_MODE(eth0_hd.Init.DuplexMode));
                    HAL_ETH_ReadPHYRegister(&eth0_hd, REG_00H_BASIC_CTRL, &reg_value);
                    if (ETH_SPEED_100M == eth0_hd.Init.Speed) {
                        BIT_SET(reg_value, BIT(REG_00H_BIT_SPEED_SELECT));
                    } else {
                        BIT_CLEAR(reg_value, BIT(REG_00H_BIT_SPEED_SELECT));
                    }
                    if (ETH_MODE_FULLDUPLEX == eth0_hd.Init.DuplexMode) {
                        BIT_SET(reg_value, BIT(REG_00H_BIT_DUPLEX_MODE));
                    } else {
                        BIT_CLEAR(reg_value, BIT(REG_00H_BIT_DUPLEX_MODE));
                    }
                    /* Set MAC Speed and Duplex Mode to PHY */
                    HAL_ETH_WritePHYRegister(&eth0_hd, REG_00H_BASIC_CTRL, reg_value);
                }
            }
            }
            break;
        case DRV_REQ_KS8721BL_PHY_ID_TEST:
            {
            U32 phy_id_hi;
            U32 phy_id_lo;
            HAL_ETH_ReadPHYRegister(&eth0_hd, REG_02H_PHY_ID1, &phy_id_hi);  // 0x0022
            HAL_ETH_ReadPHYRegister(&eth0_hd, REG_03H_PHY_ID2, &phy_id_lo);  // 0x1619
            if ((0x0022 == phy_id_hi) &&
                (0x1619 == phy_id_lo)) {
                    s = S_OK;
                } else {
                    s = S_HARDWARE_FAULT;
                }
            }
            break;
        case DRV_REQ_KS8721BL_PHY_RESET:
            {
            U32 reg_value = 0;
            HAL_IO U32 tick_start = 0;
            s = S_OK;
            HAL_ETH_ReadPHYRegister(&eth0_hd, REG_00H_BASIC_CTRL, &reg_value);
            /* Enable Reset process */
            BIT_SET(reg_value, BIT(REG_00H_BIT_RESET));
            HAL_ETH_WritePHYRegister(&eth0_hd, REG_00H_BASIC_CTRL, reg_value);
            /* Get tick */
            tick_start = HAL_GetTick();
            /* Wait until the reset will be completed */
            do {
                HAL_ETH_ReadPHYRegister(&eth0_hd, REG_00H_BASIC_CTRL, &reg_value);
                /* Check for the Timeout ( 3s ) */
                if ((HAL_GetTick() - tick_start) > OS_MS_TO_TICKS(3000)) {
                    /* In case of timeout */
                    s = S_HARDWARE_FAULT;
                }
            } while (BIT_TEST(reg_value, BIT(REG_00H_BIT_RESET)));
            }
            break;
        default:
            HAL_LOG_S(D_WARNING, S_UNDEF_REQ_ID);
            break;
    }
    return s;
}
