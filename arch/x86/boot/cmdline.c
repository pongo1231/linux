// SPDX-License-Identifier: GPL-2.0-only
/* -*- linux-c -*- ------------------------------------------------------- *
 *
 *   Copyright (C) 1991, 1992 Linus Torvalds
 *   Copyright 2007 rPath, Inc. - All Rights Reserved
 *
 * ----------------------------------------------------------------------- */

/*
 * Simple command-line parser for early boot.
 */

#include "boot.h"
#include <asm/setup.h>
#include <asm/bootparam.h>

static inline int myisspace(u8 c)
{
	return c <= ' ';	/* Close enough approximation */
}

#ifdef _SETUP
typedef const char __seg_fs *cptr_t;
static inline cptr_t get_cptr(void)
{
	/*
	 * Note: there is no reason to check ext_cmd_line_ptr here,
	 * because it falls outside of boot_params.hdr and therefore
	 * will always be zero when entering through the real-mode
	 * entry point.
	 */
	unsigned long ptr = boot_params.hdr.cmd_line_ptr;

	/*
	 * The -16 here serves two purposes:
	 * 1. It means the segbase >= 0x100000 check also doubles as
	 *    a check for the command line pointer being zero.
	 * 2. It means this routine won't return a NULL pointer for
	 *    a valid address; it will always return a pointer in the
	 *    range 0x10-0x1f inclusive.
	 */
	unsigned long segbase = (ptr - 16) & ~15;
	if (segbase >= 0x100000)
		return NULL;

	set_fs(segbase >> 4);
	return (cptr_t)(ptr - segbase);
}
#else
unsigned long get_cmd_line_ptr(void)
{
	unsigned long ptr = boot_params_ptr->hdr.cmd_line_ptr;
	if (sizeof(unsigned long) > 4)
		ptr += (u64)boot_params_ptr->ext_cmd_line_ptr << 32;
	else if (boot_params_ptr->ext_cmd_line_ptr)
		return 0;	/* Inaccessible due to pointer overflow */

	return ptr;
}
typedef const char *cptr_t;
static inline cptr_t get_cptr(void)
{
	return (cptr_t)get_cmd_line_ptr();
}
#endif

/*
 * Find a non-boolean option, that is, "option=argument".  In accordance
 * with standard Linux practice, if this option is repeated, this returns
 * the last instance on the command line.
 *
 * Returns the length of the argument (regardless of if it was
 * truncated to fit in the buffer), or -1 on not found.
 */
int cmdline_find_option(const char *option, char *buffer, int bufsize)
{
	cptr_t cptr, eptr;
	char c;
	int len = -1;
	const char *opptr = NULL;
	char *bufptr = buffer;
	enum {
		st_wordstart,	/* Start of word/after whitespace */
		st_wordcmp,	/* Comparing this word */
		st_wordskip,	/* Miscompare, skip */
		st_bufcpy	/* Copying this to buffer */
	} state = st_wordstart;

	cptr = get_cptr();
	if (!cptr)
		return -1;	/* No command line or invalid pointer */
	eptr = cptr + COMMAND_LINE_SIZE - 1;

	while (cptr < eptr && (c = *cptr++)) {
		switch (state) {
		case st_wordstart:
			if (myisspace(c))
				break;

			/* else */
			state = st_wordcmp;
			opptr = option;
			fallthrough;

		case st_wordcmp:
			if (c == '=' && !*opptr) {
				len = 0;
				bufptr = buffer;
				state = st_bufcpy;
			} else if (myisspace(c)) {
				state = st_wordstart;
			} else if (c != *opptr++) {
				state = st_wordskip;
			}
			break;

		case st_wordskip:
			if (myisspace(c))
				state = st_wordstart;
			break;

		case st_bufcpy:
			if (myisspace(c)) {
				state = st_wordstart;
			} else {
				if (len < bufsize-1)
					*bufptr++ = c;
				len++;
			}
			break;
		}
	}

	if (bufsize)
		*bufptr = '\0';

	return len;
}

/*
 * Find a boolean option (like quiet,noapic,nosmp....)
 *
 * Returns the position of that option (starts counting with 1)
 * or 0 on not found
 */
int cmdline_find_option_bool(const char *option)
{
	cptr_t cptr, eptr;
	char c;
	int pos = 0, wstart = 0;
	const char *opptr = NULL;
	enum {
		st_wordstart,	/* Start of word/after whitespace */
		st_wordcmp,	/* Comparing this word */
		st_wordskip,	/* Miscompare, skip */
	} state = st_wordstart;

	cptr = get_cptr();
	if (!cptr)
		return -1;	/* No command line or invalid pointer */
	eptr = cptr + COMMAND_LINE_SIZE - 1;

	while (cptr <= eptr) {
		c = *cptr++;
		pos++;

		switch (state) {
		case st_wordstart:
			if (!c)
				return 0;
			else if (myisspace(c))
				break;

			state = st_wordcmp;
			opptr = option;
			wstart = pos;
			fallthrough;

		case st_wordcmp:
			if (!*opptr)
				if (!c || myisspace(c))
					return wstart;
				else
					state = st_wordskip;
			else if (!c)
				return 0;
			else if (c != *opptr++)
				state = st_wordskip;
			break;

		case st_wordskip:
			if (!c)
				return 0;
			else if (myisspace(c))
				state = st_wordstart;
			break;
		}
	}

	return 0;	/* Buffer overrun */
}
