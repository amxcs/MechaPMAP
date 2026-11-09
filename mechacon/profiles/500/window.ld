ENTRY(window_start)
SECTIONS {
    . = 0x02001c44;
    .text : { *(.text) *(.rodata*) }
    /DISCARD/ : { *(.ARM.exidx*) *(.ARM.attributes) *(.comment) }
}
