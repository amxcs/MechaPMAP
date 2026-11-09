ENTRY(pmap_entry)
SECTIONS {
    . = 0x02002c90;
    .text : { *(.text) *(.rodata*) }
    /DISCARD/ : { *(.ARM.exidx*) *(.ARM.attributes) *(.comment) }
}
