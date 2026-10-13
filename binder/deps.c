#include <linux/task_work.h>
#include <linux/sched.h>
#include <linux/file.h>
#include <linux/fdtable.h>
#include <linux/atomic.h>
#include <linux/ipc_namespace.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/kallsyms.h>
#include <linux/kprobes.h>
#include <linux/linkage.h>
#include <linux/objtool.h>
#include <linux/version.h>
#include <linux/task_work.h>

#include "deps.h"

#ifdef CONFIG_X86_KERNEL_IBT
/*
 * With IBT the kernel seals the ENDBR64 of functions it never calls
 * indirectly (kallsyms_lookup_name, can_nice, the security_binder_* hooks...),
 * so an indirect call to an address looked up here raises #CP (kernel BUG at
 * arch/x86/kernel/cet.c). Reach them with push+ret instead, which IBT does
 * not track; the callee then returns straight to our caller.
 */
unsigned long binder_ibt_call(void *fn, unsigned long a, unsigned long b,
			      unsigned long c, unsigned long d);
asm(".pushsection .text\n"
    ".type binder_ibt_call, @function\n"
    "binder_ibt_call:\n"
    "	mov %rdi, %r11\n"
    "	mov %rsi, %rdi\n"
    "	mov %rdx, %rsi\n"
    "	mov %rcx, %rdx\n"
    "	mov %r8, %rcx\n"
    "	push %r11\n"
    "	" ASM_RET
    ".size binder_ibt_call, .-binder_ibt_call\n"
    ".popsection\n");
STACK_FRAME_NON_STANDARD(binder_ibt_call);
#define IBT_ARGS(a, b, c, d, ...) \
	(unsigned long)(a), (unsigned long)(b), (unsigned long)(c), (unsigned long)(d)
#define deps_call(fn, ...) binder_ibt_call((void *)(fn), IBT_ARGS(__VA_ARGS__, 0, 0, 0, 0))
#else
#define deps_call(fn, ...) (fn)(__VA_ARGS__)
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,7,0))

#ifndef CONFIG_KPROBES
# error "Your kernel does not support KProbes, but this is required to compile binder as a kernel module on kernel 5.7 and later"
#endif

typedef unsigned long (*kallsyms_lookup_name_t)(const char *name);

static int dummy_kprobe_handler(struct kprobe *p, struct pt_regs *regs)
{
	return 0;
}

static kallsyms_lookup_name_t get_kallsyms_lookup_name_ptr(void)
{
	struct kprobe probe;
	int ret;
	kallsyms_lookup_name_t addr;

	memset(&probe, 0, sizeof(probe));
	probe.pre_handler = dummy_kprobe_handler;
	probe.symbol_name = "kallsyms_lookup_name";
	ret = register_kprobe(&probe);
	if (ret)
		return NULL;
	addr = (kallsyms_lookup_name_t) probe.addr;
	unregister_kprobe(&probe);

	return addr;
}
#endif

/*
 * On kernel 5.7 and later, kallsyms_lookup_name() can no longer be called from a kernel
 * module for reasons described here: https://lwn.net/Articles/813350/
 * As binder really needs to use kallsysms_lookup_name() to access some kernel
 * functions that otherwise wouldn't be accessible, KProbes are used on later
 * kernels to get the address of kallsysms_lookup_name(). The function is
 * afterwards used just as before. This is a very dirty hack though and the much
 * better solution would be if all the functions that are currently resolved
 * with kallsysms_lookup_name() would get an EXPORT_SYMBOL() annotation to
 * make them directly accessible to kernel modules.
 */
static unsigned long kallsyms_lookup_name_wrapper(const char *name)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,7,0))
	static kallsyms_lookup_name_t func_ptr = NULL;
	if (!func_ptr)
		func_ptr = get_kallsyms_lookup_name_ptr();

	return deps_call(func_ptr, name);
#else
	return kallsyms_lookup_name(name);
#endif
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,19,0))
static struct file *(*close_fd_get_file_ptr)(unsigned int fd)
#else
static int (*close_fd_get_file_ptr)(unsigned int fd, struct file **res)
#endif
        = NULL;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6,8,0))
struct file *file_close_fd(unsigned int fd)
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(5,19,0))
struct file *close_fd_get_file(unsigned int fd)
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(5,11,0))
int close_fd_get_file(unsigned int fd, struct file **res)
#else
int __close_fd_get_file(unsigned int fd, struct file **res)
#endif
{
	if (!close_fd_get_file_ptr)
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6,8,0))
		close_fd_get_file_ptr = kallsyms_lookup_name_wrapper("file_close_fd");
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(5,11,0))
		close_fd_get_file_ptr = kallsyms_lookup_name_wrapper("close_fd_get_file");
#else
		close_fd_get_file_ptr = kallsyms_lookup_name_wrapper("__close_fd_get_file");
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,19,0))
    return (struct file *)deps_call(close_fd_get_file_ptr, fd);
#else
	return (int)deps_call(close_fd_get_file_ptr, fd, res);
#endif
}

static int (*can_nice_ptr)(const struct task_struct *, const int) = NULL;

int can_nice(const struct task_struct *p, const int nice)
{
	if (!can_nice_ptr)
		can_nice_ptr = kallsyms_lookup_name_wrapper("can_nice");
	return (int)deps_call(can_nice_ptr, p, nice);
}

static void (*mmput_async_ptr)(struct mm_struct *mm) = NULL;

void mmput_async(struct mm_struct *mm)
{
	if (!mmput_async_ptr)
		mmput_async_ptr = kallsyms_lookup_name_wrapper("mmput_async");
	deps_call(mmput_async_ptr, mm);
}

static int (*security_binder_set_context_mgr_ptr)(struct task_struct *mgr) = NULL;

int security_binder_set_context_mgr(struct task_struct *mgr)
{
	if (!security_binder_set_context_mgr_ptr)
		security_binder_set_context_mgr_ptr = kallsyms_lookup_name_wrapper("security_binder_set_context_mgr");
	return (int)deps_call(security_binder_set_context_mgr_ptr, mgr);
}

static int (*security_binder_transaction_ptr)(struct task_struct *from, struct task_struct *to) = NULL;

int security_binder_transaction(struct task_struct *from, struct task_struct *to)
{
	if (!security_binder_transaction_ptr)
		security_binder_transaction_ptr = kallsyms_lookup_name_wrapper("security_binder_transaction");
	return (int)deps_call(security_binder_transaction_ptr, from, to);
}

static int (*security_binder_transfer_binder_ptr)(struct task_struct *from, struct task_struct *to) = NULL;

int security_binder_transfer_binder(struct task_struct *from, struct task_struct *to)
{
	if (!security_binder_transfer_binder_ptr)
		security_binder_transfer_binder_ptr = kallsyms_lookup_name_wrapper("security_binder_transfer_binder");
	return (int)deps_call(security_binder_transfer_binder_ptr, from, to);
}

static int (*security_binder_transfer_file_ptr)(struct task_struct *from, struct task_struct *to, struct file *file) = NULL;

int security_binder_transfer_file(struct task_struct *from, struct task_struct *to, struct file *file)
{
	if (!security_binder_transfer_file_ptr)
		security_binder_transfer_file_ptr = kallsyms_lookup_name_wrapper("security_binder_transfer_file");
	return (int)deps_call(security_binder_transfer_file_ptr, from, to, file);
}

static int (*task_work_add_ptr)(struct task_struct *task, struct callback_head *work,
		  enum task_work_notify_mode notify) = NULL;

int task_work_add(struct task_struct *task, struct callback_head *work,
		  enum task_work_notify_mode notify)
{
	if (!task_work_add_ptr)
		task_work_add_ptr = kallsyms_lookup_name_wrapper("task_work_add");
	return (int)deps_call(task_work_add_ptr, task, work, notify);
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6,3,0))
static void (*zap_page_range_single_ptr)(struct vm_area_struct *, unsigned long, unsigned long, struct zap_details *) = NULL;

void zap_page_range_single(struct vm_area_struct *vma, unsigned long address, unsigned long size, struct zap_details *details)
{
	if (!zap_page_range_single_ptr)
		zap_page_range_single_ptr = kallsyms_lookup_name_wrapper("zap_page_range_single");
	deps_call(zap_page_range_single_ptr, vma, address, size, details);
}
#else
static void (*zap_page_range_ptr)(struct vm_area_struct *, unsigned long, unsigned long) = NULL;

void zap_page_range(struct vm_area_struct *vma, unsigned long address, unsigned long size)
{
	if (!zap_page_range_ptr)
		zap_page_range_ptr = kallsyms_lookup_name_wrapper("zap_page_range");
	deps_call(zap_page_range_ptr, vma, address, size);
}
#endif

static void (*put_ipc_ns_ptr)(struct ipc_namespace *ns) = NULL;

void put_ipc_ns(struct ipc_namespace *ns)
{
	if (!put_ipc_ns_ptr)
		put_ipc_ns_ptr = kallsyms_lookup_name_wrapper("put_ipc_ns");
	deps_call(put_ipc_ns_ptr, ns);
}

static struct ipc_namespace *init_ipc_ns_ptr = NULL;

struct ipc_namespace *get_init_ipc_ns_ptr(void)
{
	if (!init_ipc_ns_ptr)
		init_ipc_ns_ptr = kallsyms_lookup_name_wrapper("init_ipc_ns");
	return init_ipc_ns_ptr;
}
