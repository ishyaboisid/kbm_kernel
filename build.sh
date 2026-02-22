nasm -f elf32 kernel.asm -o build/kasm.o
x86_64-elf-gcc -m32 -ffreestanding -fno-stack-protector -fno-pic -g -c kernel.c -o build/kc.o
x86_64-elf-ld -m elf_i386 -T linker.ld -o build/kernel.bin build/kasm.o build/kc.o
cp build/kernel.bin iso/boot/kernel.bin
i686-elf-grub-mkrescue -o kernel.iso iso
qemu-system-i386 -kernel iso/boot/kernel.bin -cpu core2duo -S -s 
exit 0