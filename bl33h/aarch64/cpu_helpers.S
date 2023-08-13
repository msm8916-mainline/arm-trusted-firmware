/*
 * Copyright (c) 2014-2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch.h>
#include <asm_macros.S>
#include <assert_macros.S>
#include <common/bl_common.h>
#include <common/debug.h>
#include <cpu_macros.S>
#include <lib/cpus/cpu_ops.h>
#include <lib/cpus/errata.h>
#include <lib/el3_runtime/cpu_data.h>

	/*
	 * The reset handler common to all platforms.  After a matching
	 * cpu_ops structure entry is found, the correponding reset_handler
	 * in the cpu_ops is invoked.
	 * Clobbers: x0 - x19, x30
	 */
	.globl	reset_handler
func reset_handler
	b	plat_reset_handler
endfunc reset_handler

	/*
	 * void prepare_cpu_pwr_dwn(unsigned int power_level)
	 *
	 * Prepare CPU power down function for all platforms. The function takes
	 * a domain level to be powered down as its parameter. After the cpu_ops
	 * pointer is retrieved from cpu_data, the handler for requested power
	 * level is called.
	 */
	.globl	prepare_cpu_pwr_dwn
func prepare_cpu_pwr_dwn
	ret
endfunc prepare_cpu_pwr_dwn


	/*
	 * Initializes the cpu_ops_ptr if not already initialized
	 * in cpu_data. This can be called without a runtime stack, but may
	 * only be called after the MMU is enabled.
	 * clobbers: x0 - x6, x10
	 */
	.globl	init_cpu_ops
func init_cpu_ops
	ret
endfunc init_cpu_ops

#if CRASH_REPORTING
	/*
	 * The cpu specific registers which need to be reported in a crash
	 * are reported via cpu_ops cpu_reg_dump function. After a matching
	 * cpu_ops structure entry is found, the correponding cpu_reg_dump
	 * in the cpu_ops is invoked.
	 */
	.globl	do_cpu_reg_dump
func do_cpu_reg_dump
	ret
endfunc do_cpu_reg_dump
#endif

	.globl print_errata_status
func print_errata_status
	ret
endfunc print_errata_status
