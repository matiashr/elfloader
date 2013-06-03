FLAGS=-nostdlib -pie -fPIE -mcpu=cortex-m4 -O0 -fomit-frame-pointer -falign-functions=16 -Wall -Wextra -Wstrict-prototypes -Wunused-parameter -DCORTEX_USE_FPU=FALSE -DNODE_CODE -DTHUMB_PRESENT -mno-thumb-interwork -DTHUMB_NO_INTERWORKING  -mthumb -DTHUMB -DARM 
CC=arm-none-eabi-gcc

all:
	@echo "APP"
	$(CC) $(FLAGS) test.c -o test
	@echo "info"
	arm-none-eabi-objcopy --input binary --output elf32-bigarm --binary-architecture arm test test.o
	cp test.o ..

clean:
	rm -f test test.o

