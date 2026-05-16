# CMPSC 473 — Virtual Memory Manager

A **user-space virtual memory manager** implemented in C for **PSU CMPSC 473 (Operating Systems) — Fall 2025, Project 3**.

The system uses `mmap` and `SIGSEGV` signal handling to simulate demand paging, and supports two configurable page replacement policies: **FIFO** and **Third Chance**.

## Page Replacement Policies

| Policy | Flag | Description |
|--------|------|-------------|
| **FIFO** | 1 | Evicts the page that has been in memory the longest |
| **Third Chance** | 2 | Clock-based algorithm — pages get up to 3 reference chances before eviction |

## How It Works

1. A contiguous virtual address space is `mmap`'d but not yet backed by physical frames
2. On first access, a `SIGSEGV` is caught by a custom signal handler
3. The handler allocates a physical frame (evicting a page if needed) and maps it in
4. Read/write bits in the page table entries are used by the Third Chance policy to decide eviction order

## Repository Structure

```
.
├── main.c                      # Entry point — reads ops file, calls mm_read/mm_write
├── Makefile
├── Report.pdf                  # Project write-up and policy comparison
├── libmemmanager/
│   ├── vmm.c / vmm.h           # Core VMM: page table, frame table, fault handler
│   ├── init.c                  # mmap setup and signal handler registration
│   ├── logger.c                # Stats and fault logging
│   ├── api.h                   # mm_read / mm_write / mm_stats API
│   └── Makefile
├── sample_input/               # 12 test input files (input_1 … input_12)
└── sample_output/              # Expected outputs: result-{policy}-{frames}-{input}
```

## Build & Run

```bash
make

# Usage: ./main <policy> <num_frames> <input_file>
#   Policy 1 = FIFO
#   Policy 2 = Third Chance

./main 1 4  sample_input/input_1     # FIFO, 4 physical frames
./main 2 8  sample_input/input_3     # Third Chance, 8 physical frames
./main 2 16 sample_input/input_12    # Third Chance, 16 physical frames
```

## Input Format

```
read  <page_number> <offset>
write <page_number> <offset> <value>
```

## Page Table Entry

```c
typedef struct {
    int frame_no;
    int present;
    int ref_bit;    // used by Third Chance
    int mod_bit;    // dirty bit — triggers writeback on eviction
} PageTableEntry;
```

## Output / Stats

Per-operation log: fault type, evicted VPN (if any), cumulative fault counts. Validated against `sample_output/result-{policy}-{frames}-{input}`.

## Report

Design decisions, policy comparison, and results: [`Report.pdf`](Report.pdf)
