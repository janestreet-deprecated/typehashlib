/***********************************************************************/
/*                                                                     */
/*                           Objective Caml                            */
/*                                                                     */
/*            Xavier Leroy, projet Cristal, INRIA Rocquencourt         */
/*                                                                     */
/*  Copyright 1996 Institut National de Recherche en Informatique et   */
/*  en Automatique.  All rights reserved.  This file is distributed    */
/*  under the terms of the GNU Library General Public License, with    */
/*  the special exception on linking described in file ../LICENSE.     */
/*                                                                     */
/***********************************************************************/

/* $Id: hash.c 9547 2010-01-22 12:48:24Z doligez $ */

/* The generic hashing primitive */

/* The interface of this file is in "mlvalues.h" */

#include "caml/mlvalues.h"
#include "caml/custom.h"
#include "caml/memory.h"

/* COPY PASTED FROM MEMORY.H */
#define Not_in_heap 0
#define In_heap 1
#define In_young 2
#define In_static_data 4
#define In_code_area 8

#ifdef ARCH_SIXTYFOUR

/* 64 bits: Represent page table as a sparse hash table */
int caml_page_table_lookup(void * addr);
#define Classify_addr(a) (caml_page_table_lookup((void *)(a)))

#else

/* 32 bits: Represent page table as a 2-level array */
#define Pagetable2_log 11
#define Pagetable2_size (1 << Pagetable2_log)
#define Pagetable1_log (Page_log + Pagetable2_log)
#define Pagetable1_size (1 << (32 - Pagetable1_log))
CAMLextern unsigned char * caml_page_table[Pagetable1_size];

#define Pagetable_index1(a) (((uintnat)(a)) >> Pagetable1_log)
#define Pagetable_index2(a) \
  ((((uintnat)(a)) >> Page_log) & (Pagetable2_size - 1))
#define Classify_addr(a) \
  caml_page_table[Pagetable_index1(a)][Pagetable_index2(a)]

#endif
#define Is_in_value_area(a) \
  (Classify_addr(a) & (In_heap | In_young | In_static_data))
#define Is_in_heap(a) (Classify_addr(a) & In_heap)
#define Is_in_heap_or_young(a) (Classify_addr(a) & (In_heap | In_young))
/* END COPY PASTED FROM MEMORY.H */


static uintnat hash_accu;
static intnat hash_univ_limit, hash_univ_count;

static void hash_aux(value obj);

CAMLprim value caml_hash_univ_param_3_12(value count, value limit, value obj)
{
  hash_univ_limit = Long_val(limit);
  hash_univ_count = Long_val(count);
  hash_accu = 0;
  hash_aux(obj);
  return Val_long(hash_accu & 0x3FFFFFFF);
  /* The & has two purposes: ensure that the return value is positive
     and give the same result on 32 bit and 64 bit architectures. */
}

#define Alpha 65599
#define Beta 19
#define Combine(new)  (hash_accu = hash_accu * Alpha + (new))
#define Combine_small(new) (hash_accu = hash_accu * Beta + (new))

static void hash_aux(value obj)
{
  unsigned char * p;
  mlsize_t i, j;
  tag_t tag;

  hash_univ_limit--;
  if (hash_univ_count < 0 || hash_univ_limit < 0) return;

 again:
  if (Is_long(obj)) {
    hash_univ_count--;
    Combine(Long_val(obj));
    return;
  }

  /* Pointers into the heap are well-structured blocks. So are atoms.
     We can inspect the block contents. */

  if (Is_in_value_area(obj)) {
    tag = Tag_val(obj);
    switch (tag) {
    case String_tag:
      hash_univ_count--;
      i = caml_string_length(obj);
      for (p = &Byte_u(obj, 0); i > 0; i--, p++)
        Combine_small(*p);
      break;
    case Double_tag:
      /* For doubles, we inspect their binary representation, LSB first.
         The results are consistent among all platforms with IEEE floats. */
      hash_univ_count--;
#ifdef ARCH_BIG_ENDIAN
      for (p = &Byte_u(obj, sizeof(double) - 1), i = sizeof(double);
           i > 0;
           p--, i--)
#else
      for (p = &Byte_u(obj, 0), i = sizeof(double);
           i > 0;
           p++, i--)
#endif
        Combine_small(*p);
      break;
    case Double_array_tag:
      hash_univ_count--;
      for (j = 0; j < Bosize_val(obj); j += sizeof(double)) {
#ifdef ARCH_BIG_ENDIAN
      for (p = &Byte_u(obj, j + sizeof(double) - 1), i = sizeof(double);
           i > 0;
           p--, i--)
#else
      for (p = &Byte_u(obj, j), i = sizeof(double);
           i > 0;
           p++, i--)
#endif
        Combine_small(*p);
      }
      break;
    case Abstract_tag:
      /* We don't know anything about the contents of the block.
         Better do nothing. */
      break;
    case Infix_tag:
      hash_aux(obj - Infix_offset_val(obj));
      break;
    case Forward_tag:
      obj = Forward_val (obj);
      goto again;
    case Object_tag:
      hash_univ_count--;
      Combine(Oid_val(obj));
      break;
    case Custom_tag:
      /* If no hashing function provided, do nothing */
      if (Custom_ops_val(obj)->hash != NULL) {
        hash_univ_count--;
        Combine(Custom_ops_val(obj)->hash(obj));
      }
      break;
    default:
      hash_univ_count--;
      Combine_small(tag);
      i = Wosize_val(obj);
      while (i != 0) {
        i--;
        hash_aux(Field(obj, i));
      }
      break;
    }
    return;
  }

  /* Otherwise, obj is a pointer outside the heap, to an object with
     a priori unknown structure. Use its physical address as hash key. */
  Combine((intnat) obj);
}
