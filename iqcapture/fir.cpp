 /*   (c) DJ0ABR
 *   www.dj0abr.de
 *   
FIR Filter, damit lassen sich alle möglichen Filter realisieren.
Um ein Filter zu bauen benötigt man Koeffizienten. Diese kann man zB mit dem Programm Iowa Hills Hilbert Filters (für Hilbert Trasformationen)
erstellen. Oder auch mit WinFilter.exe von winfilter.20m.com

Zunächt werden alle Samples aus einem wav File eingelesen,
dann mit einem Filter bearbeitet,
und schließlich wieder als wav File geschrieben.
Mit aplay kann man sich das Ergebnis anhören.
 */
 
#include "iqcapture.h"

// ================ HAMMING FILTER für +/- 45 Grad Phasenverschiebung ====================

float firplus(float NewSample, float *coefftab) 
{
    static float x[hamming48size]; //input samples
    float y=0;            //output sample
    int n;

    //shift the old samples
    for(n= hamming48size -1; n>0; n--)
       x[n] = x[n-1];

    //Calculate the new output
    x[0] = NewSample;
    for(n=0; n<hamming48size; n++)
        y += coefftab[n] * x[n];
    
    return y;
}

float firminus45(float NewSample) 
{
    static float x[hamming48size]; //input samples
    float y=0;            //output sample
    int n;

    //shift the old samples
    for(n= hamming48size -1; n>0; n--)
       x[n] = x[n-1];

    //Calculate the new output
    x[0] = NewSample;
    for(n=0; n<hamming48size; n++)
        y += FIRCoefm45[n] * x[n];
    
    return y;
}

// ========= 4kHz Filter ================

float fir_4k_48k_TP(float NewSample) 
{
	static float x[Ntap4k_48k]; //input samples
	float y = 0;            //output sample
	int n;

	//shift the old samples
	for (n = Ntap4k_48k - 1; n>0; n--)
		x[n] = x[n - 1];

	//Calculate the new output
	x[0] = NewSample;
	for (n = 0; n<Ntap4k_48k; n++)
		y += FIRCoef_4k_48k[n] * x[n];

	return y;
}