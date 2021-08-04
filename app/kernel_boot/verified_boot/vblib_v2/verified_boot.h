/*
 * Copyright (c) 2015-2018, NVIDIA Corporation.	All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#ifndef __VERIFIED_BOOT__
#define __VERIFIED_BOOT__

#include <boot.h>
#include <tegrabl_bootimg.h>
#include <tegrabl_error.h>
#include <libavb/libavb.h>

/**
 * The verified boot states Red, Yellow and Green are arranged in increasing
 * order of level of trust for binaries verified by the bootloader
 * (Currently, boot.img and kernel-dtb)
 */
/*macro boot_state*/
typedef uint32_t boot_state_t;
#define VERIFIED_BOOT_UNKNOWN_STATE 0
#define VERIFIED_BOOT_RED_STATE 1
#define VERIFIED_BOOT_YELLOW_STATE 2
#define VERIFIED_BOOT_GREEN_STATE 3
#define VERIFIED_BOOT_ORANGE_STATE 4

#define SMC_SET_ROOT_OF_TRUST 0x3200FFFF
#define SMC_TOS_RESTART_LAST  0x3C000000

/* All sizes are in bytes */
#define HASH_DIGEST_SIZE 32
#define VERITY_KEY_SIZE 256

/* This structure must _always_ be in sync with the corresponding Secure OS
 * structure, currently defined within trusty at lib/trusty/verified_boot.h
 */

#define MAGIC_HEADER 0xCAFEFEED
#define VERSION 0

/* Root Of Trust is defined as Public Key + Lock/Unlock State */
struct root_of_trust {
	uint32_t magic_header;
	uint32_t version;
	uint8_t dtb_pub_key[VERITY_KEY_SIZE];
	uint8_t boot_pub_key[VERITY_KEY_SIZE];
	uint8_t verified_boot_state;
};

/**
 * @brief Get the boot state of the device. If the device is unlocked, boot in
 *		  orange state. Else, depending upon the integrity of vbmeta and
 *		  the key used to validate the vbmeta, boot in green, yellow or red
 *		  state
 *
 * @param bs boot state value returned
 * @param slot_data verify result info (output parameter)
 *
 * @return NO_ERROR if boot state is conclusively determined, else apt error
 */
status_t verified_boot_get_boot_state(boot_state_t *bs,
									  AvbSlotVerifyData **slot_data);

/**
 * @brief Display UI for certain states
 *
 * @param bs boot state to display the UI for
 * @param slot_data verify result from libavb, which contains public key used to
 *                  veriry all vbmeta
 *
 * @return NO_ERROR on successfully getting confirmation from user to boot in
 *					the relevant state
 */
status_t verified_boot_ui(boot_state_t bs, AvbSlotVerifyData *slot_data);

/**
 * @brief SMC Call into the monitor
 *
 * @param arg0 SMC handler to invoke
 * @param args1-2 Args in expected format for specific SMC handler
 * @return NO_ERROR 0 on success / appropriate error on failure.
 */
uint32_t smc_call(uint32_t arg0, uintptr_t arg1, uintptr_t arg2);

/**
 * @brief process all verified boot related issues aka verify boot.img
 *        signature, pass params to tlk, show verified boot UI
 *
 * @param hdr boot.img header address
 * @param kernel_dtb kernel-dtb address
 * @param kernel_dtbo kernel-dtbo address
 *
 * @return NO_ERROR if all process is successfully, else apt error
 */

tegrabl_error_t verify_boot(union tegrabl_bootimg_header *hdr,
							void *kernel_dtb, void *kernel_dtbo);

#endif
