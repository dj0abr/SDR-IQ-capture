#ifdef _WIN32
#define _WIN32_
#else
#define _LINUX_
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <fftw3.h>
#include <math.h>
#include "fir.h"
#include "meanvalue.h"

#ifdef _LINUX_
#include <unistd.h>
#endif

#define VERSION "V1.01 Jan 2016"

#define CAP_RATE	48000	// capture rate for SDR radios
#define OUT_RATE	12000	// WSPR sample rate, in/out for normal mode or out for SDR mode

void init_soundcard();
void init_fft();
void exit_fft();
void exit_soundcard();
void runloop();
void capture1s(short *samples);
float firplus(float NewSample, float *);
float firminus45(float NewSample);
void shiftdown_20kHz();
void write_wavfile();
void playsound(short * samples, int len);
float fir_4k_48k_TP(float NewSample);
void save_spectrum1s();
int exit_fft12();
int init_fft12();
void fft12(short *samples);


extern char sndcard[20];
extern char wavfilename[300];
extern float usbsamples[CAP_RATE];
extern short samples12k[OUT_RATE];
extern int if_qrg;
extern char opmode;
extern char playback;
