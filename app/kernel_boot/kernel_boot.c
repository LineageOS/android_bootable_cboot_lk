/*
 * Copyright (c) 2016-2022, NVIDIA Corporation.	All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#define MODULE TEGRABL_ERR_KERNELBOOT

#include <tegrabl_error.h>
#include <tegrabl_debug.h>
#include <tegrabl_linuxboot_helper.h>
#include <tegrabl_soc_misc.h>
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
#include <platform.h>
#include <boot.h>
#include <linux_load.h>
#if defined(CONFIG_ENABLE_VERIFIED_BOOT)
#include <verified_boot.h>
#endif
#include <tegrabl_parse_bmp.h>
#include <tegrabl_wdt.h>
#include <tegrabl_display.h>
#include <tegrabl_devicetree.h>
#include <tegrabl_exit.h>
#include <lib/menu.h>
#include <tegrabl_a_b_boot_control.h>
#if defined(CONFIG_OS_IS_ANDROID)
#include <tos_param.h>
#endif

#if defined(IS_T186)
#include <device_config.h>
#include <android_boot_menu.h>
#include <tegrabl_profiler.h>
#endif

#if defined(CONFIG_ENABLE_FASTBOOT)
#include <fastboot.h>
#endif

#define LOCAL_TRACE 0

#if defined(CONFIG_ENABLE_DISPLAY) && defined(CONFIG_ENABLE_NVBLOB)
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

#ifdef CONFIG_ENABLE_NVDISP_INIT
// Start of UEFI code in combo nvdisp-init+UEFI binary
extern uintptr_t uefi_start;
#endif

tegrabl_error_t load_and_boot_kernel(struct tegrabl_kernel_bin *kernel)
{
	void *kernel_entry_point = NULL;
	void *kernel_dtb = NULL;
	void (*kernel_entry)(uint64_t x0, uint64_t x1, uint64_t x2,
						 uint64_t x3);
#if !defined(CONFIG_ENABLE_NVDISP_INIT)
	tegrabl_error_t err = TEGRABL_NO_ERROR;
	struct tegrabl_kernel_load_callbacks callbacks;

#if defined(CONFIG_ENABLE_VERIFIED_BOOT)
	callbacks.verify_boot = verify_boot;
#else
	callbacks.verify_boot = NULL;
#endif

	kernel->load_from_storage = true;

	err = tegrabl_load_kernel_and_dtb(kernel, &kernel_entry_point,
						  &kernel_dtb, &callbacks, NULL, 0);
#endif

#if defined(CONFIG_ENABLE_A_B_SLOT)
	/*
	 * Update smd if a/b retry counter changed
	 * The slot priorities are rotated here too,
	 * in case kernel or kernel-dtb load failed.
	*/
	tegrabl_a_b_update_smd();

	if (err != TEGRABL_NO_ERROR) {
		TEGRABL_SET_HIGHEST_MODULE(err);
		pr_error("kernel boot failed, will reset.\n");
		tegrabl_reset();
		return err;
	}
#endif	// ENABLE_A_B_SLOT
#if defined(CONFIG_OS_IS_ANDROID)
	tegrabl_send_tos_param();
#endif

#if defined(CONFIG_ENABLE_DISPLAY) && defined(CONFIG_ENABLE_NVBLOB)
	err = display_boot_logo();
	if (err != TEGRABL_NO_ERROR)
		pr_warn("Boot logo display failed...\n");
#endif

#if defined(IS_T186)
	tegrabl_profiler_record("kernel_boot exit", 0, DETAILED);
#endif

#ifdef CONFIG_ENABLE_NVDISP_INIT
	// After boot logo, jump to UEFI binary entry point
	kernel_entry_point = (void *)uefi_start;
	pr_info("%s: Jumping to UEFI @ %p ...\n", __func__, kernel_entry_point);
#else
	pr_info("Kernel EP: %p, DTB: %p\n", kernel_entry_point, kernel_dtb);
#endif

	platform_uninit();

	/* The MMU is off here. Don't call any code, such as printf or
	 * compiler library functions, which might perform unaligned accesses.
	 * They would raise an exception since all memory access are treated
	 * as device accesses when the MMU is off, and device memory doesn't
	 * support unaligned accesses.
	 */
	kernel_entry = (void *)kernel_entry_point;
	kernel_entry((uint64_t)kernel_dtb, 0, 0, 0);

	return TEGRABL_NO_ERROR;
}

static status_t kernel_boot(void)
{
	status_t err = NO_ERROR;
	struct tegrabl_kernel_bin kernel;

#if defined(CONFIG_ENABLE_FASTBOOT)
	bool is_enter_fastboot = false;
#endif

#if defined(IS_T186)
	tegrabl_error_t ret = TEGRABL_NO_ERROR;
	bool is_skip_menu = true;

	tegrabl_profiler_record("kernel_boot entry", 0, DETAILED);
#endif

	/* Init the menu early since fastboot and verified boot both need menu */
	menu_init();

#if defined(CONFIG_ENABLE_FASTBOOT)
	err = check_enter_fastboot(&is_enter_fastboot);
	if (err) {
		goto fail;
	}

	if (is_enter_fastboot) {
#if defined(CONFIG_ENABLE_WDT)
		/* disable cpu-wdt before kernel handoff */
		tegrabl_wdt_disable(TEGRABL_WDT_LCCPLEX);
#endif /* CONFIG_ENABLE_WDT */
		pr_info("Entering fastboot mode...\n");
		fastboot_init();
	}
#endif

#if defined(IS_T186)
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

#endif

#if defined(CONFIG_ENABLE_DISPLAY) && defined(CONFIG_ENABLE_NVBLOB)
	err = tegrabl_load_bmp_blob("bootlogo");
	if (err != TEGRABL_NO_ERROR)
		pr_warn("Loading bmp blob to memory failed\n");

#if defined(IS_T186)
	tegrabl_profiler_record("Load BMP blob", 0, DETAILED);
#endif
#endif

	kernel.bin_type = tegrabl_get_kernel_type();

	err = load_and_boot_kernel(&kernel);
	if (err)
		goto fail;

fail:
	return err;
}

static void kernel_boot_init(const struct app_descriptor *app)
{
	return;
}

static void kernel_boot_entry(const struct app_descriptor *app, void *args)
{
#if defined(CONFIG_ENABLE_A_B_SLOT)
	status_t err = NO_ERROR;
	err = kernel_boot();
	if (err != NO_ERROR && !tegrabl_is_wdt_enable()) {
		pr_critical("failed to load kernel, reboot it\n");
		tegrabl_reset();
	}
#else
	kernel_boot();
#endif
}

APP_START(kernel_boot_app)
	.init = kernel_boot_init,
	.entry = kernel_boot_entry,
#if !START_ON_BOOT
	.flags = APP_FLAG_DONT_START_ON_BOOT,
#endif
#if defined(CONFIG_ENABLE_DTB_OVERLAY)
	.stack_size = 32768,
	.flags = APP_FLAG_CUSTOM_STACK_SIZE,
#endif
APP_END

#if WITH_LIB_CONSOLE
#include <lib/console.h>

static status_t kernel_boot_cmd(int argc, const cmd_args *argv)
{
	return kernel_boot();
}

STATIC_COMMAND_START
STATIC_COMMAND("boot", "kernel boot", &kernel_boot_cmd)
STATIC_COMMAND_END(kernel_boot_cmd);
#endif

