# SDR-IQ-capture
captures I and Q samples from a sound card, makes SSB demodulation and writes various results to disk. 

I was building an SDR Receiver to receive WSPR but did not find any "simple" IQ decoder which does just the SSB demoduation without GUI or other overhead. Most other SDR software has tons of graphics and other gimmics which I don't need. The program should be simple and shoud run also on slow computers like the Raspberry PI.
So I decided to write it by myself. Here it is, in the hope that it is useful for anybody playing with SDr receivers.

This program is written in C++ and can be compiled on any linux computer. 
It needs the fftw3 development libs as well as asound libs.
A windows version is planned for later time.

See iqcapture.cpp for a documentation of input parameters and output files.

Compilation:  make
the program is called:  iqcapture

Short description:
==================

1. capture stereo sound from a sound card. Left channel = I and right channel = Q signal. Samplerate=48000, Format S16_LE, channels=2 (stereo).
2. shift the I channel by -45 degrees using a Hamming FIR filter
3. shift the Q-channel by +45 degrees using a Hamming FIR filter
4. add the resulting signals which generates the USB signal
5. using a FFT shift down this USB signal from the ZF to the baseband (0 Hz). The ZF frequency can be specified by parameter and must be 20.000 Hz or less (since we sample with 48kHz).

the result of this operation is sent back to the output of the sound card.
Additionally the full spectrum from 0 - 23.500 Hz is written to a file which can be used for a waterfall display.
The original samples are also written to a file and can be used for a spectrum display.

For my application as a WSPR receiver the program does some more actions:

6. re-sample (decimate) the demodulated stream to 12000 samples/s which is required by K9AN's/K1JT's WSPR decoder. The result is written into a WAV file.


