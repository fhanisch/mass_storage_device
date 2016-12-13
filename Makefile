EXEC=flashdisk

all: $(EXEC)

$(EXEC): flashdisk.c
	gcc -Wall -o $@ $+ -lusb-1.0
	
clean:
	rm $(EXEC)
