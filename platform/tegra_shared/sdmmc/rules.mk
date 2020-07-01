LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

GLOBAL_INCLUDES += \
	$(LOCAL_DIR)/include \
	$(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/sdmmc_bdev.c \
	$(LOCAL_DIR)/sdmmc_protocol.c \
	$(LOCAL_DIR)/sdmmc_host.c \
	$(LOCAL_DIR)/sdmmc_rpmb.c

GLOBAL_DEFINES += \
	SDMMC_DEBUG=0

include make/module.mk
