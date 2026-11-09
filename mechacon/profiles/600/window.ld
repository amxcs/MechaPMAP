ENTRY(window_start)
SECTIONS {
    . = 0x02001cbc;
    .text : { *(.text) *(.rodata*) }
    /DISCARD/ : { *(.ARM.exidx*) *(.ARM.attributes) *(.comment) }
}
