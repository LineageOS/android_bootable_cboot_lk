/*
 * Copyright (c) 2017-2019, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited
 */

#define MODULE TEGRABL_ERR_NO_MODULE

#include <inttypes.h>
#include <tegrabl_blockdev.h>
#include <tegrabl_partition_manager.h>
#include <tegrabl_debug.h>
#include <tegrabl_error.h>
#include <tegrabl_malloc.h>
#include <string.h>
#include <nvboot_warm_boot_0.h>
#include <tegrabl_binary_types.h>


#define IMAGE_CHUNK_SIZE  (8 * 1024)


static tegrabl_error_t tegrabl_storage_get_bin_size(
											tegrabl_binary_type_t bin_type,
											const char *bin_name,
											struct tegrabl_partition *partition,
											uint32_t *image_size)
{
	tegrabl_bdev_t *block_dev = NULL;
	uint32_t block_size_log2;
	uint32_t load_size;
	uint32_t start_block;
	uint32_t num_blocks;
	void *buffer = NULL;
	tegrabl_error_t err = TEGRABL_NO_ERROR;

	if (partition == NULL) {
		pr_error("NULL partition arg passed\n");
		err = TEGRABL_ERROR(TEGRABL_ERR_INVALID, 2);
		goto fail;
	}

	switch (bin_type) {

	case TEGRABL_BINARY_SC7_RESUME_FW:

		pr_debug("Get bin: (%s) size\n", bin_name);

		if (partition->partition_info == NULL) {
			pr_error("Invalid partition info\n");
			err = TEGRABL_ERROR(TEGRABL_ERR_INVALID, 3);
			goto fail;
		}

		block_dev = partition->block_device;
		start_block = partition->partition_info->start_sector;
		block_size_log2 = TEGRABL_BLOCKDEV_BLOCK_SIZE_LOG2(block_dev);
		/* Round up to block size bytes as storage driver loads in blocks
		 * This will ensure the size is 32 bytes aligned
		 */
		load_size = ROUND_UP_POW2(sizeof(NvBootWb0RecoveryHeader),
														1 << block_size_log2);
		num_blocks = DIV_CEIL_LOG2(load_size, block_size_log2);

		/* Allocate memory to load header of binary */
		buffer = tegrabl_malloc(load_size);
		if (buffer == NULL) {
			err = TEGRABL_ERROR(TEGRABL_ERR_NO_MEMORY, 1);
			pr_error("Failed to allocate memory\n");
			goto fail;
		}

		err = block_dev->read_block(block_dev, buffer, start_block, num_blocks);
		if (err != TEGRABL_NO_ERROR) {
			pr_error("Failed to load header of bin: %s from storage into "
					 "buffer\n", bin_name);
			goto fail;
		}

		*image_size = ((NvBootWb0RecoveryHeader *)buffer)->LengthInsecure;
		break;

	/* TODO: Add support for binaries having generic header */

	default:
		pr_error("Uknown bin type\n");
		break;
	}

fail:
	if (buffer) {
		tegrabl_free(buffer);
	}
	return err;
}

tegrabl_error_t load_and_compare_sc7_images(bool *are_bins_same)
{
	struct tegrabl_partition partition1 = {0};
	struct tegrabl_partition partition2 = {0};
	tegrabl_bdev_t *block_dev1 = NULL;
	tegrabl_bdev_t *block_dev2 = NULL;
	void *primary_buffer = NULL;
	void *recovery_buffer = NULL;
	uint32_t block_size_log2;
	uint32_t num_blocks;
	uint32_t start_block;
	uint32_t already_read_num_blocks = 0;
	int32_t remaining_size = 0;
	uint32_t primary_image_size;
	uint32_t recovery_image_size;
	tegrabl_error_t err = TEGRABL_NO_ERROR;


	/* Get partition info of primary SC7 image */
	err = tegrabl_partition_open("sc7", &partition1);
	if (err != TEGRABL_NO_ERROR) {
		pr_error("Failed to get partiton info of image: %s\n", "sc7");
		goto fail;
	}
	/* Get primary SC7 size */
	err = tegrabl_storage_get_bin_size(TEGRABL_BINARY_SC7_RESUME_FW, "sc7",
									   &partition1, &primary_image_size);
	if (err != TEGRABL_NO_ERROR) {
		goto fail;
	}


	/* Get partition info of recovery SC7 image */
	err = tegrabl_partition_open("sc7_b", &partition2);
	if (err != TEGRABL_NO_ERROR) {
		pr_error("Failed to get partiton info of image: %s\n", "sc7-r");
		goto fail;
	}
	/* Get recovery SC7 size */
	err = tegrabl_storage_get_bin_size(TEGRABL_BINARY_SC7_RESUME_FW, "sc7_b",
									   &partition2, &recovery_image_size);
	if (err != TEGRABL_NO_ERROR) {
		goto fail;
	}


	if (primary_image_size != recovery_image_size) {
		pr_error("Length of primary(%u bytes) and recovery(%u bytes) image of "
				 "SC7 fw are different\n", primary_image_size,
				 recovery_image_size);
		err = TEGRABL_ERROR(TEGRABL_ERR_INVALID, 1);
		goto fail;
	}


	block_dev1 = partition1.block_device;
	block_dev2 = partition2.block_device;


	/* Allocate memory to buffers to load noth primary and recovery images */
	primary_buffer = tegrabl_malloc(IMAGE_CHUNK_SIZE);
	if (primary_buffer == NULL) {
		err = TEGRABL_ERROR(TEGRABL_ERR_NO_MEMORY, 0);
		pr_error("Failed to allocate memory to primary image buffer\n");
		goto fail;
	}
	recovery_buffer = tegrabl_malloc(IMAGE_CHUNK_SIZE);
	if (recovery_buffer == NULL) {
		err = TEGRABL_ERROR(TEGRABL_ERR_NO_MEMORY, 1);
		pr_error("Failed to allocate memory to recovery image buffer\n");
		goto fail;
	}


	/* Load both primary and recovery images and compare them */
	block_size_log2 = TEGRABL_BLOCKDEV_BLOCK_SIZE_LOG2(block_dev1);
	already_read_num_blocks = 0;
	remaining_size = (int32_t)primary_image_size;

	while(remaining_size > 0) {

		memset(primary_buffer, 0, IMAGE_CHUNK_SIZE);
		memset(recovery_buffer, 0, IMAGE_CHUNK_SIZE);

		/* Load some part of primary image */
		start_block = partition1.partition_info->start_sector +
													already_read_num_blocks;
		num_blocks = DIV_CEIL_LOG2(IMAGE_CHUNK_SIZE, block_size_log2);
		err = block_dev1->read_block(block_dev1, primary_buffer, start_block,
									 num_blocks);
		if (err != TEGRABL_NO_ERROR) {
			pr_error("Failed to read %u blocks from storage into primary "
					 "buffer\n", num_blocks);
			goto fail;
		}

		/* Load some part of recovery image */
		start_block = partition2.partition_info->start_sector +
													already_read_num_blocks;
		err = block_dev2->read_block(block_dev2, recovery_buffer, start_block,
									 num_blocks);
		if (err != TEGRABL_NO_ERROR) {
			pr_error("Failed to read %u blocks from storage into recovery "
					 "buffer\n", num_blocks);
			goto fail;
		}

		/* Compare both the images */
		if (memcmp(primary_buffer, recovery_buffer, IMAGE_CHUNK_SIZE)) {
			*are_bins_same = false;
			goto fail;
		} else {
			*are_bins_same = true;
		}

		already_read_num_blocks = num_blocks;
		remaining_size = remaining_size - IMAGE_CHUNK_SIZE;
	}

fail:
	pr_debug("Primary and Recovery SC7 images are %s\n",
			 (*are_bins_same) ? "same" : "not same");

	if (primary_buffer) {
		tegrabl_free(primary_buffer);
	}
	if (recovery_buffer) {
		tegrabl_free(recovery_buffer);
	}
	tegrabl_partition_close(&partition1);
	tegrabl_partition_close(&partition2);

	return err;
}
