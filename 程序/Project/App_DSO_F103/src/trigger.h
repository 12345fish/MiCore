#ifndef __TRIGGER_H__
#define __TRIGGER_H__

#include "base.h"

// ����ģʽö��
enum {
    TRIG_MODE_ROLL,     // �����������д�����ֱ�Ӹ��´���״̬���������ʽϵ�ʱ���ã�
    TRIG_MODE_SINGLE,   // ���Σ�ֻ����һ�δ���Ȼ��ֹͣ
    TRIG_MODE_NORMAL,   // ��ͨ��ÿ�����㴥�����������д�����δ����ʱ��ˢ��״̬
    TRIG_MODE_AUTO,     // �Զ���ÿ�����㴥�����������д�����ͬʱδ����ʱҲˢ��״̬
    TRIG_MODE_MAX
};

// ��������ö��
enum {
    TRIG_EDGE_RISE,     // ������
    TRIG_EDGE_FALL,     // �½���
    TRIG_EDGE_MAX
};

void Trig_Init(void);

void Trig_Start(void);

void Trig_Stop(void);

BOOL Trig_CheckUpdate(void);

void Trig_ClearUpdate(void);

void Trig_SetMode(uint8_t mode);

void Trig_SetEdge(uint8_t edge);

void Trig_SetLevel(uint16_t level);

void Trig_SetPreSize(uint16_t size);

uint16_t Trig_GetSampleCnt(void);

#endif // __TRIGGER_H__
