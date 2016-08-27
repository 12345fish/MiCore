/* ����ģ�� for ��ƵƵ�׷���
 * eleqian 2016-7-3
 * ʵ�ֹ滮��
 * ADC����ͨ����0��GPIO��PA0
 * ʹ��TIM1_CC1����ADC1������ʹ�ò�����Ϊ8kHz��32kHz
 * ADC1ת�������DMA�͵��ڴ棬�ﵽ512��2048��ʱ֪ͨ�ϲ㴦��
 * TIM1ʱ��72MHz��ADC1ʱ��12MHz��6��Ƶ��
 */

#include "base.h"
#include "sample.h"

// �������ݣ�����volatile����ΪDMA�����ݴ�����ͬʱ����
uint16_t gSampleValues[SAMPLES_MAX];

/*-----------------------------------*/

// ��������
static void Sample_PinConfig(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    // PA0��Ϊģ��ͨ��0����
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // PA8��ΪTIM1_CH1���(����ʱ�鿴����)
    //GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
    //GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    //GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    //GPIO_Init(GPIOA, &GPIO_InitStructure);
}

// ADC����
static void Sample_ADCConfig(void)
{
    ADC_InitTypeDef ADC_InitStructure;

    // 72MHz / 6 = 12MHz
    RCC_ADCCLKConfig(RCC_PCLK2_Div6);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);

    ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
    ADC_InitStructure.ADC_ScanConvMode = DISABLE;
    ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T1_CC1;
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfChannel = 1;
    ADC_Init(ADC1, &ADC_InitStructure);

    // ͨ��0
    ADC_RegularChannelConfig(ADC1, ADC_Channel_0, 1, ADC_SampleTime_28Cycles5);

    // Enable ADC1 external trigger
    ADC_ExternalTrigConvCmd(ADC1, ENABLE);

    // Enable ADC1 DMA
    ADC_DMACmd(ADC1, ENABLE);

    // Enable ADC1
    ADC_Cmd(ADC1, ENABLE);

    // У׼
    ADC_ResetCalibration(ADC1);
    while (ADC_GetResetCalibrationStatus(ADC1));
    ADC_StartCalibration(ADC1);
    while (ADC_GetCalibrationStatus(ADC1));
}

// DMA����
static void Sample_DMAConfig(uint16_t size)
{
    DMA_InitTypeDef DMA_InitStructure;

    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

    // DMA1 channel1 configuration
    DMA_DeInit(DMA1_Channel1);
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&ADC1->DR;
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)gSampleValues;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
    DMA_InitStructure.DMA_BufferSize = size;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(DMA1_Channel1, &DMA_InitStructure);

    // Enable DMA1 Channel1 Transfer Complete interrupt
    DMA_ITConfig(DMA1_Channel1, DMA_IT_TC, ENABLE);

    // Enable DMA1 channel1
    DMA_Cmd(DMA1_Channel1, ENABLE);
}

// ������ʱ������
static void Sample_TimerConfig(uint16_t period)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_OCInitTypeDef TIM_OCInitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);

    // TIM1ʱ��72MHz
    TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);
    TIM_TimeBaseStructure.TIM_Period = period;
    TIM_TimeBaseStructure.TIM_Prescaler = 0x0;
    TIM_TimeBaseStructure.TIM_ClockDivision = 0x0;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);

    /* TIM1 channel1 configuration in PWM mode */
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse = period / 2;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low;
    TIM_OC1Init(TIM1, &TIM_OCInitStructure);

    TIM_CtrlPWMOutputs(TIM1, ENABLE);

    //��ʼ��ʱ��������ʱ
    //TIM_Cmd(TIM1, ENABLE);
}

// DMA�ж�����
static void Sample_NVICConfig(void)
{
    NVIC_InitTypeDef NVIC_InitStructure;

    /* Enable DMA1 channel1 IRQ Channel */
    NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

/*-----------------------------------*/

// ��ʼ��
void Sample_Init(void)
{
    Sample_PinConfig();
    Sample_DMAConfig(SAMPLES_MAX);
    Sample_ADCConfig();
    Sample_TimerConfig(SAMPLE_TIMER_PERIOD_8K);
    Sample_NVICConfig();
}

// ��ʼ����
void Sample_Start(uint16_t speriod, uint16_t scount)
{
    // ����DMA�����������ADC����������
    Sample_DMAConfig(scount);

    // ���ö�ʱ�����ڣ���ADC�������ʣ�
    Sample_TimerConfig(speriod);

    // ����ʱ��������ʼ������
    TIM_Cmd(TIM1, ENABLE);
}

/**
  * @brief  This function handles DMA1 Channel 1 interrupt request.
  * @param  None
  * @retval None
  */
void DMA1_Channel1_IRQHandler(void)
{
    /* Test on DMA1 Channel1 Transfer Complete interrupt */
    if (DMA_GetITStatus(DMA1_IT_TC1)) {
        // �ض�ʱ������ֹͣ������
        TIM_Cmd(TIM1, DISABLE);

        // ֹͣDMA����ʵ�ϲ���Ҫ����Ϊ�������
        DMA_Cmd(DMA1_Channel1, DISABLE);

        /* Clear DMA1 Channel1 Half Transfer, Transfer Complete and Global interrupt pending bits */
        DMA_ClearITPendingBit(DMA1_IT_GL1);

        // �����¼�
        OnSampleComplete();
    }
}
