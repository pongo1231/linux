/*
 * setup.ld
 *
 * Linker script for the i386 setup code
 */
OUTPUT_FORMAT("elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(_start)

SECTIONS
{
	. = 0;
	.header		: { *(.header) }
	.entrytext	: { *(.entrytext) }	= 0xcccccccc
	.inittext	: { *(.inittext) }	= 0xcccccccc
	.initdata	: { *(.initdata) }
	__end_init = .;

	.text		: { *(.text .text.*) }	= 0xcccccccc
	.text32		: { *(.text32) }	= 0xcccccccc

	.pecompat	: { *(.pecompat) }
	PROVIDE(pecompat_fsize = setup_size - pecompat_fstart);

	. = ALIGN(16);
	.rodata		: { *(.rodata*) }

	.videocards	: {
		video_cards = .;
		*(.videocards)
		video_cards_end = .;
	}

	. = ALIGN(16);
	.data		: { *(.data*) }

	.signature	: {
		*(.signature)
		setup_size = ALIGN(ABSOLUTE(.), 4096);
		setup_sects = ABSOLUTE(setup_size / 512);
		ASSERT(setup_sects >= 5, "The setup must be at least 5 sectors in size");
		ASSERT(setup_sects <= 64, "The setup must be at most 64 sectors in size");
	}

	. = ALIGN(16);
	.bss		:
	{
		__bss_start = .;
		*(.bss)
		__bss_end = .;
	}
	. = ALIGN(16);
	_end = .;

	/DISCARD/	: {
		*(.note*)
	}

	/*
	 * The ASSERT() sink to . is intentional. A bare ASSERT()
	 * outside of an output section is believed to have been broken
	 * in some binutils versions, although it is supposed to have
	 * been supported since binutils 2.15. Either way, it doesn't hurt,
	 * so there is no reason to drop it.
	 */
	. = ASSERT(_end <= 0x8000, "Setup too big!");
	. = ASSERT(hdr == 0x1f1, "The setup header has the wrong offset!");
	. = ASSERT(end_of_bzheader <= 512+2+127, "bzImage header overflow!");
	. = ASSERT(end_of_bzheader == start_of_setup, "padding bytes between .bzheader and .entrytext!");
	/* Necessary for the very-old-loader check to work... */
	. = ASSERT(__end_init <= 5*512, "init sections too big!");
}
