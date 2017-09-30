all: *.c *.h
	cc *.c -o mfm -I. -O2
