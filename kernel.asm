;; @file kernel.asm
;; starting point for kernel, invoke extern function in C --> halt prg flow

bits 32                         ; nasm directive - 32 bit

section .text                   ; first 8 bytes for GRUB with multiboot spec
    align 4
    dd 0x1BADB002               ; magic ; dd defines double word of 4 bytes
    dd 0x00                     ; flags
    dd -(0x1BADB002 + 0x00)     ; checksum, m+f+c should be zero

global _start
global keyboard_handler
global read_port
global write_port
global load_idt

;; defined in C
extern kmain
extern keyboard_handler_main

read_port:
    mov edx, [esp + 4]         ; eps = stack ptr, edx = read out register
    in al, dx                  ; al is lower 8 bits of eax ; dx is lower 16 bits of edx
    ret 

write_port:
    mov edx, [esp + 4]
    mov al, [esp + 8]
    out dx, al
    ret

load_idt:
    mov edx, [esp + 4]
    lidt [edx]
    ret

keyboard_handler: ; changed by sid
    call keyboard_handler_main
    iretd                       ; interrupt return

_start:
    cli                         ; block interrupts
    mov esp, stack_space + 8192 ; changed by sid
    call kmain
    hlt                         ; halt CPU

section .bss
stack_space: 
    resb 8192 ; 8kb for stack ; grows downward ;