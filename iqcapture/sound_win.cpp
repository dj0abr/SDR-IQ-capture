/*
iqcapture:

sound_win.cpp ... all routines for soundcard and wav-file under windows
*/

#include "iqcapture.h"


meanvalue meanlevel;

void init_soundcard()
{

}

void exit_soundcard()
{

}

// capture 1s from sound card
void capture1s(short *samples)
{

}

// send a stream to the sound card for playback
int pbfirst = 1;
void playsound(short * samples, int len)
{

}

// write a 1s long stream to a wav file
// this wav file contains the result, the demodulated sound (in opmode='S') or the original sound (in opmode='N')
void write_wavfile()
{

}
