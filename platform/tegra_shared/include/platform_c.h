/*
 * Copyright (c) 2014-2016, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited
 */

#ifndef __PLATFORM_C_H
#define __PLATFORM_C_H

#include <sys/types.h>
#include <err.h>
#include <platform_reg.h>
#include <reg.h>

/* different platform types */
enum platform_type {
	PLATFORM_INVALID = -1,
	PLATFORM_SILICON,
	PLATFORM_QT,
	PLATFORM_FPGA,
};

/* pmc scratch0 bit positions */
typedef enum{
	PMC_SCRATCH0_FORCED_RECOVERY_FLAG = 1,
	PMC_SCRATCH0_FASTBOOT_FLAG = 30,
	PMC_SCRATCH0_BOOT_RECOVERY_KERNEL_FLAG = 31,
} scratch0_flag;

/**
 * Specifies enum for accessing platform information.
 */
typedef enum {
	/* get core edp voltage */
	PLATFORM_CORE_EDP_VOLTAGE = 0x0,

	/* get core edp voltage */
	PLATFORM_CORE_EDP_CURRENT,

} platform_query_type;

/**
* @brief Initialize interrupts.
*/
void platform_init_interrupts(void);

/**
* @brief Initialize timers.
*/
void platform_init_timer(void);

/**
* @brief Initialize debug.
*/
void tegra_debug_init(void);

/**
* @brief Deinitialize timers.
*/
void platform_uninit_timer(void);

/**
* @brief Initialize device tree.
*/
void platform_init_dt(void);

/**
* @brief Initialize clocks.
*/
void platform_init_clocks(void);

/**
* @brief Determine the platform type.
*
* @return Specifies the platform type.
*/
enum platform_type get_platform_type(void);

/**
* @brief Determine the chip ID.
*
* @return Specifies the chip ID.
*/
uint32_t get_chipid(void);

/**
* @brief Waits in micro seconds.
*
* @param usecs Specifies time in micro seconds.
*/
void udelay(uint32_t usecs);

/**
* @brief Waits in milli seconds.
*
* @param msecs Specifies time in milli seconds.
*/
void mdelay(uint32_t msecs);

/**
 * @brief Initializes pinmuxes.
 *
 * @return NO_ERROR on success otherwise ERR_* type.
 */
status_t platform_pinmux_init(void);

/**
 *  @brief finds vdd core voltage to be set before loading kernel.
 *
 *  @return Specifies how much voltage to be set to.
 */
uint32_t platform_find_core_voltage(void);

/** @brief sets vdd core voltage.
 *
 *  @param volts Specifies how much voltage to be set to.
 *
 *  @return NO_ERROR if successful.
 */
status_t platform_set_core_voltage(uint32_t volts);

/**
* @brief Gets time stamp in milli seconds. This function uses timerus
*        and can be called at any stage in the code as compared to
*        current_time().
*
* @return returns time stamp in lk_time_t.
*/
lk_time_t get_time_stamp_ms(void);

/**
* @brief Gets time stamp in micro seconds. This function uses timerus
*        and can be called at any stage in the code as compared to
*        current_time_hires().
*
* @return returns time stamp in lk_bigtime_t.
*/
lk_bigtime_t get_time_stamp_us(void);

/**
* @brief reset and boot the board in recovery mode
*/
void tegra_boot_recovery_mode(void);

/**
* @brief reset the board
*/
void tegra_pmc_reset(void);

/** @brief read pmc scratch0 for fastboot, recovery and forced recovery usecases
 *
 *  @param flag specifies type of scratch0_flag
 *  @is_set variable will be set if the flag is set in scratch0
 *
 *  @return NO_ERROR if successful.
 */
status_t tegra_get_pmc_scratch0(scratch0_flag flag, bool *is_set);
status_t tegra_set_pmc_scratch0(scratch0_flag flag, bool to_set);

/** @brief Get odmdata value
 *
 *  @return odmdata value read from PMC SCRATCH20 register
 */
static inline uint32_t odmdata_get(void)
{
	return (uint32_t)readl(PMC_BASE_ADDRESS + PMC_SCRATCH20);
}

/** @brief Set odmdata value
 *
 *	@param odmdata Value odmdata need to be set to
 */
static inline void odmdata_set(uint32_t odmdata)
{
	/* Updating Odm data in SCRATCH20 */
	writel(odmdata, PMC_BASE_ADDRESS + PMC_SCRATCH20);

	/* Update Odm data in BCT partition */
/*	update_bct_odm_data(odmdata); */
}

/** @brief Detects whether power key was long pressed from a normal OFF boot and
 *         stores the the state for later use.
 *
 *  @param use_nvtboot_result specifies to use NvTboot result from boot_arg info
 *         BOOT_ARG_POWERKEY_LONG_PRESS_STATE or detect based on
 *         the time from DTB prop '/chosen/long-press-power-key-on-time'
 *  @param abort_on_charger_connect specifies whether detection should abort with
 *         current detection status immediately once charger is connected
 *  @param timeout specifies the timeout in ms of detection
 *
 *  @return NO_ERROR if successful.
 */
status_t platform_detect_power_key_long_press(bool use_nvtboot_result,
					      bool abort_on_charger_connect,
					      uint32_t timeout);

/**
 *  @brief Check if power key was long pressed from a normal OFF boot.
 *
 *  @return true if power key was long pressed from a normal OFF boot, else false.
 */
bool platform_is_power_key_long_pressed(void);
#endif
