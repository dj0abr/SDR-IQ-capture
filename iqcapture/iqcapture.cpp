/*
iqcapture: I/Q sampler and SSB demodulator for WSPR Receivers
=============================================================
Author: DJ0ABR
Date: Jan 2016

*   (c) DJ0ABR
*   www.dj0abr.de
*
*   This program is free software; you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation; either version 2 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program; if not, write to the Free Software
*   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

* reads an I/Q datastream from a soundcard,
* mirror supression,
* USB demodulation
* writes the resulting stream into a WAV file
* two mode: IQ demodulation or normal capture for standard radios

command line options:
---------------------
iqcapture soundcard outfilename iffrequency duration IQmode Playback

soundcard ... hw:1,0 or similar
outfilename ... name of the resulting wav file
iffrequency ... frequency of the SDR IF in Hz
duration ... duration of recording in seconds
IQmode ... S=use IQ demodulation for SDR Receivers,  N=just sample, no IQ demodulation (for normal receivers)
Playback ... N=no playback P=playback the demodulated signal at the soundcard's output

predefined parameters for IQmode=S:
-----------------------------------
capture rate: 48000
channels: 2 (I and Q), 16 bit signed, alternating I and Q
sample rate of resulting wav file: 12000
mode of wav file: 1 channel mono, 16 bit signed S16_LE

predefined parameters for IQmode=N:
-----------------------------------
capture rate: 48000
channels: 1, mono
sample rate of resulting wav file: 12000
mode of wav file: 1 channel mono, 16 bit signed S16_LE

Output: this program generates these data files:
================================================
* The resulting data stream is written in WAV file format into the file /tmp/outfilename. This file is
compatible with the C-Version of K9AN/K1JTs WSPR Decoder.
* File: /tmp/srcdata.out: contains the samples (16 bit) of 1 second of sound data. This is intended to be used for a spectrum display
* File: /tmp/specdata.out: contains 24000 ints (32bit, MSB first) with 1 second of FFT output data. This is intended to be used for a waterfall diagram.


Principle of operation:
-----------------------

make a 90 deg. phase shift between I and Q by a hamming FIR filter of -45 and +45 deg,
this eliminates the mirror. Coefficients between 25 to 65 deg are available to optimize
mirror supression.
Add the resulting samples which does the USB demodulation
FFT-shift the USB signal from IF into the baseband of 0 Hz, this fft is also
used to filter the stream by setting all unwanted frequencies to 0. This eliminates the
need for an aditional low pass filter (which is prepared in case we need it).

The FIR coefficients are calculated by: Iowa Hills Hilbert Filters and by
WinFilter.exe (winfilter.20m.com)
*/

#include "iqcapture.h"

char sndcard[20];		// name of the soundcard i.e. "pulse", "hw:1,0" ...
char wavfilename[300];	// name of the wavfile where the captured (and in opmode 'S' demodulated) data are stored
char s_if[30];			// ZF frequency of the SDR radio as text
int if_qrg = 20000;		// ZF frequency of the SDR radio as int
char s_dur[30];			// duration for one capture, the program terminates after that time, as text
int duration;			// duration as int
char opmode = 'S';		// opmode: S...SDR mode includes USB demodulation, N...normal mode, just capturing
char playback = 'N';	// playback: N...do not playback and don't init the soundcard for playback. P...do playback

int main(int argc, char *argv[])
{
	printf("Sound Capture and USB IQ Demodulator (DJ0ABR) %s\n", VERSION);

#ifdef _LINUX_
	signal(SIGCHLD, SIG_IGN);	// delete old fork proceses
#endif

	if (argc != 7)
	{
		printf("wring number of parameters, exit program\n");
		exit(1);
	}

	strcpy(sndcard, argv[1]);
	strcpy(wavfilename, argv[2]);
	strcpy(s_if, argv[3]);
	strcpy(s_dur, argv[4]);
	opmode = *(argv[5]);
	playback = *(argv[6]);

	if_qrg = atoi(s_if);		// convert if_qrg to an integer
	duration = atoi(s_dur);		// convert duration to an integer
	if (duration == 0)
	{
		printf("no recording time specified\n");
		exit(1);
	}

	init_soundcard();			// init soundcard for capture and playback and init the resulting WAV file
	if (fftw_import_wisdom_from_filename("visualwspr_fft.dat") == 0)
	{
		printf("fft wisdom not found, recreating ...\n");
	}
	init_fft();					// init SDR fft for IQ ZF shifting and complete spectrum and waterfall (only used for IQ receiver)
	init_fft12();				// init fft for WSPR waterfall
	if (fftw_export_wisdom_to_filename("visualwspr_fft.dat") == 0)
	{
		printf("export fft wisdom error\n");
	}

	runloop();

	exit_fft();
	exit_fft12();
	exit_soundcard();
}

/*
Main loop: eruns for "duration" seconds, the terminates.
Capture and process samples for duration (112s is one WSPR interval)
1s has 48000 samples from the sound card
*/

short samples[CAP_RATE * 2];	// 1s I and 1s Q, alternating
float isamples[CAP_RATE];
float qsamples[CAP_RATE];
float usbsamples[CAP_RATE];
short samples12k[OUT_RATE];

void runloop()
{
	int i;

	// printf("enterrun loop\n");
	for (int loops = 0; loops < duration; loops++)
	{
		//printf("loop: %d\n",loops);
		// capture 1s of sound data
		capture1s(samples);
		//printf("captured\n");

		if (opmode == 'S')
		{
			// SDR mode, do the USB demodulation
			// printf("separate I and Q and convert to float\n");
			for (i = 0; i < CAP_RATE; i++)
			{
				isamples[i] = (float)samples[i * 2];
				qsamples[i] = (float)samples[i * 2 + 1];
			}

			// printf("Hilbert Transformation -45 deg. I Samples\n");
			for (i = 0; i < CAP_RATE; i++)
			{
				isamples[i] = firminus45(isamples[i]);
			}

			// printf("Hilbert Transformation +45 deg. Q Samples\n");
			for (i = 0; i < CAP_RATE; i++)
			{
				// normally choose FIRCoefp45.
				// choose other values to optimize mirror image supression
				qsamples[i] = firplus(qsamples[i], FIRCoefp30);
			}

			// make USB by summing I and Q
			for (i = 0; i < CAP_RATE; i++)
			{
				usbsamples[i] = isamples[i] + qsamples[i];
			}
		}
		else
		{
			// normal mode, already demodulated signal from a transceiver
			for (i = 0; i < CAP_RATE; i++)
			{
				usbsamples[i] = samples[i];
			}
		}

		// SDR mode: shift the passband from IF to baseband (0 Hz) and calculate the spectrum for the GUI
		// normal mode: just calculate the spectrum for the GUI
		shiftdown_20kHz();

		// 4 kHz low pass filter (not required since the fft already band passes the WSPR band only)
		/*for (i = 0; i < CAP_RATE; i++)
		{
			usbsamples[i] = fir_4k_48k_TP(usbsamples[i]);
		}*/

		// play to soundcard
		short ssamples[CAP_RATE];
		for (i = 0; i < CAP_RATE; i++)
		{
			ssamples[i] = (short)(usbsamples[i]);
		}
		playsound(ssamples, CAP_RATE);

		// Decimate by 4: reduce samplerate to 12000
		int n = 0;
		for (i = 0; i < CAP_RATE; i += 4)
		{
			samples12k[n++] = (short)usbsamples[i];
		}
		// make an fft for a spectrum and waterfall diagram
		fft12(samples12k);
		
		write_wavfile();
	}
}
