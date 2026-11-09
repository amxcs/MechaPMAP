ENTRY(window_start)
SECTIONS {
    . = 0x02001cc4;
    .text : { *(.text) *(.rodata*) }
    /DISCARD/ : { *(.ARM.exidx*) *(.ARM.attributes) *(.comment) }
}
