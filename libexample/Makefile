FLAGS=-nostdlib -fPIC -pie -g -mcpu=cortex-m4 -O0 -fomit-frame-pointer -falign-functions=16 -Wall -Wextra -Wstrict-prototypes -Wunused-parameter -DCORTEX_USE_FPU=FALSE -DNODE_CODE -DTHUMB_PRESENT -mno-thumb-interwork -DTHUMB_NO_INTERWORKING  -mthumb -DTHUMB -DARM 
CC=arm-none-eabi-gcc

all:
	@echo "LIB"
	$(CC) $(FLAGS) -shared lib.c -o libapp.so
	@echo "APP"
	$(CC) $(FLAGS) -c test.c -o test.c.o
	$(CC) $(FLAGS) -L. -lapp test.c.o -o test
#	$(CC) $(FLAGS) -Wl,-Tlink.ld -L. -lapp test.c.o -o test
	@echo "info"
	arm-none-eabi-objcopy --input binary --output elf32-bigarm --binary-architecture arm test test.o
	cp test.o ..

clean:
	rm -f test libapp.so test.o test.c.o


