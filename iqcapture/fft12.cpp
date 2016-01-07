/*
fft of an 12000 samples/s WSPR stream 
to generate data for spectrum and waterfall diagrams
*/

#include "iqcapture.h"

int newfile = 1;
const int sampleduration = 2;	// work on 2s chunks
int outcnt;
double *din12;
fftw_complex *cpout12;
fftw_plan plan12;

int init_fft12()
{
	//printf("alloc input \n");
	din12 = (double *)malloc(sizeof(double) * OUT_RATE*sampleduration);

	//printf("alloc output \n");
	cpout12 = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * OUT_RATE*sampleduration);

	//printf("make plan \n");
	plan12 = fftw_plan_dft_r2c_1d(OUT_RATE*sampleduration, din12, cpout12, FFTW_MEASURE);

	return 0;
}

int exit_fft12()
{
	fftw_destroy_plan(plan12);
	free(din12);
	fftw_free(cpout12);

	return 0;
}

/*
do the FFT for the WSPR waterfall
this function is called every 1s but we want 2s chunks, collect samples until 2s are full
*/

void fft12(short *samples)
{
	int i;
	double real, imag, mag;
	static int chunk = 0;

	// copy input data (samples)
	//printf("copy input data \n");

	if (chunk == 0)
	{
		// first chunk of 1s
		for (i = 0; i<OUT_RATE; i++)
			din12[i] = (double)samples[i];
		chunk = 1;
		return;
	}

	// second chunk of an additional second
	for (i = 0; i<OUT_RATE; i++)
		din12[i + OUT_RATE] = (double)samples[i];
	chunk = 0;

	// we have 2s of captured data, run the fft
	fftw_execute(plan12);

	outcnt = (OUT_RATE*sampleduration) / 2 + 1;
	//printf("outcnt: %d \n",outcnt);

	// frequency steps in fft's output
	//double fstep = (double)OUT_RATE / (double)outcnt / 2.0;

	//printf("print output data \n");

	// save the FFT's result to a file
	//double freq = 0;
	int val;
	FILE *fline = fopen("/tmp/dline.out", "w");
	for (i = 0; i<outcnt; i++)
	{
		real = cpout12[i][0];
		imag = cpout12[i][1];
		mag = sqrt((real * real) + (imag * imag));

		//printf("%d:%.3f\n", i,mag);

		val = (int)(mag / 100);

		// make a marker in the FFTs data, if they are set to 10001 then the GUI may draw a separator line in the spectrum
		if (val == 10000) val = 10001;
		if (newfile == 1) val = 10000; // separator line

		fprintf(fline, "%c", (val >> 8) & 0xff);
		fprintf(fline, "%c", val & 0xff);
		//freq += fstep;
	}
	fclose(fline);
	newfile = 0;
}
