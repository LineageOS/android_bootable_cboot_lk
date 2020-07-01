LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/bq27441.c

ifneq ($(findstring loki, ${TARGET_PRODUCT}),)
	MODULE_SRCS += \
		$(LOCAL_DIR)/loki_bq27441_firmware.c
else
	MODULE_SRCS += \
		$(LOCAL_DIR)/bq27441_firmware.c
endif

include make/module.mk
