#ifndef __SAMPLE_H__
#define __SAMPLE_H__

#include "base.h"

// ��Ч�������
#define SAMPLE_RESULT_INVALID       0xffff

// �������ֵ
#define SAMPLE_RESULT_MAX           4095

// ������Сֵ
#define SAMPLE_RESULT_MIN           0

// ������ö��
typedef enum _sampleRate_enum {
    SAMPLE_RATE_20,
    SAMPLE_RATE_50,
    SAMPLE_RATE_100,
    SAMPLE_RATE_200,
    SAMPLE_RATE_500,
    SAMPLE_RATE_1K,
    SAMPLE_RATE_2K,
    SAMPLE_RATE_5K,
    SAMPLE_RATE_10K,
    SAMPLE_RATE_20K,
    SAMPLE_RATE_50K,
    SAMPLE_RATE_100K,
    SAMPLE_RATE_200K,
    SAMPLE_RATE_500K,
    SAMPLE_RATE_1M,
    SAMPLE_RATE_2M,
    SAMPLE_RATE_MAX
} sampleRate_t;

// ������ö�ٶ�Ӧ��ʵ��ֵ
extern const uint32_t tblSampleRateValue[SAMPLE_RATE_MAX];

// ��ʼ��
void Sample_Init(void);

// ���ò�����
// ������������ö�٣�20Hz~2MHz����1-2-5������
void Sample_SetRate(sampleRate_t rate);

// ��ʼ����
void Sample_Start(void);

// ֹͣ����
void Sample_Stop(void);

// ��������¼�֪ͨ���������ⲿʵ��
// ����������������������Ŀ
extern void OnSampleComplete(const uint16_t *buf, uint16_t count);

#endif //__SAMPLE_H__
