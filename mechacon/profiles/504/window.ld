ENTRY(window_start)
SECTIONS {
    . = 0x02001c54;
    .text : { *(.text) *(.rodata*) }
    /DISCARD/ : { *(.ARM.exidx*) *(.ARM.attributes) *(.comment) }
}
