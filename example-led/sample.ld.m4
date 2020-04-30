/*
 * ST32L4 memory setup.
 */
MEMORY
{
    flash  : org = 0x08000000, len = 256k
    ram : org = 0x20000000, len = 48k
}

__ram_start__           = ORIGIN(ram);
__ram_size__            = 20k;
__ram_end__             = __ram_start__ + __ram_size__;

SECTIONS
{
    . = 0;

    _text = .;

    .startup : ALIGN(128) SUBALIGN(128)
    {
        KEEP(*(.startup.vectors))
        . = ALIGN(16);
	_sys = .;
	. = ALIGN(16);
	KEEP(*(.sys.version))
	KEEP(*(.sys.board_id))
	KEEP(*(.sys.board_name))
	build/sys-*.o(.text)
	build/sys-*.o(.text.*)
	build/sys-*.o(.rodata)
	build/sys-*.o(.rodata.*)
	. = ALIGN(1024);
    } > flash =0xffffffff

    .text : ALIGN(16) SUBALIGN(16)
    {
        *(.text.startup.*)
        *(.text)
        *(.text.*)
        *(.rodata)
        *(.rodata.*)
        *(.glue_7t)
        *(.glue_7)
        *(.gcc*)
	. = ALIGN(8);
    } > flash

    .ARM.extab : {*(.ARM.extab* .gnu.linkonce.armextab.*)} > flash

    .ARM.exidx : {
        PROVIDE(__exidx_start = .);
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
        PROVIDE(__exidx_end = .);
     } > flash

    .eh_frame_hdr : {*(.eh_frame_hdr)} > flash

    .eh_frame : ONLY_IF_RO {*(.eh_frame)} > flash

    .textalign : ONLY_IF_RO { . = ALIGN(8); } > flash

    _etext = .;
    _textdata = _etext;

    .stacks (NOLOAD) :
    {
        *(.main_stack)
        *(.process_stack.0)
        *(.process_stack.1)
        *(.process_stack.2)
        *(.process_stack.3)
        . = ALIGN(8);
    } > ram

    .data :
    {
        . = ALIGN(4);
        PROVIDE(_data = .);
        *(.data)
        . = ALIGN(4);
        *(.data.*)
        . = ALIGN(4);
        *(.ramtext)
        . = ALIGN(4);
        PROVIDE(_edata = .);
    } > ram AT > flash

    .bss :
    {
        . = ALIGN(4);
        PROVIDE(_bss_start = .);
        *(.bss)
        . = ALIGN(4);
        *(.bss.*)
        . = ALIGN(4);
        *(COMMON)
        . = ALIGN(4);
        PROVIDE(_bss_end = .);
    } > ram

    PROVIDE(end = .);
    _end            = .;
}

__heap_base__   = _end;
__heap_end__    = __ram_end__;
