/**
 * @file kernel.c
 */

#include "keyboard_map.h"

#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64
#define IDT_SIZE 256
#define PIC1_COMMAND 0x20
#define PIC1_DATA 0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA 0xA1

extern unsigned char keyboard_map[128];
volatile extern void keyboard_handler(void);
extern char read_port(unsigned short port);
extern void write_port(unsigned short port, unsigned char data);
extern void load_idt(unsigned long *idt_ptr);
// making them volatile as Keyboard I/O slower than CPU speed
volatile unsigned int current_loc = 0; // current cursor location
volatile char *vidptr = (char*)0xb8000; // video mem begins here

/* Interrupt Descriptor Table (IDT) */
struct IDT_entry {
    unsigned short int offset_lowerbits;
    unsigned short int selector;
    unsigned char zero;
    unsigned char type_attr;
    unsigned short int offset_higherbits;
} __attribute__((packed)); // changed by sid

struct IDT_entry IDT[IDT_SIZE];

void idt_init(void) {
    unsigned long keyboard_address;
    unsigned long idt_address;
    unsigned long idt_ptr[2];

    /* clear IDT */
    for (int i = 0; i < IDT_SIZE; i++) {
        IDT[i] = (struct IDT_entry){0};
    }
    /*                Ports - Programmable Interrupt Controller
    x86 systems have 2 PIC chips with 8 input lines. 
     *        PIC1 (IRQ0-7)  PIC2 (IRQ8-15)
     *Command 0x20           0xA0
     *Data    0x21           0xA1
    Initialized with 8 bit ICW
                           ICW1 = 0x11 
                           ICW2 = vector offset, added to input line #
                           ICW3 = Master / slave
                           ICW4 = Info on env.
    */

    keyboard_address = (unsigned long)keyboard_handler;
    IDT[0x21].offset_lowerbits = keyboard_address & 0xFFFF;
    IDT[0x21].selector = 0x08; // KERNEL_CODE_SEGMENT_OFFSET
    IDT[0x21].zero = 0;
    IDT[0x21].type_attr = 0x8E; // INTERRUPT_GATE
    IDT[0x21].offset_higherbits = keyboard_address >> 16;

    // ICW1
    write_port(PIC1_COMMAND, 0x11); // 0x11 = Initialize Cmd ICW1
    write_port(PIC2_COMMAND, 0x11);

    // ICW2: vector offsets - remap PICs beyond 0x20 (32 bits reserved)
    write_port(PIC1_DATA, 0x20);  // IRQ0–7 → 0x20–0x27
    write_port(PIC2_DATA, 0x28);  // IRQ8–15 → 0x28–0x2F

    // ICW3: wiring - setup cascading (slave on IRQ2)
    write_port(PIC1_DATA, 0x04);  // slave PIC at IRQ2
    write_port(PIC2_DATA, 0x02);

    // ICW4: env info - 8086 mode
    write_port(PIC1_DATA, 0x01);
    write_port(PIC2_DATA, 0x01);
    // write_port(PIC1_DATA, 0xFF);
    // write_port(PIC2_DATA, 0xFF);

    // fill IDT
    idt_address = (unsigned long)IDT;
    idt_ptr[0] = (sizeof(struct IDT_entry) * IDT_SIZE) | ((idt_address & 0xFFFF) << 16);
    idt_ptr[1] = idt_address >> 16;

    load_idt(idt_ptr);
}

void kb_init(void) {
    // 0xFD = 11111101 - enables only IRQ1 (keyboard);
    write_port(PIC1_DATA, 0xFD);
}

void clear_screen(void) {
    unsigned int j = 0;
    // loop clears the screen, 25 lines of 80 column, 2bytes for each element
    while (j < 80 * 25 * 2) {
        vidptr[j] = ' ';
        vidptr[j+1] = 0x07;
        j += 2;
    }
    j = 0;
}

void keyboard_handler_main(void) {
    unsigned char status;
    unsigned char keycode;

    status = read_port(KEYBOARD_STATUS_PORT);
    
    if (status & 0x01) { // lowest bit will be set if buffer not empty
        keycode = read_port(KEYBOARD_DATA_PORT);
        if (keycode < 0) { 
            write_port(PIC1_COMMAND, 0x20);
            return; 
        }
        vidptr[current_loc++] = keyboard_map[(unsigned char)keycode];
        vidptr[current_loc++] = 0x07; // color light grey on black
        if (current_loc >= 80*25*2) current_loc = 0; // prevent overflow
    }
    write_port(PIC1_COMMAND, 0x20); // signal EOI (End of Interrupt to PIC)
}

void kmain(void) {
    const char *str = "Custom Kernel";
    clear_screen();

    /* loop writes string to video memory */
    unsigned int i = 0, j = 0;
    while (str[j] != '\0') {
        vidptr[i] = str[j];
        vidptr[i+1] = 0x07;
        j++;
        i += 2;
    }
    current_loc = i;
    
    idt_init();
    kb_init();
    asm volatile("sti");  // safe point: all IDT entries loaded, PIC initialized
    while(1) {
    }

    return;
}