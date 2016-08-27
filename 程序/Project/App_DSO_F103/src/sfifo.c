/* ʾ��������FIFOģ��
 * eleqian 2014-8-22
 */

#include "base.h"
#include <string.h>
#include "sfifo.h"

// FIFO�洢�ռ�
static uint16_t gSFIFOBuf[2][SFIFO_SIZE];

// FIFOģʽ
static uint8_t gSFIFOMode;

// FIFO��д��Ե�ַ
static uint16_t gReadAddr;
static uint16_t gWriteAddr;

// ˫FIFOģʽ�¶���дFIFO����
static uint8_t gReadFIFOIdx;
static uint8_t gWriteFIFOIdx;

/*-----------------------------------*/

// ���16bits����
// ����������ָ�룬 ����ֵ��������
static uint16_t *memset16(uint16_t *dest, uint16_t val, size_t n)
{
    uint16_t *p;
    uint16_t *end = dest + n;

    for (p = dest; p < end; p++) {
        *p = val;
    }

    return dest;
}

/*-----------------------------------*/

// ��ʼ��
void SFIFO_Init(void)
{
    gSFIFOMode = SFIFO_MODE_SEPARATE;
    gReadAddr = 0;
    gWriteAddr = 0;
    gReadFIFOIdx = 0;
    gWriteFIFOIdx = 1;
    memset(gSFIFOBuf, 0xff, sizeof(gSFIFOBuf));
}

// ��λ
void SFIFO_Reset(void)
{
    gReadAddr = 0;
    gWriteAddr = 0;
    gReadFIFOIdx = 0;
    gWriteFIFOIdx = 1;
    memset(gSFIFOBuf, 0xff, sizeof(gSFIFOBuf));
}

// д����
// ���������ݻ�������д����Ŀ
void SFIFO_Write(const uint16_t *buf, uint16_t count)
{
    uint16_t *pWr;
    uint16_t newAddr;
    //#include "timebase.h"
    //volatile uint32_t t1, t2;

    if (count == 0) {
        return;
    }

    if (gSFIFOMode == SFIFO_MODE_NORMAL) {
        pWr = &gSFIFOBuf[0][0];
    } else {
        pWr = &gSFIFOBuf[gWriteFIFOIdx][0];
    }

    //t1 = micros();
    // ��������
    if (gWriteAddr + count <= SFIFO_SIZE) {
        memcpy(pWr + gWriteAddr, buf, count << 1);
    } else {
        memcpy(pWr + gWriteAddr, buf, (SFIFO_SIZE - gWriteAddr) << 1);
        memcpy(pWr, (buf + SFIFO_SIZE - gWriteAddr), (gWriteAddr + count - SFIFO_SIZE) << 1);
    }
    /*t2 = micros() - t1;
    if (t2 > 30) {
        t1 = t2;
    }

    if (t2 < 30 && count == 256) {
        t1 = t2;
    }*/

    // �޸�д��ַ
    newAddr = gWriteAddr + count;
    if (newAddr >= SFIFO_SIZE) {
        newAddr -= SFIFO_SIZE;
    }

    // �޸Ķ���ַ
    if (gSFIFOMode == SFIFO_MODE_NORMAL) {
        if (gWriteAddr == gReadAddr) {
            // FIFO��
            gReadAddr = newAddr + 1;
        } else if (gWriteAddr < gReadAddr) {
            if (gWriteAddr + count >= gReadAddr) {
                gReadAddr = newAddr + 1;
            }
        } else {
            if (newAddr > gReadAddr) {
                gReadAddr = newAddr + 1;
            }
        }

        if (gReadAddr >= SFIFO_SIZE) {
            gReadAddr -= SFIFO_SIZE;
        }
    }

    gWriteAddr = newAddr;
}

// �������
// ������Ҫ���Ϊ�����ݣ������Ŀ
void SFIFO_Fill(const uint16_t val, uint16_t count)
{
    uint16_t *pWr;
    uint16_t newAddr;

    if (count == 0) {
        return;
    }

    if (gSFIFOMode == SFIFO_MODE_NORMAL) {
        pWr = &gSFIFOBuf[0][0];
    } else {
        pWr = &gSFIFOBuf[gWriteFIFOIdx][0];
    }

    // ��������
    if (gWriteAddr + count <= SFIFO_SIZE) {
        memset16(pWr + gWriteAddr, val, count << 1);
    } else {
        memset16(pWr + gWriteAddr, val, (SFIFO_SIZE - gWriteAddr) << 1);
        memset16(pWr, val, (gWriteAddr + count - SFIFO_SIZE) << 1);
    }

    // �޸�д��ַ
    newAddr = gWriteAddr + count;
    if (newAddr >= SFIFO_SIZE) {
        newAddr -= SFIFO_SIZE;
    }

    // �޸Ķ���ַ
    if (gSFIFOMode == SFIFO_MODE_NORMAL) {
        if (gWriteAddr == gReadAddr) {
            // FIFO��
            gReadAddr = newAddr + 1;
        } else if (gWriteAddr < gReadAddr) {
            if (gWriteAddr + count >= gReadAddr) {
                gReadAddr = newAddr + 1;
            }
        } else {
            if (newAddr > gReadAddr) {
                gReadAddr = newAddr + 1;
            }
        }

        if (gReadAddr >= SFIFO_SIZE) {
            gReadAddr -= SFIFO_SIZE;
        }
    }

    gWriteAddr = newAddr;
}

// ֱ�Ӷ����ݣ����ı�FIFOָ��
// ������ƫ�ƣ�����ڵ�ǰ��ָ�룩�����ݻ���������ȡ��Ŀ
void SFIFO_ReadDirect(uint16_t offset, uint16_t *buf, uint16_t count)
{
    uint16_t *pRd;
    uint16_t rdAddr;

    if (gSFIFOMode == SFIFO_MODE_NORMAL) {
        pRd = &gSFIFOBuf[0][0];
    } else {
        pRd = &gSFIFOBuf[gReadFIFOIdx][0];
    }

    rdAddr = (gReadAddr + offset) % SFIFO_SIZE;

    if (rdAddr + count <= SFIFO_SIZE) {
        memcpy(buf, (pRd + rdAddr), count << 1);
    } else {
        memcpy(buf, (pRd + rdAddr), (SFIFO_SIZE - rdAddr) << 1);
        memcpy((buf + SFIFO_SIZE - rdAddr), pRd, (rdAddr + count - SFIFO_SIZE) << 1);
    }
}

// ����FIFOģʽ
// ������ģʽ����FIFO��˫FIFO��
void SFIFO_SetMode(uint8_t mode)
{
    gSFIFOMode = mode;
    SFIFO_Reset();
}

// ��������FIFO���͡�дFIFO��
void SFIFO_Switch_RW(void)
{
    uint8_t tmpIdx;

    if (gSFIFOMode == SFIFO_MODE_NORMAL) {
        return;
    }

    // ������дFIFO����
    tmpIdx = gReadFIFOIdx;
    gReadFIFOIdx = gWriteFIFOIdx;
    gWriteFIFOIdx = tmpIdx;

    gReadAddr = gWriteAddr;
    gWriteAddr = 0;
}

// �ӡ���FIFO��ĩβ�������ݵ���дFIFO����ͷ
// ���������Ƶ����ݸ���
void SFIFO_Copy_R2W(uint16_t count)
{
    uint16_t *pRd, *pWr;

    if (gSFIFOMode == SFIFO_MODE_NORMAL) {
        return;
    }

    pRd = &gSFIFOBuf[gReadFIFOIdx][0];
    pWr = &gSFIFOBuf[gWriteFIFOIdx][0];

    if (gReadAddr >= count) {
        memcpy((pWr + SFIFO_SIZE - count), (pRd + gReadAddr - count), count << 1);
    } else {
        memcpy((pWr + SFIFO_SIZE - gReadAddr), pRd, gReadAddr << 1);
        memcpy((pWr + SFIFO_SIZE - count), (pRd + SFIFO_SIZE + gReadAddr - count), (count - gReadAddr) << 1);
    }
}
