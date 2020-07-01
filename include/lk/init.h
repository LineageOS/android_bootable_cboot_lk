#pragma once

#include <compiler.h>
#include <sys/types.h>

/*
 * LK's init system
 */

int lk_init_level(uint level);

typedef void (*lk_init_hook)(uint level);

/* macro lk init level */
typedef uint32_t lk_init_level_t;
#define LK_INIT_LEVEL_EARLIEST 1

#define LK_INIT_LEVEL_ARCH_EARLY 0x10000
#define LK_INIT_LEVEL_PLATFORM_EARLY 0x20000
#define LK_INIT_LEVEL_TARGET_EARLY 0x30000
#define LK_INIT_LEVEL_HEAP 0x40000
#define LK_INIT_LEVEL_KERNEL 0x50000
#define LK_INIT_LEVEL_THREADING 0x60000
#define LK_INIT_LEVEL_ARCH 0x70000
#define LK_INIT_LEVEL_PLATFORM 0x80000
#define LK_INIT_LEVEL_TARGET 0x90000
#define LK_INIT_LEVEL_APPS 0xa0000

#define LK_INIT_LEVEL_LAST UINT_MAX

struct lk_init_struct {
    uint level;
    lk_init_hook hook;
    const char *name;
};

#define LK_INIT_HOOK(_name, _hook, _level) \
    const struct lk_init_struct _init_struct_##_name __SECTION(".lk_init") = { \
        .level = _level, \
        .hook = _hook, \
        .name = #_name, \
    };

// vim: set ts=4 sw=4 expandtab:
