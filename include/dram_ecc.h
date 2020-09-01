/*
 * Copyright (c) 2018-2019, NVIDIA CORPORATION.  All rights reserved.
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

/*
 * Scrub function called from platform.c
 * This function enables the FW required for VIC engine,
 * i.e. HOST1x and VIC Falcon
 * Return:	Error
*/
tegrabl_error_t cb_dram_ecc_scrub(void);

/*
 * Check for the cboot scrub based on status of register(s)
 * Scrub is enabled only if the MC_ECC_CONTROL_0 value is set
 * and disable_staged_scrub value given through bct is not equal to 1
 * Return:	true, enabled
 *		false, disabled
*/
bool cboot_dram_ecc_enabled(void);
#endif
