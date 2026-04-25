/**
 * @FilePath: /ZZZ-OS/kernel/kernel.c
 * @Description:
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-05-07 19:18:08
 * @LastEditTime: 2025-12-03 18:26:55
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 */

#include <os/kmalloc.h>
#include <os/check.h>
#include <os/printk.h>
#include <os/mm.h>
#include <os/sched.h>
#include <os/fdt.h>
#include <os/of_platform.h>
#include <fs/fs.h>
// #include <os/elf.h>
#include <os/irq.h>
#include <os/timer_chip.h>
#include <os/timekeeping.h>
#include <mm/memblock.h>
#include <mm/symbols.h>
#include <mm/early_malloc.h>
#include <os/device.h>
#include <os/timerqueue.h>
#include <os/cpu.h>
#include <os/completion.h>
#include <os/string.h>
#include <os/kva.h>
#include <mm/vma.h>
#include <mm/pgtbl.h>
#include <mm/pgtbl_types.h>
#include <fs/binfmt.h>
#include <asm/process.h>
#include <asm/ptrace.h>

#define USER_TEST_CODE_VA  0x00010000
#define USER_TEST_UART_VA  0x02020000
#define PGTBL_TEST_DATA_VA 0x00020000

extern const uint8_t __user_mode_smoke_start[];
extern const uint8_t __user_mode_smoke_end[];
extern phys_addr_t uart_base;

static void low_uart_putc(char c) {
    volatile uint32_t *utxd = (volatile uint32_t *)0x02020040;
    volatile uint32_t *uts =  (volatile uint32_t *)0x020200B4;

    while (((*uts) >> 4) & 1) {
    }
    *utxd = (uint32_t)c;
}

static void low_uart_puts(const char *s) {
    while (*s) {
        low_uart_putc(*s++);
    }
}

void touch_abort() {
    volatile int *p = (int *)0x10000000; // 一个很可能未映射的地址
    *p = 42; // 触发数据访问异常
}

static void sync_user_exec_code(void *addr, size_t size) {
    uintptr_t start = ALIGN_DOWN((uintptr_t)addr, 32);
    uintptr_t end = ALIGN_UP((uintptr_t)addr + size, 32);

    for (uintptr_t p = start; p < end; p += 32) {
        asm volatile(
            "mcr p15, 0, %0, c7, c10, 1\n"
            :
            : "r"(p)
            : "memory"
        );
    }

    asm volatile(
        "dsb\n"
        "mov r0, #0\n"
        "mcr p15, 0, r0, c7, c5, 0\n"
        "mcr p15, 0, r0, c7, c5, 6\n"
        "dsb\n"
        "isb\n"
        :
        :
        : "r0", "memory"
    );
}

static void pgtbl_switch_selftest(void) {
    struct mm_struct *mm;
    void *data_page;
    phys_addr_t data_pa;
    volatile uint32_t *probe;
    uint32_t *src;
    uint32_t read_ok = 0;
    uint32_t write_ok = 0;

    printk("pgtbl-test: preparing switch selftest\n");

    mm = mm_alloc();
    CHECK(mm != NULL, "pgtbl-test: mm_alloc failed", panic("pgtbl selftest failed\n"););
    copy_kernel_mapping(mm);

    CHECK(vma_add(mm, USER_TEST_UART_VA, PAGE_SIZE,
                  PROT_READ | PROT_WRITE | PROT_DEVICE) == 0,
          "pgtbl-test: add uart vma failed", panic("pgtbl selftest failed\n"););
    CHECK(map(mm->pgdir, USER_TEST_UART_VA, USER_TEST_UART_VA, PAGE_SIZE,
              PROT_READ | PROT_WRITE | PROT_DEVICE) == 0,
          "pgtbl-test: map uart page failed", panic("pgtbl selftest failed\n"););

    data_page = page_alloc(1);
    CHECK(data_page != NULL, "pgtbl-test: alloc data page failed", panic("pgtbl selftest failed\n"););
    memset(data_page, 0, PAGE_SIZE);
    src = (uint32_t *)data_page;
    src[0] = 0x13579bdfu;
    src[1] = 0x2468ace0u;
    data_pa = KERNEL_PA(data_page);

    CHECK(vma_add(mm, PGTBL_TEST_DATA_VA, PAGE_SIZE,
                  PROT_READ | PROT_WRITE) == 0,
          "pgtbl-test: add data vma failed", panic("pgtbl selftest failed\n"););
    CHECK(map(mm->pgdir, PGTBL_TEST_DATA_VA, data_pa, PAGE_SIZE,
              PROT_READ | PROT_WRITE) == 0,
          "pgtbl-test: map data page failed", panic("pgtbl selftest failed\n"););

    printk("pgtbl-test: before switch lookup va=%xu -> pa=%xu prot=%xu\n",
           PGTBL_TEST_DATA_VA,
           pgtbl_lookup(mm->pgdir, PGTBL_TEST_DATA_VA),
           pgtbl_lookup_prot(mm->pgdir, PGTBL_TEST_DATA_VA));

    pgtbl_switch_to(mm->pgdir);
    low_uart_putc('A');
    low_uart_puts("PTSW:");

    probe = (volatile uint32_t *)PGTBL_TEST_DATA_VA;
    if (probe[0] == 0x13579bdfu && probe[1] == 0x2468ace0u) {
        read_ok = 1;
        low_uart_putc('R');
    } else {
        low_uart_putc('r');
    }

    probe[2] = 0xdeadbeefu;
    if (probe[2] == 0xdeadbeefu) {
        write_ok = 1;
        low_uart_putc('W');
    } else {
        low_uart_putc('w');
    }
    low_uart_putc('\n');

    pgtbl_switch_to(init_mm.pgdir);
    printk("pgtbl-test: switched back read_ok=%d write_ok=%d\n", read_ok, write_ok);

    while (1) {
    }
}

static void user_mode_smoke_test(void) {
    struct mm_struct *mm;
    void *code_page;
    void *stack_page;
    phys_addr_t code_pa;
    phys_addr_t stack_pa;
    struct pt_regs *regs;
    size_t smoke_size;

    printk("user-test: preparing smoke test\n");
    mm = mm_alloc();
    CHECK(mm != NULL, "user-test: mm_alloc failed", panic("user smoke test failed\n"););
    copy_kernel_mapping(mm);
    code_page = page_alloc(1);
    CHECK(code_page != NULL, "user-test: alloc code page failed", panic("user smoke test failed\n"););
    memset(code_page, 0, PAGE_SIZE);
    smoke_size = __user_mode_smoke_end - __user_mode_smoke_start;
    CHECK(smoke_size != 0 && smoke_size <= PAGE_SIZE,
          "user-test: invalid smoke code size", panic("user smoke test failed\n"););
    memcpy(code_page, __user_mode_smoke_start, smoke_size);
    sync_user_exec_code(code_page, smoke_size);
    code_pa = KERNEL_PA(code_page);

    CHECK(vma_add(mm, USER_TEST_CODE_VA, PAGE_SIZE,
                  PROT_USER | PROT_READ | PROT_WRITE | PROT_EXEC) == 0,
          "user-test: add code vma failed", panic("user smoke test failed\n"););
    CHECK(map(mm->pgdir, USER_TEST_CODE_VA, code_pa, PAGE_SIZE,
              PROT_USER | PROT_READ | PROT_WRITE | PROT_EXEC) == 0,
          "user-test: map code page failed", panic("user smoke test failed\n"););

    CHECK(vma_add(mm, USER_TEST_UART_VA, PAGE_SIZE,
                  PROT_USER | PROT_READ | PROT_WRITE | PROT_DEVICE) == 0,
          "user-test: add uart vma failed", panic("user smoke test failed\n"););
    CHECK(map(mm->pgdir, USER_TEST_UART_VA, USER_TEST_UART_VA, PAGE_SIZE,
              PROT_USER | PROT_READ | PROT_WRITE | PROT_DEVICE) == 0,
          "user-test: map uart page failed", panic("user smoke test failed\n"););

    stack_page = page_alloc(1);
    CHECK(stack_page != NULL, "user-test: alloc stack page failed", panic("user smoke test failed\n"););
    memset(stack_page, 0, PAGE_SIZE);
    stack_pa = KERNEL_PA(stack_page);
    CHECK(vma_add(mm, USER_STACK_TOP - PAGE_SIZE, PAGE_SIZE,
                  PROT_USER | PROT_READ | PROT_WRITE) == 0,
          "user-test: add stack vma failed", panic("user smoke test failed\n"););
    CHECK(map(mm->pgdir, USER_STACK_TOP - PAGE_SIZE, stack_pa, PAGE_SIZE,
              PROT_USER | PROT_READ | PROT_WRITE) == 0,
          "user-test: map stack page failed", panic("user smoke test failed\n"););

    current->mm = mm;
    current->active_mm = mm;
    current->flags &= ~PF_KTHREAD;
    mm->start_code = USER_TEST_CODE_VA;
    mm->end_code = USER_TEST_CODE_VA + smoke_size;
    mm->start_stack = USER_STACK_TOP;

    regs = task_pt_regs(current);
    start_thread(regs, USER_TEST_CODE_VA, USER_STACK_TOP);

    {
        int l0_index = pgtbl_level_index(mm->pgdir, 0, USER_TEST_CODE_VA);
        pte_t *root = (pte_t *)mm->pgdir->root;
        phys_addr_t lookup_pa = pgtbl_lookup(mm->pgdir, USER_TEST_CODE_VA);
        pgprot_t lookup_prot = pgtbl_lookup_prot(mm->pgdir, USER_TEST_CODE_VA);

        printk("user-test: mm root=%xu root_pa=%xu l0_index=%d l0_pte=%xu\n",
               mm->pgdir->root, mm->pgdir->root_pa, l0_index, root[l0_index].val);
        printk("user-test: lookup code va=%xu -> pa=%xu prot=%xu\n",
               USER_TEST_CODE_VA, lookup_pa, lookup_prot);
    }

    if (uart_base != 0) {
        printk("user-test: uart_base=%xu init_lookup=%xu new_lookup=%xu\n",
               uart_base,
               pgtbl_lookup(init_mm.pgdir, uart_base),
               pgtbl_lookup(mm->pgdir, uart_base));
    }

    printk("user-test: entering user mode at pc=%xu sp=%xu\n", regs->pc, regs->sp);
    printk("user-test: expected result is swi: ... mode=0x10 after user svc #0\n");

    pgtbl_switch_to(mm->pgdir);
    printk("off pc=%d off cpsr=%d size=%d\n",
        offsetof(struct pt_regs, pc),
        offsetof(struct pt_regs, cpsr),
        sizeof(struct pt_regs));
 
    pt_regs_dump(regs);
    arch_user_enter(regs);
}

int kernel_init(void *arg) {
    printk("kernel init \n");

    of_platform_populate(NULL,of_default_bus_match_table,NULL);
    driver_init();
    fs2_init();
    printk("kernel init end\n");
    char *argv[] = { "/proc1", NULL };
    do_execve(-1, "/proc1", argv, 0);
    // pgtbl_switch_selftest();
    // user_mode_smoke_test();
    // sched_kthread_test();
    return 0;
}



uint8_t is_init = 0;

void start_kernel(int cpuid,void *dtb) {
    local_irq_disable();
    if (cpuid == 0) {
        // printk("kernel init start\n");
        symbols_init();
		early_malloc_init();
		fdt_init(dtb);
        memblock_init();
        initial_mm_init();
        kmalloc_init();
        irq_init();
        time_init();
        sched_init();
      
        kernel_thread(kernel_init, "kernel_init", CLONE_FS);
        pid_t pid= kernel_thread(kthreadd, "kthreadd", CLONE_FS);
        kthreadd_task = find_task_by_pid(pid); 

        while(1) {
            sched();
            cpu_idle();
        }
        // is_init = 1;
    }

}
