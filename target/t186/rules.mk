LOCAL_DIR := $(GET_LOCAL_DIR)

PLATFORM := t186

#For now, we'll show only 1GB memory to LK
MEMSIZE 	:= 0x40000000	# 1GB

IS_ENABLE_DISPLAY	:= 0
IS_ENABLE_DSI		:= 0
IS_ENABLE_HDMI		:= 0
IS_ENABLE_PROFILING	:= 0
ENABLE_LPDDR4_TRAINING := 0

#disabling static heap for now as the bootloader is already being loaded @ 0x83D80000
GLOBAL_DEFINES += \
	WITH_STATIC_HEAP=0 \
	START_ON_BOOT=1 \
	ENABLE_NVDUMPER=1 \
	ANDROID=1 \
	IS_T186=1

#Minimal boot support flags
GLOBAL_DEFINES += \
	IS_ENABLE_BPMP_FW=1 \

GLOBAL_DEFINES += \
	SDRAM_SIZE=$(MEMSIZE) \
	HEAP_START=$(HEAP_START_ADDR) \
	HEAP_LEN=$(HEAP_LENGTH)

# FASTBOOT_PRODUCT name should match the product name in
# $TOP/device/nvidia-t18x/t186/board-info.txt
GLOBAL_DEFINES += FASTBOOT_PRODUCT=\"$(TARGET_PRODUCT)\"
