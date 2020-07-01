LOCAL_DIR := $(GET_LOCAL_DIR)

PLATFORM := t210

#For now, we'll show only 1GB memory to LK
MEMSIZE 	:= 0x40000000	# 1GB

IS_ENABLE_DISPLAY	:= 1
IS_ENABLE_DSI		:= 1
IS_ENABLE_HDMI		:= 1
IS_ENABLE_PROFILING	:= 0
ENABLE_LPDDR4_TRAINING := 0

#disabling static heap for now as the bootloader is already being loaded @ 0x83D80000
GLOBAL_DEFINES += \
	WITH_STATIC_HEAP=0 \
	VPR_EXISTS=1 \
	TSEC_EXISTS=0 \
	ENABLE_LP0=1 \
	START_ON_BOOT=1 \
	ENABLE_NVDUMPER=1 \
	TIME_STAMP=1 \
	CORE_EDP_MA=4000 \
	CORE_EDP_MV=1125 \
	ANDROID=1 \
	IS_T210=1 \
	IS_ENABLE_DISPLAY=$(IS_ENABLE_DISPLAY) \
	ENABLE_LPDDR4_TRAINING=0

#Minimal boot support flags
GLOBAL_DEFINES += \
	IS_ENABLE_BPMP_FW=1 \
	IS_ENABLE_APE_UNPOWERGATE=1

GLOBAL_DEFINES += \
	SDRAM_SIZE=$(MEMSIZE) \
	HEAP_START=$(HEAP_START_ADDR) \
	HEAP_LEN=$(HEAP_LENGTH)

# FASTBOOT_PRODUCT name should match the product name in
# $TOP/device/nvidia/platform/t210/board-info.txt
ifneq ($(findstring hawkeye, ${TARGET_PRODUCT}),)
	GLOBAL_DEFINES += FASTBOOT_PRODUCT=\"hawkeye\"
else ifneq ($(findstring loki, ${TARGET_PRODUCT}),)
	GLOBAL_DEFINES += FASTBOOT_PRODUCT=\"loki\"
else ifneq ($(findstring foster, ${TARGET_PRODUCT}),)
	GLOBAL_DEFINES += FASTBOOT_PRODUCT=\"foster\"
else
	GLOBAL_DEFINES += FASTBOOT_PRODUCT=\"t210ref\"
endif
