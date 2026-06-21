// SPDX-License-Identifier: GPL-2.0

#include <linux/ipc_namespace.h>
#include <linux/mm.h>

struct ipc_namespace* get_init_ipc_ns_ptr(void);
void zap_page_range_single(struct vm_area_struct *vma, unsigned long address,
			    unsigned long size, struct zap_details *details);
