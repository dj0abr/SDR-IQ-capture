/*
iqcapture:

sound.cpp ... all routines for soundcard and wav-file
*/

#include "iqcapture.h"
#include <alsa/asoundlib.h>
#include <sndfile.h>

snd_pcm_t *capture_handle, *playback_handle;

SNDFILE *wavfile;
sf_count_t ret;
SF_INFO rec_info;

meanvalue meanlevel;

void init_soundcard()
{
	int err;
	int channels = 2;
	snd_pcm_hw_params_t *hw_params;
	unsigned int rate = CAP_RATE;

	if (opmode == 'N')
		channels = 1;

	// printf("initialize soundcard CAPTURE %s\n", sndcard);

	if ((err = snd_pcm_open(&capture_handle, sndcard, SND_PCM_STREAM_CAPTURE, 0)) < 0)
	{
		printf("cannot open audio device %s (%s)\n", sndcard,snd_strerror(err));
		exit(1);
	}

	if ((err = snd_pcm_hw_params_malloc(&hw_params)) < 0) {
		printf("cannot allocate hardware parameter structure (%s)\n",snd_strerror(err));
		exit(1);
	}

	if ((err = snd_pcm_hw_params_any(capture_handle, hw_params)) < 0) {
		printf("cannot initialize hardware parameter structure (%s)\n",snd_strerror(err));
		exit(1);
	}

	if ((err = snd_pcm_hw_params_set_access(capture_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
		printf("cannot set access type (%s)\n",snd_strerror(err));
		exit(1);
	}

	if ((err = snd_pcm_hw_params_set_format(capture_handle, hw_params, SND_PCM_FORMAT_S16_LE)) < 0) {
		printf("cannot set sample format (%s)\n",snd_strerror(err));
		exit(1);
	}

	if ((err = snd_pcm_hw_params_set_rate_near(capture_handle, hw_params, &rate, 0)) < 0) {
		printf("cannot set sample rate (%s)\n",snd_strerror(err));
		exit(1);
	}

	if ((err = snd_pcm_hw_params_set_channels(capture_handle, hw_params, channels)) < 0) {
		printf("cannot set channel count (%s)\n",snd_strerror(err));
		exit(1);
	}

	if ((err = snd_pcm_hw_params(capture_handle, hw_params)) < 0) {
		printf("cannot set parameters (%s)\n",snd_strerror(err));
		exit(1);
	}

	snd_pcm_hw_params_free(hw_params);

	if ((err = snd_pcm_prepare(capture_handle)) < 0) {
		printf("cannot prepare audio interface for use (%s)\n",snd_strerror(err));
		exit(1);
	}

	/*
	initialize the soundcards output. This is not required for the WSPR capturing,
	but we can playback the demodulated sound for test purposes
	*/
	// printf("initialize soundcard PLAYBACK %s\n", sndcard);
	if (playback == 'P')
	{
		if ((err = snd_pcm_open(&playback_handle, sndcard, SND_PCM_STREAM_PLAYBACK, 0)) < 0)
		{
			printf("please disable playback. cannot open audio device %s (%s)\n", sndcard, snd_strerror(err));
			playback = 'N';
		}

		else if ((err = snd_pcm_hw_params_malloc(&hw_params)) < 0) {
			printf("please disable playback.cannot allocate hardware parameter structure (%s)\n", snd_strerror(err));
			playback = 'N';
		}

		else if ((err = snd_pcm_hw_params_any(playback_handle, hw_params)) < 0) {
			printf("please disable playback. cannot initialize hardware parameter structure (%s)\n", snd_strerror(err));
			playback = 'N';
		}

		else if ((err = snd_pcm_hw_params_set_access(playback_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
			printf("please disable playback.cannot set access type (%s)\n", snd_strerror(err));
			playback = 'N';
		}

		else if ((err = snd_pcm_hw_params_set_format(playback_handle, hw_params, SND_PCM_FORMAT_S16_LE)) < 0) {
			printf("please disable playback.cannot set sample format (%s)\n", snd_strerror(err));
			playback = 'N';
		}

		else if ((err = snd_pcm_hw_params_set_rate_near(playback_handle, hw_params, &rate, 0)) < 0) {
			printf("please disable playback. cannot set sample rate (%s)\n", snd_strerror(err));
			playback = 'N';
		}

		else if ((err = snd_pcm_hw_params_set_channels(playback_handle, hw_params, 1)) < 0) {
			printf("please disable playback. cannot set channel count (%s)\n", snd_strerror(err));
			playback = 'N';
		}

		else if ((err = snd_pcm_hw_params(playback_handle, hw_params)) < 0) {
			printf("please disable playback. cannot set parameters (%s)\n", snd_strerror(err));
			playback = 'N';
		}
		else
		{
			snd_pcm_hw_params_free(hw_params);

			if ((err = snd_pcm_prepare(playback_handle)) < 0) {
				printf("please disable playback. cannot prepare audio interface for use (%s)\n", snd_strerror(err));
				playback = 'N';
			}
		}
	}

	// printf("%s successfully initialized\n", sndcard);

	// Initialize the output file where the (demodulated) stream will be saved

	rec_info.samplerate = OUT_RATE;
	rec_info.channels = 1;
	rec_info.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;

	// open wav file
	wavfile = sf_open(wavfilename, SFM_WRITE, &rec_info);
	if (wavfile == NULL)
	{
		printf("12kwrite: Failed to open file: %s error\n", wavfilename);
		exit(1);
	}
	// printf("%s opened\n", wavfilename);
}

void exit_soundcard()
{
	snd_pcm_close(capture_handle);
	if (playback == 'P') snd_pcm_close(playback_handle);
	sf_close(wavfile);
}

// capture 1s from sound card
void capture1s(short *samples)
{
	int err;
	int rate = CAP_RATE;

	// capture a chunk of 1 second
	if ((err = snd_pcm_readi(capture_handle, samples, rate)) != rate) {
		printf("read from audio interface failed (%s)\n", snd_strerror(err));
		exit(1);
	}

	// calculate the mean value, peak value and save everything in a file srcdata.out
	// this file is used by the GUI to display a Spectrum and show the levels
	if (opmode == 'S')
	{
		for (int i = 0; i < (rate * 2); i++)
			meanlevel.add((float)samples[i]);

		// do not disturb the capture process, so fork the file-save to a child process
		int pid = fork();
		if (pid == 0)
		{
			FILE *fw = fopen("/tmp/srcdata.out", "w");
			if (fw)
			{
				// save mean value, MSB first
				short val = (short)(meanlevel.getval());
				fprintf(fw, "%c%c", (val >> 8) & 0xff, val & 0xff);

				// save peak value, MSB first
				val = (short)(meanlevel.getmaxval());
				fprintf(fw, "%c%c", (val >> 8) & 0xff, val & 0xff);

				// save samples (I channel only), MSB first
				for (int i = 0; i < (rate * 2); i += 2)
				{
					fprintf(fw, "%c%c", (samples[i] >> 8) & 0xff, samples[i] & 0xff);
				}

				fclose(fw);
				exit(0);
			}
		}
	}
	else
	{
		for (int i = 0; i < rate; i++)
			meanlevel.add((float)samples[i]);

		// do not disturb the capture process, so fork the file-save to a child process
		int pid = fork();
		if (pid == 0)
		{
			FILE *fw = fopen("/tmp/srcdata.out", "w");
			if (fw)
			{
				// save mean value, MSB first
				short val = (short)(meanlevel.getval());
				fprintf(fw, "%c%c", (val >> 8) & 0xff, val & 0xff);

				// save peak value, MSB first
				val = (short)(meanlevel.getmaxval());
				fprintf(fw, "%c%c", (val >> 8) & 0xff, val & 0xff);

				// save samples, MSB first
				for (int i = 0; i < rate; i++)
				{
					fprintf(fw, "%c%c", (samples[i] >> 8) & 0xff, samples[i] & 0xff);
				}

				fclose(fw);
				exit(0);
			}
		}
	}
}

// send a stream to the sound card for playback
int pbfirst = 1;
void playsound(short * samples, int len)
{
	int err;

	if (playback != 'P') return;

	if ((err = snd_pcm_writei(playback_handle, samples, len)) != len) {
		printf("write to audio interface failed (%s)\n", snd_strerror(err));
		exit(1);
	}

	// write the first frame twice to fill the buffer and aviod underrun
	if (pbfirst == 1)
	{
		pbfirst = 0;
		if ((err = snd_pcm_writei(playback_handle, samples, len)) != len) {
			printf("write to audio interface failed (%s)\n", snd_strerror(err));
			exit(1);
		}
	}
}

// write a 1s long stream to a wav file
// this wav file contains the result, the demodulated sound (in opmode='S') or the original sound (in opmode='N')
void write_wavfile()
{
	//for (int i = 0; i < 200; i++)
		//printf("%d:%d\n", i, samples12k[i]);


	// do not disturb the capture process, so fork the file-save to a child process
	int pid = fork();
	if (pid == 0)
	{
		// write the samples (16 bit samples)
		ret = sf_write_short(wavfile, samples12k, OUT_RATE);
		if (ret <= 0)
		{
			printf("write samples error \n");
			sf_close(wavfile);
		}
		exit(0);
	}
}
