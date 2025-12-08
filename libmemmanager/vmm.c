#include "vmm.h"
#define REG_ERR 19
static int evict_page_fifo();
static int evict_page_third();
static void update_page_protection_third(int vpn);
static void* find_pgaddr(int vpn);

//Signal Handler for SIGSEGV
void signal_handler(int sig, siginfo_t *si, void *ucontext) {
    void *fault_addr = si->si_addr;
    if (fault_addr < vm_begin || fault_addr >= (vm_begin + num_of_pages * pgsize)) {
        sigaction(SIGSEGV, &old_sa, NULL);
        raise(SIGSEGV);
        return;
    }
    ptrdiff_t offset_in_vm = (char *)fault_addr - (char *)vm_begin;
    int vpn = offset_in_vm / pgsize;
    int offset = offset_in_vm % pgsize;

    // Identifying the access type whether it is read or write
    ucontext_t *ctx = (ucontext_t *)ucontext;
    greg_t err_code = ctx->uc_mcontext.__gregs[REG_ERR];
    bool is_write = (err_code & FAULT_WAS_WRITE);
    int fault_type = 0;
    int evicted_vpn = -1;
    int write_back = 0;
    uint64_t phy_addr = 0;
    PageTableEntry *pte = &pt[vpn];
    if (!pte->present) {
        fault_type = is_write ? 1 : 0;
        int fr_idx;
        if (fr_used < num_of_fr) {
            fr_idx = fr_used;
            fr_used++;
        } 
        else {
            if (type == MM_FIFO) {
                fr_idx = evict_page_fifo();
            } else {
                fr_idx = evict_page_third();
            }
            evicted_vpn = fr[fr_idx].vpn;
            PageTableEntry *pte_tmp = &pt[evicted_vpn];
            if (pte_tmp->mod_bit) {
                write_back = 1;
            }
            pte_tmp->present = 0; //Evict
            pte_tmp->frame_no = -1;
            pte_tmp->ref_bit = 0;
            pte_tmp->mod_bit = 0;
            mprotect(find_pgaddr(evicted_vpn), pgsize, PROT_NONE);
        }
        fr[fr_idx].vpn = vpn;
        pte->frame_no = fr_idx;
        pte->present = 1;
        pte->ref_bit = 1;
        pte->mod_bit = is_write ? 1 : 0;
        if (type == MM_FIFO) {
            queue[head] = vpn;
            head = (head + 1) % num_of_fr;
        }

    } 
    else {
        if (is_write && !pte->mod_bit) {
            fault_type = 2;
            pte->mod_bit = 1;
        } 
        else if (!is_write && !pte->ref_bit) {
            fault_type = 3;
        } 
        else if (is_write && !pte->ref_bit) {
            fault_type = 4;
            pte->mod_bit = 1;
        }
        pte->ref_bit = 1;
    }
    if (type == MM_FIFO) {
        update_page_protection_fifo(vpn);
    } 
    else {
        update_page_protection_third(vpn);
    }
    phy_addr = (pte->frame_no * pgsize) + (offset_in_vm % pgsize);
    mm_logger(stats, vpn, fault_type, evicted_vpn, write_back, phy_addr);
}

static int evict_page_fifo() {
    int vpn_tmp = queue[tail];
    tail = (tail + 1) % num_of_fr;
    return pt[vpn_tmp].frame_no;
}

static int evict_page_third() {
    while (1) {
        for (int i = 0; i < num_of_fr; i++) {
            int vpn = fr[clk].vpn;
            PageTableEntry *pte = &pt[vpn];
            if (pte->ref_bit == 0 && pte->mod_bit == 0) {
                int fr_tmp = clk;
                clk = (clk + 1) % num_of_fr;
                return fr_tmp;
            }
            clk = (clk + 1) % num_of_fr;
        }
        for (int i = 0; i < num_of_fr; i++) {
            int vpn = fr[clk].vpn;
            PageTableEntry *pte = &pt[vpn];
            if (pte->ref_bit == 0 && pte->mod_bit == 1) {
                int fr_tmp = clk;
                clk = (clk + 1) % num_of_fr;
                return fr_tmp;
            }
            if (pte->ref_bit == 1) {
                pte->ref_bit = 0;
                update_page_protection_third(vpn);
            }
            clk = (clk + 1) % num_of_fr;
        }
    }
}

static void update_page_protection_fifo(int vpn) {
    PageTableEntry *pte = &pt[vpn];
    void *addr = find_pgaddr(vpn);
    if (!pte->present) {
        mprotect(addr, pgsize, PROT_NONE);
    } else if (pte->mod_bit) {
        mprotect(addr, pgsize, PROT_READ | PROT_WRITE);
    } else {
        mprotect(addr, pgsize, PROT_READ);
    }
}


static void update_page_protection_third(int vpn) {
    PageTableEntry *pte = &pt[vpn];
    void *addr = find_pgaddr(vpn);
    if (!pte->present) {
        mprotect(addr, pgsize, PROT_NONE);
    } else if (pte->ref_bit == 0) {
        mprotect(addr, pgsize, PROT_NONE);
    } else if (pte->ref_bit == 1 && pte->mod_bit == 0) {
        mprotect(addr, pgsize, PROT_READ);
    } else {
        mprotect(addr, pgsize, PROT_READ | PROT_WRITE);
    }
}

static void* find_pgaddr(int vpn) {
    return (char*)vm_begin + (vpn * pgsize);
}