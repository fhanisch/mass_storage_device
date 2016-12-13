all: flashdisk

flashdisk: flashdisk.c
	gcc -Wall -o $@ $+ -lusb-1.0
	
clean:
	rm flashdisk
