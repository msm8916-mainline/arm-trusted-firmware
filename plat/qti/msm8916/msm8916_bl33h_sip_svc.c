/*
 * Copyright (c) 2018, ARM Limited and Contributors. All rights reserved.
 * Copyright (c) 2018-2020, The Linux Foundation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <common/debug.h>
#include <common/runtime_svc.h>
#include <context.h>
#include <lib/utils_def.h>
#include <smccc_helpers.h>
#include <tools_share/uuid.h>

static uintptr_t qti_sip_handler(uint32_t smc_fid,
				 u_register_t x1,
				 u_register_t x2,
				 u_register_t x3,
				 u_register_t x4,
				 void *cookie, void *handle, u_register_t flags)
{
	u_register_t x5 = SMC_GET_GP(handle, CTX_GPREG_X5);
	smc_args_t args;

	/* Forward to TZ */
	args = smc_helper(smc_fid, x1, x2, x3, x4, x5, 0, 0);
	SMC_RET3(handle, args._regs[0], args._regs[1], args._regs[2]);
	return (uintptr_t) handle;
}

/* Define a runtime service descriptor for both fast & yield SiP calls */
DECLARE_RT_SVC(qti_sip_fast_svc, OEN_SIP_START,
	       OEN_SIP_END, SMC_TYPE_FAST, NULL, qti_sip_handler);

DECLARE_RT_SVC(qti_sip_yield_svc, OEN_SIP_START,
	       OEN_SIP_END, SMC_TYPE_YIELD, NULL, qti_sip_handler);
