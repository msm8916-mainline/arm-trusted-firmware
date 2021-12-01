/*
 * Copyright (c) 2021, Stephan Gerhold <stephan@gerhold.net>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <lib/mmio.h>
#include <lib/utils_def.h>

#include "msm8916_pm.h"

#define SAW_CFG				0x008
#define SAW_SPM_CTL			0x030
#define SAW_SPM_DLY			0x034
#define SAW_SPM_SLP_SEQ_ENTRY_V3	0x400

#define SAW_CFG_CLK_DIV(div)		(((div) - 1) << U(0))

#define SAW_SPM_CTL_EN			BIT_32(0)
#define SAW_SPM_CTL_START_ADR(n)	((n) << U(4))
#define SAW_SPM_CTL_SYS_PC_MODE		BIT_32(16)

#define SAW_SPM_DLY_1(ticks)		((ticks) << U(0))
#define SAW_SPM_DLY_2(ticks)		((ticks) << U(10))
#define SAW_SPM_DLY_3(ticks)		((ticks) << U(20))

#define SAW_ENTRY_SEQ_MAX_LEN		64

struct spm_config {
	uint32_t cfg;
	uint32_t dly;
	uint8_t entry_seq[SAW_ENTRY_SEQ_MAX_LEN];
	uint8_t pc_start_adr;
};

#include "msm8916_spm_config.c"

static void spm_write_seq(uintptr_t base, const uint32_t *seq)
{
	uintptr_t off;

	for (off = 0; off < SAW_ENTRY_SEQ_MAX_LEN; off += sizeof(*seq), seq++) {
		mmio_write_32(base + SAW_SPM_SLP_SEQ_ENTRY_V3 + off, *seq);
	}
}

static void spm_init(uintptr_t base, const struct spm_config *sc)
{
	spm_write_seq(base, (const uint32_t *)sc->entry_seq);
	mmio_write_32(base + SAW_CFG, sc->cfg);
	mmio_write_32(base + SAW_SPM_DLY, sc->dly);
	mmio_write_32(base + SAW_SPM_CTL, SAW_SPM_CTL_EN);
}

static void spm_set_pc(uintptr_t base, const struct spm_config *sc)
{
	mmio_write_32(base + SAW_SPM_CTL,
		      SAW_SPM_CTL_EN | SAW_SPM_CTL_SYS_PC_MODE |
		      SAW_SPM_CTL_START_ADR(sc->pc_start_adr));
}

void msm8916_spm_cpu_init(uintptr_t base)
{
	spm_init(base, &msm8916_spm_v3_cpu);
}

void msm8916_spm_cpu_set_pc(uintptr_t base)
{
	spm_set_pc(base, &msm8916_spm_v3_cpu);
}
