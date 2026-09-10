# alloc-malloc

A drop-in heap allocator written in C: the `malloc`/`free` machinery from the
ground up. It requests memory from the OS with `sbrk`, hands out chunks from a
free list, and merges freed blocks back together. Written for **SFU CMPT 276**.

> 🖼️ **Diagram needed**: a picture of the heap showing the OS break, allocated blocks each with a 16-byte header, and the free list threading through the gaps. Save to `docs/heap.png` and replace this line with `![Heap layout](docs/heap.png)`.

## Quickstart

```bash
git clone https://github.com/simar-s2/alloc-malloc.git
cd alloc-malloc

cmake -S . -B build
cmake --build build

./build/main            # runs the bundled test suite
```

## How it works

The public API is four functions (`include/alloc.h`):

| Function | Role |
|---|---|
| `alloc(size)` | return a pointer to at least `size` bytes, or `NULL` |
| `dealloc(ptr)` | return a block to the free list |
| `allocopt(alg, limit)` | choose the placement algorithm and cap the heap size; **resets the allocator** |
| `allocinfo()` | report free bytes, free-chunk count, largest/smallest chunk |

**Memory comes from the OS in fixed steps.** When the free list can't satisfy a
request, `expand_heap()` calls `sbrk(INCREMENT)` (256 bytes), unless that would
cross the limit set by `allocopt`, in which case `alloc` returns `NULL`. A fresh
`sbrk` region that happens to sit right after an existing free block is coalesced
into it.

**Every block carries a header**
(`struct header { uint64_t size; struct header *next; }`, packed). Allocated blocks
are `header + payload`; the pointer returned to the caller points just past the
header. The free list is a singly linked chain through those same headers.

**Three placement strategies**, selectable at runtime:

- `FIRST_FIT`: first block big enough
- `BEST_FIT`: smallest block that still fits
- `WORST_FIT`: largest block available

**Freeing coalesces.** `dealloc` walks the free list and, if the block being freed
is physically adjacent to a free neighbour, the two are joined so the space stays
usable for large future requests.

## Testing

`src/main.c` is a self-contained harness: 246 assertions across the three
algorithms covering exact sizing, heap growth, the `sbrk` step boundary, and
placement order. It prints a running `Score` / `Success cases` line. Run a single
group with `./build/main <alg 0-2> <test 0-3>`.

## Built with

C · CMake · POSIX `sbrk`/`brk`

## License

Released under the [MIT License](LICENSE).
