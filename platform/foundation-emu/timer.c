/*
 * Copyright (c) 2008-2012 Travis Geiselbrecht
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
#include <sys/types.h>
#include <err.h>
#include <stdio.h>
#include <trace.h>
#include <kernel/thread.h>
#include <arch/arm64.h>
#include <platform.h>
#include <platform/interrupts.h>
#include <platform/timer.h>
#include <platform/foundation-emu.h>
#include "platform_p.h"

#define LOCAL_TRACE 0

static platform_timer_callback t_callback;

/* armv8 specified timer */

/* relative to CNTControl */
#define CNTCR 0x00
#define CNTSR 0x04
#define CNTCV 0x08
#define CNTFID0 0x20

/* relative to CNTReadBase */
#define CNTCV2 0x00

static uint64_t interval_delta;
static uint64_t last_compare;
static uint32_t timer_freq;
static uint32_t usec_ratio;
static uint32_t msec_ratio;

static uint64_t read_counter(void)
{
    return ARM64_READ_SYSREG(CNTPCT_EL0);
}

status_t platform_set_periodic_timer(platform_timer_callback callback, void *arg, lk_time_t interval)
{
    enter_critical_section();

    LTRACEF("callback %p, arg %p, interval %lu\n", callback, arg, interval);
    t_callback = callback;

    /* disable the timer */
    ARM64_WRITE_SYSREG(CNTP_CTL_EL0, 0);

    /* calculate the compare delta and set the comparison register */
    interval_delta = (uint64_t)timer_freq * interval / 1000U;
    last_compare = read_counter() + interval;
    ARM64_WRITE_SYSREG(CNTP_CVAL_EL0, last_compare);

    /* set the countdown register to max */
    ARM64_WRITE_SYSREG(CNTP_TVAL_EL0, 0xffffffff);

    ARM64_WRITE_SYSREG(CNTP_CTL_EL0, 1);

    unmask_interrupt(INT_PPI_NSPHYS_TIMER);

    exit_critical_section();

    return NO_ERROR;
}

lk_bigtime_t current_time_hires(void)
{
    return read_counter() / usec_ratio;
}

lk_time_t current_time(void)
{
    return read_counter() / msec_ratio;
}

static enum handler_return platform_tick(void *arg)
{
    /* reset the compare register ahead of the physical counter */
    last_compare += interval_delta;
    ARM64_WRITE_SYSREG(CNTP_CVAL_EL0, last_compare);

    /* reset the countdown register to max to avoid it ticking */
    //ARM64_WRITE_SYSREG(CNTP_TVAL_EL0, 0x7fffffff);

    if (t_callback) {
        return t_callback(arg, current_time());
    } else {
        return INT_NO_RESCHEDULE;
    }
}

void platform_init_timer(void)
{
    TRACE_ENTRY;
    /* read the base frequency from the control block */
    timer_freq = *REG32(REFCLK_CNTControl + CNTFID0);
    printf("timer running at %d Hz\n", timer_freq);

    /* calculate the ratio of microseconds and milliseconds */
    usec_ratio = timer_freq / 1000000U;
    msec_ratio = timer_freq / 1000U;

    /* start the physical timer */
    *REG32(REFCLK_CNTControl + CNTCR) = 1;

    mask_interrupt(INT_PPI_NSPHYS_TIMER);
    register_int_handler(INT_PPI_NSPHYS_TIMER, &platform_tick, NULL);
}

