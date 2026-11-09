ENTRY(window_start)
SECTIONS {
    . = 0x02001d10;
    .text : { *(.text) *(.rodata*) }
    /DISCARD/ : { *(.ARM.exidx*) *(.ARM.attributes) *(.comment) }
}
