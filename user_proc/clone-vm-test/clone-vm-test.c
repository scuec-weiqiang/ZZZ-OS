#include <stdint.h>
#include <stdio.h>

#define SYS_exit 93
#define SYS_clone 220
#define SYS_wait4 260

#define SIGCHLD 17

#define CLONE_VM 0x00000100UL
#define CLONE_SETTLS 0x00080000UL
#define CLONE_PARENT_SETTID 0x00100000UL
#define CLONE_CHILD_CLEARTID 0x00200000UL
#define CLONE_CHILD_SETTID 0x01000000UL

#define STACK_SIZE 8192
#define SHARED_MAGIC 0x5a5a1234

static unsigned char child_stack[STACK_SIZE] __attribute__((aligned(16)));
static unsigned long tls_area[8] __attribute__((aligned(16)));

static volatile int shared_value = 0x11111111;
static volatile int parent_tid_slot = -1;
static volatile int child_tid_slot = -1;
static volatile unsigned long child_seen_tp;

static long raw_syscall4(long nr, long arg0, long arg1, long arg2, long arg3)
{
    register long a0 asm("a0") = arg0;
    register long a1 asm("a1") = arg1;
    register long a2 asm("a2") = arg2;
    register long a3 asm("a3") = arg3;
    register long a7 asm("a7") = nr;

    asm volatile("ecall"
                 : "+r"(a0)
                 : "r"(a1), "r"(a2), "r"(a3), "r"(a7)
                 : "memory");
    return a0;
}

static unsigned long read_tp(void)
{
    unsigned long tp;

    asm volatile("mv %0, tp" : "=r"(tp));
    return tp;
}

__attribute__((noreturn, noinline)) void clone_vm_child_entry(void)
{
    child_seen_tp = read_tp();
    shared_value = SHARED_MAGIC;

    register long a0 asm("a0") = 23;
    register long a7 asm("a7") = SYS_exit;

    asm volatile("ecall" : : "r"(a0), "r"(a7) : "memory");
    for (;;) {
    }
}

static long clone_and_run_child(unsigned long flags, void *stack,
                                int *parent_tid, unsigned long tls,
                                int *child_tid)
{
    register long a0 asm("a0") = (long)flags;
    register long a1 asm("a1") = (long)stack;
    register long a2 asm("a2") = (long)parent_tid;
    register long a3 asm("a3") = (long)tls;
    register long a4 asm("a4") = (long)child_tid;
    register long a7 asm("a7") = SYS_clone;

    asm volatile(
        "ecall\n"
        "bnez a0, 1f\n"
        "call clone_vm_child_entry\n"
        "1:\n"
        : "+r"(a0)
        : "r"(a1), "r"(a2), "r"(a3), "r"(a4), "r"(a7)
        : "memory");

    return a0;
}

static int check_eq_long(const char *name, long got, long expected)
{
    if (got == expected) {
        printf("ok: %s = %ld\n", name, got);
        return 0;
    }

    printf("FAIL: %s got %ld expected %ld\n", name, got, expected);
    return 1;
}

static int check_eq_hex(const char *name, unsigned long got, unsigned long expected)
{
    if (got == expected) {
        printf("ok: %s = 0x%lx\n", name, got);
        return 0;
    }

    printf("FAIL: %s got 0x%lx expected 0x%lx\n", name, got, expected);
    return 1;
}

int main(void)
{
    unsigned long flags;
    unsigned long stack_top;
    long pid;
    long waited;
    int status = 0;
    int fails = 0;

    stack_top = (unsigned long)child_stack + sizeof(child_stack);
    stack_top &= ~0xfUL;

    flags = CLONE_VM | CLONE_SETTLS | CLONE_PARENT_SETTID |
            CLONE_CHILD_SETTID | CLONE_CHILD_CLEARTID | SIGCHLD;

    printf("clone-vm-test: flags=0x%lx\n", flags);

    pid = clone_and_run_child(flags, (void *)stack_top,
                              (int *)&parent_tid_slot,
                              (unsigned long)tls_area,
                              (int *)&child_tid_slot);
    if (pid < 0) {
        printf("FAIL: clone returned %ld\n", pid);
        printf("hint: -22 usually means the kernel rejected one clone flag\n");
        return 1;
    }

    printf("parent: child pid=%ld parent_tid=%d child_tid(before wait)=%d\n",
           pid, parent_tid_slot, child_tid_slot);

    waited = raw_syscall4(SYS_wait4, pid, (long)&status, 0, 0);
    printf("parent: wait4 ret=%ld status=0x%x\n", waited, status);

    fails += check_eq_long("wait4 ret", waited, pid);
    fails += check_eq_long("parent_tid", parent_tid_slot, pid);
    fails += check_eq_long("child_tid after exit", child_tid_slot, 0);
    fails += check_eq_hex("shared_value", shared_value, SHARED_MAGIC);
    fails += check_eq_hex("child tp", child_seen_tp, (unsigned long)tls_area);

    if (fails == 0) {
        printf("PASS\n");
        return 0;
    }

    printf("FAIL: %d checks failed\n", fails);
    return 1;
}
