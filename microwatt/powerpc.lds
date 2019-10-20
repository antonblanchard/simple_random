SECTIONS
{
	. = 0;
	_start = .;
	.head : {
		KEEP(*(.head))
	}

	.bss : {
		__bss_start = .;

		*(.dynbss)
			*(.bss .bss.* .gnu.linkonce.b.*)
			*(COMMON)
	}
	__bss_end = .;
}
