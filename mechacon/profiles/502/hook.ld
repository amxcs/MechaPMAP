ENTRY(pmap_entry)
SECTIONS {
    . = 0x02002c5c;
    .text : { *(.text) *(.rodata*) }
    /DISCARD/ : { *(.ARM.exidx*) *(.ARM.attributes) *(.comment) }
}
