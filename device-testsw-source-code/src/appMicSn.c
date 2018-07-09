
/*
 * sample-ai-ref.c
 *
 * Copyright (C) 2017 Ingenic Semiconductor Co.,Ltd
 */
//mips-linux-uclibc-gnu-g++ -O2 -Wall -march=mips32r2 -lpthread -lm -lrt -ldl -muclibc ../../lib/uclibc/libalog.a ../../lib/uclibc/libimp.a  -I../../include sample-Ai-Ref.c -o audio.o

#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include <imp/imp_isp.h>
#include <imp/imp_audio.h>
#include <imp/imp_log.h>

#define TAG "Sample-Ai-Ref"

#define REF_AEC_SAMPLE_RATE  16000
#define REF_AEC_SAMPLE_TIME  10

#define REF_AUDIO_BUF_SIZE  (REF_AEC_SAMPLE_RATE * sizeof(short) * REF_AEC_SAMPLE_TIME / 1000)
#define REF_AUDIO_RECORD_NUM 500

#define REF_AUDIO_RECORD_FILE_FOR_PLAY "/tmp/ref_test_for_play.pcm"
#define REF_AUDIO_RECORD_FILE "/tmp/ref_test_record.pcm"

#include "FFT.c"

int g_PlaySinOver = 0;
int g_TestOK = 0;

#if 0
static void *IMP_Audio_Record_Thread(void *argv)
{
    int ret = -1;
    int record_num = 0;
    if (argv == NULL) {
        IMP_LOG_ERR(TAG, "Please input the record file name.\n");
        return NULL;
    }
    FILE *record_file = fopen(argv, "wb");
    if (record_file == NULL) {
        IMP_LOG_ERR(TAG, "fopen %s failed\n", argv);
        return NULL;
    }

    /* Step 1: set public attribute of AI device. */
    int devID = 1;
    IMPAudioIOAttr attr;
    attr.samplerate = AUDIO_SAMPLE_RATE_16000;
    attr.bitwidth = AUDIO_BIT_WIDTH_16;
    attr.soundmode = AUDIO_SOUND_MODE_MONO;
    attr.frmNum = 40;
    attr.numPerFrm = 640;
    attr.chnCnt = 1;
    ret = IMP_AI_SetPubAttr(devID, &attr);
    if (ret != 0) {
        IMP_LOG_ERR(TAG, "set ai %d attr err: %d\n", devID, ret);
        return NULL;
    }

    memset(&attr, 0x0, sizeof(attr));
    ret = IMP_AI_GetPubAttr(devID, &attr);
    if (ret != 0) {
        IMP_LOG_ERR(TAG, "get ai %d attr err: %d\n", devID, ret);
        return NULL;
    }

    IMP_LOG_INFO(TAG, "Audio In GetPubAttr samplerate : %d\n", attr.samplerate);
    IMP_LOG_INFO(TAG, "Audio In GetPubAttr   bitwidth : %d\n", attr.bitwidth);
    IMP_LOG_INFO(TAG, "Audio In GetPubAttr  soundmode : %d\n", attr.soundmode);
    IMP_LOG_INFO(TAG, "Audio In GetPubAttr     frmNum : %d\n", attr.frmNum);
    IMP_LOG_INFO(TAG, "Audio In GetPubAttr  numPerFrm : %d\n", attr.numPerFrm);
    IMP_LOG_INFO(TAG, "Audio In GetPubAttr     chnCnt : %d\n", attr.chnCnt);

    /* Step 2: enable AI device. */
    ret = IMP_AI_Enable(devID);
    if (ret != 0) {
        IMP_LOG_ERR(TAG, "enable ai %d err\n", devID);
        return NULL;
    }

    /* Step 3: set audio channel attribute of AI device. */
    int chnID = 0;
    IMPAudioIChnParam chnParam;
    chnParam.usrFrmDepth = 40;
    ret = IMP_AI_SetChnParam(devID, chnID, &chnParam);
    if (ret != 0) {
        IMP_LOG_ERR(TAG, "set ai %d channel %d attr err: %d\n", devID, chnID, ret);
        return NULL;
    }

    memset(&chnParam, 0x0, sizeof(chnParam));
    ret = IMP_AI_GetChnParam(devID, chnID, &chnParam);
    if (ret != 0) {
        IMP_LOG_ERR(TAG, "get ai %d channel %d attr err: %d\n", devID, chnID, ret);
        return NULL;
    }

    IMP_LOG_INFO(TAG, "Audio In GetChnParam usrFrmDepth : %d\n", chnParam.usrFrmDepth);

    /* Step 4: enable AI channel. */
    ret = IMP_AI_EnableChn(devID, chnID);
    if (ret != 0) {
        IMP_LOG_ERR(TAG, "Audio Record enable channel failed\n");
        return NULL;
    }

    /* Step 5: Set audio channel volume. */
    int chnVol = 60;
    ret = IMP_AI_SetVol(devID, chnID, chnVol);
    if (ret != 0) {
        IMP_LOG_ERR(TAG, "Audio Record set volume failed\n");
        return NULL;
    }

    ret = IMP_AI_GetVol(devID, chnID, &chnVol);
    if (ret != 0) {
        IMP_LOG_ERR(TAG, "Audio Record get volume failed\n");
        return NULL;
    }
    IMP_LOG_INFO(TAG, "Audio In GetVol    vol : %d\n", chnVol);

    int aigain = 28;
    ret = IMP_AI_SetGain(devID, chnID, aigain);
    if (ret != 0) {
        IMP_LOG_ERR(TAG, "Audio Record Set Gain failed\n");
        return NULL;
    }

    ret = IMP_AI_GetGain(devID, chnID, &aigain);
    if (ret != 0) {
        IMP_LOG_ERR(TAG, "Audio Record Get Gain failed\n");
        return NULL;
    }
    IMP_LOG_INFO(TAG, "Audio In GetGain    gain : %d\n", aigain);

    while(1) {
        /* Step 6: get audio record frame. */

        ret = IMP_AI_PollingFrame(devID, chnID, 1000);
        if (ret != 0 ) {
            IMP_LOG_ERR(TAG, "Audio Polling Frame Data error\n");
        }
        IMPAudioFrame frm;
        ret = IMP_AI_GetFrame(devID, chnID, &frm, BLOCK);
        if (ret != 0) {
            IMP_LOG_ERR(TAG, "Audio Get Frame Data error\n");
            return NULL;
        }

        /* Step 7: Save the recording data to a file. */
        fwrite(frm.virAddr, 1, frm.len, record_file);

        /* Step 8: release the audio record frame. */
        ret = IMP_AI_ReleaseFrame(devID, chnID, &frm);
        if (ret != 0) {
            IMP_LOG_ERR(TAG, "Audio release frame data error\n");
            return NULL;
        }

        // ld@apical.com.cn
        if (g_PlaySinOver) {
            printf("play test 1Khz pcm sound over\r\n");
            break;
        }

        if (++record_num >= REF_AUDIO_RECORD_NUM)
            break;
    }

    /* Step 9: disable the audio channel. */
    ret = IMP_AI_DisableChn(devID, chnID);
    if (ret != 0) {
        IMP_LOG_ERR(TAG, "Audio channel disable error\n");
        return NULL;
    }

    /* Step 10: disable the audio devices. */
    ret = IMP_AI_Disable(devID);
    if (ret != 0) {
        IMP_LOG_ERR(TAG, "Audio device disable error\n");
        return NULL;
    }

    fclose(record_file);
    pthread_exit(0);
}
#endif

static void* IMP_Audio_Record_Ref_Thread(void *argv)
{
    int ret = -1;
    int record_num = 0;

    char buf[1025];
    int  bufpos = 0;
    int  pos    = 0;

//  printf("REC IMP_Audio_Record_Ref_Thread start \r\n");

    if (argv == NULL) {
        printf("REC:Please input the record file name.\n");
        return NULL;
    }

    /* set public attribute of AI device. */
    int devID = 1;
    IMPAudioIOAttr attr;
    attr.samplerate = AUDIO_SAMPLE_RATE_16000;
    attr.bitwidth = AUDIO_BIT_WIDTH_16;
    attr.soundmode = AUDIO_SOUND_MODE_MONO;
    attr.frmNum = 40;
    attr.numPerFrm = 640;
    attr.chnCnt = 1;
    ret = IMP_AI_SetPubAttr(devID, &attr);
    if (ret != 0) {
        printf("REC:set ai %d attr err: %d\n", devID, ret);
        return NULL;
    }

    memset(&attr, 0x0, sizeof(attr));
    ret = IMP_AI_GetPubAttr(devID, &attr);
    if (ret != 0) {
        printf("REC:get ai %d attr err: %d\n", devID, ret);
        return NULL;
    }

    IMP_LOG_INFO(TAG, "Audio In GetPubAttr samplerate : %d\n", attr.samplerate);
    IMP_LOG_INFO(TAG, "Audio In GetPubAttr   bitwidth : %d\n", attr.bitwidth);
    IMP_LOG_INFO(TAG, "Audio In GetPubAttr  soundmode : %d\n", attr.soundmode);
    IMP_LOG_INFO(TAG, "Audio In GetPubAttr     frmNum : %d\n", attr.frmNum);
    IMP_LOG_INFO(TAG, "Audio In GetPubAttr  numPerFrm : %d\n", attr.numPerFrm);
    IMP_LOG_INFO(TAG, "Audio In GetPubAttr     chnCnt : %d\n", attr.chnCnt);

    /* enable AI device. */
    ret = IMP_AI_Enable(devID);
    if (ret != 0) {
        printf("REC:enable ai %d err\n", devID);
        return NULL;
    }

    /* set audio channel attribute of AI device. */
    int chnID = 0;
    IMPAudioIChnParam chnParam;
    chnParam.usrFrmDepth = 40;
    ret = IMP_AI_SetChnParam(devID, chnID, &chnParam);
    if (ret != 0) {
        printf("REC:set ai %d channel %d attr err: %d\n", devID, chnID, ret);
        return NULL;
    }

    memset(&chnParam, 0x0, sizeof(chnParam));
    ret = IMP_AI_GetChnParam(devID, chnID, &chnParam);
    if (ret != 0) {
        printf("REC:get ai %d channel %d attr err: %d\n", devID, chnID, ret);
        return NULL;
    }

    IMP_LOG_INFO(TAG, "Audio In GetChnParam usrFrmDepth : %d\n", chnParam.usrFrmDepth);

    /* enable AI channel. */
    ret = IMP_AI_EnableChn(devID, chnID);
    if (ret != 0) {
        printf("REC:Audio Record enable channel failed\n");
        return NULL;
    }

    ret = IMP_AI_EnableAecRefFrame(devID, chnID, 0, 0);
    if (ret != 0) {
        printf("REC:Audio Record enable channel failed\n");
        return NULL;
    }

    /* Set audio channel volume. */
    int chnVol = 60;
    ret = IMP_AI_SetVol(devID, chnID, chnVol);
    if (ret != 0) {
        printf("REC:Audio Record set volume failed\n");
        return NULL;
    }

    ret = IMP_AI_GetVol(devID, chnID, &chnVol);
    if (ret != 0) {
        printf("REC:Audio Record get volume failed\n");
        return NULL;
    }
    IMP_LOG_INFO(TAG, "Audio In GetVol    vol : %d\n", chnVol);

    int aigain = 20;
    ret = IMP_AI_SetGain(devID, chnID, aigain);
    if (ret != 0) {
        printf("REC:Audio Record Set Gain failed\n");
        return NULL;
    }

    ret = IMP_AI_GetGain(devID, chnID, &aigain);
    if (ret != 0) {
        printf("REC:Audio Record Get Gain failed\n");
        return NULL;
    }
    IMP_LOG_INFO(TAG, "Audio In GetGain    gain : %d\n", aigain);

    while(1) {
        /* get audio record frame. */
//      printf("REC:Audio Frame 1\n");
        ret = IMP_AI_PollingFrame(devID, chnID, 1000);
        if (ret != 0 ) {
            printf("REC:Audio Polling Frame Data error\n");
        }
        IMPAudioFrame frm;
        IMPAudioFrame ref;
        ret = IMP_AI_GetFrameAndRef(devID, chnID, &frm, &ref,BLOCK);
        if (ret != 0) {
            printf("REC:Audio Get Frame Data error\n");
            return NULL;
        }
//      printf("REC:Audio Frame :%d, %d\n", frm.len, ref.len);

        pos += frm.len;
        if (pos > 0x4000) {
            if ((bufpos+frm.len) <= sizeof(buf)) {
                memcpy(buf+bufpos, frm.virAddr, frm.len);
                bufpos += frm.len;
                printf("copy buf %d bytes bufpos=%d\r\n", frm.len, bufpos);
            } else {
                if (bufpos < sizeof(buf)) {
                    memcpy(buf+bufpos, frm.virAddr, sizeof(buf)-bufpos);
                    bufpos = sizeof(buf);
                    printf("End: copy buf %d(of%d) bytes bufpos=%d\r\n", sizeof(buf)-bufpos, frm.len, bufpos);
                }
            }
        }
//      printf("REC:Audio Frame 3\n");
        /* release the audio record frame. */
        ret = IMP_AI_ReleaseFrame(devID, chnID, &frm);
        if (ret != 0) {
            printf("REC:Audio release frame data error\n");
            return NULL;
        }

//      printf("REC:Audio Frame 4\n");
        //ld@apical.com.cn
        if (g_PlaySinOver) {
            printf("REC: g_PlaySinOver\n");
            break;
        }

        if (++record_num >= REF_AUDIO_RECORD_NUM) {
            printf("REC: record_num:%d >= %d\n", record_num, REF_AUDIO_RECORD_NUM);
            break;
        }
    }

    if (1) {
        printf("start FFT (%x %x)!\r\n", buf[0], buf[1]);
        g_TestOK = Is1Khz(buf);
        printf("end FFT!\r\n");
    }

    ret = IMP_AI_DisableAecRefFrame(devID, chnID, 0, 0);
    if (ret != 0) {
        printf("REC:IMP_AI_DisableAecRefFrame\n");
        return NULL;
    }

    ret = IMP_AI_DisableChn(devID, chnID);
    if (ret != 0) {
        printf("REC:Audio channel disable error\n");
        return NULL;
    }

    /* disable the audio devices. */
    ret = IMP_AI_Disable(devID);
    if (ret != 0) {
        printf("REC:Audio device disable error\n");
        return NULL;
    }

    pthread_exit(0);
}

static int g_play_loop = 0;
static int g_play_stop = 0;
static int g_play_vol  = 0;
static int g_need_sleep= 0;
static void *IMP_Audio_Play_Thread(void *argv)
{
    unsigned char *buf = NULL;
    int size =  0;
    int ret  = -1;

    if (argv == NULL) {
        IMP_LOG_ERR(TAG, "[ERROR] %s: Please input the play file name.\n", __func__);
        return NULL;
    }

    /* Step 1: set public attribute of AO device. */
    int devID = 0;
    IMPAudioIOAttr attr;
    attr.samplerate = AUDIO_SAMPLE_RATE_16000;
    attr.bitwidth = AUDIO_BIT_WIDTH_16;
    attr.soundmode = AUDIO_SOUND_MODE_MONO;
    attr.frmNum = 20;
    attr.numPerFrm = 640;
    attr.chnCnt = 1;
    ret = IMP_AO_SetPubAttr(devID, &attr);
    if (ret != 0) {
        IMP_LOG_ERR(TAG, "set ao %d attr err: %d\n", devID, ret);
        return NULL;
    }

    memset(&attr, 0x0, sizeof(attr));
    ret = IMP_AO_GetPubAttr(devID, &attr);
    if (ret != 0) {
        IMP_LOG_ERR(TAG, "get ao %d attr err: %d\n", devID, ret);
        return NULL;
    }

    IMP_LOG_INFO(TAG, "Audio Out GetPubAttr samplerate:%d\n", attr.samplerate);
    IMP_LOG_INFO(TAG, "Audio Out GetPubAttr   bitwidth:%d\n", attr.bitwidth);
    IMP_LOG_INFO(TAG, "Audio Out GetPubAttr  soundmode:%d\n", attr.soundmode);
    IMP_LOG_INFO(TAG, "Audio Out GetPubAttr     frmNum:%d\n", attr.frmNum);
    IMP_LOG_INFO(TAG, "Audio Out GetPubAttr  numPerFrm:%d\n", attr.numPerFrm);
    IMP_LOG_INFO(TAG, "Audio Out GetPubAttr     chnCnt:%d\n", attr.chnCnt);

    /* Step 2: enable AO device. */
    ret = IMP_AO_Enable(devID);
    if (ret != 0) {
        IMP_LOG_ERR(TAG, "enable ao %d err\n", devID);
        return NULL;
    }

    /* Step 3: enable AI channel. */
    int chnID = 0;
    ret = IMP_AO_EnableChn(devID, chnID);
    if (ret != 0) {
        IMP_LOG_ERR(TAG, "Audio play enable channel failed\n");
        return NULL;
    }

    /* Step 4: Set audio channel volume. */
    int chnVol = g_play_vol ? g_play_vol : 60;
    ret = IMP_AO_SetVol(devID, chnID, chnVol);
    if (ret != 0) {
        IMP_LOG_ERR(TAG, "Audio Play set volume failed\n");
        return NULL;
    }

    ret = IMP_AO_GetVol(devID, chnID, &chnVol);
    if (ret != 0) {
        IMP_LOG_ERR(TAG, "Audio Play get volume failed\n");
        return NULL;
    }
    IMP_LOG_INFO(TAG, "Audio Out GetVol    vol:%d\n", chnVol);

    int aogain = 28;
    ret = IMP_AO_SetGain(devID, chnID, aogain);
    if (ret != 0) {
        IMP_LOG_ERR(TAG, "Audio Play Set Gain failed\n");
        return NULL;
    }

    ret = IMP_AO_GetGain(devID, chnID, &aogain);
    if (ret != 0) {
        IMP_LOG_ERR(TAG, "Audio Play Get Gain failed\n");
        return NULL;
    }
    IMP_LOG_INFO(TAG, "Audio Out GetGain    gain : %d\n", aogain);

    buf = (unsigned char *)malloc(REF_AUDIO_BUF_SIZE);
    if (buf == NULL) {
        IMP_LOG_ERR(TAG, "[ERROR] %s: malloc audio buf error\n", __func__);
        return NULL;
    }

    FILE *play_file = fopen(argv, "rb");
    if (play_file == NULL) {
        IMP_LOG_ERR(TAG, "[ERROR] %s: fopen %s failed\n", __func__, argv);
        return NULL;
    }

    while (!g_play_stop) {
        size = fread(buf, 1, REF_AUDIO_BUF_SIZE, play_file);
        if (size < REF_AUDIO_BUF_SIZE) {
            if (g_play_loop) {
                fseek(play_file, 0, SEEK_SET);
                continue;
            } else {
                break;
            }
        }

        /* Step 5: send frame data. */
        IMPAudioFrame frm;
        frm.virAddr = (uint32_t *)buf;
        frm.len = size;
        ret = IMP_AO_SendFrame(devID, chnID, &frm, BLOCK);
        if (ret != 0) {
            IMP_LOG_ERR(TAG, "send Frame Data error\n");
            return NULL;
        }

        IMPAudioOChnState play_status;
        ret = IMP_AO_QueryChnStat(devID, chnID, &play_status);
        if (ret != 0) {
            IMP_LOG_ERR(TAG, "IMP_AO_QueryChnStat error\n");
            return NULL;
        }

        IMP_LOG_INFO(TAG, "Play: TotalNum %d, FreeNum %d, BusyNum %d\n",
                play_status.chnTotalNum, play_status.chnFreeNum, play_status.chnBusyNum);
    }

    if (g_need_sleep) {
        sleep(g_need_sleep);
    }

    g_play_vol   = 0;
    g_need_sleep = 0;
    fclose(play_file);
    free(buf);

    /* Step 6: disable the audio channel. */
    ret = IMP_AO_DisableChn(devID, chnID);
    if (ret != 0) {
        IMP_LOG_ERR(TAG, "Audio channel disable error\n");
        return NULL;
    }

    /* Step 7: disable the audio devices. */
    ret = IMP_AO_Disable(devID);
    if (ret != 0) {
        IMP_LOG_ERR(TAG, "Audio device disable error\n");
        return NULL;
    }

    pthread_exit(0);
}

#include "server.c"

int TEST_SPK_MIC(int argc, FACTORYTEST_DATA* plFtD, int vol)
{
    int ret;
    pthread_t tid_record, tid_play;
    g_PlaySinOver = 0;

    InitFFT();

    g_play_vol = vol;
    ret = pthread_create(&tid_play, NULL, IMP_Audio_Play_Thread, "./sin1khz.pcm");
    if (ret != 0) {
        IMP_LOG_ERR(TAG, "[ERROR] %s: pthread_create Audio Play failed\n", __func__);
        DeinitFFT();
        return -1;
    }

#if 1
    ret = pthread_create(&tid_record, NULL, IMP_Audio_Record_Ref_Thread, REF_AUDIO_RECORD_FILE);
    if (ret != 0) {
        IMP_LOG_ERR(TAG, "[ERROR] %s: pthread_create Audio Record failed\n", __func__);
        DeinitFFT();
        return -1;
    }
#else
    ret = pthread_create(&tid_record, NULL, IMP_Audio_Record_Thread, REF_AUDIO_RECORD_FILE_FOR_PLAY);
    if (ret != 0) {
        IMP_LOG_ERR(TAG, "[ERROR] %s: pthread_create Audio Record failed\n", __func__);
        DeinitFFT();
        return -1;
    }
#endif

    pthread_join(tid_play  , NULL);
    g_PlaySinOver = 1;
    pthread_join(tid_record, NULL);
    printf("g_TestOK=%d (%d)\r\n", g_TestOK, plFtD->rtMic);

    if (g_TestOK) {
        plFtD->rtMic = 1;
    } else {
        plFtD->rtMic = 0;
    }

#if 1
    if (argc >= 2) {
        g_need_sleep = 1;
        if (g_TestOK) {
            plFtD->rtMic = 1;
            if (argc >= 2) {
                ret = pthread_create(&tid_play, NULL, IMP_Audio_Play_Thread, "./testOK.pcm");
            }
        } else {
            plFtD->rtMic = 0;
            if (argc >= 2) {
                ret = pthread_create(&tid_play, NULL, IMP_Audio_Play_Thread, "./testFail.pcm");
            }
        }
        if (ret != 0) {
            IMP_LOG_ERR(TAG, "[ERROR] %s: pthread_create Audio Play failed\n", __func__);
            DeinitFFT();
            return -1;
        }
        pthread_join(tid_play, NULL);
    }
#endif

    DeinitFFT();
//  printf("test spk & mic over\r\n");
    return 0;
}

pthread_t g_tid_play = 0;
int TEST_PLAY_MUSIC(int onoff, int vol)
{
//  printf("++TEST_PLAY_MUSIC onoff = %d, vol = %d\r\n", onoff, vol);
    if (onoff) {
        if (!g_tid_play || pthread_kill(g_tid_play, 0) == ESRCH) {
            g_play_loop = 1;
            g_play_stop = 0;
            g_play_vol  = vol;
            printf("create playback thread\r\n");
            pthread_create(&g_tid_play, NULL, IMP_Audio_Play_Thread, "./startrecord.pcm");
        }
    } else {
        g_play_stop = 1;
        g_play_loop = 0;
        if (g_tid_play) {
            printf("is playing, wait play stop..\r\n");
            pthread_join(g_tid_play, NULL);
            printf("play stoped\r\n");
        }
        g_play_loop = 0;
        g_play_stop = 0;
    }
//  printf("--TEST_PLAY_MUSIC onoff = %d, vol = %d\r\n", onoff, vol);
    return 0;
}

