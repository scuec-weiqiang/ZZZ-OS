#include <os/stacktrace.h>
#include <os/printk.h>

void print_stack_trace(struct stack_trace *trace) {
    // 打印结果
	printk("stack backtrace:\n");
	for (int i = 0; i < trace->nr_entries; i++) {
		printk("  [<%xu>] %xS\n", trace->entries[i], (void *)trace->entries[i]);
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