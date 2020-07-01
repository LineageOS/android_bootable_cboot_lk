/*
 * Copyright (c) 2008-2009,2012 Travis Geiselbrecht
 * Copyright (c) 2009 Corey Tabaka
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
/*
 * Copyright (c) 2014, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited
 */

#include <debug.h>
#include <trace.h>
#include <assert.h>
#include <err.h>
#include <list.h>
#include <rand.h>
#include <stdio.h>
#include <string.h>
#include <kernel/thread.h>
#include <kernel/mutex.h>
#include <lib/heap.h>
#include <malloc.h>

#define LOCAL_TRACE 0

#define DEBUG_HEAP 0
#define ALLOC_FILL 0x99
#define FREE_FILL 0x77
#define PADDING_FILL 0x55
#define PADDING_SIZE 64

#define ROUNDUP(a, b) (((a) + ((b)-1)) & ~((b)-1))

#define HEAP_MAGIC 'HEAP'

#if WITH_STATIC_HEAP

#if !defined(HEAP_START) || !defined(HEAP_LEN)
#error WITH_STATIC_HEAP set but no HEAP_START or HEAP_LEN defined
#endif

#else
// end of the binary
extern int _end;

// end of memory
extern void *_heap_end;

#undef HEAP_START
#undef HEAP_LEN
#define HEAP_START ((uintptr_t)&_end)
#define HEAP_LEN ((uintptr_t)_heap_end - (uintptr_t)&_end)
#endif

struct free_heap_chunk {
	struct list_node node;
	size_t len;
};

struct heap {
	void *base;
	size_t len;
	size_t remaining;
	size_t low_watermark;
	mutex_t lock;
	struct list_node free_list;
	struct list_node delayed_free_list;
};

// heap static vars
static struct heap theheap;

// structure placed at the beginning every allocation
struct alloc_struct_begin {
#if LK_DEBUGLEVEL > 1
	unsigned int magic;
#endif
	void *ptr;
	size_t size;
#if DEBUG_HEAP
	void *padding_start;
	size_t padding_size;
#endif
};

static void dump_free_chunk(struct free_heap_chunk *chunk)
{
	dprintf(INFO, "\t\tbase %p, end 0x%lx, len 0x%zx\n", chunk, (vaddr_t)chunk + chunk->len, chunk->len);
}

static void heap_dump(void)
{
	dprintf(INFO, "Heap dump:\n");
	dprintf(INFO, "\tbase %p, len 0x%zx\n", theheap.base, theheap.len);
	dprintf(INFO, "\tfree list:\n");

	mutex_acquire(&theheap.lock);

	struct free_heap_chunk *chunk;
	list_for_every_entry(&theheap.free_list, chunk, struct free_heap_chunk, node) {
		dump_free_chunk(chunk);
	}

	dprintf(INFO, "\tdelayed free list:\n");
	list_for_every_entry(&theheap.delayed_free_list, chunk, struct free_heap_chunk, node) {
		dump_free_chunk(chunk);
	}
	mutex_release(&theheap.lock);
}

static void heap_test(void)
{
	void *ptr[16];

	ptr[0] = heap_alloc(8, 0);
	ptr[1] = heap_alloc(32, 0);
	ptr[2] = heap_alloc(7, 0);
	ptr[3] = heap_alloc(0, 0);
	ptr[4] = heap_alloc(98713, 0);
	ptr[5] = heap_alloc(16, 0);

	heap_free(ptr[5]);
	heap_free(ptr[1]);
	heap_free(ptr[3]);
	heap_free(ptr[0]);
	heap_free(ptr[4]);
	heap_free(ptr[2]);

	heap_dump();

	int i;
	for (i=0; i < 16; i++)
		ptr[i] = 0;

	for (i=0; i < 32768; i++) {
		unsigned int index = (unsigned int)rand() % 16;

		if ((i % (16*1024)) == 0)
			printf("pass %d\n", i);

//		printf("index 0x%x\n", index);
		if (ptr[index]) {
//			printf("freeing ptr[0x%x] = %p\n", index, ptr[index]);
			heap_free(ptr[index]);
			ptr[index] = 0;
		}
		unsigned int align = 1 << ((unsigned int)rand() % 8);
		ptr[index] = heap_alloc((unsigned int)rand() % 32768, align);
//		printf("ptr[0x%x] = %p, align 0x%x\n", index, ptr[index], align);

		DEBUG_ASSERT(((addr_t)ptr[index] % align) == 0);
//		heap_dump();
	}

	for (i=0; i < 16; i++) {
		if (ptr[i])
			heap_free(ptr[i]);
	}

	heap_dump();
}

// try to insert this free chunk into the free list, consuming the chunk by merging it with
// nearby ones if possible. Returns base of whatever chunk it became in the list.
static struct free_heap_chunk *heap_insert_free_chunk(struct free_heap_chunk *chunk)
{
#if LK_DEBUGLEVEL > INFO
	vaddr_t chunk_end = (vaddr_t)chunk + chunk->len;
#endif

//	dprintf("%s: chunk ptr %p, size 0x%lx, chunk_end 0x%x\n", __FUNCTION__, chunk, chunk->len, chunk_end);

	struct free_heap_chunk *next_chunk;
	struct free_heap_chunk *last_chunk;

	mutex_acquire(&theheap.lock);

	theheap.remaining += chunk->len;

	// walk through the list, finding the node to insert before
	list_for_every_entry(&theheap.free_list, next_chunk, struct free_heap_chunk, node) {
		if (chunk < next_chunk) {
			DEBUG_ASSERT(chunk_end <= (vaddr_t)next_chunk);

			list_add_before(&next_chunk->node, &chunk->node);

			goto try_merge;
		}
	}

	// walked off the end of the list, add it at the tail
	list_add_tail(&theheap.free_list, &chunk->node);

	// try to merge with the previous chunk
try_merge:
	last_chunk = list_prev_type(&theheap.free_list, &chunk->node, struct free_heap_chunk, node);
	if (last_chunk) {
		if ((vaddr_t)last_chunk + last_chunk->len == (vaddr_t)chunk) {
			// easy, just extend the previous chunk
			last_chunk->len += chunk->len;

			// remove ourself from the list
			list_delete(&chunk->node);

			// set the chunk pointer to the newly extended chunk, in case
			// it needs to merge with the next chunk below
			chunk = last_chunk;
		}
	}

	// try to merge with the next chunk
	if (next_chunk) {
		if ((vaddr_t)chunk + chunk->len == (vaddr_t)next_chunk) {
			// extend our chunk
			chunk->len += next_chunk->len;

			// remove them from the list
			list_delete(&next_chunk->node);
		}
	}

	mutex_release(&theheap.lock);

	return chunk;
}

static struct free_heap_chunk *heap_create_free_chunk(void *ptr, size_t len, bool allow_debug)
{
	DEBUG_ASSERT((len % sizeof(void *)) == 0); // size must be aligned on pointer boundary

#if DEBUG_HEAP
	if (allow_debug)
		memset(ptr, FREE_FILL, len);
#endif

	struct free_heap_chunk *chunk = (struct free_heap_chunk *)ptr;
	chunk->len = len;

	return chunk;
}

static void heap_free_delayed_list(void)
{
	struct list_node list;

	list_initialize(&list);

	enter_critical_section();

	struct free_heap_chunk *chunk;
	while ((chunk = list_remove_head_type(&theheap.delayed_free_list, struct free_heap_chunk, node))) {
		list_add_head(&list, &chunk->node);
	}
	exit_critical_section();

	while ((chunk = list_remove_head_type(&list, struct free_heap_chunk, node))) {
		LTRACEF("freeing chunk %p\n", chunk);
		heap_insert_free_chunk(chunk);
	}
}

void *heap_alloc(size_t size, unsigned int alignment)
{
	void *ptr;
#if DEBUG_HEAP
	size_t original_size = size;
#endif

	LTRACEF("size %zd, align %d\n", size, alignment);

	// deal with the pending free list
	if (unlikely(!list_is_empty(&theheap.delayed_free_list))) {
		heap_free_delayed_list();
	}

	// alignment must be power of 2
	if (alignment & (alignment - 1))
		return NULL;

	// we always put a size field + base pointer + magic in front of the allocation
	size += sizeof(struct alloc_struct_begin);
#if DEBUG_HEAP
	size += PADDING_SIZE;
#endif

	// make sure we allocate at least the size of a struct free_heap_chunk so that
	// when we free it, we can create a struct free_heap_chunk struct and stick it
	// in the spot
	if (size < sizeof(struct free_heap_chunk))
		size = sizeof(struct free_heap_chunk);

	// round up size to a multiple of native pointer size
	size = ROUNDUP(size, sizeof(void *));

	// deal with nonzero alignments
	if (alignment > 0) {
		if (alignment < 16)
			alignment = 16;

		// add alignment for worst case fit
		size += alignment;
	}

	mutex_acquire(&theheap.lock);

	// walk through the list
	ptr = NULL;
	struct free_heap_chunk *chunk;
	list_for_every_entry(&theheap.free_list, chunk, struct free_heap_chunk, node) {
		DEBUG_ASSERT((chunk->len % sizeof(void *)) == 0); // len should always be a multiple of pointer size

		// is it big enough to service our allocation?
		if (chunk->len >= size) {
			ptr = chunk;

			// remove it from the list
			struct list_node *next_node = list_next(&theheap.free_list, &chunk->node);
			list_delete(&chunk->node);

			if (chunk->len > size + sizeof(struct free_heap_chunk)) {
				// there's enough space in this chunk to create a new one after the allocation
				struct free_heap_chunk *newchunk = heap_create_free_chunk((uint8_t *)ptr + size, chunk->len - size, true);

				// truncate this chunk
				chunk->len -= chunk->len - size;

				// add the new one where chunk used to be
				if (next_node)
					list_add_before(next_node, &newchunk->node);
				else
					list_add_tail(&theheap.free_list, &newchunk->node);
			}

			// the allocated size is actually the length of this chunk, not the size requested
			DEBUG_ASSERT(chunk->len >= size);
			size = chunk->len;

#if DEBUG_HEAP
			memset(ptr, ALLOC_FILL, size);
#endif

			ptr = (void *)((addr_t)ptr + sizeof(struct alloc_struct_begin));

			// align the output if requested
			if (alignment > 0) {
				ptr = (void *)ROUNDUP((addr_t)ptr, alignment);
			}

			struct alloc_struct_begin *as = (struct alloc_struct_begin *)ptr;
			as--;
#if LK_DEBUGLEVEL > 1
			as->magic = HEAP_MAGIC;
#endif
			as->ptr = (void *)chunk;
			as->size = size;
			theheap.remaining -= size;

			if (theheap.remaining < theheap.low_watermark) {
				theheap.low_watermark = theheap.remaining;
			}
#if DEBUG_HEAP
			as->padding_start = ((uint8_t *)ptr + original_size);
			as->padding_size = (((addr_t)chunk + size) - ((addr_t)ptr + original_size));
//			printf("padding start %p, size %u, chunk %p, size %u\n", as->padding_start, as->padding_size, chunk, size);

			memset(as->padding_start, PADDING_FILL, as->padding_size);
#endif

			break;
		}
	}

	mutex_release(&theheap.lock);

	LTRACEF("returning ptr %p\n", ptr);

	return ptr;
}

void heap_free(void *ptr)
{
	if (ptr == 0)
		return;

	LTRACEF("ptr %p\n", ptr);

	// check for the old allocation structure
	struct alloc_struct_begin *as = (struct alloc_struct_begin *)ptr;
	as--;

	DEBUG_ASSERT(as->magic == HEAP_MAGIC);

#if DEBUG_HEAP
	{
		uint i;
		uint8_t *pad = (uint8_t *)as->padding_start;

		for (i = 0; i < as->padding_size; i++) {
			if (pad[i] != PADDING_FILL) {
				printf("free at %p scribbled outside the lines:\n", ptr);
				hexdump(pad, as->padding_size);
				panic("die\n");
			}
		}
	}
#endif

	LTRACEF("allocation was %zd bytes long at ptr %p\n", as->size, as->ptr);

	// looks good, create a free chunk and add it to the pool
	heap_insert_free_chunk(heap_create_free_chunk(as->ptr, as->size, true));
}

void heap_delayed_free(void *ptr)
{
	LTRACEF("ptr %p\n", ptr);

	// check for the old allocation structure
	struct alloc_struct_begin *as = (struct alloc_struct_begin *)ptr;
	as--;

	DEBUG_ASSERT(as->magic == HEAP_MAGIC);

	struct free_heap_chunk *chunk = heap_create_free_chunk(as->ptr, as->size, false);

	enter_critical_section();
	list_add_head(&theheap.delayed_free_list, &chunk->node);
	exit_critical_section();
}

void heap_get_stats(struct heap_stats *ptr)
{
	struct free_heap_chunk *chunk;

	if ((struct heap_stats*)NULL==ptr) {
		return;
	}
	//flush the delayed free list
	if (unlikely(!list_is_empty(&theheap.delayed_free_list))) {
		heap_free_delayed_list();
	}

	ptr->heap_start = theheap.base;
	ptr->heap_len = theheap.len;
	ptr->heap_free=0;
	ptr->heap_max_chunk = 0;

	mutex_acquire(&theheap.lock);

	list_for_every_entry(&theheap.free_list, chunk, struct free_heap_chunk, node) {
		ptr->heap_free += chunk->len;

		if (chunk->len > ptr->heap_max_chunk) {
			ptr->heap_max_chunk = chunk->len;
		}
	}

	ptr->heap_low_watermark = theheap.low_watermark;

	mutex_release(&theheap.lock);

}

void heap_init(void)
{
	LTRACE_ENTRY;

	// set the heap range
	theheap.base = (void *)HEAP_START;
	theheap.len = HEAP_LEN;
	theheap.remaining =0; // will get set by heap_insert_free_chunk()
	theheap.low_watermark = theheap.len;
	LTRACEF("base %p size %zd bytes\n", theheap.base, theheap.len);

	// create a mutex
	mutex_init(&theheap.lock);

	// initialize the free list
	list_initialize(&theheap.free_list);

	// initialize the delayed free list
	list_initialize(&theheap.delayed_free_list);

	// create an initial free chunk
	heap_insert_free_chunk(heap_create_free_chunk(theheap.base, theheap.len, false));

	// dump heap info
//	heap_dump();

//	dprintf(INFO, "running heap tests\n");
//	heap_test();
}

/* add a new block of memory to the heap */
void heap_add_block(void *ptr, size_t len)
{
	heap_insert_free_chunk(heap_create_free_chunk(ptr, len, false));
}

#include <lib/console.h>

static int cmd_heap(int argc, const cmd_args *argv);

STATIC_COMMAND_START
STATIC_COMMAND("heap", "heap debug commands", &cmd_heap)
STATIC_COMMAND_END(heap);

static int cmd_heap(int argc, const cmd_args *argv)
{
	if (argc < 2)
	{
		printf("not enough arguments\n");
usage:
		printf("%s info\n", argv[0].str);
		printf("%s alloc <count_bytes> <alignment>\n", argv[0].str);
		printf("%s free <address>\n", argv[0].str);
		printf("%s alloc_uncached <count_bytes> <alignment>\n", argv[0].str);
		printf("%s free_uncached <address> <asize>\n", argv[0].str);
		printf("         (asize should be same as alloc_uncached)\n");
		return -1;
	}

	if (strcmp(argv[1].str, "info") == 0)
	{
		heap_dump();
	}
	else if (strcmp(argv[1].str, "alloc") == 0)
	{
		if (argc < 3)
		{
			printf("not enough arguments:\n");
			goto usage;
		}
		uint8_t *memory = NULL;
		uint32_t len = argv[2].u;
		uint32_t boundary = 0;

		if (argc == 4)
			boundary = argv[3].u;

		memory = memalign(boundary, len);

		if (!memory)
			printf("unable to allocate memory \n");
		else
			printf("pointer to %u bytes is %p \n", len, memory);
	}
	else if (strcmp(argv[1].str, "free") == 0)
	{
		if (argc < 3)
		{
			printf("not enough arguments:\n");
			goto usage;
		}
		addr_t memory = argv[2].u;

		if (memory)
		{
			free((void *)memory);
			printf("memory is deallocated \n");
		}
		else
			printf("memory is NULL \n");
	}
	else if (strcmp(argv[1].str, "alloc_uncached") == 0)
	{
		if (argc < 3)
		{
			printf("not enough arguments:\n");
			goto usage;
		}
		uint8_t *memory = NULL;
		uint32_t len = argv[2].u;
		uint32_t boundary = 0;
		size_t asize;

		if (argc == 4)
			boundary = argv[3].u;

		memory = memalign_uncached(boundary, len, &asize);

		if (!memory)
			printf("unable to allocate memory \n");
		else
			printf("pointer to %u bytes is %p (asize: %zu)\n",
				   len, memory, asize);
	}
	else if (strcmp(argv[1].str, "free_uncached") == 0)
	{
		if (argc < 4)
		{
			printf("not enough arguments:\n");
			goto usage;
		}
		addr_t memory = argv[2].u;
		size_t asize = argv[3].u;

		if (memory)
		{
			free_uncached((void *)memory, asize);
			printf("memory is deallocated \n");
		}
		else
			printf("memory is NULL \n");
	}
	else
	{
		printf("unrecognized command\n");
		goto usage;
	}

	return 0;
}

/* vim: set ts=4 sw=4 noexpandtab: */

