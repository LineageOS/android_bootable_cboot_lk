/*
 * Copyright (c) 2016-2018, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited
 */

#define MODULE TEGRABL_ERR_NO_MODULE

#include <debug.h>
#include <arch/arm64.h>
#include <arch/mmu.h>
#include <arch/ops.h>
#include <platform_c.h>
#include <string.h>
#include <err.h>
#include <tegrabl_exit.h>
#include <tegrabl_debug.h>
#include <tegrabl_console.h>
#include <tegrabl_blockdev.h>
#include <tegrabl_sdmmc_bdev.h>
#include <tegrabl_ufs_bdev.h>
#include <tegrabl_mb1_bct.h>
#include <tegrabl_mb1bct_lib.h>
#include <tegrabl_brbct.h>
#include <address_map_new.h>
#include <arscratch.h>
#include <tegrabl_cpubl_params.h>
#include <tegrabl_carveout_usage.h>
#include <tegrabl_linuxboot_helper.h>
#include <tegrabl_profiler.h>
#include <inttypes.h>
#include <tegrabl_i2c.h>
#include <tegrabl_power_i2c.h>
#include <tegrabl_soc_misc.h>
#include <tegrabl_malloc.h>
#include <tegrabl_wdt.h>
#include <tegrabl_partition_manager.h>
#include <tegrabl_partition_loader.h>
#include <tegrabl_nct.h>
#include <tegrabl_bpmp_fw_interface.h>
#include <tegrabl_clock.h>
#include <tegrabl_module.h>
#include <tegrabl_display.h>
#include <address_map_new.h>
#include <tegrabl_gpio.h>
#include <tegrabl_tca9539_gpio.h>
#include <tegrabl_parse_bmp.h>
#include <tegrabl_mce.h>
#include <tegrabl_devicetree.h>
#include <tegrabl_board_info.h>
#include <tegrabl_mce.h>
#include <tegrabl_odmdata_lib.h>
#include <tegrabl_regulator.h>
#include <tegrabl_pmic.h>
#include <tegrabl_page_allocator.h>
#include <tegrabl_global_defs.h>
#include <tegrabl_sata.h>
#if defined(CONFIG_VIC_SCRUB)
#include <tegrabl_vic.h>
#endif
#include <dram_ecc.h>
#if defined(CONFIG_ENABLE_PMIC_MAX77620)
#include <tegrabl_pmic_max77620.h>
#endif
#include <tegrabl_ufs_int.h>
#include <tegrabl_sdram_usage.h>
#include <tegrabl_prevent_rollback.h>
#include <tegrabl_qspi_flash.h>
#include <tegrabl_gpcdma.h>
#include <tegrabl_storage.h>
#include <ratchet_update.h>
#if defined(CONFIG_ENABLE_XUSBH)
#include <tegrabl_usbh.h>
#include <fastboot.h>
#endif
#define LOCAL_TRACE 0
extern int __version_start;

extern int _end;
struct tboot_cpubl_params *boot_params;
static bool is_ufs_storage;

#define CBOOT_HEAP_END_ADDR RAMDISK_ADDRESS

#define CBOOT_HEAP_START ((uintptr_t)&_end)
#define CBOOT_HEAP_LEN ((uintptr_t)(CBOOT_HEAP_END_ADDR) - (uintptr_t)&_end)

#define UPHY_ODM_BIT (1 << 28)
#define SDRAM_START_ADDRESS 0x80000000

struct mmio_mapping_info {
	    uint64_t addr;
	    uint64_t size;
};

struct mmio_mapping_info mmio_mappings[] = {
	{ NV_ADDRESS_MAP_SYSRAM_0_BASE, NV_ADDRESS_MAP_SYSRAM_0_IMPL_SIZE },
	{ NV_ADDRESS_MAP_ARM_ICTLR_MAIN_BASE, NV_ADDRESS_MAP_ARM_ICTLR_MAIN_SIZE  },
	{ NV_ADDRESS_MAP_TOP0_HSP_DB_0_BASE, NV_ADDRESS_MAP_TOP0_HSP_DB_0_SIZE },
	{ NV_ADDRESS_MAP_UARTA_BASE, NV_ADDRESS_MAP_UARTA_SIZE },
	{ NV_ADDRESS_MAP_UARTB_BASE, NV_ADDRESS_MAP_UARTB_SIZE },
	{ NV_ADDRESS_MAP_UARTC_BASE, NV_ADDRESS_MAP_UARTC_SIZE },
	{ NV_ADDRESS_MAP_UARTD_BASE, NV_ADDRESS_MAP_UARTD_SIZE },
	{ NV_ADDRESS_MAP_UARTE_BASE, NV_ADDRESS_MAP_UARTE_SIZE },
	{ NV_ADDRESS_MAP_UARTF_BASE, NV_ADDRESS_MAP_UARTF_SIZE },
	{ NV_ADDRESS_MAP_UARTG_BASE, NV_ADDRESS_MAP_UARTG_SIZE },
	{ NV_ADDRESS_MAP_SDMMC4_BASE, NV_ADDRESS_MAP_SDMMC4_SIZE },
	{ NV_ADDRESS_MAP_GPCDMA_BASE, NV_ADDRESS_MAP_GPCDMA_SIZE },
	{ NV_ADDRESS_MAP_PADCTL_A5_BASE, NV_ADDRESS_MAP_PADCTL_A5_SIZE },
	{ NV_ADDRESS_MAP_XUSB_DEV_BASE, NV_ADDRESS_MAP_XUSB_DEV_SIZE },
	{ NV_ADDRESS_MAP_XUSB_PADCTL_BASE, NV_ADDRESS_MAP_XUSB_PADCTL_SIZE },
	{ NV_ADDRESS_MAP_I2C1_BASE, NV_ADDRESS_MAP_I2C1_SIZE },
	{ NV_ADDRESS_MAP_I2C2_BASE, NV_ADDRESS_MAP_I2C2_SIZE },
	{ NV_ADDRESS_MAP_I2C3_BASE, NV_ADDRESS_MAP_I2C3_SIZE },
	{ NV_ADDRESS_MAP_I2C4_BASE, NV_ADDRESS_MAP_I2C4_SIZE },
	{ NV_ADDRESS_MAP_I2C5_BASE, NV_ADDRESS_MAP_I2C5_SIZE },
	{ NV_ADDRESS_MAP_I2C6_BASE, NV_ADDRESS_MAP_I2C6_SIZE },
	{ NV_ADDRESS_MAP_I2C7_BASE, NV_ADDRESS_MAP_I2C7_SIZE },
	{ NV_ADDRESS_MAP_I2C8_BASE, NV_ADDRESS_MAP_I2C8_SIZE },
	{ NV_ADDRESS_MAP_I2C9_BASE, NV_ADDRESS_MAP_I2C9_SIZE },
	{ NV_ADDRESS_MAP_I2C10_BASE, NV_ADDRESS_MAP_I2C10_SIZE },
	{ NV_ADDRESS_MAP_PMC_BASE, NV_ADDRESS_MAP_PMC_SIZE },
	{ NV_ADDRESS_MAP_MISC_BASE, NV_ADDRESS_MAP_MISC_SIZE },
	{ NV_ADDRESS_MAP_MCB_BASE, NV_ADDRESS_MAP_MCB_SIZE },
	{ NV_ADDRESS_MAP_EMCB_BASE, NV_ADDRESS_MAP_EMCB_SIZE },
	{ NV_ADDRESS_MAP_TSCUS_BASE, NV_ADDRESS_MAP_TSCUS_SIZE },
	{ NV_ADDRESS_MAP_QSPI_BASE, NV_ADDRESS_MAP_QSPI_SIZE },
	{ NV_ADDRESS_MAP_FUSE_BASE, NV_ADDRESS_MAP_FUSE_SIZE },
	{ NV_ADDRESS_MAP_SCRATCH_BASE, NV_ADDRESS_MAP_SCRATCH_SIZE },
	{ NV_ADDRESS_MAP_SATA_BASE, NV_ADDRESS_MAP_SATA_SIZE },
	{ NV_ADDRESS_MAP_UPHY_0_BASE, NV_ADDRESS_MAP_UPHY_0_SIZE },
	{ NV_ADDRESS_MAP_UPHY_4_BASE, NV_ADDRESS_MAP_UPHY_4_SIZE },
	{ NV_ADDRESS_MAP_UPHY_LANE5_BASE, NV_ADDRESS_MAP_UPHY_LANE5_SIZE },
	{ NV_ADDRESS_MAP_TOP_TKE_BASE, NV_ADDRESS_MAP_TOP_TKE_SIZE },
	{ NV_ADDRESS_MAP_SCRATCH_BASE, NV_ADDRESS_MAP_SCRATCH_SIZE },
	{ NV_ADDRESS_MAP_SOR1_BASE, NV_ADDRESS_MAP_SOR1_SIZE},
	{ NV_ADDRESS_MAP_SOR_BASE, NV_ADDRESS_MAP_SOR_SIZE},
	{ NV_ADDRESS_MAP_DISPLAY_BASE, NV_ADDRESS_MAP_DISPLAY_SIZE},
	{ NV_ADDRESS_MAP_DISPLAYB_BASE, NV_ADDRESS_MAP_DISPLAYB_SIZE},
	{ NV_ADDRESS_MAP_DISPLAYC_BASE, NV_ADDRESS_MAP_DISPLAYC_SIZE},
	{ NV_ADDRESS_MAP_DPAUX_BASE, NV_ADDRESS_MAP_DPAUX_SIZE},
	{ NV_ADDRESS_MAP_DPAUX1_BASE, NV_ADDRESS_MAP_DPAUX1_SIZE},
	{ NV_ADDRESS_MAP_AON_GPIO_0_BASE, NV_ADDRESS_MAP_AON_GPIO_0_SIZE },
	{ NV_ADDRESS_MAP_GPIO_CTL_BASE, NV_ADDRESS_MAP_GPIO_CTL_SIZE },
	{ NV_ADDRESS_MAP_SE0_BASE, NV_ADDRESS_MAP_SE0_SIZE },
	{ NV_ADDRESS_MAP_SE1_BASE, NV_ADDRESS_MAP_SE1_SIZE },
	{ NV_ADDRESS_MAP_SE2_BASE, NV_ADDRESS_MAP_SE2_SIZE },
	{ NV_ADDRESS_MAP_SE3_BASE, NV_ADDRESS_MAP_SE3_SIZE },
	{ NV_ADDRESS_MAP_SE4_BASE, NV_ADDRESS_MAP_SE4_SIZE },
	{ NV_ADDRESS_MAP_SPI1_BASE, NV_ADDRESS_MAP_SPI1_SIZE },
	{ NV_ADDRESS_MAP_SPI2_BASE, NV_ADDRESS_MAP_SPI2_SIZE },
	{ NV_ADDRESS_MAP_SPI3_BASE, NV_ADDRESS_MAP_SPI3_SIZE },
	{ NV_ADDRESS_MAP_SPI4_BASE, NV_ADDRESS_MAP_SPI4_SIZE },
	{ NV_ADDRESS_MAP_UFSHC_0_BASE, NV_ADDRESS_MAP_UFSHC_0_SIZE},
	{ NV_ADDRESS_MAP_UFSHC_0_UNIPRO_BASE, NV_ADDRESS_MAP_UFSHC_0_UNIPRO_SIZE},
	{ NV_ADDRESS_MAP_UFS_CAR_BASE, NV_ADDRESS_MAP_UFS_CAR_SIZE},
	{ NV_ADDRESS_MAP_MPHY_L0_BASE, NV_ADDRESS_MAP_MPHY_L0_SIZE},
	{ NV_ADDRESS_MAP_MPHY_L1_BASE, NV_ADDRESS_MAP_MPHY_L1_SIZE},
	{ NV_ADDRESS_MAP_PCIE0_BASE, NV_ADDRESS_MAP_PCIE0_SIZE},
	{ NV_ADDRESS_MAP_PCIE1_BASE, NV_ADDRESS_MAP_PCIE1_SIZE},
	{ NV_ADDRESS_MAP_PCIE2_BASE, NV_ADDRESS_MAP_PCIE2_SIZE},
	{ NV_ADDRESS_MAP_PCIE_AFI_BASE, NV_ADDRESS_MAP_PCIE_AFI_SIZE},
	{ NV_ADDRESS_MAP_PCIE_PADS_BASE, NV_ADDRESS_MAP_PCIE_PADS_SIZE},
#if defined(CONFIG_VIC_SCRUB)
	{ NV_ADDRESS_MAP_VIC_BASE, NV_ADDRESS_MAP_VIC_SIZE},
#endif
#if defined(CONFIG_ENABLE_XUSBH)
	{ NV_ADDRESS_MAP_XUSB_HOST_BAR0_BASE, NV_ADDRESS_MAP_XUSB_HOST_BAR0_SIZE },
	{ NV_ADDRESS_MAP_XUSB_HOST_CFG_BASE, NV_ADDRESS_MAP_XUSB_HOST_CFG_SIZE },
#endif
};

static inline void platform_init_boot_param(void)
{
	uint32_t temp = 0;

	temp = NV_READ32(NV_ADDRESS_MAP_SCRATCH_BASE + SCRATCH_SCRATCH_7);

	boot_params = (struct tboot_cpubl_params *)(uintptr_t)
				  (temp < SDRAM_START_ADDRESS ?
				  (uint64_t) temp << PAGE_SIZE_LOG2 : temp);
}

tegrabl_error_t cpubl_params_version_check(struct tboot_cpubl_params *params)
{
	if (!params)
		return TEGRABL_ERROR(TEGRABL_ERR_INVALID, 0);

	switch (params->version) {
	case 0:
	case 1:
	case 2:
		/* Fall through. */
	case 3:
		/* Add new versions here. */
	default:
		break;
	}

	return TEGRABL_NO_ERROR;
}

status_t platform_early_init(void)
{
	status_t error;
	enum carve_out_type carveout;
	/* IPC initialization to use BPMP for clk config*/
	tegrabl_ipc_init();

	if (boot_params == NULL) {
		pr_critical("boot_param is null\n");
		error = TEGRABL_ERR_INVALID;
		goto fail;
	}

	error = cpubl_params_version_check(boot_params);
	if (error != TEGRABL_NO_ERROR) {
		pr_critical("Failed to check cpubl param version");
		goto fail;
	}

	int64_t cboot_init_timestamp = tegrabl_get_timestamp_us();

	tegrabl_profiler_init(boot_params->global_data.profiling_carveout,
			      CPUBL_PROFILER_OFFSET, CPUBL_PROFILER_SIZE);

	tegrabl_profiler_record("CBoot start", cboot_init_timestamp, MINIMAL);

	error = tegrabl_brbct_init(boot_params->global_data.brbct_carveout);
	if (error != TEGRABL_NO_ERROR) {
		pr_critical("Failed to initialize brbct\n");
		goto fail;
	}

#if defined(CONFIG_ENABLE_WDT)
	tegrabl_wdt_enable(TEGRABL_WDT_LCCPLEX, 0, 0, TEGRABL_WDT_SRC_TSCCNT_29_0);
#endif

#if defined(CONFIG_ENABLE_UART)
	/* initialize debug console */
	error = tegrabl_console_register(TEGRABL_CONSOLE_UART,
				boot_params->uart_instance, NULL);

	tegrabl_profiler_record("Console register", 0, DETAILED);

	if ((error != TEGRABL_NO_ERROR) &&
		(TEGRABL_ERROR_REASON(error) != TEGRABL_ERR_NOT_SUPPORTED))
		goto fail;
#endif

	error = tegrabl_debug_init(boot_params->enable_log);
	if ((error != TEGRABL_NO_ERROR) &&
		(TEGRABL_ERROR_REASON(error) != TEGRABL_ERR_NOT_SUPPORTED))
		goto fail;

	pr_info("Welcome to Cboot\n");
	pr_info("Cboot Version: %s\n", (char *) &__version_start);
	pr_info("CPU-BL Params @ %p\n", boot_params);

	for (carveout = 0; carveout < CARVEOUT_NUM; carveout++) {
		pr_info("%2u) Base:0x%08"PRIx64" Size:0x%08"PRIx64"\n", carveout,
				boot_params->global_data.carveout[carveout].base,
				boot_params->global_data.carveout[carveout].size);
	}

	/* Enable SError */
	arm64_enable_serror();

	tegrabl_profiler_record("ARM64 enable serror", 0, DETAILED);

	/* initialize the interrupt controller */
	platform_init_interrupts();

	tegrabl_profiler_record("Platform: init interrupts", 0, DETAILED);

	/* initialize the timer block */
	platform_init_timer();

fail:
	return error;
}

int platform_use_identity_mmu_mappings(void)
{
	/* Use only the mappings specified in this file. */
	return 0;
}

void platform_init_mmu_mappings(void)
{
	uint32_t idx, i;
	struct tegrabl_linuxboot_memblock *free_dram_regions = NULL;
	uint32_t free_dram_block_count;

	for (idx = 0; idx < ARRAY_SIZE(mmio_mappings); idx++) {
		arm64_mmu_map(mmio_mappings[idx].addr, mmio_mappings[idx].addr,
				mmio_mappings[idx].size, MMU_FLAG_EXECUTE_NOT |
				MMU_FLAG_READWRITE | MMU_FLAG_DEVICE);
	}

	platform_init_boot_param();
	/*
	 * Map the complete CARVEOUT_CPUBL_PARAMS to access CPUBL Params, BR-BCT
	 * profiler, GR
	*/
	arm64_mmu_map((uintptr_t)boot_params, (uintptr_t)boot_params,
					TEGRABL_CARVEOUT_CPUBL_PARAMS_SIZE,
					MMU_FLAG_CACHED | MMU_FLAG_READWRITE);

	free_dram_block_count = get_free_dram_regions_info(&free_dram_regions);

	for (i = 0; i < free_dram_block_count; i++) {
		arm64_mmu_map((uintptr_t)free_dram_regions[i].base,
				(uintptr_t)free_dram_regions[i].base, free_dram_regions[i].size,
				MMU_FLAG_CACHED | MMU_FLAG_READWRITE);
	}
}

static void platform_disable_clocks(void)
{
	uint32_t odmdata = 0;
	tegrabl_error_t err = TEGRABL_NO_ERROR;
	/* -- Disable clocks not needed by kernel -- */
	/* Disable SE clock */
	tegrabl_car_clk_disable(TEGRABL_MODULE_SE, 0);
#if defined(CONFIG_ENABLE_UFS)
		if (is_ufs_storage == true) {
			odmdata = tegrabl_odmdata_get();
			pr_debug("odm data is %0x\n", odmdata);
			if ((odmdata & UPHY_ODM_BIT) != 0) {
				/* switch to pwm mode */
				err = tegrabl_ufs_change_gear(1);
				if (err != TEGRABL_NO_ERROR)
					pr_critical("pwm mode switch failed\n");
				/* put the device in default state */
				tegrabl_ufs_default_state();
				tegrabl_ufs_clock_deinit();
			}
		}
#endif
}

void platform_uninit(void)
{
#if defined(CONFIG_ENABLE_WDT)
	/* disable cpu-wdt before kernel handoff */
	tegrabl_wdt_disable(TEGRABL_WDT_LCCPLEX);
#endif /* CONFIG_ENABLE_WDT */

	platform_uninit_timer();
	arch_disable_ints();
#if WITH_MMU
	arch_disable_mmu();
	tegrabl_mce_roc_cache_flush();
	tegrabl_mce_roc_cache_clean();
#endif
	platform_disable_clocks();
	arm64_disable_serror();

	tegrabl_profiler_record("CBoot end (platform uninit)", 0, MINIMAL);
}

static tegrabl_error_t platform_init_power(void)
{
	tegrabl_error_t err = TEGRABL_NO_ERROR;

	tegrabl_pmic_init();

	err = tegrabl_regulator_init();
	if (TEGRABL_NO_ERROR != err)
		goto fail;

#if defined(CONFIG_ENABLE_PMIC_MAX77620)
	err = tegrabl_max77620_init(TEGRABL_INSTANCE_POWER_I2C_BPMPFW);
#endif

fail:
	if (TEGRABL_NO_ERROR != err)
		TEGRABL_SET_HIGHEST_MODULE(err);
	return err;
}

void platform_init(void)
{
	tegrabl_storage_type_t boot_device;
	uint32_t instance;
	tegrabl_error_t err = TEGRABL_NO_ERROR;
	struct tegrabl_mb1bct_device_params *dev_param = NULL;
#if defined(CONFIG_DT_SUPPORT)
	void *bl_dtb = NULL;
#endif
	struct board_id_info *id_info;
	struct tegrabl_rollback *rb;
	struct tegrabl_device *storage_devs = NULL;
	bool hang_up = false;
#if defined(CONFIG_ENABLE_XUSBH)
	bool enter_fastboot = false;
#endif

	dev_param = (struct tegrabl_mb1bct_device_params *)(uintptr_t)
					boot_params->dev_params_address;
	TEGRABL_UNUSED(dev_param);

	tegrabl_profiler_record("Platform_init start", 0, DETAILED);

	err = tegrabl_register_prod_settings(
			(uint32_t *)(uintptr_t)boot_params->controller_prod_settings,
			boot_params->controller_prod_settings_size);
	if (err != TEGRABL_NO_ERROR)
		goto fail;

	rb = (struct tegrabl_rollback *)boot_params->rollback_data_address;
	err = tegrabl_init_rollback_data(rb);
	if (err != TEGRABL_NO_ERROR) {
		goto fail;
	}

	tegrabl_profiler_record("Init rollback data", 0, DETAILED);

	/* Initialise the exit framework */
	err = tegrabl_exit_register();
	if (err != TEGRABL_NO_ERROR)
		goto fail;

	tegrabl_blockdev_init();

	/* Get boot device */
	err = tegrabl_soc_get_bootdev(&boot_device, &instance);
	if (err != TEGRABL_NO_ERROR) {
		pr_critical("Failed to get boot device information\n");
		goto fail;
	}

	if (boot_device == TEGRABL_STORAGE_QSPI_FLASH) {
		dev_param->qspi.dma_type = DMA_GPC;
	}

	/* Initialize boot device */
	err = tegrabl_storage_init_dev(boot_device, instance, dev_param,
								   SDMMC_INIT_REINIT, false);
	if (err != TEGRABL_NO_ERROR) {
		pr_error("Failed to initialize boot device\n");
		goto fail;
	}

#if defined(CONFIG_ENABLE_DRAM_ECC)
	if (cboot_dram_ecc_enabled() &&
			(boot_params->global_data.disable_staged_scrub == 0)) {
		err = cb_dram_ecc_scrub();
		if (err != TEGRABL_NO_ERROR) {
			pr_error("Failed to Scrub DRAM\n");
			goto fail;
		}
	}
#endif

	/* Initialize available storage devices */

	storage_devs = boot_params->storage_devices;

	/* Storage devices info is added in version 5 of tboot_cpubl_params, hence
	 * retain old storage initialization code to support previous versions.
	 */
	if ((boot_params->version < 5) || (storage_devs[0].type == 0)) {

		pr_debug("Initialize storage devices as per old code because either "
				 "tboot_cpubl_params version is less then 5 or storage info is "
				 "not available\n");

#if defined(CONFIG_ENABLE_EMMC)
		if (boot_device == TEGRABL_STORAGE_QSPI_FLASH) {

			err = tegrabl_storage_init_dev(TEGRABL_STORAGE_SDMMC_BOOT, 0,
										   dev_param, SDMMC_INIT, true);
			if (err != TEGRABL_NO_ERROR) {
				goto fail;
			}
		}
#endif

#if defined(CONFIG_ENABLE_UFS)
		/* Initialize UFS only if odm data is available */
		if (tegrabl_is_ufs_enable()) {

			err = tegrabl_storage_init_dev(TEGRABL_STORAGE_UFS, 0, dev_param,
										   SDMMC_INIT, true);
			if (err != TEGRABL_NO_ERROR) {
				goto fail;
			}
		}
#endif

	} else {

		/* Code to support version 5 */

		err = tegrabl_storage_init_storage_devs(storage_devs, dev_param,
												boot_device, SDMMC_INIT, true);
		if (err != TEGRABL_NO_ERROR) {
			goto fail;
		}

		is_ufs_storage = tegrabl_storage_is_storage_enabled(storage_devs,
										TEGRABL_STORAGE_UFS,
										0);
	}

	tegrabl_profiler_record("Init storage devs", 0, DETAILED);

#if defined(CONFIG_ENABLE_I2C)
	tegrabl_i2c_register();

	err = tegrabl_i2c_set_bus_freq_info(
			(uint32_t *)(uintptr_t)boot_params->i2c_bus_frequency_address,
			TEGRABL_MB1BCT_MAX_I2C_BUSES);
	if (err != TEGRABL_NO_ERROR) {
		pr_error("Error while saving i2c frequency info.\n");
		goto fail;
	}
#endif

#if defined(CONFIG_ENABLE_PARTITION_MANAGER)
	err = tegrabl_partition_manager_init();
	if (err != TEGRABL_NO_ERROR) {
		pr_critical("partition manager init failed\n");
		goto fail;
	}
	tegrabl_profiler_record("Partition manager", 0, DETAILED);
#endif

	err = update_ratchet_fuse();
	if (err != TEGRABL_NO_ERROR) {
		hang_up = true;
		goto fail;
	}

#if defined(CONFIG_DT_SUPPORT)
	/* BL-dtb loaded mb2 is signed with sigheader */
	bl_dtb = (void *)boot_params->bl_dtb_load_address;
	err = tegrabl_dt_set_fdt_handle(TEGRABL_DT_BL, bl_dtb);
	if (err != TEGRABL_NO_ERROR) {
		pr_error("Bl-dtb init failed\n");
		goto fail;
	}
	pr_info("Bl_dtb @%p\n", bl_dtb);

	tegrabl_profiler_record("DTB load", 0, DETAILED);
#endif

#if defined(CONFIG_ENABLE_NCT)
	pr_debug("Load in NCT Partition and Init\n");
	err = tegrabl_nct_init();
	/* TODO: need handle errors properly */
	if (err != TEGRABL_NO_ERROR)
		pr_warn("tegra_nct_init failed with ret:%d\n", err);

	tegrabl_profiler_record("Init NCT", 0, DETAILED);
#endif

#if defined(CONFIG_ENABLE_GPIO)
	gpio_framework_init();
	err = tegrabl_gpio_driver_init();
	if (err != TEGRABL_NO_ERROR) {
		pr_error("GPIO driver init failed\n");
		goto fail;
	}
	err = tegrabl_tca9539_init();
	if (err != TEGRABL_NO_ERROR) {
		pr_error("GPIO TCA9539 driver init failed\n");
		/* WAR for gpio not supporting device tree parsing
		 * Uncomment after adding device tree support
		 * goto fail;
		 */
	}
	tegrabl_profiler_record("Init GPIO driver", 0, DETAILED);
#endif

#if defined(CONFIG_ENABLE_NVBLOB)
	err = tegrabl_load_bmp_blob("BMP");
	if (err != TEGRABL_NO_ERROR)
		pr_warn("Loading bmp blob to memory failed\n");

	tegrabl_profiler_record("Load BMP blob", 0, DETAILED);
#endif

	err = platform_init_power();
	if (TEGRABL_NO_ERROR != err) {
		pr_debug("power init failed\n");
		goto fail;
	}

	tegrabl_profiler_record("Power init", 0, DETAILED);

#if defined(CONFIG_ENABLE_XUSBH)
	pr_info("init usb host\n");
	if (check_enter_fastboot(&enter_fastboot) == TEGRABL_NO_ERROR) {
		if (enter_fastboot == false) {
			err = tegrabl_usbh_init();
			if (err != TEGRABL_NO_ERROR) {
				pr_warn("USB Host is not supported\n");
			}
		}
	}
#endif

	/* TODO: Remove this hack, which skips display initialization for quill-pb
	 * board as for now we don't have power rail configuration done for this
	 * board which results in hdmi gpio detect pin returning wrong status.
	*/
	/* Skip display initialization for E4581 as well since it causes SMMU errors
	 */
	id_info = tegrabl_malloc(sizeof(struct board_id_info));
	if (id_info == NULL) {
		pr_debug("memory allocation failed\n");
		err = TEGRABL_ERROR(TEGRABL_ERR_NO_MEMORY, 0);
		goto fail;
	}
	err = tegrabl_get_board_ids(id_info);
	tegrabl_profiler_record("Get board IDs", 0, DETAILED);

#if !defined(CONFIG_ENABLE_DP)
	if (!strncmp((char *)id_info->part[0].part_no, "3301", 4))
		goto fail; /*skip display*/
#endif
	if (!strncmp((char *)id_info->part[0].part_no, "4581", 4))
		goto fail; /*skip display*/

#if defined(CONFIG_ENABLE_DISPLAY)
	err = tegrabl_display_init();
	if (err != TEGRABL_NO_ERROR)
		pr_warn("display init failed\n");

	tegrabl_profiler_record("Init display", 0, DETAILED);
#endif

fail:
	tegrabl_profiler_record("Platform_init end", 0, DETAILED);

	if (hang_up) {
		pr_error("hang up ...\n");
		while (1)
			;
	}

	return;
}

uint32_t platform_find_core_voltage(void)
{
	return 0;
}

status_t platform_init_heap(void)
{
	const size_t heap_size = CBOOT_HEAP_LEN;
	return tegrabl_heap_init(TEGRABL_HEAP_DEFAULT, CBOOT_HEAP_START,
							 heap_size);
}
