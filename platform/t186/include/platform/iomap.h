/*
 * Copyright (c) 2014, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited
 */

#ifndef __PLATFORM_T132_IOMAP_H_
#define __PLATFORM_T132_IOMAP_H_

/* Base Addresses of Nvidia modules */
/* UART */
#define	TEGRA_UARTA_BASE		0x70006000
#define	TEGRA_UARTB_BASE		0x70006040
#define	TEGRA_UARTC_BASE		0x70006200
#define	TEGRA_UARTD_BASE		0x70006300
#define	TEGRA_UARTE_BASE		0x70006400

/* TIMER */
#define	TEGRA_TIMERUS_BASE		0x60005010
#define	TEGRA_TIMER0_BASE		0x60005000
#define	TEGRA_TIMER1_BASE		0x60005008

/* PMC */
#define	TEGRA_PMC_BASE			0x7000E400

/* APB_MISC */
#define APB_MISC_PA_BASE		0x70000000

/* CLUSTER CLOCKS */
#define CLUSTER_CLOCKS_PA_BASE	0x70040000

#endif /* __PLATFORM_T132_IOMAP_H_ */

