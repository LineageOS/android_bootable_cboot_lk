/*
 * Copyright (c) 2014 Travis Geiselbrecht
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <debug.h>
#include <arch.h>
#include <arch/ops.h>
#include <arch/arm64.h>
#include <platform.h>
#include <arch/mmu.h>

extern int _end_of_ram;
void *_heap_end = &_end_of_ram;

void print_cpuid(void);

void arch_early_init(void)
{
    /* set the vector base */
    ARM64_WRITE_TARGET_SYSREG(VBAR_ELx, (uint64_t)&arm64_exception_base);

    /* switch from EL3 */
    unsigned int current_el = ARM64_READ_SYSREG(CURRENTEL) >> 2;
#if ARM64_WITH_EL2
    if (current_el > 2) {
        arm64_el3_to_el2();
    }
#else
    if (current_el > 1) {
        arm64_el3_to_el1();
    }
#endif

#if WITH_MMU
	arm64_mmu_init();

	platform_init_mmu_mappings();
#endif
}

void arch_init(void)
{
	print_cpuid();
}

void arch_quiesce(void)
{
}

void arch_idle(void)
{
    __asm__ volatile("wfi");
}

