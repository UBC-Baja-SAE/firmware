MEMORY
{
    FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 64K
    RAM (rwx)  : ORIGIN = 0x20000000, LENGTH = 20K
}

SECTIONS
{
    .text : {
        *(.isr_vector)     /* Interrupt vector table */
        *(.text)           /* Program code */
        *(.rodata)         /* Read-only data */
        *(.ARM.exidx)      /* Exception index table */
        . = ALIGN(4);
    } > FLASH

    .data : {
        __data_start__ = .; /* Start of initialized data */
        *(.data)
        . = ALIGN(4);
        __data_end__ = .;
    } > RAM AT > FLASH
    __data_load__ = LOADADDR(.data); /* Load address in Flash */

    .bss : {
        __bss_start__ = .; /* Start of BSS section */
        *(.bss)
        *(COMMON)
        . = ALIGN(4);
        __bss_end__ = .; /* End of BSS section */
    } > RAM

    /* Define heap start and end */
    .heap (NOLOAD) : {
        __heap_start__ = .;
        . = ORIGIN(RAM) + LENGTH(RAM) - 4K; /* Reserve space for stack */
        __heap_end__ = .;
    } > RAM

    /* Define stack */
    .stack (NOLOAD) : {
        . = ALIGN(8);
        __stack_top__ = ORIGIN(RAM) + LENGTH(RAM); /* Stack starts at the end of RAM */
    } > RAM

    PROVIDE(end = __bss_end__); /* End of static memory allocation */
}
