MEMORY
{
  FIRMWARE (rx) : ORIGIN = 0x00000000, LENGTH = 0x2B28C
  DATA     (rx) : ORIGIN = 0x04000000, LENGTH = 0x334
  SMALL    (rx) : ORIGIN = 0xC0001100, LENGTH = 0xD88
}

SECTIONS
{
  .firmware : {
    KEEP(*(.firmware))
  } > FIRMWARE

  .data : {
    KEEP(*(.data))
  } > DATA
  .small : {
    KEEP(*(.small))
  } > SMALL

  /DISCARD/ : { *(.comment) *(.note*) }
}
