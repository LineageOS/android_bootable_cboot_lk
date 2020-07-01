/*
 * Copyright (c) 2016-2017, NVIDIA Corporation.	All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#define MODULE TEGRABL_ERR_ANDROIDBOOT

#include <tegrabl_error.h>
#include <tegrabl_debug.h>
#include <tegrabl_linuxboot_helper.h>
#include <tegrabl_soc_misc.h>
#include <tegrabl_a_b_boot_control.h>
#include <app.h>
#include <debug.h>
#include <err.h>
#include <trace.h>
#include <string.h>
#include <sys/types.h>
#include <assert.h>
#include <kernel/thread.h>
#include <arch/ops.h>
#include <reg.h>
#include <device_config.h>
#include <linux_load.h>
#include <fastboot.h>
#include <platform.h>
#include <verified_boot.h>
#include <menu.h>
#include <boot.h>
#include <android_boot_menu.h>
#include <tegrabl_wdt.h>
#include <tegrabl_profiler.h>

#define LOCAL_TRACE 0

#if defined(CONFIG_ENABLE_DISPLAY)
static tegrabl_error_t display_boot_logo(void)
{
	struct tegrabl_image_info image;
	tegrabl_error_t ret = TEGRABL_NO_ERROR;

	/* Display boot logo */
	image.type = IMAGE_NVIDIA;
	image.format = TEGRABL_IMAGE_FORMAT_BMP;
	image.image_buf = NULL;

	ret = tegrabl_display_clear();
	if (ret != TEGRABL_NO_ERROR) {
		TEGRABL_SET_HIGHEST_MODULE(ret);
		return ret;
	}
	ret = tegrabl_display_show_image(&image);
	if (ret != TEGRABL_NO_ERROR) {
		TEGRABL_SET_HIGHEST_MODULE(ret);
		return ret;
	}

	return TEGRABL_NO_ERROR;
}
#endif

tegrabl_error_t load_and_boot_kernel(struct tegrabl_kernel_bin *kernel)
{
	void *kernel_entry_point = NULL;
	void *kernel_dtb = NULL;
	void (*kernel_entry)(uint64_t x0, uint64_t x1, uint64_t x2,
						 uint64_t x3);
	tegrabl_error_t err = TEGRABL_NO_ERROR;
	struct tegrabl_kernel_load_callbacks callbacks;

#if defined(CONFIG_ENABLE_VERIFIED_BOOT)
	callbacks.verify_boot = verify_boot;
#else
	callbacks.verify_boot = NULL;
#endif

	kernel->load_from_storage = true;

	err = tegrabl_load_kernel_and_dtb(kernel, &kernel_entry_point,
									  &kernel_dtb, &callbacks, NULL);
	if (err != TEGRABL_NO_ERROR) {
		TEGRABL_SET_HIGHEST_MODULE(err);
		pr_error("kernel boot failed\n");
		return err;
	}

#if defined(CONFIG_ENABLE_DISPLAY)
	err = display_boot_logo();
	if (err != TEGRABL_NO_ERROR)
		pr_warn("Boot logo display failed...\n");
#endif
	tegrabl_profiler_record("android_boot exit", 0, DETAILED);

	tegrabl_a_b_update_smd();

	platform_uninit();

	kernel_entry = (void *)kernel_entry_point;
	kernel_entry((uint64_t)kernel_dtb, 0, 0, 0);

	return TEGRABL_NO_ERROR;
}

static status_t android_boot(void)
{
	status_t err = NO_ERROR;
	tegrabl_error_t ret = TEGRABL_NO_ERROR;
	bool is_enter_fastboot = false;
	bool is_skip_menu = true;
	struct tegrabl_kernel_bin kernel;

	tegrabl_profiler_record("android_boot entry", 0, DETAILED);
	err = check_enter_fastboot(&is_enter_fastboot);
	if (err)
		goto fail;

	/* Init the menu early since fastboot and verified boot both need menu */
	menu_init();

	if (is_enter_fastboot) {
#if defined(CONFIG_ENABLE_WDT)
		/* disable cpu-wdt before kernel handoff */
		tegrabl_wdt_disable(TEGRABL_WDT_LCCPLEX);
#endif /* CONFIG_ENABLE_WDT */
		pr_info("Entering fastboot mode...\n");
		fastboot_init();
	}

	/* TODO: get is_skip_menu from odmdata */

	if (!is_skip_menu) {
		pr_info("start bootloader menu\n");

		/* start bootloader menu */
		ret = menu_draw(&android_menu);
		if (ret != TEGRABL_NO_ERROR &&
			TEGRABL_ERROR_REASON(ret) != TEGRABL_ERR_TIMEOUT) {
			pr_critical("failed to load boot menu\n");
			err = ERR_NOT_VALID;
			goto fail;
		}

		err = menu_handle_on_select(&android_menu);
		if (err)
			goto fail;
	}
	tegrabl_profiler_record("menu init", 0, DETAILED);

	kernel.bin_type = tegrabl_get_kernel_type();

	err = load_and_boot_kernel(&kernel);
	if (err)
		goto fail;

fail:
	return err;
}

static void android_boot_init(const struct app_descriptor *app)
{
	return;
}

static void android_boot_entry(const struct app_descriptor *app, void *args)
{
	android_boot();
}

APP_START(android_boot_app)
	.init = android_boot_init,
	.entry = android_boot_entry,
#if !START_ON_BOOT
	.flags = APP_FLAG_DONT_START_ON_BOOT,
#endif
APP_END

#if WITH_LIB_CONSOLE
#include <lib/console.h>

static status_t android_boot_cmd(int argc, const cmd_args *argv)
{
	return android_boot();
}

STATIC_COMMAND_START
STATIC_COMMAND("boot", "android boot", &android_boot_cmd)
STATIC_COMMAND_END(android_boot_cmd);
#endif

