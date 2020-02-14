/* bfd initialization stuff
   Copyright (C) 1990-2020 Free Software Foundation, Inc.
   Written by Steve Chamberlain of Cygnus Support.

   This file is part of BFD, the Binary File Descriptor library.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston,
   MA 02110-1301, USA.  */

#include "sysdep.h"
#include "bfd.h"
#include "libbfd.h"

/* This controls if the coff 64k support shall be disabled or not.
   By default always enabled.  */
/* #if defined(__DJGPP__) || defined(COFF_GO32_EXE) || defined(COFF_GO32) */
bfd_boolean coff_64k_relocation_enabled = TRUE;
/* #endif */

/*
SECTION
	Initialization

SUBSECTION
	Initialization functions

	These are the functions that handle initializing a BFD.
*/

/*
FUNCTION
	bfd_init

SYNOPSIS
	unsigned int bfd_init (void);

DESCRIPTION
	This routine must be called before any other BFD function to
	initialize magical internal data structures.
	Returns a magic number, which may be used to check
	that the bfd library is configured as expected by users.
.
.{* Value returned by bfd_init.  *}
.
.#define BFD_INIT_MAGIC (sizeof (struct bfd_section))
*/

/* Actually, there is currently nothing for this function to do.
   However, someday it may be needed, so keep it around.  */

unsigned int
bfd_init (void)
{
#if defined(__DJGPP__) || defined(COFF_GO32_EXE) || defined(COFF_GO32)
  const char *disable_64k_reloc_support = getenv("DISABLE_64K_RELOC_SUPPORT");
  coff_64k_relocation_enabled = !(disable_64k_reloc_support &&
                                  (disable_64k_reloc_support[0] == 'y' || disable_64k_reloc_support[0] == 'Y') &&
                                  disable_64k_reloc_support[1] == '\0') ? TRUE : FALSE;
#endif
  return BFD_INIT_MAGIC;
}
