/* ����ģ��
 * eleqian 2014-3-9
 * ʵ��ϸ�ڣ�
 * ADC����ͨ��0(PA0)
 * ADCʱ��14MHz������ʱ��1.5����
 * ������=2Mʱʹ��˫ADC���ٽ���ģʽ������=1Mʱʹ��ADC1����������<1Mʱʹ��TIM2_CC2����ADC1����
 * ����ʹ��DMA���䣬��DMA�жϴ���
 */

#include "base.h"
#include "sample.h"

// �򿪺�ʱʹ�ܶ�ʱ���������(PA1)
//#define SAMPLE_TRIG_TEST

// ADCģʽö��
enum {
    ADC_MODE_DISABLE,       // ����ADC
    ADC_MODE_TWO_FAST_DMA,  // ˫ADC���ٽ���ģʽ+DMA
    ADC_MODE_ONE_CONT_DMA,  // ��ADC����ת��ģʽ+DMA
    ADC_MODE_ONE_TIM_DMA,   // ��ADC��ʱ������ģʽ+DMA
    ADC_MODE_MAX
};

// DMAģʽö��
enum {
    DMA_MODE_DISABLE,   // ����DMA
    DMA_MODE_1ADC,      // DMA��ADCģʽ
    DMA_MODE_2ADC,      // DMA˫ADCģʽ
    DMA_MODE_MAX
};

// ������ʽ��־λ
#define SAMPLE_FLAG_TIM_TRIG    0x1     // ��ʾͨ����ʱ�����������������������
#define SAMPLE_FLAG_SWAP_ADC    0x2     // ��ʾ��Ҫ����DMA���������������ݣ�˫ADCʱ��

// DMA����������С��ÿ����˫������*2��
#define DMA_BUF_SIZE_MAX        (256)

// ������ö�ٶ�Ӧ��ʵ��ֵ
const uint32_t tblSampleRateValue[SAMPLE_RATE_MAX] = {
    20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000, 50000, 100000, 200000, 500000, 1000000, 2000000
};

// ������ö�ٶ�Ӧ��DMA��������С��ÿ����
static const uint16_t tblDMABufSize[SAMPLE_RATE_MAX] = {
    1, 1, 1, 1, 1, 1, 2, 4, 8, 16, 32, 64, 128, 256, 256, 256
};

// DMA˫������
static uint16_t gDMABuf[DMA_BUF_SIZE_MAX * 2];

// ʵ��ʹ�õ�DMA��������С��ÿ����
static uint16_t gDMABufPerSize;

// ��־λ
static uint8_t gSampleFlags;

/*-----------------------------------*/

// ���ú����CONTλ����������ת��ģʽ����ͣ�������û�ṩ����������ʵ��
void ADC_ContinuousCmd(ADC_TypeDef *ADCx, FunctionalState NewState)
{
    // Check the parameters
    assert_param(IS_ADC_ALL_PERIPH(ADCx));
    assert_param(IS_FUNCTIONAL_STATE(NewState));

    if (NewState != DISABLE) {
        ADCx->CR2 |= ADC_CR2_CONT;
    } else {
        ADCx->CR2 &= ~ADC_CR2_CONT;
    }
}

/*-----------------------------------*/

// ��������
static void Config_Pin(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    // PA0��ΪADC����
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

#ifdef SAMPLE_TRIG_TEST
    // PA1��ΪTIM2_CH2���(����ʱ����ADC����Ƶ��)
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
#endif //SAMPLE_TRIG_TEST
}

// ADC����
static void Config_ADC(uint8_t mode)
{
    static uint8_t last_mode = ADC_MODE_DISABLE;
    ADC_InitTypeDef ADC_InitStructure;

    // ģʽ��ͬʱ���ظ�����
    if (mode == last_mode) {
        return;
    }

    // 56MHz / 4 = 14MHz
    RCC_ADCCLKConfig(RCC_PCLK2_Div4);

    // �ر����輰��ʱ�ӣ����͹���
    if (mode == ADC_MODE_DISABLE) {
        ADC_DeInit(ADC1);
        ADC_DeInit(ADC2);
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, DISABLE);
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC2, DISABLE);
        last_mode = mode;
        return;
    } else if (mode != ADC_MODE_TWO_FAST_DMA) {
        ADC_DeInit(ADC2);
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC2, DISABLE);
    }

    /* ����ADC1 */

    // ��һ������
    if (last_mode == ADC_MODE_DISABLE) {
        // ��ADC1ʱ��
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
    }

    ADC_StructInit(&ADC_InitStructure);
    if (mode == ADC_MODE_TWO_FAST_DMA) {
        // ˫ADC���ٽ�������ת��
        ADC_InitStructure.ADC_Mode = ADC_Mode_FastInterl;
        ADC_InitStructure.ADC_ScanConvMode = DISABLE;
        ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
        ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
        ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
        ADC_InitStructure.ADC_NbrOfChannel = 1;
    } else if (mode == ADC_MODE_ONE_CONT_DMA) {
        // ��ADC����ת��
        ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
        ADC_InitStructure.ADC_ScanConvMode = DISABLE;
        ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
        ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
        ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
        ADC_InitStructure.ADC_NbrOfChannel = 1;
    } else if (mode == ADC_MODE_ONE_TIM_DMA) {
        // ��ADC��ʱ������
        ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
        ADC_InitStructure.ADC_ScanConvMode = DISABLE;
        ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
        ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T2_CC2;
        ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
        ADC_InitStructure.ADC_NbrOfChannel = 1;
    }
    ADC_Init(ADC1, &ADC_InitStructure);

    // ADC1ͨ��0
    ADC_RegularChannelConfig(ADC1, ADC_Channel_0, 1, ADC_SampleTime_1Cycles5);

    if (mode == ADC_MODE_ONE_TIM_DMA) {
        // ʹ��ADC1�ⲿ����
        ADC_ExternalTrigConvCmd(ADC1, ENABLE);
    }

    // ʹ��ADC1��DMA
    ADC_DMACmd(ADC1, ENABLE);

    // ��һ������
    if (last_mode == ADC_MODE_DISABLE) {
        // ʹ��ADC1
        ADC_Cmd(ADC1, ENABLE);

        // У׼ADC1
        ADC_ResetCalibration(ADC1);
        while (ADC_GetResetCalibrationStatus(ADC1));
        ADC_StartCalibration(ADC1);
        while (ADC_GetCalibrationStatus(ADC1));
    }

    /* ����ADC2 */

    if (mode == ADC_MODE_TWO_FAST_DMA) {
        // ��ADC2ʱ��
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC2, ENABLE);
        
        // ˫ADC���ٽ�������ת��
        ADC_StructInit(&ADC_InitStructure);
        ADC_InitStructure.ADC_Mode = ADC_Mode_FastInterl;
        ADC_InitStructure.ADC_ScanConvMode = DISABLE;
        ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
        ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
        ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
        ADC_InitStructure.ADC_NbrOfChannel = 1;
        ADC_Init(ADC2, &ADC_InitStructure);

        // ADC2ͨ��0
        ADC_RegularChannelConfig(ADC2, ADC_Channel_0, 1, ADC_SampleTime_1Cycles5);

        // ʹ��ADC2�ⲿ��������ADC1�Զ�������
        ADC_ExternalTrigConvCmd(ADC2, ENABLE);

        // ʹ��ADC2
        ADC_Cmd(ADC2, ENABLE);

        // У׼ADC2
        ADC_ResetCalibration(ADC2);
        while (ADC_GetResetCalibrationStatus(ADC2));
        ADC_StartCalibration(ADC2);
        while (ADC_GetCalibrationStatus(ADC2));
    }

    last_mode = mode;
}

// DMA����
static void Config_DMA(uint8_t mode, uint16_t size)
{
    DMA_InitTypeDef DMA_InitStructure;

    // �ر����輰��ʱ�ӣ����͹���
    if (mode == DMA_MODE_DISABLE) {
        DMA_DeInit(DMA1_Channel1);
        RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, DISABLE);
        return;
    }

    // ��DMA1ʱ��
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

    // ����ǰ�ȳ��ܣ��������
    DMA_Cmd(DMA1_Channel1, DISABLE);

    // DMA1ͨ��1����
    DMA_StructInit(&DMA_InitStructure);
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&ADC1->DR;
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)gDMABuf;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    if (mode == DMA_MODE_2ADC) {
        // ˫ADCģʽʱADC1_DR�Ĵ�����16λΪADC2�������16λΪADC1���
        // ע�⣬�������䵽RAM���ǰ�ADC1_ADC2...��˳�����У���ʵ������ADC2�Ȳ�������Ҫ����˳��
        DMA_InitStructure.DMA_BufferSize = size * 2 / 2;
        DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Word;
        DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Word;
    } else if (mode == DMA_MODE_1ADC) {
        // ��ADCģʽADC1_DR�Ĵ���ֻ�е�16λ��Ч������Ҫ��32λ����
        DMA_InitStructure.DMA_BufferSize = size * 2;
        DMA_InitStructure.DMA_PeripheralDataSize =  DMA_PeripheralDataSize_Word;
        DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
    }
    DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(DMA1_Channel1, &DMA_InitStructure);

    // ������ȫ���жϣ�ʵ��˫����
    DMA_ITConfig(DMA1_Channel1, DMA_IT_HT | DMA_IT_TC, ENABLE);

    // ʹ��DMA1ͨ��1
    DMA_Cmd(DMA1_Channel1, ENABLE);
}

// ������ʱ������
// ����������Ƶ�ʣ�1Hz~1MHz��=0ʱ�رգ�
static void Config_Timer(uint32_t freq)
{
    const uint32_t usTicks = 56;    // 1us��ʱ��ʱ����
    static uint32_t last_freq = 0;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_OCInitTypeDef TIM_OCInitStructure;
    uint16_t period;
    uint16_t prescaler;

    // Ƶ����ͬʱ���ظ�����
    if (freq == last_freq) {
        return;
    }
    last_freq = freq;

    // �ر����輰��ʱ�ӣ����͹���
    if (freq == 0) {
        TIM_DeInit(TIM2);
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, DISABLE);
        return;
    }

    // С��1kʱ����1kԤ��Ƶ�����ⳬ����ʱ����Χ
    if (freq < 1000) {
        prescaler = 1000;
        period = usTicks * 1000 / freq - 1;
    } else {
        prescaler = 0;
        period = usTicks * 1000000 / freq - 1;
    }

    // ��TIM2ʱ��
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    // ����ǰ�ȳ��ܣ��������
    TIM_Cmd(TIM2, DISABLE);

    // TIM2ʱ��
    TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);
    TIM_TimeBaseStructure.TIM_Period = period;
    TIM_TimeBaseStructure.TIM_Prescaler = prescaler;
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

    // TIM2 CH2���ڴ���ADC
    TIM_OCStructInit(&TIM_OCInitStructure);
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
//#ifdef SAMPLE_TRIG_TEST
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
//#else
    //TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Disable;
//#endif //SAMPLE_TRIG_TEST
    TIM_OCInitStructure.TIM_Pulse = period / 2;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low;
    TIM_OC2Init(TIM2, &TIM_OCInitStructure);

    // ��������ʼ����������ʱ������
    //TIM_Cmd(TIM2, ENABLE);
}

// �ж�����
static void Config_NVIC(void)
{
    NVIC_InitTypeDef NVIC_InitStructure;

    // DMA1ͨ��1�жϣ�ADC��
    NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

/*-----------------------------------*/

// ��ʼ��
void Sample_Init(void)
{
    gSampleFlags = 0;
    Config_Pin();
    Config_NVIC();
    Config_Timer(0);
    Config_ADC(ADC_MODE_DISABLE);
    Config_DMA(DMA_MODE_DISABLE, 0);
}

// ���ò�����
// ������������ö�٣�20Hz~2MHz����1-2-5������
void Sample_SetRate(sampleRate_t rate)
{
    uint32_t bufsize;

    bufsize = tblDMABufSize[rate];
    gDMABufPerSize = bufsize;
    
    //Config_Timer(0);
    Config_ADC(ADC_MODE_DISABLE);
    //Config_DMA(DMA_MODE_DISABLE, 0);

    if (rate == SAMPLE_RATE_2M) {
        // 2Mʱʹ��˫ADC���ٽ������
        gSampleFlags = SAMPLE_FLAG_SWAP_ADC;
        Config_ADC(ADC_MODE_TWO_FAST_DMA);
        Config_DMA(DMA_MODE_2ADC, bufsize);
        Config_Timer(0);
    } else if (rate == SAMPLE_RATE_1M) {
        // 1Mʱʹ��ADC1��������
        gSampleFlags = 0;
        Config_ADC(ADC_MODE_ONE_CONT_DMA);
        Config_DMA(DMA_MODE_1ADC, bufsize);
        Config_Timer(0);
    } else {
        // <1Mʱʹ�ö�ʱ������ADC1����
        gSampleFlags = SAMPLE_FLAG_TIM_TRIG;
        Config_ADC(ADC_MODE_ONE_TIM_DMA);
        Config_DMA(DMA_MODE_1ADC, bufsize);
        Config_Timer(tblSampleRateValue[rate]);
    }
}

// ��ʼ����
void Sample_Start(void)
{
    if (gSampleFlags & SAMPLE_FLAG_TIM_TRIG) {
        // ��ʱ������
        TIM_Cmd(TIM2, ENABLE);
    } else {
        // �������������ת��ģʽ
        ADC_ContinuousCmd(ADC1, ENABLE);
        ADC_ContinuousCmd(ADC2, ENABLE);
        ADC_SoftwareStartConvCmd(ADC1, ENABLE);
    }
}

// ֹͣ����
void Sample_Stop(void)
{
    if (gSampleFlags & SAMPLE_FLAG_TIM_TRIG) {
        // ��ʱ������
        TIM_Cmd(TIM2, DISABLE);
    } else {
        // ���������ȡ������ת��ģʽ����ֹͣת��
        ADC_ContinuousCmd(ADC1, DISABLE);
        ADC_ContinuousCmd(ADC2, DISABLE);
    }
}

/*-----------------------------------*/

// DMA�жϴ���
void DMA1_Channel1_IRQHandler(void)
{
    uint16_t *pBuf;

    if (DMA_GetITStatus(DMA1_IT_HT1)) {
        DMA_ClearITPendingBit(DMA1_IT_HT1);
        pBuf = gDMABuf;
    } else if (DMA_GetITStatus(DMA1_IT_TC1)) {
        DMA_ClearITPendingBit(DMA1_IT_TC1);
        pBuf = gDMABuf + gDMABufPerSize;
    } else {
        DMA_ClearITPendingBit(DMA1_IT_GL1);
        return;
    }

    // ����ADC1��ADC2�Ľ����ʹ�䰴ʱ��˳������
    if (gSampleFlags & SAMPLE_FLAG_SWAP_ADC) {
        uint32_t *pBuf32;
        uint32_t tmp;
        size_t bufsize;

        pBuf32 = (uint32_t *)pBuf;
        bufsize = gDMABufPerSize >> 1;;
        while (bufsize--) {
            tmp = *pBuf32;
            tmp = (tmp >> 16) | (tmp << 16);
            *pBuf32++ = tmp;
        }
    }

    // �����¼�
    OnSampleComplete(pBuf, gDMABufPerSize);
}
