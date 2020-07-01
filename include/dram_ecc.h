/*
 * Copyright (c) 2018, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

/**
 * @file
 * @brief <b>NVIDIA C-boot Interface: DRAM_ECC  </b>
 *
 * @b Description: This file declares APIs for DRAM ECC scrubbing
*/


#ifndef INCLUDED_DRAMECC_H
#define INCLUDED_DRAMECC_H

/* Specifies the enabling of GPCDMA debug messages.*/
#ifdef CB_DRAMECC_DEBUG
#define CB_DRAMECC_DBG(fmt, args...)	pr_info(fmt, ##args)
#else
#define CB_DRAMECC_DBG(fmt, args...)
#endif

#define FIXED_PATTERN 0xBABABABA


/* Returns the size of DRAM */
static uint64_t CbGetSdramSize(void);


/*
 * Compares the base addresses of the two carveouts
 * Input:      Id of carveouts a and b to compare
 * Return: -1, base(a) < base(b)
 *                     0, equal
 *                     1, base(a) > base(b)
*/
static int32_t compare(const uint32_t a, const uint32_t b);


/* Sorts the list of DRAM carveouts */
static void CbDramEccSortCarveout(void);


/*
 * Seperate DRAM carveouts from SYSRAM carveouts and
 * carveouts with 0 size
*/
static void CbDramEccSortCarveoutList(void);

#ifdef CONFIG_VIC_SCRUB
/*
 * Make the block to be scrubbed size aligned to
 * MB if size > 1MB
 * KB if size < 1MB
*/
static void CbDramEccScrubAlignmentCheck(VicTransferConfig *pVicConfig);

/*
 * This function writes a defined fixed pattern into Src Address, which is
 * later copied to any place given by Dest in DRAM as part of VIC scrub
 * function called in the same function
 * Input:	Dest			- Address where pattern is written in DRAM
 *			Src				- Address where pattern is taken to write at dest
 *			ScrubSize		- Size of the DRAM to be scrubbed starting from Dest
 *			ScrubBlockSize	- Size of block which contains fixed pattern
 *								starting from Src
 * Return:	Error
*/
static tegrabl_error_t CbDramEccVICScrubAddrRange(uint64_t Dest, uint64_t Src,
		uint64_t ScrubSize, uint32_t ScrubBlockSize, uint32_t Type);
#endif

/*  Selects the medium of scrub VIC or GPCDMA based on defconfig enabled */
static tegrabl_error_t CbScrubFunction(uint64_t Dest, uint64_t Src,
		uint64_t ScrubSize);

/*
 * Scrub function called from platform.c
 * This function enables the FW required for VIC engine,
 * i.e. HOST1x and VIC Falcon
 * Return:	Error
*/
tegrabl_error_t CbDramEccScrub(void);

/*
 * Implements the cboot scrub based on status of register(s)
 * Scrub is enabled only if the MC_ECC_CONTROL_0 value is set
 * TODO: Implement check on disable_staged_scrub value given through bct
 * Return:	true, enabled
 *		false, disabled
*/
bool cboot_dram_ecc_enabled(void);
#endif
