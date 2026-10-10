/* Test large formatted output to an obstack.
   Copyright (C) 2026 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <https://www.gnu.org/licenses/>.  */

#include <array_length.h>
#include <errno.h>
#include <obstack.h>
#include <printf.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <support/check.h>
#include <support/support.h>

static void *
allocate_chunk (void *arg, long size)
{
  ++*(int *) arg;
  return malloc (size);
}

static void
free_chunk (void *arg, void *chunk)
{
  free (chunk);
}

static int
print_v (struct obstack *ob, const char *format, ...)
{
  va_list ap;
  va_start (ap, format);
  int result = obstack_vprintf (ob, format, ap);
  va_end (ap);
  return result;
}

static void
check_boundary (int (*print) (struct obstack *, const char *, ...),
                const char *input, int length, int room)
{
  int allocations = 0;
  struct obstack ob;
  obstack_specify_allocation_with_arg (&ob, 256, 0,
                                       allocate_chunk, free_chunk,
                                       &allocations);
  int prefix = obstack_room (&ob) - room;
  TEST_VERIFY_EXIT (prefix > 0);
  char *expected = xmalloc (prefix + length + 3);
  memset (expected, '#', prefix);
  expected[prefix] = 'a';
  memcpy (expected + prefix + 1, input, length);
  expected[prefix + length + 1] = '\0';
  expected[prefix + length + 2] = '!';
  obstack_grow (&ob, expected, prefix);

  int written = -1;
  TEST_COMPARE (print (&ob, "%c%.*s%c%n", 'a', length, input, '\0', &written),
                length + 2);
  TEST_COMPARE (written, length + 2);
  if (length >= 4096)
    TEST_COMPARE (allocations, 2);
  TEST_COMPARE (print (&ob, "%s", ""), 0);
  TEST_COMPARE (print (&ob, "%c", '!'), 1);
  TEST_COMPARE_BLOB (obstack_base (&ob), obstack_object_size (&ob),
                     expected, prefix + length + 3);

  obstack_free (&ob, NULL);
  free (expected);
}

static void
check_error (int (*print) (struct obstack *, const char *, ...))
{
  int allocations = 0;
  struct obstack ob;
  obstack_specify_allocation_with_arg (&ob, 256, 0,
                                       allocate_chunk, free_chunk,
                                       &allocations);
  int prefix = obstack_room (&ob);
  obstack_blank (&ob, prefix);
  memset (obstack_base (&ob), '#', prefix);

  /* Leave a character in the one-byte buffer on conversion failure.  */
  errno = 0;
  TEST_COMPARE (print (&ob, "%c%lc", 'a', (wint_t) 0x100), -1);
  TEST_COMPARE (errno, EILSEQ);
  TEST_COMPARE (obstack_object_size (&ob), prefix + 1);
  TEST_COMPARE (((char *) obstack_base (&ob))[prefix], 'a');
  TEST_COMPARE (print (&ob, "%s", "bc"), 2);
  TEST_COMPARE_BLOB ((char *) obstack_base (&ob) + prefix, 3, "abc", 3);
  obstack_free (&ob, NULL);
}

static int
print_string (FILE *stream, const struct printf_info *info,
              const void *const *args)
{
  const char *s = *(const char *const *) args[0];
  if (info->alt)
    return fprintf (stream, "%s", s);
  size_t length = strlen (s);
  if (fwrite (s, 1, length, stream) != length)
    return -1;
  return length;
}

static int
string_arginfo (const struct printf_info *info, size_t n,
                int *types, int *sizes)
{
  if (n > 0)
    types[0] = PA_STRING;
  return 1;
}

static void
check_custom (const char *input, int length)
{
  TEST_COMPARE (register_printf_specifier ('Q', print_string, string_arginfo),
                0);
  int allocations = 0;
  struct obstack ob;
  obstack_specify_allocation_with_arg (&ob, 256, 0,
                                       allocate_chunk, free_chunk,
                                       &allocations);
  int prefix = obstack_room (&ob);
  obstack_blank (&ob, prefix);
  memset (obstack_base (&ob), '#', prefix);
  int written = -1;
  TEST_COMPARE (print_v (&ob, "%c%Q%n", 'a', input, &written), length + 1);
  TEST_COMPARE (written, length + 1);
  TEST_COMPARE (allocations, 2);
  TEST_COMPARE (obstack_object_size (&ob), prefix + length + 1);
  TEST_COMPARE (((char *) obstack_base (&ob))[prefix], 'a');
  TEST_COMPARE_BLOB ((char *) obstack_base (&ob) + prefix + 1, length,
                     input, length);

  TEST_COMPARE (print_v (&ob, "%#Q%n", input, &written), length);
  TEST_COMPARE (written, length);
  TEST_COMPARE (obstack_object_size (&ob), prefix + 2 * length + 1);
  TEST_COMPARE_BLOB ((char *) obstack_base (&ob) + prefix + length + 1, length,
                     input, length);
  obstack_free (&ob, NULL);
}

static int
do_test (void)
{
  enum { length = 1024 * 1024 };
  char *input = xmalloc (length + 1);
  for (int i = 0; i < length; ++i)
    input[i] = 'a' + i % 26;
  input[length] = '\0';

  int allocations = 0;
  struct obstack ob;
  obstack_specify_allocation_with_arg (&ob, 128, 0,
                                       allocate_chunk, free_chunk,
                                       &allocations);

  TEST_COMPARE (obstack_printf (&ob, "pre"), 3);
  TEST_COMPARE (obstack_printf (&ob, "%s", input), length);
  TEST_COMPARE (allocations, 2);
  TEST_COMPARE (obstack_printf (&ob, "%c", '!'), 1);
  TEST_COMPARE (obstack_object_size (&ob), length + 4);
  TEST_COMPARE_BLOB (obstack_base (&ob), 3, "pre", 3);
  TEST_COMPARE_BLOB ((char *) obstack_base (&ob) + 3, length, input, length);
  TEST_COMPARE (*((char *) obstack_base (&ob) + length + 3), '!');

  obstack_free (&ob, NULL);

  int (*const printers[]) (struct obstack *, const char *, ...)
    = { obstack_printf, print_v };
  const int lengths[] = { 0, 1, 2, 15, 16, 255, 4096, length };
  const int rooms[] = { 0, 1, 2, 16 };
  for (size_t i = 0; i < array_length (printers); ++i)
    {
      for (size_t j = 0; j < array_length (lengths); ++j)
        for (size_t k = 0; k < array_length (rooms); ++k)
          check_boundary (printers[i], input, lengths[j], rooms[k]);
      check_error (printers[i]);
    }
  check_custom (input, length);

  free (input);
  return 0;
}

#include <support/test-driver.c>
