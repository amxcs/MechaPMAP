ENTRY(pmap_entry)
SECTIONS {
    . = 0x02002e74;
    .text : { *(.text) *(.rodata*) }
    /DISCARD/ : { *(.ARM.exidx*) *(.ARM.attributes) *(.comment) }
}
