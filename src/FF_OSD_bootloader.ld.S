/*
 * FF_OSD_bootloader.ld.S
 *
 * Linker script for FlashFloppy-OSD when built with bootloader support.
 * Application starts at 0x08002000 (after 8KB bootloader).
 *
 * Memory map:
 *   0x08000000 - 0x08001FFF: Bootloader (8KB)
 *   0x08002000 - 0x0800FFFF: Application (56KB for C8T6)
 *   0x08002000 - 0x0801DFFF: Application (120KB for CBT6)
 *   0x0801F800 - 0x0801FFFF: Flash storage (2KB, CBT6 only)
 *
 * Build with: make BOOTLOADER=y
 */

ENTRY(vector_table)

MEMORY
{
  /* Application starts after 8KB bootloader */
  FLASH (rx)      : ORIGIN = 0x08002000, LENGTH = 56K
  RAM (rwx)       : ORIGIN = 0x20000000, LENGTH = 20K
}
REGION_ALIAS("RO", FLASH);
REGION_ALIAS("RW", RAM);

SECTIONS
{
  .text : {
    _stext = .;
    *(.vector_table)
    *(.text)
    *(.text*)
    *(.rodata)
    *(.rodata*)
    KEEP (*(.init))
    KEEP (*(.fini))
    . = ALIGN(4);
    _etext = .;
  } >RO

  .data : AT (_etext) {
    . = ALIGN(4);
    _sdat = .;
    *(.data)
    *(.data*)
    . = ALIGN(4);
    _edat = .;
    _ldat = LOADADDR(.data);
  } >RW

  .bss : {
    . = ALIGN(8);
    _irq_stackbottom = .;
    . = . + 512;
    _irq_stacktop = .;
    _thread_stackbottom = .;
    . = . + 512;
    _thread_stacktop = .;
    _sbss = .;
    *(.bss)
    *(.bss*)
    . = ALIGN(4);
    _ebss = .;
  } >RW

  /DISCARD/ : {
    *(.eh_frame)
  }

  .ARM.attributes 0 : { *(.ARM.attributes) }
}
