/*
iqcapture:

fft.cpp ... all routines for fft processing
*/

#include "iqcapture.h"

double *din48;					// input data for  fft, output data frim ifft
fftw_complex *cpout48;			// ouput data from fft, input data to ifft, 24000 values = 1 value/Hz
fftw_plan plan, iplan;
int fftcnt;

meanvalue srclevel, dstlevel;

void init_fft()
{
	// printf("init fft\n");
	din48 = (double *)malloc(sizeof(double) * CAP_RATE);

	cpout48 = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * CAP_RATE);

	plan = fftw_plan_dft_r2c_1d(CAP_RATE, din48, cpout48, FFTW_MEASURE);
	iplan = fftw_plan_dft_c2r_1d(CAP_RATE, cpout48, din48, FFTW_MEASURE);
}

void exit_fft()
{
	// printf("clean up fft\n");
	fftw_destroy_plan(plan);
	fftw_destroy_plan(iplan);
	free(din48);
	fftw_free(cpout48);
}

void shiftdown_20kHz()
{
	int i;
	int fmin = 1300;
	int fmax = 1700;
	float scale=1;

	float max1 = 0;
	for (i = 0; i < CAP_RATE; i++)
	{
		// fill samples into the fft input buffer
		din48[i] = usbsamples[i];
		if (fabs(din48[i]) > max1) max1 = fabs(din48[i]);
	}
	// measure maximum level of demodulated soundcard data
	srclevel.add(max1);

	// calculate spectrum of a 1s long stream
	fftw_execute(plan);
	fftcnt = CAP_RATE / 2 + 1;

	save_spectrum1s();

	// since we converted a 1s stream, the resulting spectrum goes from 0 to CAP_RATE/2+1
	// with a resolution of 1Hz. The length of the spectrum is fftcnt.

	if (opmode == 'N')
	{
		// normal mode: we are ready
		return;
	}

	// shift the demodulated signal from the IF into the baseband (0 Hz)
	// fmin and fmax are the minimum and maximum audio frequencies of interest
	for (i = fmin; i < fmax; i++)
	{
		cpout48[i][0] = cpout48[i + if_qrg][0];
		cpout48[i][1] = cpout48[i + if_qrg][1];
	}

	// NULL all frequencies below fmin and above fmax
	// this eliminates the need of an additional filter
	for (i = 0; i < fmin; i++)
	{
		cpout48[i][0] = 0;
		cpout48[i][1] = 0;
	}
	for (i = fmax; i < fftcnt; i++)
	{
		cpout48[i][0] = 0;
		cpout48[i][1] = 0;
	}
	// Notch Filter
	/*for (i = 1490; i < 1499; i++)
	{
		cpout48[i][0] = 0;
		cpout48[i][1] = 0;
	}*/

	// with invers fft convert the spectrum back to samples
	fftw_execute(iplan);

	// since the ifft's result is scaled to somewhere, we need to scale it down to match the level of the original stream
	// measure the level produced by ifft
	float max2=0;
	for (i = 0; i < CAP_RATE; i++)
	{
		if (fabs(din48[i]) > max2) max2 = fabs(din48[i]);
	}
	dstlevel.add(max2);

	// calculate the required scaling
	scale = dstlevel.getval() / srclevel.getval();
	
	// and copy scaled samples into the resulting buffer
	for (i = 0; i < CAP_RATE; i++)
		usbsamples[i] = din48[i] / scale;

	// check for overflow. We need to put the samples into a 16 bit short, test if it fits
	int maxfval = 0;
	for (i = 0; i < CAP_RATE; i++)
	{
		if (usbsamples[i] > maxfval)
			maxfval = usbsamples[i];
	}

	if (maxfval > 32767)
	{
		// it dows not fit, scale down to the maximum possible value, which is 32767
		for (i = 0; i < CAP_RATE; i++)
		{
			usbsamples[i] = usbsamples[i] * 32767 / maxfval;
		}
	}
}

// save 1s spectrum into a file
//float maxmag = 0;
void save_spectrum1s()
{
	float real, imag, mag;
	unsigned int ival;

#ifdef _LINUX_
	// do not disturb the capture process, so fork the file-save to a child process
	int pid = fork();
	if (pid == 0)
#endif
	{
		FILE *fw = fopen("/tmp/specdata.out", "w");
		if (fw)
		{
			for (int i = 0; i < fftcnt; i++)
			{
				// calculate absolute value
				real = cpout48[i][0];
				imag = cpout48[i][1];
				mag = sqrt((real * real) + (imag * imag));

				// scaling not required since max possible value fits in int
				// the max possible value is 32768 * 24000 (input size * N)

				ival = (unsigned int)(mag);

				// save value, MSB first
				fprintf(fw, "%c%c%c%c", (ival >> 24) & 0xff, (ival >> 16) & 0xff, (ival >> 8) & 0xff, ival & 0xff);
			}

			fclose(fw);
			exit(0);
		}
	}
}
