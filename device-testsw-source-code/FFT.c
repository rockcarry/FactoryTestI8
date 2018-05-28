#include "math.h"

#define pi ((double)3.14159265359)

// 复数定义
typedef struct
{
    double re;
    double im;
} COMPLEX;

// 复数加运算
COMPLEX Add(COMPLEX c1, COMPLEX c2)
{
    COMPLEX c;
    c.re = c1.re+c2.re;
    c.im = c1.im+c2.im;
    return c;
}

// 复数乘运算
COMPLEX Mul(COMPLEX c1, COMPLEX c2)
{
    COMPLEX c;
    c.re = c1.re*c2.re - c1.im*c2.im;
    c.im = c1.re*c2.im + c2.re*c1.im;
    return c;
}

// 复数减运算
COMPLEX Sub(COMPLEX c1, COMPLEX c2)
{
    COMPLEX c;
    c.re = c1.re - c2.re;
    c.im = c1.im - c2.im;
    return c;
}

// 快速傅里叶变换
void FFT(COMPLEX *TD, COMPLEX *FD, int power)
{
    int count;
    int i, j , k, bfsize, p;
    double angle;
    COMPLEX *W, *X1, *X2, *X;

    count = 1<<power;

    W  = (COMPLEX*) malloc(sizeof(COMPLEX) * count);
    X1 = (COMPLEX*) malloc(sizeof(COMPLEX) * count);
    X2 = (COMPLEX*) malloc(sizeof(COMPLEX) * count);

    for (i=0; i<count/2; i++) {
        angle = -i*pi*2/count;
        W[i].re = cos(angle);
        W[i].im = sin(angle);
    }

    memcpy(X1, TD, sizeof(COMPLEX)*count);

    for (k=0; k<power; k++) {
        for (j=0; j<1<<k; j++) {
            bfsize = 1<<(power-k);
            for (i=0; i<bfsize/2; i++) {
                p = j*bfsize;
                X2[i+p] = Add(X1[i+p], X1[i+p+bfsize/2]);
                X2[i+p+bfsize/2] = Mul(Sub(X1[i+p], X1[i+p+bfsize/2]), W[i*(1<<k)]);
            }
        }
        X  = X1;
        X1 = X2;
        X2 = X;
    }

    for (j=0; j<count; j++) {
        p = 0;
        for (i=0;i<power;i++) {
            if (j&(1<<i)) {
                p += 1<<(power-i-1);
            }
        }
        FD[j] = X1[p];
    }

    free(W);
    free(X1);
    free(X2);
}

// 逆变换
void IFFT(COMPLEX *FD, COMPLEX *TD, int power)
{
    int i, count;
    COMPLEX *x;

    count = 1<<power;
    x = (COMPLEX *)malloc(sizeof(COMPLEX)*count);
    memcpy(x, FD, sizeof(COMPLEX)*count);

    for (i=0; i<count; i++) {
        x[i].im = -x[i].im;
    }
    FFT(x, TD, power);

    for (i=0; i<count; i++) {
        TD[i].re /= count;
        TD[i].im  = -TD[i].im/count;
    }
    free(x);
}

#define POWER   10
#define N      (1<<POWER)
COMPLEX *TV = NULL, *FV = NULL;

int InitFFT() {
    TV = (COMPLEX *)malloc(sizeof(COMPLEX)*N);
    FV = (COMPLEX *)malloc(sizeof(COMPLEX)*N);
    return 0;
}

int Is1Khz(char *pcm)
{
    int res = 16000/1024; // 16K 采样, 1024 点 FFT
    int skipHz  = 60;
    int skippos = skipHz/res;

    double Max = 0;
    int MaxPos = 0;

    double sMax= 0;
    int sMaxPos= 0;

    double mag[N/2+1];
    int    i;
    char*  pPcm = pcm;

    for (i=0; i<N; i++) {
        int sample = 0;
        sample  = (*pPcm);
        pPcm++;
        sample += ((*pPcm)<<8);
        pPcm++;

        TV[i].re = sample;
        TV[i].im = 0;
    }

    FFT(TV, FV, POWER);

    mag[0] = (double) sqrt(FV[0].re * FV[0].re + FV[0].im * FV[0].im) / (double) N;
    if (mag[0] > Max) {
        Max = mag[0];
        MaxPos = 0;
//      printf("mag[%d]=%f\r\n", 0, mag[0]);
    }
    for (i = 1; i < N/2; i++) {
        mag[i] = (double)((2.0F * (double) sqrt(FV[i].re * FV[i].re + FV[i].im * FV[i].im))/(double)N);
//      printf("mag[%d]=%f\r\n", i, mag[i]);
        if (mag[i] > Max) {
            Max = mag[i];
            MaxPos = i;
        }
    }

    for (i = 0; i < N/2; i++) {
        if (i == MaxPos) { // skip max
            continue;
        }

        if ((i > MaxPos-skippos) && (i < MaxPos+skippos)) { // skip
            printf("skip pos:%d %dHz\r\n", i, i*res);
            continue;
        }

        if (mag[i] > sMax) {
            sMax = mag[i];
            sMaxPos = i;
        }
    }
    printf("MaxPos=:%d, mag[%d]=%f freq:%dHz  sMaxPos(mag[%d]=%f %dHz) %f\r\n", MaxPos, MaxPos, Max, res*MaxPos, sMaxPos, sMax, res*sMaxPos, Max/sMax);

    //ee(TV);
    //ee(FV);
    if ((MaxPos > 66-skippos) && (MaxPos < 66+skippos) && (Max > 2000.0)) {
        printf("get 1K Hz OK\r\n");
        return 1;
    }
    if ((MaxPos > 192-skippos) && (MaxPos < 192+skippos) && (Max > 2000.0)) {
        printf("get 1K Hz OK\r\n");
        return 1;
    }
    if ((MaxPos > 320-skippos) && (MaxPos < 320+skippos) && (Max > 2000.0)) {
        printf("get 1K Hz OK\r\n");
        return 1;
    }

    return 0;
}

int DeinitFFT()
{
    free(TV);
    free(FV);
    return 0;
}

