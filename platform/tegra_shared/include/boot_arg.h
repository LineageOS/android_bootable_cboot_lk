/*
 * Copyright (c) 2014 - 2015, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited
 */

#ifndef __BOOT_ARG_H
#define __BOOT_ARG_H

#include <sys/types.h>

/*---------------------------------------------------------------------- */
/* NOTE: The contents of this file should always be in sync with nvtboot */
/*---------------------------------------------------------------------- */

/**
  * Maximum possible wb0 size(bytes)
  */
#define WBO_MAXIMUM_CODE_SIZE  4096

 /**
  * Maximum possible EKS size(bytes)
  */
#define EKS_MAXIMUM_CODE_SIZE  512

 /**
  * Maximum possible DTB size(bytes)
  */
#define DTB_MAXIMUM_CODE_SIZE  (200*1024)

 /**
  * Defines the memory layout type to be queried for its base/size
  */
typedef enum {
	/* Invalid entry */
	MEM_LAYOUT_NONE = 0x0,

	/* Primary memory layout */
	MEM_LAYOUT_PRIMARY,

	/* Extended memory layout */
	MEM_LAYOUT_EXTENDED,

	/* VPR carveout information */
	MEM_LAYOUT_VPR,

	/* TSEC carveout information */
	MEM_LAYOUT_TSEC,

	/* SECUREOS carveout information */
	MEM_LAYOUT_SECUREOS,

	/* LP0 carveout information */
	MEM_LAYOUT_LP0,

	/* NCK carveout information */
	MEM_LAYOUT_NCK,

	/* DEBUG carveout information */
	MEM_LAYOUT_DEBUG,

	/* BPMP FW carveout information */
	MEM_LAYOUT_BPMPFW,

	/* RAMDump carveout information */
	MEM_LAYOUT_NVDUMPER,

	/* carveout for GSC1 */
	MEM_LAYOUT_GSC1,

	/* carveout for GSC2 */
	MEM_LAYOUT_GSC2,

	/* carveout for GSC3 */
	MEM_LAYOUT_GSC3,

	/* carveout for GSC4 */
	MEM_LAYOUT_GSC4,

	/* carveout for GSC5 */
	MEM_LAYOUT_GSC5,

	/* Total types of memory layout */
	MEM_LAYOUT_NUM,
} mem_layout;

/**
 * Reason for poweron
 */
typedef enum {
	poweron_source_pmcpor,
	poweron_source_apwdt_rst,
	poweron_source_sensor_rst,
	poweron_source_sw_rst,
	poweron_source_deepsleep_rst,
	poweron_source_aotag_rst,
	poweron_source_onkey,
	poweron_source_resetkey,
	poweron_source_usbhotplug,
	poweron_source_rtcalarm,
	poweron_source_pmuwdt,
	poweron_source_unknown = 0x7fffffff,
 } poweron_source;

 /**
  * Tracks the base and size of the Carveout
  */
typedef struct {
	/* Base of the carveout */
	 uint64_t base;

	/* Size of the carveout */
	 uint64_t size;
} carveout_info;

 /**
  * Stores the information related to memory params
  */
typedef struct {
	/* Size and Base of the carveout */
	carveout_info car_info[MEM_LAYOUT_NUM];

	/* DRAM bottom information */
	uint64_t dram_carveout_bottom;
} mem_info;

/**
 * Stores the information related to memory params
 */
typedef enum
{
    /// Specifies the memory type to be undefined
    dram_memory_Type_None = 0,

    /// Specifies the memory type to be LPDDR2 SDRAM
    dram_memory_type_lpddr2,

    /// Specifies the memory type to be DDR3 SDRAM
    dram_memory_type_ddr3,

    dram_memory_type_lpddr4,

    dram_memory_type_num,
    dram_memory_type_force32 = 0x7FFFFFF
} dram_memory_type;

/**
 * Defines the BootDevice Type available
 */
typedef enum {
	/* boot device is sdmmc */
	BOOT_DEVICE_SDMMC,

	/* boot device is snor */
	BOOT_DEVICE_SNOR_FLASH,

	/* boot device is spi */
	BOOT_DEVICE_SPI_FLASH,

	/* boot device is sata */
	BOOT_DEVICE_SATA,

	/* boot device is usb3 */
	BOOT_DEVICE_USB3,

	/*reserved boot device */
	BOOT_DEVICE_RESVD,

	/* max number of boot devices */
	BOOT_DEVICE_MAX,
} fuse_boot_device;

/**
 * Device type and instance
 */
typedef struct {
	/* boot of storage device type */
	fuse_boot_device device;

	/* instance of the device */
	uint32_t instance;
} dev_info;

/**
 * Holds the chip's unique ID.
 */
typedef struct {
	/* Bytes 3-0 of the ecid */
	uint32_t ecid_0;

	/* Bytes 7-4 of ecid */
	uint32_t ecid_1;

	/* Bytes 11-8 of ecid */
	uint32_t ecid_2;

	/* Bytes 15-12 of ecid */
	uint32_t ecid_3;
} unique_chip;

/**
 * Defines the board information stored on eeprom
 */
typedef struct {
	/* Holds the version number. */
	uint16_t version;

	/* Holds the size of 2 byte value of \a BoardId data that follows. */
	uint16_t size;

	/* Holds the board number. */
	uint16_t board_id;

	/* Holds the SKU number. */
	uint16_t sku;

	/* Holds the FAB number. */
	uint8_t fab;

	/* Holds the part revision. */
	uint8_t revision;

	/* Holds the minor revision level. */
	uint8_t minor_revision;

	/* Holds the memory type. */
	uint8_t mem_type;

	/* Holds the power configuration values. */
	uint8_t power_config;

	/* Holds the SPL reworks, mech. changes. */
	uint8_t misc_config;

	/* Holds the modem bands value. */
	uint8_t modem_bands;

	/* Holds the touch screen configs. */
	uint8_t touch_screen_configs;

	/* Holds the display configs. */
	uint8_t display_configs;
} board_info;

/**
 * Specifies different modes of booting.
 */
typedef enum {
	/* Specifies a default (unset) value. */
	BOOT_MODE_NONE = 0,

	/* Specifies a cold boot */
	BOOT_MODE_COLD,

	/* Specifies the BR entered RCM */
	BOOT_MODE_RECOVERY,

	/* Specifies UART boot (only available internal to NVIDIA) */
	BOOT_MODE_UART,
} boot_mode;

/**
 * Specifies different operating modes.
 */
typedef enum {
	/* Invalid Operating mode */
	OPMODE_NONE = 0,

	/* Preproduction mode */
	OPMODE_PRE_PRODUCTION,

	/* failure analysis mode */
	OPMODE_FAILURE_ANALYSIS,

	/* nvproduction mode */
	OPMODE_NV_PRODUCTION,

	/* non-secure mode */
	OPMODE_ODM_PRODUCTION_NON_SECURE,

	/* sbk mode */
	OPMODE_ODM_PRODUCTION_SECURE_SBK,

	/* pkc mode */
	OPMODE_ODM_PRODUCTION_SECURE_PKC,
} opmode_type;

typedef struct {
	/* Active BCT block */
	uint32_t active_bct_block;

	/* Active BCT sector */
	uint32_t active_bct_sector;

} active_bct_info;

/**
 * Specifies shared structure.
 */
typedef struct {
	/* Revision of common structure */
	uint32_t revision;

	/* CRC of this structure */
	uint32_t crc;

	/* Early Uart Address */
	uint64_t early_uart_addr;

	/* Memory layout Information */
	mem_info  params;

	/* Poweron Source */
	poweron_source pwron_src;

	/* Pmic shutdown reason */
	uint8_t pmic_reset_reason;

	/* Information about bootdevice */
	dev_info boot_dev;

	/* Information about storage device */
	dev_info storage_dev;

	/* Unique Chip Id */
	unique_chip uid;

	dram_memory_type dram_type;

	/* Processor Board Details */
	board_info proc_board_details;

	/* Pmu Board Details */
	board_info pmu_board_details;

	/* Display BoardDetails */
	board_info display_board_details;

	/* Size of Bct in bytes */
	uint32_t bct_size;

	/* Size of start Offset of tboot in bytes */
	uint64_t tboot_start_offset_inbytes;

	/* Size of boot device block size in  bytes */
	uint32_t block_size_inbytes;

	/* Size of boot device page size in bytes */
	uint32_t page_size_inbytes;

	/* Boot mode can be cold boot, uart or recovery */
	boot_mode boot_type;

	/* Operating Mode of device */
	opmode_type op_mode;

	/* Size of start Offset of mts preboot in bytes */
	uint64_t mts_start_offset_inbytes;

	/* Info of the bct being used */
	active_bct_info active_bct;

	/* Bootloader dtb load address */
	uint32_t bl_dtb_load_addr;

	/* Kernel dtb load address */
	uint32_t kernel_dtb_load_addr;

	/* powerkey long press state */
	uint32_t powerkey_long_press_state;

	/* Reserved Bytes at the end */
	uint8_t reserved[252];
} boot_arg;


/**
 * Specifies enum for accessing shared information.
 */
typedef enum {
	/* get revision */
	BOOT_ARG_REVISION = 0x0,

	/* get crc of shared structure */
	BOOT_ARG_CRC,

	/* get early uart base */
	BOOT_ARG_UART_ADDR,

	/* get dram botton size */
	BOOT_ARG_DRAM_BOTTOM,

	/* get primary mem layout */
	BOOT_ARG_MEM_LAYOUT_PRIMARY,

	/* get extended mem layout */
	BOOT_ARG_MEM_LAYOUT_EXTENDED,

	/* get vpr mem layout */
	BOOT_ARG_MEM_LAYOUT_VPR,

	/* get tsec mem layout */
	BOOT_ARG_MEM_LAYOUT_TSEC,

	/* get secureos mem layout */
	BOOT_ARG_MEM_LAYOUT_SECUREOS,

	/* get lp0 mem layout */
	BOOT_ARG_MEM_LAYOUT_LP0,

	/* get xusb mem layout */
	BOOT_ARG_MEM_LAYOUT_XUSB,

	/* get nck mem layout */
	BOOT_ARG_MEM_LAYOUT_NCK,

	/* get debug mem layout */
	BOOT_ARG_MEM_LAYOUT_DEBUG,

	/* get poweron source */
	BOOT_ARG_POWERON_SRC,

	/* get poweron source */
	BOOT_ARG_PMIC_RESET_REASON,

	/* get secondary boot device info */
	BOOT_ARG_BOOT_DEV,

	/* get storage boot device info */
	BOOT_ARG_STORAGE_DEV,

	/* get uid of the chip */
	BOOT_ARG_UID,

	/* get memory type of dram */
	BOOT_ARG_MEM_TYPE,

	/* get processor board id */
	BOOT_ARG_PROC_BOARD,

	/* get pmu board id */
	BOOT_ARG_PMU_BOARD,

	/* get display board id */
	BOOT_ARG_DISPLAY_BOARD,

	/* get bct size in bytes */
	BOOT_ARG_BCT_SIZE,

	/* get tboot start offset */
	BOOT_ARG_TBOOT_START,

	/* get block sie in bytes */
	BOOT_ARG_BLOCK_SIZE,

	/* get page size in bytes */
	BOOT_ARG_PAGE_SIZE,

	/* get boot type  (cold, uart or recovery) */
	BOOT_ARG_BOOT_TYPE,

	/* get the operating mode */
	BOOT_ARG_OPMODE,

	/* get the Mts Start Address */
	BOOT_ARG_MTS_START,
	/* get Bpmp FW mem layout */
	BOOT_ARG_MEM_LAYOUT_BPMPFW,

	/* get RAM dump mem layout */
	BOOT_ARG_MEM_LAYOUT_NVDUMPER,

	/* get Active BCT info */
	BOOT_ARG_ACTIVE_BCT,

	/* get bl dtb load address */
	BOOT_ARG_BL_DTB_LOAD_ADDR,

	/* get kernel dtb load address */
	BOOT_ARG_KERNEL_DTB_LOAD_ADDR,

	/* get powerkey long press state */
	BOOT_ARG_POWERKEY_LONG_PRESS_STATE,
} boot_arg_type;

/** @brief Validates CRC of the shared structure.
 *
 *  @param Void.
 *
 *  @return Void.
 */
void boot_arg_validation(void);

/** @brief Prints the contents of the shared structure.
 *
 *  @param Void.
 *
 *  @return Void.
 */
void  boot_arg_print_info(void);

/** @brief Queries the contents of the members of shared structure.
 *
 *  @param type Type of the input boot argument
 *              whose contents is to be queried.
 *
 *  @param buf Pointer to store contents of the boot argument.
 *  @param size Pointer to store size of the boot argument.
 *
 *  @return NO_ERROR if query contents is successful.
 */
status_t boot_arg_query_info(boot_arg_type type,
							 void *buf, uint32_t *size);
#endif /* __BOOT_ARG_H */

