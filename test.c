#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <speex/speex_preprocess.h>
#include <speex/speex_echo.h>
#include "wavdev.h"
#include "wavfile.h"

typedef struct {
    void                 *wavdev ;
    void                 *wavfile;
    SpeexPreprocessState *speexps;
    SpeexEchoState       *speexes;
    uint8_t               ringbuf[160 * 3 * sizeof(int16_t)];
    int                   head;
    int                   tail;
    int                   size;
    pthread_mutex_t       mutex;
    uint8_t              *rec_bufaddr;
    int                   rec_bufsize;
    int                   rec_curpos;

    FILE                 *pcmfile;
    int16_t               pcmbuf[160];
} TESTCTXT;

int ringbuf_write(uint8_t *rbuf, int maxsize, int tail, uint8_t *src, int len)
{
    uint8_t *buf1 = rbuf    + tail;
    int      len1 = maxsize - tail < len ? maxsize - tail : len;
    uint8_t *buf2 = rbuf;
    int      len2 = len - len1;
    memcpy(buf1, src + 0   , len1);
    memcpy(buf2, src + len1, len2);
    return len2 ? len2 : tail + len1;
}

int ringbuf_read(uint8_t *rbuf, int maxsize, int head, uint8_t *dst, int len)
{
    uint8_t *buf1 = rbuf    + head;
    int      len1 = maxsize - head < len ? maxsize - head : len;
    uint8_t *buf2 = rbuf;
    int      len2 = len - len1;
    if (dst) memcpy(dst + 0   , buf1, len1);
    if (dst) memcpy(dst + len1, buf2, len2);
    return len2 ? len2 : head + len1;
}

static void play_pcm_file_buf(TESTCTXT *test)
{
    int ret = fread(test->pcmbuf, 1, 160 * sizeof(int16_t), test->pcmfile);
    if (ret == 160 * sizeof(int16_t)) wavdev_play(test->wavdev, test->pcmbuf, 160 * sizeof(int16_t));
}

static void wavin_callback_proc(void *ctxt, void *buf, int len)
{
    TESTCTXT *test = (TESTCTXT*)ctxt;
    int16_t  *out  = (int16_t *)buf ;
    int16_t   ref[160], tmp[160];

    pthread_mutex_lock(&test->mutex);
    if (test->size >= sizeof(ref)) {
        test->head  = ringbuf_read(test->ringbuf, sizeof(test->ringbuf), test->head, (uint8_t*)ref, sizeof(ref));
        test->size -= sizeof(ref);
        speex_echo_cancellation(test->speexes, (spx_int16_t*)buf, (spx_int16_t*)ref, (spx_int16_t*)tmp); out = tmp;
    }
    pthread_mutex_unlock(&test->mutex);

    speex_preprocess_run(test->speexps, (spx_int16_t*)out);
    if (test->rec_curpos + len < test->rec_bufsize) {
        memcpy(test->rec_bufaddr + test->rec_curpos, out, len);
        test->rec_curpos += len;
    }
}

static void wavout_callback_proc(void *ctxt, void *buf, int len)
{
    TESTCTXT *test = (TESTCTXT*)ctxt;
    pthread_mutex_lock(&test->mutex);
    if (len <= sizeof(test->ringbuf) - test->size) {
        test->tail  = ringbuf_write(test->ringbuf, sizeof(test->ringbuf), test->tail, buf, len);
        test->size += len;
    }
    pthread_mutex_unlock(&test->mutex);
    play_pcm_file_buf(test);
}

int main(void)
{
    void                 *dev = NULL;
    void                 *file= NULL;
    SpeexPreprocessState *sps = NULL;
    SpeexEchoState       *ses = NULL;
    TESTCTXT              test= {0};
    int                   ival, i;

    dev  = wavdev_init(8000, 1, 160, 5, wavin_callback_proc, &test, 8000, 1, 160, 5, wavout_callback_proc, &test);
    file = wavfile_create(8000, 1, 60000);
    sps  = speex_preprocess_state_init(8000 / 50, 8000);
    ses  = speex_echo_state_init(8000 / 50, 800);
    test.wavdev   = dev;
    test.wavfile  = file;
    test.speexps  = sps;
    test.speexes  = ses;
    wavfile_getval(test.wavfile, "buffer_pointer", &test.rec_bufaddr);
    wavfile_getval(test.wavfile, "buffer_size"   , &test.rec_bufsize);
    pthread_mutex_init(&test.mutex, NULL);

    ival = 1;     speex_preprocess_ctl(sps, SPEEX_PREPROCESS_SET_DENOISE       , &ival);
    ival =-15;    speex_preprocess_ctl(sps, SPEEX_PREPROCESS_SET_NOISE_SUPPRESS, &ival);
    ival = 1;     speex_preprocess_ctl(sps, SPEEX_PREPROCESS_SET_AGC           , &ival);
    ival = 32768; speex_preprocess_ctl(sps, SPEEX_PREPROCESS_SET_AGC_TARGET    , &ival);
    ival = 9;     speex_preprocess_ctl(sps, SPEEX_PREPROCESS_SET_AGC_INCREMENT , &ival);
    ival =-30;    speex_preprocess_ctl(sps, SPEEX_PREPROCESS_SET_AGC_DECREMENT , &ival);
    ival = 15;    speex_preprocess_ctl(sps, SPEEX_PREPROCESS_SET_AGC_MAX_GAIN  , &ival);
//  ival = 1;     speex_preprocess_ctl(sps, SPEEX_PREPROCESS_SET_VAD           , &ival);
    ses  = ses;   speex_preprocess_ctl(sps, SPEEX_PREPROCESS_SET_ECHO_STATE    ,  ses );

    wavdev_record(dev, 1);
    test.pcmfile = fopen("test.pcm", "rb");
    for (i=0; i<5-1; i++) play_pcm_file_buf(&test);

    while (1) {
        char cmd[256]; scanf("%256s", cmd);
        if (strcmp(cmd, "quit") == 0 || strcmp(cmd, "exit") == 0) break;
    }

    if (test.pcmfile) fclose(test.pcmfile);
    wavdev_record(dev, 0);
    pthread_mutex_destroy(&test.mutex);
    speex_preprocess_state_destroy(sps);
    speex_echo_state_destroy(ses);
    wavfile_save(file, "rec.wav", test.rec_curpos);
    wavfile_free(file);
    wavdev_exit (dev );
    return 0;
}

