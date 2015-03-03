/**************************************************************************//**
* @file    drv_cs4344.c
* @brief   CS4344 driver.
* @author  A. Filyanov
******************************************************************************/
#include "hal.h"
#include "drv_audio_bsp.h"
#include "os_debug.h"
#include "os_supervise.h"

#if (OS_AUDIO_ENABLED)
//-----------------------------------------------------------------------------
#define MDL_NAME            "drv_cs4344"

#define CS4344_I2Sx_FORCE_RESET()           __SPI3_FORCE_RESET()
#define CS4344_I2Sx_RELEASE_RESET()         __SPI3_RELEASE_RESET()

/* I2S peripheral configuration defines */
#define CS4344_I2Sx_CLK_ENABLE()            __SPI3_CLK_ENABLE()
#define CS4344_I2Sx_SCK_SD_WS_AF            GPIO_AF6_SPI3
#define CS4344_I2Sx_WS_CLK_ENABLE()         __GPIOA_CLK_ENABLE()
#define CS4344_I2Sx_SCK_SD_CLK_ENABLE()     __GPIOB_CLK_ENABLE()
#define CS4344_I2Sx_MCK_CLK_ENABLE()        __GPIOC_CLK_ENABLE()
#define CS4344_I2Sx_WS_PIN                  GPIO_PIN_15
#define CS4344_I2Sx_SCK_PIN                 GPIO_PIN_3
#define CS4344_I2Sx_SD_PIN                  GPIO_PIN_5
#define CS4344_I2Sx_MCK_PIN                 GPIO_PIN_7
#define CS4344_I2Sx_WS_GPIO_PORT            GPIOA
#define CS4344_I2Sx_SCK_SD_GPIO_PORT        GPIOB
#define CS4344_I2Sx_MCK_GPIO_PORT           GPIOC

/* I2S DMA Stream definitions */
#define CS4344_I2Sx_DMAx_CLK_ENABLE()       __DMA1_CLK_ENABLE()
#define CS4344_I2Sx_DMAx_STREAM             DMA1_Stream5
#define CS4344_I2Sx_DMAx_CHANNEL            DMA_CHANNEL_0
#define CS4344_I2Sx_DMAx_IRQn               DMA1_Stream5_IRQn

#define CS4344_I2Sx_DMAx_IRQHandler         DMA1_Stream5_IRQHandler

//------------------------------------------------------------------------------
static Status   CS4344_Init(void* args_p);
static Status   CS4344_DeInit(void* args_p);
static Status   CS4344_LL_Init(void* args_p);
//static Status   CS4344_LL_DeInit(void* args_p);
static Status   CS4344_Open(void* args_p);
static Status   CS4344_Close(void* args_p);
static Status   CS4344_IoCtl(const U32 request_id, void* args_p);

static S8       FrequencyIdxGet(const OS_AudioFreq freq);
static Status   OutputSetup(const OS_AudioDeviceIoSetupArgs* args_p);

static void     CS4344_XferCpltCallback(DMA_HandleTypeDef* hdma);
static void     CS4344_XferM1CpltCallback(DMA_HandleTypeDef* hdma);

//------------------------------------------------------------------------------
/* These PLL parameters are valide when the f(VCO clock) = 1Mhz */
static const OS_AudioBits cs4344_bits_v[]   = { 8, 16, 24, 0 };
static const OS_AudioFreq cs4344_freqs_v[]  = { 8000, 11025,  16000,  22050,  32000,  44100,  48000,  96000,  192000,   0   };
static const U32 i2s_plln_v[]               = { 256,  429,    213,    429,    213,    271,    258,    344,    394           };
static const U32 i2s_pllr_v[]               = { 5,    4,      2,      4,      2,      2,      3,      2,      2             };

static const OS_AudioDeviceCapsOutput cs4344_caps_out = {
    .sample_rates_vp= &cs4344_freqs_v[0],
    .sample_bits_vp = &cs4344_bits_v[0],
    .channels       = OS_AUDIO_CHANNELS_STEREO,
    .out            = OS_AUDIO_DEVICE_OUT_HEADPHONE
};

const OS_AudioDeviceCaps cs4344_caps = {
    .input_p = OS_NULL,
    .output_p= &cs4344_caps_out
};

static I2S_HandleTypeDef        i2s_hd;
static DMA_HandleTypeDef        dma_tx_hd;
//static DMA_HandleTypeDef        dma_rx_hd;
static CS4344_DrvArgsInit       cs4344_out_setup;
OS_AudioDeviceCallbackArgs      cs4344_callback_args;
OS_ISR_AudioDeviceCallback      CS4344_ISR_DrvAudioDeviceCallback;

HAL_DriverItf drv_cs4344 = {
    .Init   = CS4344_Init,
    .DeInit = CS4344_DeInit,
    .Open   = CS4344_Open,
    .Close  = CS4344_Close,
    .Read   = OS_NULL,
    .Write  = OS_NULL,
    .IoCtl  = CS4344_IoCtl
};

/******************************************************************************/
Status CS4344_Init(void* args_p)
{
Status s = S_UNDEF;
    //HAL_LOG(D_INFO, "Init: ");
    if (OS_NULL == args_p) { return s = S_INVALID_REF; }
    IF_STATUS(s = CS4344_LL_Init(args_p)) {
    }
    return s;
}

/******************************************************************************/
Status CS4344_LL_Init(void* args_p)
{
const CS4344_DrvArgsInit* drv_args_p = (CS4344_DrvArgsInit*)args_p;
RCC_PeriphCLKInitTypeDef RCC_ExCLKInitStruct;
GPIO_InitTypeDef GPIO_InitStruct;
const S8 freq_idx = FrequencyIdxGet(drv_args_p->info.sample_rate);

    if (S8_MAX == freq_idx) { return S_INVALID_VALUE; }

    HAL_RCCEx_GetPeriphCLKConfig(&RCC_ExCLKInitStruct);
    /* I2S clock config
    PLLI2S_VCO = f(VCO clock) = f(PLLI2S clock input) x (PLLI2SN/PLLM)
    I2SCLK = f(PLLI2S clock output) = f(VCO clock) / PLLI2SR */
    RCC_ExCLKInitStruct.PeriphClockSelection= RCC_PERIPHCLK_I2S;
    RCC_ExCLKInitStruct.PLLI2S.PLLI2SN      = i2s_plln_v[freq_idx];
    RCC_ExCLKInitStruct.PLLI2S.PLLI2SR      = i2s_pllr_v[freq_idx];
    HAL_RCCEx_PeriphCLKConfig(&RCC_ExCLKInitStruct);

    /* Initialize the i2s_hd Instance parameter */
    i2s_hd.Instance             = CS4344_I2Sx;

    /* Disable I2S block */
    __HAL_I2S_DISABLE(&i2s_hd);

    /* Enable I2S clock */
    CS4344_I2Sx_CLK_ENABLE();

    /* Enable SCK, SD and WS GPIO clock */
    CS4344_I2Sx_WS_CLK_ENABLE();
    CS4344_I2Sx_SCK_SD_CLK_ENABLE();

    /* CODEC_I2S pins configuration: WS, SCK and SD pins */
    GPIO_InitStruct.Pin         = CS4344_I2Sx_SCK_PIN;
    GPIO_InitStruct.Mode        = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull        = GPIO_NOPULL;
    GPIO_InitStruct.Speed       = GPIO_SPEED_MEDIUM;
    GPIO_InitStruct.Alternate   = CS4344_I2Sx_SCK_SD_WS_AF;
    HAL_GPIO_Init(CS4344_I2Sx_SCK_SD_GPIO_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin         = CS4344_I2Sx_SD_PIN;
    HAL_GPIO_Init(CS4344_I2Sx_SCK_SD_GPIO_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin         = CS4344_I2Sx_WS_PIN;
    HAL_GPIO_Init(CS4344_I2Sx_WS_GPIO_PORT, &GPIO_InitStruct);

    /* Enable MCK GPIO clock */
    CS4344_I2Sx_MCK_CLK_ENABLE();

    /* CODEC_I2S pins configuration: MCK pin */
    GPIO_InitStruct.Pin         = CS4344_I2Sx_MCK_PIN;
    GPIO_InitStruct.Speed       = GPIO_SPEED_FAST;
    HAL_GPIO_Init(CS4344_I2Sx_MCK_GPIO_PORT, &GPIO_InitStruct);

    /* Enable the DMA clock */
    CS4344_I2Sx_DMAx_CLK_ENABLE();

    /* Configure the dma_tx_hd handle parameters */
    dma_tx_hd.Instance                  = CS4344_I2Sx_DMAx_STREAM;

    dma_tx_hd.Init.Channel              = CS4344_I2Sx_DMAx_CHANNEL;
    dma_tx_hd.Init.Direction            = DMA_MEMORY_TO_PERIPH;
    dma_tx_hd.Init.PeriphInc            = DMA_PINC_DISABLE;
    dma_tx_hd.Init.MemInc               = DMA_MINC_ENABLE;
    dma_tx_hd.Init.PeriphDataAlignment  = (16 >= drv_args_p->info.sample_bits) ? DMA_PDATAALIGN_HALFWORD : DMA_PDATAALIGN_WORD;
    dma_tx_hd.Init.MemDataAlignment     = (16 >= drv_args_p->info.sample_bits) ? DMA_MDATAALIGN_HALFWORD : DMA_MDATAALIGN_WORD;
    dma_tx_hd.Init.Mode                 = (OS_AUDIO_DMA_MODE_NORMAL == drv_args_p->dma_mode) ? DMA_NORMAL :
                                              (OS_AUDIO_DMA_MODE_CIRCULAR == drv_args_p->dma_mode) ? DMA_CIRCULAR : ~0;
    dma_tx_hd.Init.Priority             = DMA_PRIORITY_VERY_HIGH;
    dma_tx_hd.Init.FIFOMode             = DMA_FIFOMODE_ENABLE;
    dma_tx_hd.Init.FIFOThreshold        = DMA_FIFO_THRESHOLD_FULL;
    dma_tx_hd.Init.MemBurst             = DMA_MBURST_SINGLE;
    dma_tx_hd.Init.PeriphBurst          = DMA_PBURST_SINGLE;

    /* Associate the DMA handle */
    __HAL_LINKDMA(&i2s_hd, hdmatx, dma_tx_hd);

    /* Deinitialize the Stream for new transfer */
    HAL_DMA_DeInit(&dma_tx_hd);

    /* Configure the DMA Stream */
    HAL_DMA_Init(&dma_tx_hd);

    /* I2S DMA IRQ Channel configuration */
    HAL_NVIC_SetPriority(CS4344_I2Sx_DMAx_IRQn, OS_PRIORITY_INT_MAX + 1, 0);
    HAL_NVIC_EnableIRQ(CS4344_I2Sx_DMAx_IRQn);

    i2s_hd.Init.Mode            = I2S_MODE_MASTER_TX;
    i2s_hd.Init.Standard        = I2S_STANDARD_PHILIPS;
    i2s_hd.Init.DataFormat      = (16 >= drv_args_p->info.sample_bits) ? I2S_DATAFORMAT_16B :
                                      (24 == drv_args_p->info.sample_bits) ? I2S_DATAFORMAT_24B :
                                          (32 == drv_args_p->info.sample_bits) ? I2S_DATAFORMAT_32B : ~0;
    OS_ASSERT(~0 != i2s_hd.Init.DataFormat);
    i2s_hd.Init.AudioFreq       = drv_args_p->info.sample_rate;
    i2s_hd.Init.ClockSource     = I2S_CLOCK_PLL;
    i2s_hd.Init.CPOL            = I2S_CPOL_LOW;
    i2s_hd.Init.MCLKOutput      = I2S_MCLKOUTPUT_ENABLE;
    i2s_hd.Init.FullDuplexMode  = I2S_FULLDUPLEXMODE_DISABLE;

    OS_ASSERT(HAL_I2S_STATE_RESET == HAL_I2S_GetState(&i2s_hd));
    /* Init the I2S */
    if (HAL_OK != HAL_I2S_Init(&i2s_hd)) { return S_HARDWARE_FAULT; }
    cs4344_out_setup = *drv_args_p;
    return S_OK;
}

/******************************************************************************/
Status CS4344_DeInit(void* args_p)
{
    /* Initialize the i2s_hd Instance parameter */
    i2s_hd.Instance             = CS4344_I2Sx;

    if (HAL_OK != HAL_I2S_DeInit(&i2s_hd)) { return S_HARDWARE_FAULT; }
    /*##-1- Reset peripherals ##################################################*/
    CS4344_I2Sx_FORCE_RESET();
    CS4344_I2Sx_RELEASE_RESET();

    /* Disable I2S block */
    __HAL_I2S_DISABLE(&i2s_hd);

    HAL_GPIO_DeInit(CS4344_I2Sx_MCK_GPIO_PORT,      CS4344_I2Sx_MCK_PIN);
    HAL_GPIO_DeInit(CS4344_I2Sx_SCK_SD_GPIO_PORT,   CS4344_I2Sx_SCK_PIN);
    HAL_GPIO_DeInit(CS4344_I2Sx_SCK_SD_GPIO_PORT,   CS4344_I2Sx_SD_PIN);
    HAL_GPIO_DeInit(CS4344_I2Sx_WS_GPIO_PORT,       CS4344_I2Sx_WS_PIN);

    /*##-3- Disable the DMA Streams ############################################*/
    /* De-Initialize the DMA Stream associate to transmission process */
    HAL_DMA_DeInit(i2s_hd.hdmatx);

    /*##-5- Disable the NVIC for DMA ###########################################*/
    HAL_NVIC_DisableIRQ(CS4344_I2Sx_DMAx_IRQn);
    return S_OK;
}

/******************************************************************************/
Status CS4344_Open(void* args_p)
{
    if (OS_NULL != args_p) {
        const CS4344_DrvArgsOpen* open_args_p = (CS4344_DrvArgsOpen*)args_p;
        CS4344_ISR_DrvAudioDeviceCallback = open_args_p->isr_callback_func;
        cs4344_callback_args.tstor_p = open_args_p->tstor_p;
        cs4344_callback_args.slot_qhd = open_args_p->slot_qhd;
        cs4344_callback_args.signal_id = OS_SIG_UNDEF;
    }
    return S_OK;
}

/******************************************************************************/
Status CS4344_Close(void* args_p)
{
    CS4344_ISR_DrvAudioDeviceCallback = OS_NULL;
    return S_OK;
}

/******************************************************************************/
Status CS4344_IoCtl(const U32 request_id, void* args_p)
{
Status s = S_UNDEF;
    switch (request_id) {
// Standard driver's requests.
        case DRV_REQ_STD_SYNC:
            s = S_OK;
            break;
        case DRV_REQ_STD_POWER_SET:
            switch (*(OS_PowerState*)args_p) {
                case PWR_ON:
                    s = S_OK;
                    break;
                case PWR_OFF:
                case PWR_SLEEP:
                case PWR_STOP:
                case PWR_STANDBY:
                case PWR_HIBERNATE:
                case PWR_SHUTDOWN:
                    s = S_OK;
                    break;
                default:
                    break;
            }
            break;
// Audio driver's requests.
        case DRV_REQ_AUDIO_PLAY: {
            const DrvAudioPlayArgs* drv_args_p = (DrvAudioPlayArgs*)args_p;
            if (OS_AUDIO_DMA_MODE_NORMAL == cs4344_out_setup.dma_mode) {
                while (HAL_I2S_STATE_READY != HAL_I2S_GetState(&i2s_hd)) {};
                if (HAL_OK != HAL_I2S_Transmit_DMA(&i2s_hd, (U16*)drv_args_p->data_p, (drv_args_p->size / sizeof(U16)))) {
                    s = S_HARDWARE_FAULT;
                } else {
                    s = S_OK;
                }
            } else if (OS_AUDIO_DMA_MODE_CIRCULAR == cs4344_out_setup.dma_mode) {
                //HAL_StatusTypeDef HAL_I2S_Transmit_DMA(I2S_HandleTypeDef *hi2s, uint16_t *pData, uint16_t Size);
                U32 tmp1 = 0, tmp2 = 0;

                if ((drv_args_p->data_p == NULL) || (drv_args_p->size == 0)) {
                    s = S_HARDWARE_FAULT;
                    break;
                }

                if (i2s_hd.State == HAL_I2S_STATE_READY) {
                    tmp1 = i2s_hd.Instance->I2SCFGR & (SPI_I2SCFGR_DATLEN | SPI_I2SCFGR_CHLEN);
                    tmp2 = i2s_hd.Instance->I2SCFGR & (SPI_I2SCFGR_DATLEN | SPI_I2SCFGR_CHLEN);

                    if((tmp1 == I2S_DATAFORMAT_24B)|| \
                       (tmp2 == I2S_DATAFORMAT_32B)) {
                        i2s_hd.TxXferSize  = drv_args_p->size / sizeof(U32);
                        i2s_hd.TxXferCount = drv_args_p->size / sizeof(U32);
                    } else {
                        i2s_hd.TxXferSize  = drv_args_p->size / sizeof(U16);
                        i2s_hd.TxXferCount = drv_args_p->size / sizeof(U16);
                    }

                    /* Process Locked */
                    __HAL_LOCK(&i2s_hd);

                    i2s_hd.State = HAL_I2S_STATE_BUSY_TX;
                    i2s_hd.ErrorCode = HAL_I2S_ERROR_NONE;

                    /* Set the I2S Tx DMA Half transfert complete callback */
                    //i2s_hd.hdmatx->XferHalfCpltCallback = I2S_DMATxHalfCplt;

                    /* Set the I2S Tx DMA transfert complete callback */
                    i2s_hd.hdmatx->XferCpltCallback = CS4344_XferCpltCallback;

                    /* Set the I2S Tx DMA transfert complete callback */
                    i2s_hd.hdmatx->XferM1CpltCallback = CS4344_XferM1CpltCallback;

                    /* Set the DMA error callback */
                    i2s_hd.hdmatx->XferErrorCallback = I2S_DMAError;

                    /* Enable the Tx DMA Stream */
    //                HAL_DMA_Start_IT(i2s_hd.hdmatx, *(U32*)tmp, (U32)&i2s_hd.Instance->DR, i2s_hd.TxXferSize);
                    HAL_DMAEx_MultiBufferStart_IT(i2s_hd.hdmatx, (U32)drv_args_p->data_p, (U32)&i2s_hd.Instance->DR,
                                                  (U32)(drv_args_p->data_p + drv_args_p->size), i2s_hd.TxXferSize);

                    /* Check if the I2S is already enabled */
                    if ((i2s_hd.Instance->I2SCFGR &SPI_I2SCFGR_I2SE) != SPI_I2SCFGR_I2SE) {
                        /* Enable I2S peripheral */
                        __HAL_I2S_ENABLE(&i2s_hd);
                    }

                     /* Check if the I2S Tx request is already enabled */
                    if ((i2s_hd.Instance->CR2 & SPI_CR2_TXDMAEN) != SPI_CR2_TXDMAEN) {
                        /* Enable Tx DMA Request */
                        i2s_hd.Instance->CR2 |= SPI_CR2_TXDMAEN;
                    }

                    /* Process Unlocked */
                    __HAL_UNLOCK(&i2s_hd);

                    s = S_OK;
                } else {
                    s = S_BUSY;
                }
            } else { s = S_INVALID_VALUE; }
            /* Disable the Half transfer interrupt */
            __HAL_DMA_DISABLE_IT(i2s_hd.hdmatx, DMA_IT_HT);
            }
            break;
        case DRV_REQ_AUDIO_STOP:
            if (HAL_OK != HAL_I2S_DMAStop(&i2s_hd)) {
                s = S_HARDWARE_FAULT;
            } else {
                s = S_OK;
            }
            break;
        case DRV_REQ_AUDIO_PAUSE:
            if (HAL_OK != HAL_I2S_DMAPause(&i2s_hd)) {
                s = S_HARDWARE_FAULT;
            } else {
                s = S_OK;
            }
            break;
        case DRV_REQ_AUDIO_RESUME:
            if (HAL_OK != HAL_I2S_DMAResume(&i2s_hd)) {
                s = S_HARDWARE_FAULT;
            } else {
                s = S_OK;
            }
            break;
        case DRV_REQ_AUDIO_SEEK:
            s = S_OK;
            break;
        case DRV_REQ_AUDIO_MUTE_SET:
            s = S_OK;
            break;
        case DRV_REQ_AUDIO_VOLUME_GET:
            *(OS_AudioVolume*)args_p = cs4344_out_setup.volume;
            s = S_OK;
            break;
        case DRV_REQ_AUDIO_VOLUME_SET:
            cs4344_out_setup.volume = *(OS_AudioVolume*)args_p;
            s = S_OK;
            break;
        case DRV_REQ_AUDIO_OUTPUT_BITS_GET:
            *(OS_AudioBits*)args_p = cs4344_out_setup.info.sample_bits;
            s = S_OK;
            break;
        case DRV_REQ_AUDIO_OUTPUT_BITS_SET:
            break;
        case DRV_REQ_AUDIO_OUTPUT_FREQUENCY_GET:
            *(OS_AudioFreq*)args_p = i2s_hd.Init.AudioFreq;
            s = S_OK;
            break;
        case DRV_REQ_AUDIO_OUTPUT_FREQUENCY_SET:
            break;
        case DRV_REQ_AUDIO_OUTPUT_SETUP:
            s = OutputSetup((OS_AudioDeviceIoSetupArgs*)args_p);
            break;
// Device driver's requests.
        default:
            s = S_UNDEF_REQ_ID;
            break;
    }
    return s;
}

/******************************************************************************/
S8 FrequencyIdxGet(const OS_AudioFreq freq)
{
    for (Size i = 0; 0 != cs4344_freqs_v[i]; ++i) {
        if (freq == cs4344_freqs_v[i]) {
            return i;
        }
    }
    return S8_MAX;
}

/******************************************************************************/
Status OutputSetup(const OS_AudioDeviceIoSetupArgs* args_p)
{
CS4344_DrvArgsInit* drv_args_p = args_p;
Status s = S_UNDEF;
    IF_STATUS(s = CS4344_DeInit(OS_NULL))           { return s; }
    IF_STATUS(s = CS4344_LL_Init((void*)drv_args_p)) { return s; }
    return s;
}

// I2S DMA IRQ handlers---------------------------------------------------------
/******************************************************************************/
void CS4344_I2Sx_DMAx_IRQHandler(void);
void CS4344_I2Sx_DMAx_IRQHandler(void)
{
    HAL_DMA_IRQHandler(i2s_hd.hdmatx);
}

/******************************************************************************/
void CS4344_XferCpltCallback(DMA_HandleTypeDef* hdma)
{
    cs4344_callback_args.signal_id = OS_SIG_AUDIO_TX_COMPLETE;
    CS4344_ISR_DrvAudioDeviceCallback(&cs4344_callback_args);
}

/******************************************************************************/
void CS4344_XferM1CpltCallback(DMA_HandleTypeDef* hdma)
{
    cs4344_callback_args.signal_id = OS_SIG_AUDIO_TX_COMPLETE;
    CS4344_ISR_DrvAudioDeviceCallback(&cs4344_callback_args);
}

#endif //(OS_AUDIO_ENABLED)