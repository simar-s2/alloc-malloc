#define _DEFAULT_SOURCE

#include "alloc.h"
#include <stddef.h>
#include <unistd.h>

static struct header *free_list = NULL;
static enum algs current_alg = FIRST_FIT;
static int size_limit = 0;
static void *heap_start = NULL;
static void *heap_end = NULL;
static void *initial_break = NULL;

void allocopt(enum algs alg, int limit) {
  current_alg = alg;
  size_limit = limit;
  free_list = NULL;

  if (initial_break == NULL) {
    initial_break = sbrk(0);
  }

  if (heap_start != NULL) {
    brk(heap_start);
  }

  heap_start = sbrk(0);
  heap_end = heap_start;
}

static int expand_heap(void) {
  void *current_break = sbrk(0);
  int current_heap_size = (char *)current_break - (char *)heap_start;

  if (size_limit > 0 && current_heap_size + INCREMENT > size_limit) {
    return -1;
  }

  void *new_break = sbrk(INCREMENT);
  if (new_break == (void *)-1) {
    return -1;
  }

  struct header *new_block = (struct header *)heap_end;
  new_block->size = INCREMENT;

  heap_end = (char *)heap_end + INCREMENT;

  struct header *curr = free_list;
  struct header *prev = NULL;

  while (curr != NULL) {
    char *curr_end = (char *)curr + curr->size;
    char *new_start = (char *)new_block;

    if (curr_end == new_start) {
      curr->size += new_block->size;
      return 0;
    }

    prev = curr;
    curr = curr->next;
  }

  new_block->next = free_list;
  free_list = new_block;

  return 0;
}

void *alloc(int size) {
  if (size <= 0) {
    return NULL;
  }

  if (heap_start == NULL) {
    heap_start = sbrk(0);
    heap_end = heap_start;
  }

  uint64_t total_size = sizeof(struct header) + size;
  struct header *prev = NULL;
  struct header *curr = free_list;
  struct header *best = NULL;
  struct header *best_prev = NULL;

  if (current_alg == FIRST_FIT) {
    while (curr != NULL) {
      if (curr->size >= total_size) {
        best = curr;
        best_prev = prev;
        break;
      }
      prev = curr;
      curr = curr->next;
    }
  } else if (current_alg == BEST_FIT) {
    uint64_t best_size = UINT64_MAX;
    while (curr != NULL) {
      if (curr->size >= total_size && curr->size < best_size) {
        best = curr;
        best_prev = prev;
        best_size = curr->size;
      }
      prev = curr;
      curr = curr->next;
    }
  } else if (current_alg == WORST_FIT) {
    uint64_t worst_size = 0;
    prev = NULL;
    curr = free_list;
    while (curr != NULL) {
      if (curr->size >= total_size && curr->size > worst_size) {
        best = curr;
        best_prev = prev;
        worst_size = curr->size;
      }
      prev = curr;
      curr = curr->next;
    }
  }

  if (best == NULL) {
    if (expand_heap() != 0) {
      return NULL;
    }
    return alloc(size);
  }

  uint64_t remaining = best->size - total_size;
  if (remaining > sizeof(struct header)) {
    struct header *new_free = (struct header *)((char *)best + total_size);
    new_free->size = remaining;
    new_free->next = best->next;

    best->size = total_size;

    if (best_prev == NULL) {
      free_list = new_free;
    } else {
      best_prev->next = new_free;
    }
  } else {
    if (best_prev == NULL) {
      free_list = best->next;
    } else {
      best_prev->next = best->next;
    }
  }

  return (void *)((char *)best + sizeof(struct header));
}

void dealloc(void *ptr) {
  if (ptr == NULL) {
    return;
  }

  struct header *block = (struct header *)((char *)ptr - sizeof(struct header));

  block->next = free_list;
  free_list = block;

  int merged_any = 1;
  while (merged_any) {
    merged_any = 0;

    struct header *curr = free_list;
    struct header *prev = NULL;

    while (curr != NULL) {
      struct header *scan = free_list;
      struct header *scan_prev = NULL;

      while (scan != NULL) {
        if (scan == curr) {
          scan_prev = scan;
          scan = scan->next;
          continue;
        }

        char *curr_end = (char *)curr + curr->size;
        char *scan_start = (char *)scan;
        char *scan_end = (char *)scan + scan->size;
        char *curr_start = (char *)curr;

        if (curr_end == scan_start) {
          curr->size += scan->size;

          if (scan_prev == NULL) {
            free_list = scan->next;
          } else {
            scan_prev->next = scan->next;
          }

          merged_any = 1;
          break;
        } else if (scan_end == curr_start) {
          scan->size += curr->size;

          if (prev == NULL) {
            free_list = curr->next;
          } else {
            prev->next = curr->next;
          }

          merged_any = 1;
          break;
        }

        scan_prev = scan;
        scan = scan->next;
      }

      if (merged_any) {
        break;
      }

      prev = curr;
      curr = curr->next;
    }
  }
}

struct allocinfo allocinfo(void) {
  struct allocinfo info;
  info.free_size = 0;
  info.free_chunks = 0;
  info.largest_free_chunk_size = 0;
  info.smallest_free_chunk_size = UINT64_MAX;

  struct header *curr = free_list;
  while (curr != NULL) {
    uint64_t chunk_size = curr->size - sizeof(struct header);
    info.free_size += chunk_size;
    info.free_chunks++;

    if (chunk_size > info.largest_free_chunk_size) {
      info.largest_free_chunk_size = chunk_size;
    }
    if (chunk_size < info.smallest_free_chunk_size) {
      info.smallest_free_chunk_size = chunk_size;
    }

    curr = curr->next;
  }

  if (info.free_chunks == 0) {
    info.smallest_free_chunk_size = 0;
  }

  return info;
}
