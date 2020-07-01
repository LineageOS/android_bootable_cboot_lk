/*
 * Copyright (c) 2018, NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#include <string.h>
#include <printf.h>
#include <tegrabl_cpubl_params.h>
#include <address_map_new.h>
#include <tegrabl_gpcdma.h>
#include <tegrabl_debug.h>
#if defined(CONFIG_VIC_SCRUB)
#include <tegrabl_vic.h>
#endif
#include <dram_ecc.h>
#include <tegrabl_drf.h>
#include <armc.h>
#include <tegrabl_linuxboot_helper.h>

#define RAM_BASE NV_ADDRESS_MAP_EMEM_BASE

uint64_t dram_start;
uint64_t dram_end;
struct carve_out_info *carveout;
extern struct tboot_cpubl_params *boot_params;
#if defined(CONFIG_VIC_SCRUB)
static struct vic_transfer_config s_vic_transfer_config;
#endif
static uint32_t dram_carveouts[CARVEOUT_NUM];
static uint32_t dram_carveouts_count;

/* Return the size of DRAM in MB */
static uint64_t cb_get_dram_size(void)
{
	uint32_t sdram_size;

	sdram_size = (NV_READ32(NV_ADDRESS_MAP_MCB_BASE + MC_EMEM_CFG_0) & 0x3FFF);

	return (uint64_t)sdram_size * 1024 * 1024;
}

/*
 * Seperate DRAM carveouts from SYSRAM carveouts and
 * carveouts with 0 size
*/
static void cb_dram_ecc_sort_carveout_list(void)
{
	enum carve_out_type cotype;
	dram_carveouts_count = 0;

	for (cotype = CARVEOUT_NVDEC; cotype < CARVEOUT_NUM; cotype++) {
		/* excluding sysram, primary and extended carveouts */
		if ((carveout[cotype].base < dram_start) ||
			(carveout[cotype].size == 0) || (cotype == 33) ||
			(cotype == 34)) {
			continue;
		}

		dram_carveouts[dram_carveouts_count] = (uint32_t)cotype;
		dram_carveouts_count++;
	}

	sort(dram_carveouts, dram_carveouts_count);
}

#if defined(CONFIG_VIC_SCRUB)
/*
 * Make the block to be scrubbed size aligned to
 * MB if size > 1MB
 * KB if size < 1MB
*/
static void cb_dram_ecc_scrub_alignment_check(struct vic_transfer_config *p_vic_config)
{
	if (p_vic_config->size >= SIZE_1M) {
		if (p_vic_config->dest_addr_phy & VIC_ADDR_ALIGN_MASK1) {
			/*
			 * When the addr is not 1MB aligned, Reduce the size to be
			 * aligned in KB, so that scrubbing for KB aligned Addr
			 * can happen
			*/
			p_vic_config->size =
				(SIZE_1M - ((p_vic_config->dest_addr_phy + SIZE_1M) % SIZE_1M));
		} else {
			if (p_vic_config->size & VIC_SIZE_ALIGN_MASK1) {
				/*  Make the Size 1MB aligned */
				p_vic_config->size -= (p_vic_config->size % SIZE_1M);
			}
		}
	}
}

/*
 * This function writes a defined fixed pattern into Src Address, which is
 * later copied to any place given by Dest in DRAM as part of VIC scrub
 * function called in the same function
 * Input:   Dest            - Address where pattern is written in DRAM
 *          Src             - Address where pattern is taken to write at dest
 *          ScrubSize       - Size of the DRAM to be scrubbed starting from Dest
 *          ScrubBlockSize  - Size of block which contains fixed pattern
 *                              starting from Src
 * Return:  Error
*/
static tegrabl_error_t cb_dram_ecc_vic_scrub_addr_range(uint64_t dest,
		uint64_t src, uint64_t scrubsize, uint32_t scrub_block_size,
		uint32_t type) {
	tegrabl_error_t err = TEGRABL_NO_ERROR;
	uint64_t end = (dest + scrubsize);
	struct vic_transfer_config *p_vic_transfer_config = &s_vic_transfer_config;

	memset(p_vic_transfer_config, 0, sizeof(struct vic_transfer_config));

	pr_debug("Src:0x%lx Dest:0x%lx ScrubSize:0x%lx ScrubBlockSize:0x%u\n",
			 src, dest, scrubsize, scrub_block_size);

	p_vic_transfer_config->src_addr_phy = src;
	p_vic_transfer_config->dest_addr_phy = dest;
	p_vic_transfer_config->size = scrubsize;

	if ((p_vic_transfer_config->dest_addr_phy + scrub_block_size) > end) {
		p_vic_transfer_config->size = (end -
				p_vic_transfer_config->dest_addr_phy);
	}

	/* Adjust the size based on addr alginment */
	cb_dram_ecc_scrub_alignment_check(&s_vic_transfer_config);

	while (p_vic_transfer_config->dest_addr_phy < end) {
		err = cb_vic_scrub(0, CB_VIC_TRANSFER, p_vic_transfer_config);
		if (err != TEGRABL_NO_ERROR) {
			pr_error("VIC: Transfer Error\n");
			goto fail;
		}

		/*
		 * If the single ASYNC invoking  results in multiple VIC copy
		 * (for alginment reasons) we need to make sure ASYNC VIC copy is done
		 * for last chunk and rest all VIC copy happens as SYNC
		*/
		if (((p_vic_transfer_config->dest_addr_phy +
					p_vic_transfer_config->size) >= end) && type ==
					CB_VIC_SCRUB_ASYNC) {
			break;
		}

		err = cb_vic_scrub(0, CB_VIC_WAIT_FOR_TRANSFER_COMPLETE,
					(void *)CB_VIC_WAIT_FOR_TRANSFER_COMPLETE);
		if (err != TEGRABL_NO_ERROR) {
			pr_error("VIC: Wait Error\n");
			goto fail;
		}

		/*  Increment the DestAddrPhy with size of memory just got scrubbed */
		p_vic_transfer_config->dest_addr_phy += p_vic_transfer_config->size;

		/*
		 * Re-init the Size with  original ScrubBlockSize so that we can again
		 * check for alignments and attempt maximum scrub size possible
		*/
		p_vic_transfer_config->size = scrub_block_size;

		if ((p_vic_transfer_config->dest_addr_phy +
					p_vic_transfer_config->size) > end) {
			p_vic_transfer_config->size = (end -
					p_vic_transfer_config->dest_addr_phy);
		}

		/* Adjust the size based on addr alginment */
		cb_dram_ecc_scrub_alignment_check(&s_vic_transfer_config);
	}

fail:
	if (err != TEGRABL_NO_ERROR) {
		pr_error("%s:VIC scrub error  = %x, Dest = 0x%lx\n", __func__, err,
				 p_vic_transfer_config->dest_addr_phy);
	}
	return err;
}
#endif

/* Selects the medium of scrub VIC or GPCDMA based on defconfig enabled */
static tegrabl_error_t cb_scrub_function(uint64_t dest, uint64_t src,
		uint64_t scrubsize)
{
#if defined(CONFIG_VIC_SCRUB)
	return cb_dram_ecc_vic_scrub_addr_range(dest, src, scrubsize,
			SCRUB_BLOCK_SIZE, CB_VIC_SCRUB_SYNC);
#else
	return tegrabl_init_scrub_dma(dest, src, FIXED_PATTERN, scrubsize,
			DMA_PATTERN_FILL);
#endif
}

/* Scrub Function */
tegrabl_error_t cb_dram_ecc_scrub(void)
{
	tegrabl_error_t err = TEGRABL_NO_ERROR;
	uint64_t scrub_base;
	uint64_t scrub_size = 0;
	uint32_t count = 0;
	uint64_t src = 0;
	uint64_t total_scrub_size = 0;

	pr_info("Dram Scrub in progress\n");
	dram_start = RAM_BASE;
	dram_end = dram_start + cb_get_dram_size();

	carveout = boot_params->global_data.carveout;
#if defined(CONFIG_VIC_SCRUB)
	pr_debug("VIC Scrub Enabled\n");
	/* Initialize the VIC Engine */
	err = cb_vic_init();
	if (err != TEGRABL_NO_ERROR) {
		pr_error("VIC: Init Error\n");
		goto fail;
	}

	src = carveout[CARVEOUT_CPUBL].base + carveout[CARVEOUT_CPUBL].size -
				SCRUB_BLOCK_SIZE;
	if (src == 0) {
		pr_error("VIC: Memory allocation error\n");
		return TEGRABL_NO_ERROR;
	}

	/* Write fixed pattern to the above memory in CARVEOUT_CPUBL */
	err = tegrabl_init_scrub_dma(src, 0, FIXED_PATTERN,
			SCRUB_BLOCK_SIZE, DMA_PATTERN_FILL);
	if (err != TEGRABL_NO_ERROR) {
		pr_error("VIC: Write to scrub block failed\n");
		goto fail;
	}
#else
	pr_debug("VIC Scrub Disabled\n");
#endif
	/* Sort before scrub */
	cb_dram_ecc_sort_carveout_list();
	pr_debug("Carveouts Sorted\n");

	scrub_base = dram_start;
	while (count < dram_carveouts_count) {
		if (scrub_base == carveout[dram_carveouts[count]].base) {
			scrub_base += carveout[dram_carveouts[count]].size;
			count++;
			continue; /* no need to scrub carveouts */
		}

		scrub_size = carveout[dram_carveouts[count]].base - scrub_base;

		/*  Max Scrubsize VIC and GPCDMA can scrub is 1GB */
		if (scrub_size > 1073741824) {
			scrub_size = 1073741824;
		}

		pr_debug("ScrubBase: %lx ScrubSize: %lx ScrubEnd: %lx\n", scrub_base,
				 scrub_size, scrub_base + scrub_size);
		err = cb_scrub_function(scrub_base, src, scrub_size);
		if (err != TEGRABL_NO_ERROR) {
			pr_error("Scrub failed, Scrub_Base: %lx Scrub_End: %lx", scrub_base,
					scrub_base + scrub_size);
			goto fail;
		}

		scrub_base += scrub_size;
		total_scrub_size += scrub_size;
	}

	/* for the memory from end of last carveout to end of dram */
	if (scrub_base < dram_end) {
		scrub_size = dram_end - scrub_base;
		err = cb_scrub_function(scrub_base, src, scrub_size);
		if (err != TEGRABL_NO_ERROR) {
			pr_error("Scrub Failed at end of dram\n");
			goto fail;
		}
		total_scrub_size += scrub_size;
	}

	pr_debug("Total size Scrubbed 0x%"PRIx64"\n", total_scrub_size);
#if defined(CONFIG_VIC_SCRUB)
	/*  Power off the VIC FC */
	err = cb_vic_exit();
	if (err != TEGRABL_NO_ERROR) {
		pr_error("VIC: Exit Error\n");
		goto fail;
	}
#endif
	pr_info("DRAM Scrub Successful\n");

fail:
	if (err != TEGRABL_NO_ERROR) {
		pr_error("%s:DRAM ECC scrub failed, error: %u", __func__, err);
	}
	return err;
}

bool cboot_dram_ecc_enabled(void)
{
	bool dram_ecc_enabled;

	dram_ecc_enabled = NV_DRF_VAL(MC, ECC_CONTROL, ECC_ENABLE,
			NV_READ32(NV_ADDRESS_MAP_MCB_BASE + MC_ECC_CONTROL_0));

	return dram_ecc_enabled;
}
