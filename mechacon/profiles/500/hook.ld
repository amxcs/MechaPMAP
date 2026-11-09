ENTRY(pmap_entry)
SECTIONS {
    . = 0x02002c50;
    .text : { *(.text) *(.rodata*) }
    /DISCARD/ : { *(.ARM.exidx*) *(.ARM.attributes) *(.comment) }
}
