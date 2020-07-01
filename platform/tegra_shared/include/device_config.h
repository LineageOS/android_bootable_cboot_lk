/*
 * Copyright (c) 2014, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited
 */

#ifndef _TEGRA_SHARED_DEVICE_CONFIG_H_
#define _TEGRA_SHARED_DEVICE_CONFIG_H_

/* Defines type of boot device */
typedef enum {
	BLOCK_DEVICE_TYPE_BOOT,
	BLOCK_DEVICE_TYPE_STORAGE,
	BLOCK_DEVICE_TYPE_RPMB,
	BLOCK_DEVICE_TYPE_MAX,
} block_device_type;

/* Returns a string representing the block-device as specified by type */
const char *get_blockdevice_name(block_device_type type);

#endif
