#ifndef __SFIFO_H__
#define __SFIFO_H__

#include "base.h"

// ����FIFO��С��������ȣ�
#define SFIFO_SIZE              1000

// FIFOģʽö��
enum {
    SFIFO_MODE_NORMAL,      // ��ͨģʽ��ͬ��ͨFIFO����һ��
    SFIFO_MODE_SEPARATE,    // ˫FIFOģʽ����FIFO�ռ��Ϊ�����֣�һ����һ��д
    SFIFO_MODE_MAX
};

void SFIFO_Init(void);

void SFIFO_Reset(void);

void SFIFO_SetMode(uint8_t mode);

void SFIFO_Write(const uint16_t *buf, uint16_t count);

void SFIFO_Fill(const uint16_t val, uint16_t count);

void SFIFO_ReadDirect(uint16_t offset, uint16_t *buf, uint16_t count);

void SFIFO_Switch_RW(void);

void SFIFO_Copy_R2W(uint16_t count);

#endif // __SFIFO_H__
