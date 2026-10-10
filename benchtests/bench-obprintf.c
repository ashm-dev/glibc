/* Benchmark string output to an obstack.
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
#include <obstack.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <support/support.h>
#include "bench-timing.h"
#include "json-lib.h"

#define obstack_chunk_alloc malloc
#define obstack_chunk_free free

static void
bench (json_ctx_t *json, size_t length, bool reserve)
{
  char *input = xmalloc (length + 1);
  memset (input, 'x', length);
  input[length] = '\0';
  size_t iterations = 128 * 1024 * 1024 / (length + 1);
  if (iterations > 250000)
    iterations = 250000;

  struct obstack ob;
  if (reserve)
    {
      obstack_init (&ob);
      obstack_make_room (&ob, length);
    }

  timing_t start, stop, elapsed;
  TIMING_NOW (start);
  for (size_t i = 0; i < iterations; ++i)
    {
      if (!reserve)
        obstack_init (&ob);
      obstack_printf (&ob, "%s", input);
      obstack_free (&ob, reserve ? obstack_base (&ob) : NULL);
    }
  TIMING_NOW (stop);
  TIMING_DIFF (elapsed, start, stop);

  if (reserve)
    obstack_free (&ob, NULL);
  free (input);

  json_element_object_begin (json);
  json_attr_uint (json, "length", length);
  json_attr_string (json, "reserved", reserve ? "yes" : "no");
  json_attr_double (json, "timing", (double) elapsed / iterations);
  json_element_object_end (json);
}

static int
do_test (void)
{
  json_ctx_t json;
  json_init (&json, 0, stdout);
  json_document_begin (&json);
  json_attr_string (&json, "timing_type", TIMING_TYPE);
  json_attr_object_begin (&json, "functions");
  json_attr_object_begin (&json, "obstack_printf");
  json_attr_string (&json, "bench-variant", "string");
  json_array_begin (&json, "results");

  const size_t lengths[] = { 0, 16, 256, 4096, 65536, 1024 * 1024 };
  for (size_t i = 0; i < array_length (lengths); ++i)
    {
      bench (&json, lengths[i], false);
      bench (&json, lengths[i], true);
    }

  json_array_end (&json);
  json_attr_object_end (&json);
  json_attr_object_end (&json);
  json_document_end (&json);
  return 0;
}

#include <support/test-driver.c>
