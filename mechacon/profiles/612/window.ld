ENTRY(window_start)
SECTIONS {
    . = 0x02001de8;
    .text : { *(.text) *(.rodata*) }
    /DISCARD/ : { *(.ARM.exidx*) *(.ARM.attributes) *(.comment) }
}
