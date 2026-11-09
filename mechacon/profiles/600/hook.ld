ENTRY(pmap_entry)
SECTIONS {
    . = 0x02002e4c;
    .text : { *(.text) *(.rodata*) }
    /DISCARD/ : { *(.ARM.exidx*) *(.ARM.attributes) *(.comment) }
}
