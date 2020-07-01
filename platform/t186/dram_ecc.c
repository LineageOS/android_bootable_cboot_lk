/*
 * Copyright (c) 2018, NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#include <tegrabl_cpubl_params.h>
#include <address_map_new.h>
#include <tegrabl_gpcdma.h>
#include <tegrabl_debug.h>
#if defined(CONFIG_VIC_SCRUB)
#include <vic.h>
#endif
#include <dram_ecc.h>
#include <tegrabl_drf.h>
#include <armc.h>

#define RAM_BASE NV_ADDRESS_MAP_EMEM_BASE

uint64_t Dram_Start;
uint64_t Dram_End;
struct carve_out_info *carveout;
extern struct tboot_cpubl_params *boot_params;
#if defined(CONFIG_VIC_SCRUB)
static VicTransferConfig s_VicTransferConfig;
#endif
static uint32_t DramCarveouts[CARVEOUT_NUM];
static uint32_t DramCarveoutsCount;

/* Get DRAM size */
static uint64_t CbGetSdramSize(void)
{
	uint32_t SdramSizeMb;

	SdramSizeMb = (NV_READ32(NV_ADDRESS_MAP_MCB_BASE + MC_EMEM_CFG_0) & 0x3FFF);

	return (uint64_t)SdramSizeMb * 1024 * 1024;
}

/* compare the base addresses of crveouts */
static int32_t compare(const uint32_t a, const uint32_t b)
{
	if (carveout[a].base < carveout[b].base)
		return -1;
	else if (carveout[a].base > carveout[b].base)
		return 1;
	else
		return 0;
}

/* Sorting list of Dram Carveouts */
static void CbDramEccSortCarveout(void)
{
	uint32_t val;
	uint32_t i;
	int32_t j;
	uint32_t count;
	count = CARVEOUT_NUM;

	/* Sort them based on physical address */
	if (count < 2) {
		return;
	}
	for (i = 1; i < DramCarveoutsCount; i++) {
		val = DramCarveouts[i];
		for (j = (i - 1); (j >= 0) && (compare(val, DramCarveouts[j]) < 0);
				j--) {
			DramCarveouts[j+1] = DramCarveouts[j];
		}
		DramCarveouts[j+1] = val;
	}
}

/* List of Dram Carveouts Required */
static void CbDramEccSortCarveoutList(void)
{
	enum carve_out_type cotype;
	DramCarveoutsCount = 0;

	for (cotype = CARVEOUT_NVDEC; cotype < CARVEOUT_NUM; cotype++) {
		/* excluding sysram, primary and extended carveouts */
		if ((carveout[cotype].base < Dram_Start) ||
			(carveout[cotype].size == 0) || (cotype == 33) ||
			(cotype == 34))
			continue;

		DramCarveouts[DramCarveoutsCount] = (uint32_t)cotype;
		DramCarveoutsCount++;
	}

	CbDramEccSortCarveout();
}

#if defined(CONFIG_VIC_SCRUB)
static void CbDramEccScrubAlignmentCheck(VicTransferConfig *pVicConfig)
{
	if (pVicConfig->Size >= SIZE_1M) {
		if (pVicConfig->DestAddrPhy & VIC_ADDR_ALIGN_MASK1) {
			/*
			 * When the addr is not 1MB aligned, Reduce the size to be
			 * aligned in KB, so that scrubbing for KB aligned Addr
			 * can happen
			*/
			pVicConfig->Size =
				(SIZE_1M - ((pVicConfig->DestAddrPhy + SIZE_1M) % SIZE_1M));
		} else {
			if (pVicConfig->Size & VIC_SIZE_ALIGN_MASK1) {
				/*  Make the Size 1MB aligned */
				pVicConfig->Size -= (pVicConfig->Size % SIZE_1M);
			}
		}
	}
}

static tegrabl_error_t CbDramEccVICScrubAddrRange(uint64_t Dest, uint64_t Src,
		uint64_t ScrubSize, uint32_t ScrubBlockSize, uint32_t Type) {
	tegrabl_error_t err = TEGRABL_NO_ERROR;
	uint64_t End = (Dest + ScrubSize);
	VicTransferConfig *pVicTransferConfig = &s_VicTransferConfig;

	memset(pVicTransferConfig, 0, sizeof(VicTransferConfig));

	pr_debug("Src:0x%lx Dest:0x%lx ScrubSize:0x%lx ScrubBlockSize:0x%u\n",
			 Src, Dest, ScrubSize, ScrubBlockSize);

	pVicTransferConfig->SrcAddrPhy = Src;
	pVicTransferConfig->DestAddrPhy = Dest;
	pVicTransferConfig->Size = ScrubSize;

	if ((pVicTransferConfig->DestAddrPhy + ScrubBlockSize) > End) {
		pVicTransferConfig->Size = (End - pVicTransferConfig->DestAddrPhy);
	}

	/* Adjust the size based on addr alginment */
	CbDramEccScrubAlignmentCheck(&s_VicTransferConfig);

	while (pVicTransferConfig->DestAddrPhy < End) {
		err = CbVicScrub(0, CB_VIC_TRANSFER, pVicTransferConfig);
		if (err != TEGRABL_NO_ERROR)
			goto fail;

		/*
		 * If the single ASYNC invoking  results in multiple VIC copy
		 * (for alginment reasons) we need to make sure ASYNC VIC copy is done
		 * for last chunk and rest all VIC copy happens as SYNC
		*/
		if (((pVicTransferConfig->DestAddrPhy + pVicTransferConfig->Size)
					>= End) && Type == CB_VIC_SCRUB_ASYNC) {
			break;
		}

		err = CbVicScrub(0, CB_VIC_WAIT_FOR_TRANSFER_COMPLETE,
					(void *)CB_VIC_WAIT_FOR_TRANSFER_COMPLETE);
		if (err != TEGRABL_NO_ERROR)
			goto fail;

		/*  Increment the DestAddrPhy with size of memory just got scrubbed */
		pVicTransferConfig->DestAddrPhy += pVicTransferConfig->Size;

		/*
		 * Re-init the Size with  original ScrubBlockSize so that we can again
		 * check for alignments and attempt maximum scrub size possible
		*/
		pVicTransferConfig->Size = ScrubBlockSize;

		if ((pVicTransferConfig->DestAddrPhy +
					pVicTransferConfig->Size) > End) {
			pVicTransferConfig->Size = (End - pVicTransferConfig->DestAddrPhy);
		}

		/* Adjust the size based on addr alginment */
		CbDramEccScrubAlignmentCheck(&s_VicTransferConfig);
	}

fail:
	if (err != TEGRABL_NO_ERROR) {
		pr_error("%s:VIC scrub error  = %x, Dest = 0x%lx\n", __func__, err,
				 pVicTransferConfig->DestAddrPhy);
	}
	return err;
}
#endif

static tegrabl_error_t CbScrubFunction(uint64_t Dest, uint64_t Src,
		uint64_t ScrubSize)
{
#if defined(CONFIG_VIC_SCRUB)
	return CbDramEccVICScrubAddrRange(Dest, Src, ScrubSize, SCRUB_BLOCK_SIZE,
			CB_VIC_SCRUB_SYNC);
#else
	return tegrabl_init_scrub_dma(Dest, Src, FIXED_PATTERN, ScrubSize,
			DMA_PATTERN_FILL);
#endif
}

/* Scrub Function */
tegrabl_error_t CbDramEccScrub(void)
{
	tegrabl_error_t err = TEGRABL_NO_ERROR;
	uint64_t ScrubBase;
	uint64_t ScrubSize = 0;
	uint32_t count = 0;
	uint64_t Src = 0;

	pr_info("Dram Scrub in progress\n");
	Dram_Start = RAM_BASE;
	Dram_End = Dram_Start + CbGetSdramSize();

	carveout = boot_params->global_data.carveout;
#if defined(CONFIG_VIC_SCRUB)
	pr_debug("VIC Scrub Enabled\n");
	/* Initialize the VIC Engine */
	err = CbVicInit();
	if (err != TEGRABL_NO_ERROR)
		goto fail;

	Src = carveout[CARVEOUT_CPUBL].base + carveout[CARVEOUT_CPUBL].size -
				SCRUB_BLOCK_SIZE;
	if (Src == 0) {
		pr_error("Memory allocation error\n");
		return TEGRABL_NO_ERROR;
	}

	/* Write fixed pattern to the above memory in CARVEOUT_CPUBL */
	err = tegrabl_init_scrub_dma(Src, 0, FIXED_PATTERN,
			SCRUB_BLOCK_SIZE, DMA_PATTERN_FILL);
	if (err != TEGRABL_NO_ERROR)
		goto fail;
#else
	pr_debug("VIC Scrub Disabled\n");
#endif
	/* Sort before scrub */
	CbDramEccSortCarveoutList();
	pr_debug("Carveouts Sorted\n");

	ScrubBase = Dram_Start;
	while (count < DramCarveoutsCount) {
		if (ScrubBase == carveout[DramCarveouts[count]].base) {
			ScrubBase += carveout[DramCarveouts[count]].size;
			count++;
			continue; /* no need to scrub carveouts */
		}

		ScrubSize = carveout[DramCarveouts[count]].base - ScrubBase;

		/*  Max Scrubsize VIC and GPCDMA can scrub is 1GB */
		if (ScrubSize > 1073741824)
			ScrubSize = 1073741824;

		pr_debug("ScrubBase: %lx ScrubSize: %lx ScrubEnd: %lx\n", ScrubBase,
				 ScrubSize, ScrubBase+ScrubSize);
		err = CbScrubFunction(ScrubBase, Src, ScrubSize);
		if (err != TEGRABL_NO_ERROR)
			goto fail;

		ScrubBase += ScrubSize;
	}

	/* for the memory from end of last carveout to end of dram */
	if (ScrubBase < Dram_End) {
		err = CbScrubFunction(ScrubBase, Src, ScrubSize);
		if (err != TEGRABL_NO_ERROR)
			goto fail;
	}

#if defined(CONFIG_VIC_SCRUB)
	/*  Power off the VIC FC */
	err = CbVicExit();
	if (err != TEGRABL_NO_ERROR)
		goto fail;
#endif
	pr_info("DRAM Scrub Successfull\n");

fail:
	if (err != TEGRABL_NO_ERROR)
		pr_error("%s:DRAM ECC scrub failed, error: %u", __func__, err);
	return err;
}

bool cboot_dram_ecc_enabled(void)
{
	bool dram_ecc_enabled;

	dram_ecc_enabled = NV_DRF_VAL(MC, ECC_CONTROL, ECC_ENABLE,
			NV_READ32(NV_ADDRESS_MAP_MCB_BASE + MC_ECC_CONTROL_0));

	return dram_ecc_enabled;
}
