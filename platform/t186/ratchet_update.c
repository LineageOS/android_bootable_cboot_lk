/*
 * Copyright (c) 2017, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited
 */

#define MODULE TEGRABL_ERR_NO_MODULE

#include <tegrabl_debug.h>
#include <tegrabl_fuse.h>
#include <address_map_new.h>
#include <arscratch.h>
#include <storage_loader.h>

#define DPMU_BIT_WR                   (14)
#define NV_DRF_BIT(x)                 (1 << (x))
#define OPT_IN_FUSE_MASK              (NV_DRF_BIT(14))

/*
 * MB1 ratchet fuse is defined as: hypervoltaging[15:0]
 */
#define MB1_FIELD_RATCHET_BIT_LENGTH  (16)
#define MB1_FIELD_RATCHET_FUSE_SHIFT  (0)
#define MB1_FIELD_RATCHET_MASK        \
	(((1 << MB1_FIELD_RATCHET_BIT_LENGTH) - 1) << MB1_FIELD_RATCHET_FUSE_SHIFT)
#define MB1_RATCHET_OEM_SW_VERSION_SCRATCH_REG  SCRATCH_SECURE_RSV54_SCRATCH_0

#define SC7_FIELD_RATCHET_BIT_LENGTH  (16)
#define SC7_FIELD_RATCHET_FUSE_SHIFT  (16)
#define SC7_FIELD_RATCHET_MASK        \
	(((1 << SC7_FIELD_RATCHET_BIT_LENGTH) - 1) << SC7_FIELD_RATCHET_FUSE_SHIFT)

/*
 * MTS ratchet fuse is defined as: hypervoltaging[31:24] | hypervoltaging[23:16]
 */
#define MTS_SCRATCH_SCR_OFFSET                   (0x000080d0)
#define MTS_FIELD_RATCHET_BIT_LENGTH             (8)
#define MTS_FIELD_RATCHET_FUSE_SHIFT             (24)
#define MTS_FIELD_RATCHET_FUSE_REDUNDANT_SHIFT   (16)
#define MTS_FIELD_RATCHET_MASK                                                \
	((((1 << MTS_FIELD_RATCHET_BIT_LENGTH) - 1) <<                            \
									MTS_FIELD_RATCHET_FUSE_SHIFT)          |  \
	(((1 << MTS_FIELD_RATCHET_BIT_LENGTH) - 1) <<                             \
									MTS_FIELD_RATCHET_FUSE_REDUNDANT_SHIFT))

#define MTS_RATCHET_OEM_SW_VERSION_SCRATCH_REG  SCRATCH_SECURE_RSV52_SCRATCH_1

#define is_sc7_update_successful(are_bins_same)  \
									load_and_compare_sc7_images(are_bins_same)


static tegrabl_error_t sanity_check_ratchet_version(uint32_t oem_sw_ratchet_ver,
											 uint32_t ratchet_bit_len)
{
	uint32_t max_ver;
	tegrabl_error_t err = TEGRABL_NO_ERROR;

	/* Check if the version exceeds the limit */
	max_ver = (0x1 << ratchet_bit_len) - 1;

	if (oem_sw_ratchet_ver > max_ver) {
		pr_error("Received OEM SW ratchet version (0x%08x) exceeds\n"
				 "maximum allowed version (0x%08x)\n", oem_sw_ratchet_ver,
				 max_ver);
		err = TEGRABL_ERROR(TEGRABL_ERR_INVALID, 0);
		goto fail;
	}

	/* Check if the version is thermometer encoded.
	 * OEM SW ratchet version should be like 0, 1, 3, 7 ... */
	if (oem_sw_ratchet_ver & (oem_sw_ratchet_ver + 1)) {
		pr_error("Received OEM SW ratchet version (0x%08x) is not "
				 "thermometer encoded\n", oem_sw_ratchet_ver);
		err = TEGRABL_ERROR(TEGRABL_ERR_INVALID, 1);
	}

fail:
	return err;
}

static uint32_t do_thermometer_decoding(uint32_t therm_encoded_val)
{
     return (32 - __builtin_clz(therm_encoded_val));
}

static tegrabl_error_t update_mb1_ratchet_fuse(void)
{
	uint32_t scratch_rsv54_val;
	uint32_t mb1_oem_sw_ver;
	uint32_t sc7_oem_sw_ver;
	uint32_t mb1_oem_hw_ver;
	uint32_t current_fuse_val;
	uint32_t val;
	bool is_sc7_fw_present = false;
	bool are_sc7_bins_same = false;
	tegrabl_error_t err = TEGRABL_NO_ERROR;

	scratch_rsv54_val = NV_READ32(NV_ADDRESS_MAP_SCRATCH_BASE +
										MB1_RATCHET_OEM_SW_VERSION_SCRATCH_REG);

	/* Extract lower 16 bits which represent MB1 ratchet version */
	mb1_oem_sw_ver = (scratch_rsv54_val & MB1_FIELD_RATCHET_MASK) >>
												MB1_FIELD_RATCHET_FUSE_SHIFT;

	/* Extract higher 16 bits which represent SC7 ratchet version */
	sc7_oem_sw_ver = (scratch_rsv54_val & SC7_FIELD_RATCHET_MASK) >>
												SC7_FIELD_RATCHET_FUSE_SHIFT;

	/* Non-zero SC7 OEM ratchet version indicates that SC7 FW is present in the
	 * system */
	if (sc7_oem_sw_ver != 0) {
		is_sc7_fw_present = true;
	}

	err = sanity_check_ratchet_version(mb1_oem_sw_ver,
												MB1_FIELD_RATCHET_BIT_LENGTH);
	if (err != TEGRABL_NO_ERROR) {
		pr_error("Sanity checks on MB1 OEM ratchet failed, skip fuse "
				 "burning\n");
		goto fail;
	}

	if ((is_sc7_fw_present) && (mb1_oem_sw_ver != sc7_oem_sw_ver)) {

		pr_error("MB1(%u) and SC7(%u) are not at the same ratchet "
				 "version\n", do_thermometer_decoding(mb1_oem_sw_ver),
				 do_thermometer_decoding(sc7_oem_sw_ver));
		pr_error("Need to keep both of them to the latest ratchet "
				 "version\n");
		err = TEGRABL_ERROR(TEGRABL_ERR_INVALID_VERSION, 0);
		goto fail;
	}


	err = tegrabl_fuse_read(FUSE_HYPERVOLTAGING, &val, sizeof(val));
	if (err != TEGRABL_NO_ERROR) {
		pr_error("Failed to read MB1 OEM HW ratchet\n");
		goto fail;
	}
	current_fuse_val = val;

	/* Extract lower 16 bits which represent MB1 ratchet version */
	mb1_oem_hw_ver = (val << MB1_FIELD_RATCHET_FUSE_SHIFT) &
														MB1_FIELD_RATCHET_MASK;

	/* Compare OEM SW and HW ratchets */
	if (mb1_oem_sw_ver < mb1_oem_hw_ver) {
		pr_error("Bug: MB1 OEM SW ratchet version (0x%08x) older than "
				 "MB1 OEM HW ratchet version (0x%08x)\n", mb1_oem_sw_ver,
				 mb1_oem_hw_ver);
		err = TEGRABL_ERROR(TEGRABL_ERR_INVALID, 2);
		goto fail;

	} else if (mb1_oem_sw_ver == mb1_oem_hw_ver) {

		/* Fuse value is up-to-date, so skip fuse update */
		err = TEGRABL_NO_ERROR;
		goto fail;
	}

	if (is_sc7_fw_present) {

		err = is_sc7_update_successful(&are_sc7_bins_same);
		if (err != TEGRABL_NO_ERROR) {
			pr_error("Failed to check SC7 update status, skip fuse burning\n");
			goto fail;
		}

		if (are_sc7_bins_same == false) {
			pr_error("Skip fuse burning as SC7 FW is not latest\n");
			goto fail;
		}

	} else {
		pr_info("SC7 FW is not present on the system, skip it's check before "
				"fuse burning\n");
	}

	pr_info("MB1 OEM HW ratchet version (0x%08x) older than "
			"MB1 OEM SW ratchet version (0x%08x)\n", mb1_oem_hw_ver,
			mb1_oem_sw_ver);
	pr_info("Updating ratchet fuse ...\n");

	/* Update only MB1 racthet field in FUSE_HYPERVOLTAGING fuse */
	current_fuse_val = current_fuse_val & (~MB1_FIELD_RATCHET_MASK);
	mb1_oem_sw_ver = mb1_oem_sw_ver | current_fuse_val;

	/* All pre-conditions are checked, now update the fuse */
	err = tegrabl_fuse_write(FUSE_HYPERVOLTAGING, &mb1_oem_sw_ver,
													sizeof(mb1_oem_sw_ver));
	if (err != TEGRABL_NO_ERROR) {
		pr_error("Failed to update MB1 ratchet fuse\n");
	}

fail:
	return err;
}

static tegrabl_error_t update_mts_ratchet_fuse(void)
{
	uint32_t mts_oem_sw_ver;
	uint32_t mts_oem_hw_ver;
	uint32_t val;
	uint32_t current_fuse_val;
	tegrabl_error_t err;

	mts_oem_sw_ver = NV_READ32(NV_ADDRESS_MAP_SCRATCH_BASE +
										MTS_RATCHET_OEM_SW_VERSION_SCRATCH_REG);

	/* Check if SCR has a write permission.
	 * If there is a write permission, then value in the scratch is correct */
	val = NV_READ32(NV_ADDRESS_MAP_SCRATCH_BASE + MTS_SCRATCH_SCR_OFFSET);

	if ((~val) & (0x1 << DPMU_BIT_WR)) {
		pr_error("SCR setting (0x%08x) doesn't provide write permission\n",
				 val);
		pr_error("Cannot trust the OEM SW ratchet for MTS (0x%08x) received\n"
				 "through scratch register\n", mts_oem_sw_ver);
		err = TEGRABL_ERROR(TEGRABL_ERR_INVALID, 3);
		goto fail;
	}

	err = sanity_check_ratchet_version(mts_oem_sw_ver,
												MTS_FIELD_RATCHET_BIT_LENGTH);
	if (err != TEGRABL_NO_ERROR) {
		pr_error("Sanity checks on MTS OEM ratchet failed, skip fuse "
				 "burning\n");
		goto fail;
	}

	err = tegrabl_fuse_read(FUSE_HYPERVOLTAGING, &val, sizeof(val));
	if (err != TEGRABL_NO_ERROR) {
		pr_error("Failed to read MTS OEM HW ratchet\n");
		goto fail;
	}
	current_fuse_val = val;

	/* Extract hw version from fuse value */
	val = val >> MTS_FIELD_RATCHET_FUSE_SHIFT;
	mts_oem_hw_ver = (val & 0xFF) | (val & 0x00FF);

	/* Compare MTS OEM SW and HW ratchets */
	if (mts_oem_sw_ver < mts_oem_hw_ver) {
		pr_error("Bug: MTS OEM SW ratchet version (0x%08x) older than\n"
				 "MTS OEM HW ratchet version (0x%08x)\n", mts_oem_sw_ver,
				 mts_oem_hw_ver);
		err = TEGRABL_ERROR(TEGRABL_ERR_INVALID, 4);
		goto fail;

	} else if (mts_oem_sw_ver == mts_oem_hw_ver) {

		/* Fuse value is up-to-date, so skip fuse update */
		err = TEGRABL_NO_ERROR;
		goto fail;
	}

	pr_info("MTS OEM HW ratchet (0x%08x) older than "
			"MTS OEM SW ratchet (0x%08x)\n", mts_oem_hw_ver, mts_oem_sw_ver);
	pr_info("Updating ratchet fuse ...\n");

	/* Extract Bits[31:24] | Bits[23:16] which represent MTS ratchet version */
	mts_oem_sw_ver =
		((mts_oem_sw_ver << MTS_FIELD_RATCHET_FUSE_SHIFT) |
			(mts_oem_sw_ver << MTS_FIELD_RATCHET_FUSE_REDUNDANT_SHIFT)) &
		MTS_FIELD_RATCHET_MASK;

	/* Update only MTS racthet field in FUSE_HYPERVOLTAGING fuse */
	current_fuse_val = current_fuse_val & (~MTS_FIELD_RATCHET_MASK);
	mts_oem_sw_ver = mts_oem_sw_ver | current_fuse_val;

	/* All pre-conditions are checked, now update the fuse */
	err = tegrabl_fuse_write(FUSE_HYPERVOLTAGING, &mts_oem_sw_ver,
													sizeof(mts_oem_sw_ver));
	if (err != TEGRABL_NO_ERROR) {
		pr_error("Failed to update MTS ratchet fuse\n");
	}

fail:
	return err;
}

tegrabl_error_t update_ratchet_fuse(void)
{
	tegrabl_error_t err = TEGRABL_NO_ERROR;
	uint32_t odm_fuse;

	/* Check if opt-in fuse is set */
	err = tegrabl_fuse_read(FUSE_ODM_INFO, &odm_fuse, sizeof(odm_fuse));
	if (err != TEGRABL_NO_ERROR) {
		pr_warn("Failed to read opt-in fuse\n");
		err = TEGRABL_NO_ERROR;
		goto fail;
	}

	if ((odm_fuse & OPT_IN_FUSE_MASK) == 0) {
		pr_warn("opt-in fuse is not set, skip fuse_burning\n");
		err = TEGRABL_NO_ERROR;
		goto fail;
	}

	err = update_mb1_ratchet_fuse();
	if (err != TEGRABL_NO_ERROR) {
		goto fail;
	}

	err = update_mts_ratchet_fuse();

fail:
	return err;
}
