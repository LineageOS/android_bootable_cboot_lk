/*
 * Copyright (c) 2018, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited
 */

#ifndef INCLUDED_NET_BOOT_H
#define INCLUDED_NET_BOOT_H

#if defined(CONFIG_ENABLE_ETHERNET_BOOT)
/**
 * @brief Initialize network stack
 *
 * @return TEGRABL_NO_ERROR on success, otherwise appropriate error
 */
tegrabl_error_t net_boot_stack_init(void);

/**
 * @brief Load kernel and kernel-dtb from network
 *
 * @param boot_img_load_addr boot image load address (output)
 * @param dtb_load_addr dtb load address (output)
 *
 * @return TEGRABL_NO_ERROR on success, otherwise appropriate error
 */
tegrabl_error_t net_boot_load_kernel_images(void ** const boot_img_load_addr, void ** const dtb_load_addr);
#endif /* CONFIG_ENABLE_ETHERNET_BOOT */

#endif /* INCLUDED_NET_BOOT_H */
