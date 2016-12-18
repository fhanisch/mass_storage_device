SL = libmsd.so
EXEC = flashdisk

all: $(SL) $(EXEC) install

$(SL): flashdisk.c
	gcc -shared -Wall -o $@ $+ -lusb-1.0 -D SHL

$(EXEC): flashdisk.c
	gcc -Wall -o $@ $+ -lusb-1.0
	
install:
	sudo cp msd.h /usr/include
	sudo cp $(SL) /usr/lib
	sudo cp $(EXEC) /usr/local/bin
	
clean:
	rm $(SL) $(EXEC)
