/*
 * Copyright (c) 2014 - 2016, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited
 */

#include <reg.h>
#include <platform/iomap.h>
#include <trace.h>
#include <regulator.h>
#include <dt_common.h>
#include <err.h>
#include <assert.h>
#include <platform_reg.h>
#include <platform_c.h>
#include <string.h>
#include <tegra_exit.h>

#define LOCAL_TRACE 0
#define MINORREV_SHIFT			16
#define MINORREV_MASK			(0xF << 16)

#define MAJORREV_SHIFT			4
#define MAJORREV_MASK			(0xF << 4)
#define APB_MISC_GP_HIDREV_0	0x804
#define CHIPID_SHIFT			8
#define CHIPID_MASK				(0xFF)

#define MAIN_RST_BIT			4

static void *fdt;

enum platform_type get_platform_type(void)
{
	uint32_t reg_val = 0;
	uint32_t major_rev = 0;
	uint32_t minor_rev = 0;

	reg_val =  readl(APB_MISC_PA_BASE + APB_MISC_GP_HIDREV_0);

	major_rev = (reg_val & MAJORREV_MASK) >> MAJORREV_SHIFT;
	minor_rev = (reg_val & MINORREV_MASK) >> MINORREV_SHIFT;

	if (major_rev == 0) {
		if (minor_rev ==  0)
			return PLATFORM_QT;
		else
			return PLATFORM_FPGA;
	} else if ((major_rev > 0) && (major_rev < 15)) {
		return PLATFORM_SILICON;
	} else {
		return PLATFORM_INVALID;
	}
}

uint32_t get_chipid(void)
{
	uint32_t reg_val = 0;

	reg_val = readl(APB_MISC_PA_BASE + APB_MISC_GP_HIDREV_0);
	reg_val = reg_val >> CHIPID_SHIFT;
	reg_val = reg_val & CHIPID_MASK;

	return reg_val;
}

/* If not implemented on particular postform return "0" */
__WEAK uint32_t platform_find_core_voltage(void)
{
	return 0;
}

const char *get_basename(const char *path)
{
	const char *fname = strrchr(path, '/');
	return fname ? (fname + 1): path;
}

status_t platform_set_core_voltage(uint32_t volts)
{
	int32_t node_offset;
	int32_t r_phandle;
	const uint32_t *temp = NULL;
	status_t err = NO_ERROR;
	LTRACE_ENTRY;

	if (!fdt)
		fdt = dt_get_fdt_handle();

	ASSERT(volts);

	node_offset = fdt_node_offset_by_compatible(fdt, -1,
			"nvidia,tegra-supply-tests");

	if (!node_offset) {
		dprintf(CRITICAL, "Error unable to find tegra-supply-tests node\n");
		err = ERR_NOT_FOUND;
		goto fail;
	}

	/* get vdd-core-supply property */
	temp = fdt_getprop(fdt, node_offset,
			   "vdd-core-supply", NULL);
	if (temp != NULL) {
		r_phandle = fdt32_to_cpu(*temp);
	} else {
		dprintf(CRITICAL, "Error unable to get vdd-core-supply property\n");
		err = ERR_NOT_FOUND;
		goto fail;
	}

	dprintf(INFO, "set vdd_core voltage to %u mv\n", volts);
	regulator_set_voltage(r_phandle, volts * 1000, USER_DEFINED_VOLTS);

fail:
	LTRACE_EXIT;
	return err;
}

void tegra_pmc_reset(void)
{
	uint32_t reg = 0;
	reg = readl(PMC_BASE_ADDRESS + PMC_CNTRL);
	reg |= (1 << MAIN_RST_BIT);
	writel(reg, PMC_BASE_ADDRESS + PMC_CNTRL);
}

void tegra_boot_recovery_mode(void)
{
	/* set recovery bit */
	tegra_set_pmc_scratch0(PMC_SCRATCH0_FORCED_RECOVERY_FLAG, true);

	/* reset the board */
	tegra_reset();
}

status_t tegra_set_pmc_scratch0(scratch0_flag flag, bool to_set)
{
	uint32_t reg;

	reg = readl(PMC_BASE_ADDRESS + PMC_SCRATCH0);
	switch(flag) {
		case PMC_SCRATCH0_FORCED_RECOVERY_FLAG:
		case PMC_SCRATCH0_BOOT_RECOVERY_KERNEL_FLAG:
		case PMC_SCRATCH0_FASTBOOT_FLAG:
			if (to_set)
				reg |= (1 << flag);
			else
				reg &= ~(1 << flag);
			writel(reg, PMC_BASE_ADDRESS + PMC_SCRATCH0);
			break;
		default:
			dprintf(CRITICAL, "flag %u not handled\n", flag);
	}
	return NO_ERROR;
}

status_t tegra_get_pmc_scratch0(scratch0_flag flag, bool *is_set)
{
	uint32_t reg;
	if (!is_set)
		return ERR_NOT_VALID;

	reg = readl(PMC_BASE_ADDRESS + PMC_SCRATCH0);
	switch(flag) {
		case PMC_SCRATCH0_FORCED_RECOVERY_FLAG:
		case PMC_SCRATCH0_BOOT_RECOVERY_KERNEL_FLAG:
		case PMC_SCRATCH0_FASTBOOT_FLAG:
			*is_set = reg & (1 << flag);
			break;
		default:
			dprintf(CRITICAL, "flag %u not handled\n", flag);
	}
	return NO_ERROR;
}

/* If not implemented on particular postform return "NO_ERROR" */
__WEAK status_t platform_detect_power_key_long_press(
	bool ignore_long_press_time,
	bool abort_on_charger_connect,
	uint32_t timeout)
{
	return NO_ERROR;
}

/* If not implemented on particular postform return "true" */
__WEAK bool platform_is_power_key_long_pressed(void)
{
	return true;
}
