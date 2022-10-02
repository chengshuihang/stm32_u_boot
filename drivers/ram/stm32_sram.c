// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2017, STMicroelectronics - All Rights Reserved
 * Author(s): Vikas Manocha, <vikas.manocha@st.com> for STMicroelectronics.
 */

#define LOG_CATEGORY UCLASS_RAM

#include <common.h>
#include <clk.h>
#include <dm.h>
#include <init.h>
#include <log.h>
#include <ram.h>
#include <asm/io.h>
#include <dm/device_compat.h>
#include <linux/bitops.h>
#include <linux/delay.h>

#define MEM_MODE_MASK	GENMASK(2, 0)
#define SWP_FMC_OFFSET 10
#define SWP_FMC_MASK	GENMASK(SWP_FMC_OFFSET+1, SWP_FMC_OFFSET)
#define NOT_FOUND	0xff

struct stm32_fmc_regs {
	/* 0x0 */
	u32 bcr1;	/* NOR/PSRAM Chip select control register 1 */
	u32 btr1;	/* SRAM/NOR-Flash Chip select timing register 1 */
	u32 bcr2;	/* NOR/PSRAM Chip select Control register 2 */
	u32 btr2;	/* SRAM/NOR-Flash Chip select timing register 2 */
	u32 bcr3;	/* NOR/PSRAMChip select Control register 3 */
	u32 btr3;	/* SRAM/NOR-Flash Chip select timing register 3 */
	u32 bcr4;	/* NOR/PSRAM Chip select Control register 4 */
	u32 btr4;	/* SRAM/NOR-Flash Chip select timing register 4 */
	u32 reserved1[24];

	/* 0x80 */
	u32 pcr;	/* NAND Flash control register */
	u32 sr;		/* FIFO status and interrupt register */
	u32 pmem;	/* Common memory space timing register */
	u32 patt;	/* Attribute memory space timing registers  */
	u32 reserved2[1];
	u32 eccr;	/* ECC result registers */
	u32 reserved3[27];

	/* 0x104 */
	u32 bwtr1;	/* SRAM/NOR-Flash write timing register 1 */
	u32 reserved4[1];
	u32 bwtr2;	/* SRAM/NOR-Flash write timing register 2 */
	u32 reserved5[1];
	u32 bwtr3;	/* SRAM/NOR-Flash write timing register 3 */
	u32 reserved6[1];
	u32 bwtr4;	/* SRAM/NOR-Flash write timing register 4 */
	u32 reserved7[8];

	/* 0x140 */
	u32 sdcr1;	/* SDRAM Control register 1 */
	u32 sdcr2;	/* SDRAM Control register 2 */
	u32 sdtr1;	/* SDRAM Timing register 1 */
	u32 sdtr2;	/* SDRAM Timing register 2 */
	u32 sdcmr;	/* SDRAM Mode register */
	u32 sdrtr;	/* SDRAM Refresh timing register */
	u32 sdsr;	/* SDRAM Status register */
};

/*
 * NOR/PSRAM Control register BCR1
 * FMC controller Enable, only availabe for H7
 */
#define FMC_BCR1_FMCEN		BIT(31)

/* Control register BCR */
#define FMC_BCR_CBURSTRW_SHIFT	19	/* 写入突发使能 shift */
#define FMC_BCR_CPSIZE_SHIFT    16  /* CRAM page size shift */
#define FMC_BCR_ASYNCWAIT_SHIFT	15	/* 异步传输期间的等待信号 shift */
#define FMC_BCR_EXTMOD_SHIFT	14	/* 扩展模式使能 shift */
#define FMC_BCR_WAITEN_SHIFT	13	/* 等待使能位 shift */
#define FMC_BCR_WREN_SHIFT	    12	/* 写入使能位 shift */
#define FMC_BCR_WAITCFG_SHIFT	11	/* 等待时序配置 shift */
#define FMC_BCR_WRAPMOD_SHIFT	10	/* 环回突发模式支持 shift */
#define FMC_BCR_WAITPOL_SHIFT	9	/* 等待信号极性位 shift */
#define FMC_BCR_BURSTEN_SHIFT	8	/* 突发使能位 shift */
#define FMC_BCR_FACCEN_SHIFT	6	/* flash访问使能 shift */
#define FMC_BCR_MWID_SHIFT	    4	/* 存储器数据总线宽度 shift */
#define FMC_BCR_MTYP_SHIFT	    2	/* 存储器类型 shift */
#define FMC_BCR_MUXEN_SHIFT	    1	/* 地址/数据复用使能位 shift */
#define FMC_BCR_MBKEN_SHIFT	    0	/* 存储区域使能位 shift */

/* Timings register BTR */
#define FMC_BTR_ACCMOD_SHIFT	28	/* 访问模式 shift */
#define FMC_BTR_DATLAT_SHIFT	24	/* 同步突发NOR FLASH的数据延迟 shift */
#define FMC_BTR_CLKDIV_SHIFT	20	/* CLK信号的时钟分频比 shift */
#define FMC_BTR_BUSTURN_SHIFT	16	/* 总线周转阶段的持续时间 shift */
#define FMC_BTR_DATAST_SHIFT	8	/* 数据阶段的持续时间 shift */
#define FMC_BTR_ADDHLD_SHIFT	4	/* 地址保持阶段的持续时间 shift */
#define FMC_BTR_ADDSET_SHIFT	0	/* 地址设置阶段的持续时间 shift */

/* Timings register BWTR */
#define FMC_BWTR_ACCMOD_SHIFT	28	/* 访问模式 shift */
#define FMC_BWTR_DATLAT_SHIFT	24	/* 同步突发NOR FLASH的数据延迟 shift */
#define FMC_BWTR_CLKDIV_SHIFT	20	/* CLK信号的时钟分频比 shift */
#define FMC_BWTR_BUSTURN_SHIFT	16	/* 总线周转阶段的持续时间 shift */
#define FMC_BWTR_DATAST_SHIFT	8	/* 数据阶段的持续时间 shift */
#define FMC_BWTR_ADDHLD_SHIFT	4	/* 地址保持阶段的持续时间 shift */
#define FMC_BWTR_ADDSET_SHIFT	0	/* 地址设置阶段的持续时间 shift */

#define FMC_SDCMR_NRFS_SHIFT	5

#define FMC_SDCMR_MODE_NORMAL		0
#define FMC_SDCMR_MODE_START_CLOCK	1
#define FMC_SDCMR_MODE_PRECHARGE	2
#define FMC_SDCMR_MODE_AUTOREFRESH	3
#define FMC_SDCMR_MODE_WRITE_MODE	4
#define FMC_SDCMR_MODE_SELFREFRESH	5
#define FMC_SDCMR_MODE_POWERDOWN	6

#define FMC_SDCMR_BANK_1		BIT(4)
#define FMC_SDCMR_BANK_2		BIT(3)

#define FMC_SDCMR_MODE_REGISTER_SHIFT	9

#define FMC_SDSR_BUSY			BIT(5)

#define FMC_BUSY_WAIT(regs)	do { \
		__asm__ __volatile__ ("dsb" : : : "memory"); \
		while (regs->sdsr & FMC_SDSR_BUSY) \
			; \
	} while (0)

struct stm32_sram_control {
	u8 write_burst_enable;
	u8 page_size;
	u8 wait_signal_during_asynchronous_transfers;
	u8 extended_mode_enable;
	u8 wait_enable_bit;
	u8 write_enable_bit;
	u8 wait_timing_configuration;
	u8 wrapped_burst_mode_support;
	u8 wait_signal_polarity_bit;
	u8 burst_enable_bit;
	u8 memory_databus_width;
	u8 memory_type;
	u8 address_data_multiplexing_enable_bit;
};

struct stm32_sram_timing {
	u8 access_mode;
	u8 data_latency;
	u8 clock_divide_ratio;
	u8 bus_turnaround_phase_duration;
	u8 data_phase_duration;
	u8 address_hold_phase_duration;
	u8 address_setup_phase_duration;
};

struct stm32_sram_exttiming {
	u8 access_mode;
	u8 bus_turnaround_phase_duration;
	u8 data_phase_duration;
	u8 address_hold_phase_duration;
	u8 address_setup_phase_duration;
};

enum stm32_fmc_sram_bank {
	SRAM_BANK1,
	SRAM_BANK2,
	SRAM_BANK3,
	SRAM_BANK4,
	MAX_SRAM_BANK,
};

enum stm32_fmc_family {
	STM32F4_FMC,
	STM32H7_FMC,
};

struct bank_params {
	struct stm32_sram_control *sram_control;
	struct stm32_sram_timing *sram_timing;
	struct stm32_sram_exttiming *sram_exttiming;
	u8 sram_valid_flg;
	enum stm32_fmc_sram_bank target_bank;
};

struct stm32_sram_params {
	struct stm32_fmc_regs *base;
	struct bank_params bank_params[MAX_SRAM_BANK];
	enum stm32_fmc_family family;
};

#define SDRAM_MODE_BL_SHIFT	0
#define SDRAM_MODE_CAS_SHIFT	4
#define SDRAM_MODE_BL		0

int stm32_sram_init(struct udevice *dev)
{
	struct stm32_sram_params *params = dev_get_plat(dev);
	struct stm32_sram_control *control;
	struct stm32_sram_timing *timing;
	struct stm32_sram_exttiming *exttiming;
	struct stm32_fmc_regs *regs = params->base;
	enum stm32_fmc_sram_bank target_bank;
	u32 reg;
	u8 i;

	/* disable the FMC controller */
	if (params->family == STM32H7_FMC)
		clrbits_le32(&regs->bcr1, FMC_BCR1_FMCEN);

	for (i = 0; i < MAX_SRAM_BANK; i++) {
		if(params->bank_params[i].sram_valid_flg == 0u){
			continue;
		}
		control = params->bank_params[i].sram_control;
		timing = params->bank_params[i].sram_timing;
		exttiming = params->bank_params[i].sram_exttiming;
		target_bank = params->bank_params[i].target_bank;

		reg = regs->bcr1 + i * 8u;
		writel(control->write_burst_enable << FMC_BCR_CBURSTRW_SHIFT
			| control->page_size << FMC_BCR_CPSIZE_SHIFT
			| control->wait_signal_during_asynchronous_transfers << FMC_BCR_ASYNCWAIT_SHIFT
			| control->extended_mode_enable << FMC_BCR_EXTMOD_SHIFT
			| control->wait_enable_bit << FMC_BCR_WAITEN_SHIFT
			| control->write_enable_bit << FMC_BCR_WREN_SHIFT
			| control->wait_timing_configuration << FMC_BCR_WAITCFG_SHIFT
			| control->wrapped_burst_mode_support << FMC_BCR_WRAPMOD_SHIFT
			| control->wait_signal_polarity_bit << FMC_BCR_WAITPOL_SHIFT
			| control->burst_enable_bit << FMC_BCR_BURSTEN_SHIFT
			| control->memory_databus_width << FMC_BCR_MWID_SHIFT
			| control->memory_type << FMC_BCR_MTYP_SHIFT
			| control->address_data_multiplexing_enable_bit << FMC_BCR_MUXEN_SHIFT,
			&reg);

		reg = regs->btr1 + i * 8u;
		writel(timing->access_mode << FMC_BTR_ACCMOD_SHIFT
			| timing->data_latency << FMC_BTR_DATLAT_SHIFT
			| timing->clock_divide_ratio << FMC_BTR_CLKDIV_SHIFT
			| timing->bus_turnaround_phase_duration << FMC_BTR_BUSTURN_SHIFT
			| timing->data_phase_duration << FMC_BTR_DATAST_SHIFT
			| timing->address_hold_phase_duration << FMC_BTR_ADDHLD_SHIFT
			| timing->address_setup_phase_duration << FMC_BTR_ADDSET_SHIFT,
			&reg);

		reg = regs->bwtr1 + i * 8u;
		writel(exttiming->access_mode << FMC_BWTR_ACCMOD_SHIFT
			| exttiming->bus_turnaround_phase_duration << FMC_BWTR_BUSTURN_SHIFT
			| exttiming->data_phase_duration << FMC_BWTR_DATAST_SHIFT
			| exttiming->address_hold_phase_duration << FMC_BWTR_ADDHLD_SHIFT
			| exttiming->address_setup_phase_duration << FMC_BWTR_ADDSET_SHIFT,
			&reg);

		reg = regs->bcr1 + i * 8u;
		setbits_le32(&reg, FMC_BCR_MBKEN_SHIFT);
	}

	/* enable the FMC controller */
	if (params->family == STM32H7_FMC)
		setbits_le32(&regs->bcr1, FMC_BCR1_FMCEN);

	return 0;
}

static int stm32_fmc_sram_of_to_plat(struct udevice *dev)
{
	struct stm32_sram_params *params = dev_get_plat(dev);
	struct bank_params *bank_params;
	ofnode bank_node;
	char *bank_name;
	char _bank_name[128] = {0};
	u8 bank = 0;

	dev_for_each_subnode(bank_node, dev) {
		params->bank_params[bank].sram_valid_flg = 0u;
		/* extract the bank index from DT */
		bank_name = (char *)ofnode_get_name(bank_node);
		strlcpy(_bank_name, bank_name, sizeof(_bank_name));
		bank_name = (char *)_bank_name;
		strsep(&bank_name, "@");
		if (!bank_name) {
			pr_err("missing sdram bank index");
			return -EINVAL;
		}

		bank_params = &params->bank_params[bank];
		strict_strtoul(bank_name, 10,
			       (long unsigned int *)&bank_params->target_bank);

		if (bank_params->target_bank >= MAX_SRAM_BANK) {
			pr_err("Found bank %d , but only bank 0,1,2 and 3 are supported",
			      bank_params->target_bank);
			return -EINVAL;
		}

		debug("Find bank %s %u\n", bank_name, bank_params->target_bank);

		params->bank_params[bank].sram_control =
			(struct stm32_sram_control *)
			 ofnode_read_u8_array_ptr(bank_node,
						  "st,sram-control",
						  sizeof(struct stm32_sram_control));

		if (!params->bank_params[bank].sram_control) {
			pr_err("st,sram-control not found for %s",
			      ofnode_get_name(bank_node));
			return -EINVAL;
		}


		params->bank_params[bank].sram_timing =
			(struct stm32_sram_timing *)
			 ofnode_read_u8_array_ptr(bank_node,
						  "st,sram-timing",
						  sizeof(struct stm32_sram_timing));

		if (!params->bank_params[bank].sram_timing) {
			pr_err("st,sram-timing not found for %s",
			      ofnode_get_name(bank_node));
			return -EINVAL;
		}

		params->bank_params[bank].sram_exttiming =
			(struct stm32_sram_exttiming *)
			 ofnode_read_u8_array_ptr(bank_node,
						  "st,sram-exttiming",
						  sizeof(struct stm32_sram_exttiming));

		if (!params->bank_params[bank].sram_exttiming) {
			pr_err("st,sram-exttiming not found for %s",
			      ofnode_get_name(bank_node));
			return -EINVAL;
		}

		bank_params->sram_valid_flg = 1u;

		dev_dbg(dev, "bank %d active!\n", bank);
		bank++;
	}

	return 0;
}

static int stm32_fmc_sram_probe(struct udevice *dev)
{
	struct stm32_sram_params *params = dev_get_plat(dev);
	int ret;
	fdt_addr_t addr;

	addr = dev_read_addr(dev);
	if (addr == FDT_ADDR_T_NONE)
		return -EINVAL;

	params->base = (struct stm32_fmc_regs *)addr;
	params->family = dev_get_driver_data(dev);

#ifdef CONFIG_CLK
	struct clk clk;

	ret = clk_get_by_index(dev, 0, &clk);
	if (ret < 0)
		return ret;

	ret = clk_enable(&clk);

	if (ret) {
		dev_err(dev, "failed to enable clock\n");
		return ret;
	}
#endif
	ret = stm32_sram_init(dev);
	if (ret)
		return ret;

	return 0;
}

static int stm32_fmc_sram_get_info(struct udevice *dev, struct ram_info *info)
{
	return 0;
}

static struct ram_ops stm32_fmc_sram_ops = {
	.get_info = stm32_fmc_sram_get_info,
};

static const struct udevice_id stm32_fmc_sram_ids[] = {
	{ .compatible = "st,stm32-fmc-sram", .data = STM32F4_FMC },
	{ }
};

U_BOOT_DRIVER(stm32_fmc_sram) = {
	.name = "stm32_fmc_sram",
	.id = UCLASS_RAM,
	.of_match = stm32_fmc_sram_ids,
	.ops = &stm32_fmc_sram_ops,
	.of_to_plat = stm32_fmc_sram_of_to_plat,
	.probe = stm32_fmc_sram_probe,
	.plat_auto	= sizeof(struct stm32_sram_params),
};
