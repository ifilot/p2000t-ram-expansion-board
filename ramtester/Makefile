main.bin main.map main.rom: main.c util.c memory.c stack.asm ramtest.asm ramtest.h terminal.c
	zcc \
	+embedded -clib=sdcc_iy \
	main.c \
	util.c \
	memory.c \
	stack.asm \
	ramtest.asm \
	terminal.c \
	bankcounting.c \
	stack.c \
	-startup=1 \
	-pragma-define:CRT_ORG_CODE=0x1000 \
	-pragma-define:CRT_ORG_DATA=0x6100 \
	-pragma-define:REGISTER_SP=0x9FFF \
	-pragma-define:CRT_STACK_SIZE=256 \
	-pragma-define:CRT_INCLUDE_PREAMBLE=1 \
	-pragma-define:CLIB_FOPEN_MAX=0 \
	--max-allocs-per-node2000 \
	-SO3 -bn RAMTEST.BIN \
	-create-app -m
