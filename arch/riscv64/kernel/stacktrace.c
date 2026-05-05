#include <os/compiler_attributes.h>
#include <os/pfn.h>
#include <os/sched.h>
#include <os/stacktrace.h>
#include <mm/symbols.h>
#include <asm/stacktrace.h>

int notrace unwind_frame(struct stackframe *frame)
{
    unsigned long fp = frame->fp;
    unsigned long low = frame->sp;
    unsigned long high = ALIGN_UP(low, THREAD_SIZE);

    if (fp < low + 16 || fp > high) {
        return -1;
    }

    frame->sp = fp;
    frame->ra = *(unsigned long *)(fp - 8);
    frame->fp = *(unsigned long *)(fp - 16);
    frame->pc = frame->ra;
    return 0;
}

void notrace walk_stackframe(struct stackframe *frame,
                             int (*fn)(struct stackframe *, void *), void *data)
{
    while (1) {
        if (fn(frame, data)) {
            break;
        }
        if (unwind_frame(frame) < 0) {
            break;
        }
    }
}

struct stack_trace_data {
    struct stack_trace *trace;
    unsigned long last_pc;
    unsigned int no_sched_functions;
    unsigned int skip;
};

static int save_trace(struct stackframe *frame, void *d)
{
    struct stack_trace_data *data = d;
    struct stack_trace *trace = data->trace;
    unsigned long addr = frame->pc;

    if (data->no_sched_functions && in_sched_functions(addr)) {
        return 0;
    }
    if (data->skip) {
        data->skip--;
        return 0;
    }

    trace->entries[trace->nr_entries++] = addr;
    if (trace->nr_entries >= trace->max_entries) {
        return 1;
    }

    data->last_pc = addr;
    return 0;
}

static noinline void __save_stack_trace(struct task_struct *tsk,
                                        struct stack_trace *trace,
                                        unsigned int nosched)
{
    struct stack_trace_data data;
    struct stackframe frame;

    data.trace = trace;
    data.last_pc = UINT_MAX;
    data.skip = trace->skip;
    data.no_sched_functions = nosched;

    if (tsk != current) {
        frame.fp = thread_saved_fp(tsk);
        frame.sp = thread_saved_sp(tsk);
        frame.ra = 0;
        frame.pc = thread_saved_pc(tsk);
    } else {
        data.skip += 2;
        frame.fp = (unsigned long)__builtin_frame_address(0);
        frame.sp = current_stack_pointer;
        frame.ra = (unsigned long)__builtin_return_address(0);
        frame.pc = (unsigned long)__save_stack_trace;
    }

    walk_stackframe(&frame, save_trace, &data);
    if (trace->nr_entries < trace->max_entries) {
        trace->entries[trace->nr_entries++] = UINT_MAX;
    }
}

void save_stack_trace_regs(struct pt_regs *regs, struct stack_trace *trace)
{
    struct stack_trace_data data;
    struct stackframe frame;

    data.trace = trace;
    data.last_pc = UINT_MAX;
    data.skip = trace->skip;
    data.no_sched_functions = 0;

    frame.fp = regs->s0;
    frame.sp = regs->sp;
    frame.ra = regs->ra;
    frame.pc = regs->sepc;

    walk_stackframe(&frame, save_trace, &data);
    if (trace->nr_entries < trace->max_entries) {
        trace->entries[trace->nr_entries++] = UINT_MAX;
    }
}

void save_stack_trace_tsk(struct task_struct *tsk, struct stack_trace *trace)
{
    __save_stack_trace(tsk, trace, 1);
}

void save_stack_trace(struct stack_trace *trace)
{
    __save_stack_trace(current, trace, 0);
}
