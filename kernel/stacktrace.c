#include <os/stacktrace.h>
#include <os/printk.h>
#include <os/kallsyms.h>

void print_stack_trace(struct stack_trace *trace) {
    unsigned int i;

	printk("stack backtrace:\n");
	for (i = 0; i < trace->nr_entries; i++) {
        unsigned long addr = trace->entries[i];
        unsigned long offset = 0;
        const char *name;

        if (addr == UINT_MAX) {
            break;
        }

        name = lookup_symbol_name(addr, &offset);
        if (name != NULL) {
		    printk("  [<%lx>] %s+0x%lx\n", addr, name, offset);
        } else {
		    printk("  [<%lx>] <unknown>\n", addr);
        }
	}
}

void dump_stack(void) {
	unsigned long buf[16];
	struct stack_trace trace;

	trace.nr_entries = 0;
	trace.max_entries = 16;
	trace.entries = buf;
	trace.skip = 1;  // 跳过 dump_stack 自己

	// 执行栈回溯
	save_stack_trace(&trace);
    print_stack_trace(&trace);

}
