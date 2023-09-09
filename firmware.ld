ENTRY(HandlerReset)

_estack = 0x20004000;    /* end of 16K RAM */

_Min_Heap_Size = 0;      /* required amount of heap  */
_Min_Stack_Size = 0x80;  /* required amount of stack */

MEMORY
{
	FLASH (rx)      : ORIGIN = 0x00000000, LENGTH = 60K
	RAM (xrw)       : ORIGIN = 0x20000000, LENGTH = 16K
}

SECTIONS
{
	/* Program code */
	.text :
	{
		. = ALIGN(4);
		*(.text.isr)           /* .text sections of code  */
		*(.text)           /* .text sections of code  */
		*(.text*)          /* .text* sections of code */
		*(.rodata)         /* .rodata sections        */
		*(.rodata*)        /* .rodata* sections       */
		*(.glue_7)         /* Glue arm to thumb code  */
		*(.glue_7t)        /* Glue thumb to arm code  */
		*(.eh_frame)

		KEEP(*(.fini))
		. = ALIGN(4);
		_etext = .;        /* global symbols at end   */
	} >FLASH

	/* Used by startup code */

	. = ALIGN(4);

	flash_data_start = .;

	.data :
	{
		. = ALIGN(4);
		sram_data_start = .;
		*(.sramtext)
		*(.srambss)
		*(.data)           /* .data sections              */
		*(.data*)          /* .data* sections             */

		. = ALIGN(4);
		_edata = .;        /* Global symbol at data end   */
	} >RAM AT> FLASH

	sram_data_end = .;

	/* Uninitialized data */
	. = ALIGN(4);
	.bss :
	{
		_sbss = .;         /* Global symbol at bss start */
		__bss_start__ = _sbss;
		*(.bss)
		*(.bss*)
		*(COMMON)

		. = ALIGN(4);
		_ebss = .;         /* Global symbol at bss end */
		__bss_end__ = _ebss;
	} >RAM

	/* Check that there is enough RAM */
	._user_heap_stack :
	{
		. = ALIGN(4);
		. = . + _Min_Heap_Size;
		. = . + _Min_Stack_Size;
		. = ALIGN(4);
	} >RAM
}

