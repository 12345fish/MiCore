#include "base.h"
#include "cmath.h"

#define cordic_1K       0x26DCEDB0L

// ��/2��32bit����ʱ��ֵ
#define half_pi         0x40000000L

// cordic�㷨���ұ��С
#define CORDIC_NTAB     32

// cordic�㷨���ұ�ʹ��32bit��ʾ360��
static const int32_t cordic_ctab[CORDIC_NTAB] = {
    0x20000000, 0x12E4051E, 0x09FB385B, 0x051111D4, 0x028B0D43, 0x0145D7E1, 0x00A2F61E, 0x00517C55,
    0x0028BE53, 0x00145F2F, 0x000A2F98, 0x000517CC, 0x00028BE6, 0x000145F3, 0x0000A2FA, 0x0000517D,
    0x000028BE, 0x0000145F, 0x00000A30, 0x00000518, 0x0000028C, 0x00000146, 0x000000A3, 0x00000051,
    0x00000029, 0x00000014, 0x0000000A, 0x00000005, 0x00000003, 0x00000001, 0x00000001, 0x00000000
};

/*-----------------------------------*/

// cordic�㷨
// �������Ƕ�
// ���أ�yֵ
int32_t cordic_y(int32_t theta)
{
    int k;
    int32_t tx, ty;
    int32_t x = cordic_1K, y = 0;
    int32_t z = theta;

    if ((z >= half_pi) || (z < -half_pi)) {
        z = ((uint32_t)half_pi << 1) - z;
    }

    //n = (n>CORDIC_NTAB) ? CORDIC_NTAB : n;

    for (k = 0 ; k < 20; ++k) { // 20bit
        if (z >= 0) {
            tx = x - (y >> k) ;
            ty = y + (x >> k) ;
            z = z -  cordic_ctab[k];
            x = tx;
            y = ty;
        } else {
            tx = x + (y >> k) ;
            ty = y - (x >> k) ;
            z = z +  cordic_ctab[k];
            x = tx;
            y = ty;
        }
    }

    return y;
    //*c = x; *s = y;
}

// �������ֵ
float absolute(float x)
{
    if (x < 0) {
        return (-x);
    }
    return (x);
}

// ��ƽ��
float square(float x)
{
    float guess = 1;
    int lim = 100;  // ����������100�αƽ�

    // ��αƽ�ʹ���ȴﵽ0.000002
    while ((absolute(guess * guess - x) >= 0.000002) && (lim-- > 0)) {
        guess = ((x / guess) + guess) * 0.5;
    }

    return (guess);
}

// ��������
// res = res / div
void cplxDiv(cplx *res, cplx *div)
{
    cplx tmp;
    float mod2;

    mod2 = div->Re * div->Re + div->Im * div->Im;
    tmp.Re = (res->Re * div->Re + res->Im * div->Im) / mod2;    //(a*c+b*d)/(c*c+d*d);
    tmp.Im = (res->Im * div->Re - res->Re * div->Im) / mod2;    //(b*c-a*d)/(c*c+d*d);
    res->Re = tmp.Re;
    res->Im = tmp.Im;
}

// �����˷�
// res = res * mul
void cplxMul(cplx *res, cplx *mul)
{
    cplx tmp;

    tmp.Re = (res->Re * mul->Re - res->Im * mul->Im);   // a*c - b*d
    tmp.Im = (res->Re * mul->Im + res->Im * mul->Re);   // a*d + b*c
    res->Re = tmp.Re;
    res->Im = tmp.Im;
}
