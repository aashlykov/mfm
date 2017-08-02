all: *.c *.h
	reset;cc *.c -o mfm -I. -O2
