ENTRY(window_start)
SECTIONS {
    . = 0x02001d14;
    .text : { *(.text) *(.rodata*) }
    /DISCARD/ : { *(.ARM.exidx*) *(.ARM.attributes) *(.comment) }
}
