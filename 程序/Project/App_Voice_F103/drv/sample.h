#ifndef __SAMPLE_H__
#define __SAMPLE_H__

#include "base.h"

// ��������
#define SAMPLES_MAX                 2048

// ��ʱ����������(72MHz)
#define SAMPLE_TIMER_PERIOD_32K     2250
#define SAMPLE_TIMER_PERIOD_8K      9000

// �������������
extern uint16_t gSampleValues[SAMPLES_MAX];

// ��ʼ��
void Sample_Init(void);

// ��ʼ����
// �������������ڣ�������
void Sample_Start(uint16_t speriod, uint16_t scount);

// ��������¼�֪ͨ���������ⲿʵ��
extern void OnSampleComplete(void);

#endif //__SAMPLE_H__
