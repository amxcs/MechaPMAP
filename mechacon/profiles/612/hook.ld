ENTRY(pmap_entry)
SECTIONS {
    . = 0x02002f48;
    .text : { *(.text) *(.rodata*) }
    /DISCARD/ : { *(.ARM.exidx*) *(.ARM.attributes) *(.comment) }
}
