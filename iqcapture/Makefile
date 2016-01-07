# makefile for IQ SSB Demodulator

CXXFLAGS = -Wall -O3 -Wno-write-strings -Wno-narrowing
LDFLAGS = -lsndfile -lm -lfftw3 -lasound
OBJ = iqcapture.o sound.o fft.o fir.o coeff.o meanvalue.o fft12.o

default: $(OBJ)
	g++ $(CXXFLAGS) -o iqcapture $(OBJ) $(LDFLAGS)

clean:
	rm -f *.o iqcapture

