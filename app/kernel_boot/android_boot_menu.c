/*
 * Copyright (c) 2016-2018, NVIDIA Corporation.	All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#define MODULE TEGRABL_ERR_MENU

#include <tegrabl_error.h>
#include <tegrabl_soc_misc.h>
#include <tegrabl_exit.h>
#include <menu.h>
#include <linux_load.h>
#include <boot.h>
#include <fastboot.h>

static tegrabl_error_t android_menu_continue(void *arg)
{
	tegrabl_error_t ret = TEGRABL_NO_ERROR;
	(void)arg;
	struct tegrabl_kernel_bin kernel;
	kernel.bin_type = tegrabl_get_kernel_type();

	ret = load_and_boot_kernel(&kernel);
	if (ret != TEGRABL_NO_ERROR)
		TEGRABL_SET_HIGHEST_MODULE(ret);

	/* Control should not reach here */
	return ret;
}

static tegrabl_error_t android_menu_fastboot(void *arg)
{
	(void)arg;
	fastboot_init();
	/* Control should not reach here */
	return TEGRABL_NO_ERROR;
}

static tegrabl_error_t android_menu_recovery(void *arg)
{
	tegrabl_error_t ret = TEGRABL_NO_ERROR;
	(void)arg;
	struct tegrabl_kernel_bin kernel;
	kernel.bin_type = TEGRABL_BINARY_RECOVERY_KERNEL;

	ret = load_and_boot_kernel(&kernel);
	if (ret != TEGRABL_NO_ERROR)
		TEGRABL_SET_HIGHEST_MODULE(ret);
	/* Control should not reach here */
	return ret;
}

static tegrabl_error_t android_menu_reset(void *arg)
{
	tegrabl_error_t ret = TEGRABL_NO_ERROR;
	(void)arg;
	ret = tegrabl_reset();
	/* Control should not reach here */
	return ret;
}

static tegrabl_error_t android_menu_poweroff(void *arg)
{
	tegrabl_error_t ret = TEGRABL_NO_ERROR;
	(void)arg;
	ret = tegrabl_poweroff();
	/* Control should not reach here */
	return ret;
}

static tegrabl_error_t android_menu_forced_recovery(void *arg)
{
	tegrabl_error_t ret = TEGRABL_NO_ERROR;
	(void)arg;
	ret = tegrabl_reboot_forced_recovery();
	/* Control should not reach here */
	return ret;
}

static struct menu_entry android_menu_entries[] = {
	{ /* Entry 1 */
		.ms_entry = {
			.color = WHITE,
			.data = "Continue\n"
		},
		.ms_on_select = {
			.color = GREEN,
			.data = "Continue booting ...\n\n"
		},
		.on_select_callback = android_menu_continue,
		.arg = NULL
	},
	{ /* Entry 2 */
		.ms_entry = {
			.color = WHITE,
			.data = "Fastboot protocol\n"
		},
		.ms_on_select = {
			.color = RED,
			.data = "Booting into fastboot mode ...\n\n"
		},
		.on_select_callback = android_menu_fastboot,
		.arg = NULL
	},
	{ /* Entry 3 */
		.ms_entry = {
			.color = WHITE,
			.data = "Boot recovery kernel\n"
		},
		.ms_on_select = {
			.color = RED,
			.data = "Booting recovery kernel ...!!!\n\n"
		},
		.on_select_callback = android_menu_recovery,
		.arg = NULL
	},
	{ /* Entry 4 */
		.ms_entry = {
			.color = WHITE,
			.data = "Reboot\n"
		},
		.ms_on_select = {
			.color = GREEN,
			.data = "Rebooting ...!!!\n\n"
		},
		.on_select_callback = android_menu_reset,
		.arg = NULL
	},
	{ /* Entry 5 */
		.ms_entry = {
			.color = WHITE,
			.data = "Poweroff\n"
		},
		.ms_on_select = {
			.color = GREEN,
			.data = "Powering off ...!!!\n\n"
		},
		.on_select_callback = android_menu_poweroff,
		.arg = NULL
	},
	{ /* Entry 6 */
		.ms_entry = {
			.color = WHITE,
			.data = "Forced Recovery\n"
		},
		.ms_on_select = {
			.color = GREEN,
			.data = "Booting into usb recovery mode ...\n\n"
		},
		.on_select_callback = android_menu_forced_recovery,
		.arg = NULL
	},
};

struct menu android_menu = {
	.menu_type = ANDROID_BOOT_MENU,
	.name = "android-menu",
	.menu_entries = android_menu_entries,
	.num_menu_entries = ARRAY_SIZE(android_menu_entries),
	.menu_header = {
		.valid = true,
		.ms = {
			.color = RED,
			.data = "Android Boot Menu:\n"
		}
	},
	.timeout = 10000, /* in ms */
	.current_entry = 0,
};

