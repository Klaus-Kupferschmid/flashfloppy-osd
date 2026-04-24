/* Linker script for FlashFloppy OSD with custom HID Bootloader
 * HID bootloader uses 4KB at 0x08000000 (I2C2 + Timeout support)
 * Application starts at 0x08001000
 */
ENTRY(vector_table)

MEMORY
{
  /* HID bootloader occupies first 4KB */
  FLASH (rx)      : ORIGIN = 0x08001000, LENGTH = 124K
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
