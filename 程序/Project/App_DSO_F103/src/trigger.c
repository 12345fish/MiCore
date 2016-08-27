/* ʾ��������ģ��
 * eleqian 2014-3-9
 */

#include "base.h"
#include "timebase.h"
#include "sample.h"
#include "sfifo.h"
#include "trigger.h"

// �ж�����
#define IS_RISE_EDGE(pre, cur, level)   \
    ((pre) < (cur) && (pre) <= (level) && (level) <= (cur))
#define IS_FALL_EDGE(pre, cur, level)   \
    ((pre) > (cur) && (pre) >= (level) && (level) >= (cur))

// ��������־λ
#define TRIG_FLAG_PRESAMPLE     0x1     // Ԥ������־�������״�Ԥ��������ʱ��λ������������
#define TRIG_FLAG_TRIGGERED     0x2     // ������־���ɹ����д���ʱ��λ���󴥷��������ʱ���
#define TRIG_FLAG_UPDATED       0x4     // ���±�־��һ�δ������������ʱ��λ��CLR_UPDATE����λʱ���
#define TRIG_FLAG_CLR_UPDATE    0x8     // ������±�־���ⲿ��λ���ڲ����������������

// �����Զ�ˢ��ʱ����(ms)
#define TRIG_AUTO_UPDATE_TIM    100

// ��������
static uint8_t gTriggerMode;
static uint8_t gTriggerEdge;
static uint16_t gTriggerLevel;
static uint16_t gTriggerPreSize;

// �ϴβ�������
static uint16_t gPreSample;

// �Ѳ�����Ŀ
static uint32_t gSampleCnt;

// �ϴβ�����Ŀ
static uint32_t gLastSampleCnt;

// �ϴθ���ʱ�䣬����Autoģʽ����
static uint32_t gLastUpdateTime;

// ������־
static uint8_t gTriggerFlag;

/*-----------------------------------*/

// ��ʼ��
void Trig_Init(void)
{
    gTriggerMode = TRIG_MODE_AUTO;
    gTriggerEdge = TRIG_EDGE_RISE;
    gTriggerLevel = (SAMPLE_RESULT_MAX + SAMPLE_RESULT_MIN) / 2;
    gTriggerPreSize = SFIFO_SIZE / 2;

    gPreSample = SAMPLE_RESULT_INVALID;
    gSampleCnt = 0;
    gTriggerFlag = 0;
    gLastUpdateTime = 0;
}

// ��������
void Trig_Start(void)
{
    gPreSample = SAMPLE_RESULT_INVALID;
    gSampleCnt = 0;
    gTriggerFlag = 0;
    gLastUpdateTime = millis();

    if (gTriggerMode != TRIG_MODE_ROLL) {
        // ��һ�δ���ǰ��Ҫ����Ԥ��������
        BITS_SET(gTriggerFlag, TRIG_FLAG_PRESAMPLE);
    }

    SFIFO_Reset();
    Sample_Start();
}

// ֹͣ����
void Trig_Stop(void)
{
    Sample_Stop();
    gTriggerFlag = 0;
}

/*-----------------------------------*/

// �����±�־λ
BOOL Trig_CheckUpdate(void)
{
    return BITS_CHK(gTriggerFlag, TRIG_FLAG_UPDATED);
}

// ������±�־λ
void Trig_ClearUpdate(void)
{
    if (gTriggerMode == TRIG_MODE_ROLL || gTriggerMode == TRIG_MODE_SINGLE) {
        // �����򵥴δ���ģʽ��ֱ��������±�־ģʽ����
        BITS_CLR(gTriggerFlag, TRIG_FLAG_UPDATED);
    } else {
        // ����ģʽ��Ҫ�ڲ����ж��н�����������
        BITS_SET(gTriggerFlag, TRIG_FLAG_CLR_UPDATE);
    }
}

/*-----------------------------------*/

// ���ô�����ʽ
void Trig_SetMode(uint8_t mode)
{
    gTriggerMode = mode;

    if (mode == TRIG_MODE_ROLL) {
        // ����ģʽ����Ҫʹ��˫FIFO�����ҵ�FIFO����ʹ�洢��ȼӱ�
        SFIFO_SetMode(SFIFO_MODE_NORMAL);
    } else {
        // ����ģʽʹ��˫FIFO����֤ʼ����һ�������Ĳ�����¼
        SFIFO_SetMode(SFIFO_MODE_SEPARATE);
    }
}

// ���ô�������
void Trig_SetEdge(uint8_t edge)
{
    gTriggerEdge = edge;
}

// ���ô�����ƽ
void Trig_SetLevel(uint16_t level)
{
    gTriggerLevel = level;
}

// ����Ԥ�������
void Trig_SetPreSize(uint16_t size)
{
    gTriggerPreSize = size;
}

// ȡ�ò�����
uint16_t Trig_GetSampleCnt(void)
{
    return (uint16_t)gLastSampleCnt;
}

/*-----------------------------------*/

// ���ղ�������¼�
void OnSampleComplete(const uint16_t *buf, uint16_t count)
{
    // �޴���ģʽ��ֱ��дFIFO������
    if (gTriggerMode == TRIG_MODE_ROLL) {
        SFIFO_Write(buf, count);
        BITS_SET(gTriggerFlag, TRIG_FLAG_UPDATED);
        gSampleCnt += count;
        if (gSampleCnt > SFIFO_SIZE) {
            gSampleCnt = SFIFO_SIZE;
        }
        gLastSampleCnt = gSampleCnt;
        return;
    }

    // ���ڽ����״�Ԥ��������ʱ�����д���
    if (BITS_CHK(gTriggerFlag, TRIG_FLAG_PRESAMPLE)) {
        SFIFO_Write(buf, count);
        gSampleCnt += count;
        if (gSampleCnt >= gTriggerPreSize) {
            BITS_CLR(gTriggerFlag, TRIG_FLAG_PRESAMPLE);
            gLastUpdateTime = millis();
            gSampleCnt = gTriggerPreSize;
            gPreSample = *(buf + count - 1);
        }
        return;
    }

    // ���±�־λ���ⲿ��������Խ�����һ�δ���
    if (BITS_CHK(gTriggerFlag, TRIG_FLAG_CLR_UPDATE)) {
        BITS_CLR(gTriggerFlag, TRIG_FLAG_CLR_UPDATE | TRIG_FLAG_UPDATED);
        // �ڲ����ʽϵ͵�ʱ������Ԥ���������ռ�û������ʾˢ���ڼ������
        // ͨ���������ϴβ���������Ϊ����Ԥ�����������������ˢ����
        if (gSampleCnt < gTriggerPreSize) {
            SFIFO_Copy_R2W(gTriggerPreSize - gSampleCnt);
        }
        gSampleCnt = gTriggerPreSize;
    }

    // ���±�־λû�������˵���ϴθ��»�û����
    if (BITS_CHK(gTriggerFlag, TRIG_FLAG_UPDATED)) {
        // ����������һ��Ԥ��������
        SFIFO_Write(buf, count);
        gSampleCnt += count;
        gPreSample = *(buf + count - 1);
        return;
    }

    // ��δ����
    if (!BITS_CHK(gTriggerFlag, TRIG_FLAG_TRIGGERED)) {
        size_t i;
        uint16_t level;
        uint16_t sample, presample;

        // ���ش������
        level = gTriggerLevel;
        presample = gPreSample;
        if (gTriggerEdge == TRIG_EDGE_RISE) {
            // ������
            for (i = 0; i < count; i++) {
                sample = *(buf + i);
                if (IS_RISE_EDGE(presample, sample, level)) {
                    BITS_SET(gTriggerFlag, TRIG_FLAG_TRIGGERED);
                    break;
                }
                presample = sample;
            }
        } else {
            // �½���
            for (i = 0; i < count; i++) {
                sample = *(buf + i);
                if (IS_FALL_EDGE(presample, sample, level)) {
                    BITS_SET(gTriggerFlag, TRIG_FLAG_TRIGGERED);
                    break;
                }
                presample = sample;
            }
        }
        gPreSample = *(buf + count - 1);

        // ��Ԥ������������дFIFO
        SFIFO_Write(buf, i);

        if (BITS_CHK(gTriggerFlag, TRIG_FLAG_TRIGGERED)) {
            gSampleCnt = gTriggerPreSize;
        } else {
            gSampleCnt += i;
            if (gSampleCnt > SFIFO_SIZE) {
                gSampleCnt = SFIFO_SIZE;
            }
        }

        // �������д�����������ִ���
        buf += i;
        count -= i;
    }

    // �Ѿ����������ڽ��е��Ǻ󴥷�������
    if (BITS_CHK(gTriggerFlag, TRIG_FLAG_TRIGGERED)) {
        if (gSampleCnt + count >= SFIFO_SIZE) {
            // FIFOд�������������¼�
            SFIFO_Write(buf, SFIFO_SIZE - gSampleCnt);
            BITS_CLR(gTriggerFlag, TRIG_FLAG_TRIGGERED);
            BITS_SET(gTriggerFlag, TRIG_FLAG_UPDATED);
            gLastSampleCnt = SFIFO_SIZE;

            if (gTriggerMode == TRIG_MODE_SINGLE) {
                // ����ģʽʱ�������ֹͣ
                Sample_Stop();
                SFIFO_Switch_RW();
            } else {
                // ��ͨ���Զ�ģʽ�л�FIFO��������׼����һ�δ���
                gLastUpdateTime = millis();
                gSampleCnt = gSampleCnt + count - SFIFO_SIZE;
                SFIFO_Switch_RW();
                SFIFO_Write(buf, gSampleCnt);
            }
        } else {
            // �������д�󴥷�FIFO
            SFIFO_Write(buf, count);
            gSampleCnt += count;
        }
        return;
    }

    // �Զ�ģʽ��δ����ʱ���г�ʱ����
    if (gTriggerMode == TRIG_MODE_AUTO && !BITS_CHK(gTriggerFlag, TRIG_FLAG_TRIGGERED)) {
        uint32_t tnow;

        tnow = millis();
        if (tnow >= gLastUpdateTime + TRIG_AUTO_UPDATE_TIM) {
            gLastUpdateTime = tnow;
            /*if (gSampleCnt < SFIFO_SIZE) {
                SFIFO_Copy_R2W(SFIFO_SIZE - gSampleCnt);
            }*/
            //SFIFO_Fill(SAMPLE_RESULT_INVALID, SFIFO_SIZE - gSampleCnt);
            SFIFO_Switch_RW();
            gLastSampleCnt = gSampleCnt;
            gSampleCnt = 0;
            BITS_SET(gTriggerFlag, TRIG_FLAG_UPDATED);
            return;
        }
    }
}
