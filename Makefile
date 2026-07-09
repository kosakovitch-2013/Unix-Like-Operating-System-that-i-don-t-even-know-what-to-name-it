CCFLAGS = -m32 -fno-pie -fno-pic -Wunused -Wall -Wextra -ffreestanding
CCFLAGSC = -ffreestanding -m32 -fno-exceptions -fno-stack-protector -fno-align-functions -fno-pie -fno-pic -fno-unwind-tables -fno-asynchronous-unwind-tables -I include -nostdlib -Wall -Wextra -Wl,--build-id=none -Wunused

SRC_C := $(wildcard src/*.c)
SRC_S := $(wildcard src/*.s)
SRC_ASM := $(wildcard src/*.asm)
OBJECTS := $(patsubst src/%.c,build/%.o,$(wildcard src/*.c)) $(patsubst src/%.s,build/%.o,$(wildcard src/*.s)) $(patsubst src/%.asm,build/%.o,$(SRC_ASM))

all: build build/boot.iso

build:
	@# This is your Make build stub. This is a dependency of every target, so this always runs first.
	@mkdir build
	@mkdir -p build/arch/i386

build/%.o: src/%.c | build
	@echo "Compiling $<"
	@gcc -c $< -o $@ $(CCFLAGSC)

build/%.o: src/%.s | build
	@echo "Assembling $<"
	@gcc -c $< -o $@ $(CCFLAGS)

build/%.o: src/%.asm | build
	@echo "Assembling $<"
	@nasm -f elf32 $< -o $@

build/bootImage.elf: $(OBJECTS)
	@echo "Linking"
	@rm -f build/bootImage.elf
	@# cd build && gcc -T ../kernel.ld *.o -o bootImage.elf $(CCFLAGSC)
	@cd build/arch/i386 && gcc -T ../../../kernel.ld ../../../build/*.o -o bootImage.elf $(CCFLAGSC)
	@cp build/arch/i386/bootImage.elf iso

build/boot.iso: build/bootImage.elf
	@echo "Creating ISO"
	@grub-mkrescue -d /usr/lib/grub/i386-pc -o build/boot.iso iso

clean:
	@rm -rf build iso/bootImage.elf include/generated/*.h
