#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <speex/speex_preprocess.h>
#include <speex/speex_echo.h>
#include "wavdev.h"

typedef struct {
    void                 *wavdev;
    SpeexPreprocessState *speexps;
    SpeexEchoState       *speexes;
} TESTCTXT;

static void wavin_callback_proc(void *ctxt, void *buf, int len)
{
    TESTCTXT *test = (TESTCTXT*)ctxt;
    speex_preprocess_run(test->speexps, (spx_int16_t*)buf);
//  speex_echo_capture(test->speexes, (spx_int16_t*)buf, (spx_int16_t*)buf);
    wavdev_play(test->wavdev, buf, len);
}

static void wavout_callback_proc(void *ctxt, void *buf, int len)
{
    TESTCTXT *test = (TESTCTXT*)ctxt;
//  speex_echo_playback(test->speexes, (spx_int16_t*)buf);
}

int main(void)
{
    void                 *dev = NULL;
    SpeexPreprocessState *sps = NULL;
    SpeexEchoState       *ses = NULL;
    int                   ival;
    TESTCTXT              test;

    dev  = wavdev_init(8000, 1, 160, 10, wavin_callback_proc, &test, 8000, 1, 160, 10, wavout_callback_proc, &test);
    sps  = speex_preprocess_state_init(8000 / 50, 8000);
    ses  = speex_echo_state_init(8000 / 50, 800);
    test.wavdev = dev;
    test.speexps= sps;
    test.speexes= ses;

    ival = 1;     speex_preprocess_ctl(sps, SPEEX_PREPROCESS_SET_DENOISE       , &ival);
    ival =-15;    speex_preprocess_ctl(sps, SPEEX_PREPROCESS_SET_NOISE_SUPPRESS, &ival);
    ival = 1;     speex_preprocess_ctl(sps, SPEEX_PREPROCESS_SET_AGC           , &ival);
    ival = 32768; speex_preprocess_ctl(sps, SPEEX_PREPROCESS_SET_AGC_TARGET    , &ival);
    ival = 9;     speex_preprocess_ctl(sps, SPEEX_PREPROCESS_SET_AGC_INCREMENT , &ival);
    ival =-30;    speex_preprocess_ctl(sps, SPEEX_PREPROCESS_SET_AGC_DECREMENT , &ival);
    ival = 15;    speex_preprocess_ctl(sps, SPEEX_PREPROCESS_SET_AGC_MAX_GAIN  , &ival);
//  ival = 1;     speex_preprocess_ctl(sps, SPEEX_PREPROCESS_SET_VAD           , &ival);

    while (1) {
        char cmd[256];
        scanf("%256s", cmd);
        if (strcmp(cmd, "rec_start") == 0) {
            wavdev_record(dev, 1);
        } else if (strcmp(cmd, "rec_stop") == 0) {
            wavdev_record(dev, 0);
        } else if (strcmp(cmd, "quit") == 0 || strcmp(cmd, "exit") == 0) {
            break;
        }
    }

    speex_preprocess_state_destroy(sps);
    speex_echo_state_destroy(ses);
    wavdev_exit(dev);
    return 0;
}

