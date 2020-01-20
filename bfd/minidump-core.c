/* BFD back-end for Windows minidump (.dmp) files.
   Copyright (C) 2020 Free Software Foundation, Inc.

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

#define __STDC_FORMAT_MACROS
#include "sysdep.h"
#include "bfd.h"
#include "libbfd.h"
#include "minidump-format/minidump_format.h"

/* These are stored in the bfd's tdata */

/* .lwpid and .user_tid are only valid if PROC_INFO_HAS_THREAD_ID, else they
   are set to 0.  Also, until HP-UX implements MxN threads, .user_tid and
   .lwpid are synonymous. */
struct minidump_core_struct
{
  int sig;
  int lwpid;		   /* Kernel thread ID. */
  unsigned long user_tid;  /* User thread ID. */
};

#define core_hdr(bfd) ((bfd)->tdata.minidump_core_data)
#define core_signal(bfd) (core_hdr(bfd)->sig)
#define core_kernel_thread_id(bfd) (core_hdr(bfd)->lwpid)
#define core_user_thread_id(bfd) (core_hdr(bfd)->user_tid)
#define minidump_core_core_file_matches_executable_p generic_core_file_matches_executable_p
#define minidump_core_core_file_pid _bfd_nocore_core_file_pid

static const bfd_target *minidump_core_core_file_p (bfd *);
static char *minidump_core_core_file_failing_command (bfd *);
static int minidump_core_core_file_failing_signal (bfd *);
static void swap_abort (void);

static asection *
make_bfd_asection (bfd *abfd, const char *name, flagword flags,
		   MDLocationDescriptor loc, bfd_vma vma,
		   unsigned int alignment_power)
{
  asection *asect;
  char *newname;

  newname = bfd_alloc (abfd, (bfd_size_type) strlen (name) + 1);
  if (!newname)
    return NULL;

  strcpy (newname, name);

  asect = bfd_make_section_anyway_with_flags (abfd, newname, flags);
  if (!asect)
    return NULL;

  asect->size = bfd_getl32 (&loc.data_size);
  asect->vma = vma;
  asect->filepos = bfd_getl32 (&loc.rva);
  asect->alignment_power = alignment_power;
  fprintf (stderr, "Making section at %lx len %lx\n", asect->filepos, asect->size);

  return asect;
}

/* Return true if the given core file section corresponds to a thread,
   based on its name.  */

static int
thread_section_p (bfd *abfd ATTRIBUTE_UNUSED,
		  asection *sect,
		  void *obj ATTRIBUTE_UNUSED)
{
  return CONST_STRNEQ (sect->name, ".reg/");
}

static void
handle_memory_list (bfd *abfd, MDLocationDescriptor location)
{
  MDRawMemoryList list;
  unsigned num_ranges;
  unsigned cur_range;

  if (bfd_seek (abfd, location.rva, SEEK_SET) != 0)
    return;
  if (bfd_bread (&list, (bfd_size_type) 4, abfd) != 4) /* FIXME - magic number */
    return;
  num_ranges = bfd_getl32 (&list.number_of_memory_ranges);
  for (cur_range = 0; cur_range < num_ranges; ++cur_range)
    {
      MDMemoryDescriptor desc;
      bfd_vma base_addr;
      if (bfd_bread (&desc, (bfd_size_type) sizeof desc, abfd) != sizeof desc)
	return;

      base_addr = bfd_getl64 (&desc.start_of_memory_range);
      if (!make_bfd_asection (abfd, ".data",
			      SEC_ALLOC + SEC_LOAD + SEC_HAS_CONTENTS,
			      desc.memory, base_addr, 4))
	return; /* TODO: report error */
    }
}

static const bfd_target *
minidump_core_core_file_p (bfd *abfd)
{
  MDRawHeader header;
  MDRawDirectory dir;
  unsigned stream_count;
  unsigned stream_directory_pos;
  if (bfd_bread (&header, (bfd_size_type) sizeof header, abfd) != sizeof header)
    return NULL;
  if (bfd_getl32 (&header.signature) != 0x504d444d)
    return NULL;
  stream_count = bfd_getl32 (&header.stream_count);
  if (stream_count == 0)
    return NULL;
  fprintf(stderr, "Valid DMP file!\n");

  stream_directory_pos = bfd_getl32 (&header.stream_directory_rva);

  for (unsigned i = 0; i < stream_count; ++i)
    {
      unsigned stream_type;
      if (bfd_seek (abfd, stream_directory_pos + i * sizeof dir, SEEK_SET) != 0)
	return NULL;
      if (bfd_bread (&dir, (bfd_size_type) sizeof dir, abfd) != sizeof dir)
	return NULL;
      stream_type = bfd_getl32 (&dir.stream_type);
      fprintf (stderr, "Found dir %x\n", stream_type);
      switch (stream_type)
	{
	  case MD_THREAD_LIST_STREAM:
	    break;
	  case MD_MODULE_LIST_STREAM:
	    break;
	  case MD_MEMORY_LIST_STREAM:
	    handle_memory_list (abfd, dir.location);
	    break;
	  case MD_EXCEPTION_STREAM:
	    break;
	  case MD_SYSTEM_INFO_STREAM:
	    break;
	}
    }

  core_hdr (abfd) = (struct minidump_core_struct *)
    bfd_zalloc (abfd, (bfd_size_type) sizeof (struct minidump_core_struct));
  if (!core_hdr (abfd))
    return NULL;

  return abfd->xvec;
}

static char *
minidump_core_core_file_failing_command (bfd *abfd)
{
  return "placeholder";
}

static int
minidump_core_core_file_failing_signal (bfd *abfd)
{
  return core_signal (abfd);
}


/* If somebody calls any byte-swapping routines, shoot them.  */
static void
swap_abort (void)
{
  abort(); /* This way doesn't require any declaration for ANSI to fuck up */
}

#define	NO_GET ((bfd_vma (*) (const void *)) swap_abort)
#define	NO_PUT ((void (*) (bfd_vma, void *)) swap_abort)
#define	NO_GETS ((bfd_signed_vma (*) (const void *)) swap_abort)
#define	NO_GET64 ((bfd_uint64_t (*) (const void *)) swap_abort)
#define	NO_PUT64 ((void (*) (bfd_uint64_t, void *)) swap_abort)
#define	NO_GETS64 ((bfd_int64_t (*) (const void *)) swap_abort)

const bfd_target core_minidump_vec =
  {
    "minidump-core",
    bfd_target_unknown_flavour,
    BFD_ENDIAN_LITTLE,		/* target byte order */
    BFD_ENDIAN_LITTLE,		/* target headers byte order */
    (HAS_RELOC | EXEC_P |	/* object flags */
     HAS_LINENO | HAS_DEBUG |
     HAS_SYMS | HAS_LOCALS | WP_TEXT | D_PAGED),
    (SEC_HAS_CONTENTS | SEC_ALLOC | SEC_LOAD | SEC_RELOC), /* section flags */
    0,				/* symbol prefix */
    ' ',			/* ar_pad_char */
    16,				/* ar_max_namelen */
    0,				/* match priority.  */
    NO_GET64, NO_GETS64, NO_PUT64,	/* 64 bit data */
    NO_GET, NO_GETS, NO_PUT,		/* 32 bit data */
    NO_GET, NO_GETS, NO_PUT,		/* 16 bit data */
    NO_GET64, NO_GETS64, NO_PUT64,	/* 64 bit hdrs */
    NO_GET, NO_GETS, NO_PUT,		/* 32 bit hdrs */
    NO_GET, NO_GETS, NO_PUT,		/* 16 bit hdrs */

    {				/* bfd_check_format */
      _bfd_dummy_target,		/* unknown format */
      _bfd_dummy_target,		/* object file */
      _bfd_dummy_target,		/* archive */
      minidump_core_core_file_p		/* a core file */
    },
    {				/* bfd_set_format */
      _bfd_bool_bfd_false_error,
      _bfd_bool_bfd_false_error,
      _bfd_bool_bfd_false_error,
      _bfd_bool_bfd_false_error
    },
    {				/* bfd_write_contents */
      _bfd_bool_bfd_false_error,
      _bfd_bool_bfd_false_error,
      _bfd_bool_bfd_false_error,
      _bfd_bool_bfd_false_error
    },

    BFD_JUMP_TABLE_GENERIC (_bfd_generic),
    BFD_JUMP_TABLE_COPY (_bfd_generic),
    BFD_JUMP_TABLE_CORE (minidump_core),
    BFD_JUMP_TABLE_ARCHIVE (_bfd_noarchive),
    BFD_JUMP_TABLE_SYMBOLS (_bfd_nosymbols),
    BFD_JUMP_TABLE_RELOCS (_bfd_norelocs),
    BFD_JUMP_TABLE_WRITE (_bfd_generic),
    BFD_JUMP_TABLE_LINK (_bfd_nolink),
    BFD_JUMP_TABLE_DYNAMIC (_bfd_nodynamic),

    NULL,

    NULL			/* backend_data */
  };

