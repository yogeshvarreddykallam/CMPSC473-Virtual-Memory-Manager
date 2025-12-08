#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include <unistd.h>

#include <ucontext.h>
#include <sys/ucontext.h>
#include <sys/mman.h>
#include <signal.h>
#include "api.h"

// To check if a fault was for a Read:0 or Write:1
#define FAULT_WAS_WRITE (0x2)

//PTE
typedef struct {
    int frame_no;
    int present;
    int ref_bit;
    int mod_bit;
} PageTableEntry;

//Frame Table
typedef struct {
    int vpn;
} FrameTableEntry;

extern enum policy_type type;
extern void *vm_begin;
extern int num_of_pages;
extern int num_of_fr;
extern int pgsize;
extern struct mm_stats *stats;
extern PageTableEntry *pt;
extern FrameTableEntry *fr;
extern int fr_used;
extern int *queue;   
extern int head;
extern int tail;
extern int clk; 
extern struct sigaction old_sa;

void signal_handler(int sig, siginfo_t *si, void *unused);
static void update_page_protection_third(int vpn);
static void update_page_protection_fifo(int vpn);