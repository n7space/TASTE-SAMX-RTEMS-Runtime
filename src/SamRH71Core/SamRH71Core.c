/**@file
 * This file is part of the TASTE SAMRH71 RTEMS Runtime.
 *
 * @copyright 2026 N7 Space Sp. z o.o.
 *
 * Licensed under the ESA Public License (ESA-PL) Permissive (Type 3),
 * Version 2.4 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://essr.esa.int/license/list
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "SamRH71Core.h"

#include <stdint.h>
#include <assert.h>

#include <rtems/config.h>

#include <Utils/ErrorCode.h>
#include <Systick/Systick.h>
#include <Pmc/Pmc.h>
#include <Mpu/Mpu.h>

#define CKGR_PLLAR_DIVA_Pos 0
#define CKGR_PLLAR_DIVA_Msk (0xffu << CKGR_PLLAR_DIVA_Pos)
#define CKGR_PLLAR_DIVA(value) \
	((CKGR_PLLAR_DIVA_Msk & ((value) << CKGR_PLLAR_DIVA_Pos)))
#define CKGR_PLLAR_PLLACOUNT_Pos 8
#define CKGR_PLLAR_PLLACOUNT_Msk (0x3fu << CKGR_PLLAR_PLLACOUNT_Pos)
#define CKGR_PLLAR_PLLACOUNT(value) \
	((CKGR_PLLAR_PLLACOUNT_Msk & ((value) << CKGR_PLLAR_PLLACOUNT_Pos)))
#define CKGR_PLLAR_MULA_Pos 16
#define CKGR_PLLAR_MULA_Msk (0x7ffu << CKGR_PLLAR_MULA_Pos)
#define CKGR_PLLAR_MULA(value) \
	((CKGR_PLLAR_MULA_Msk & ((value) << CKGR_PLLAR_MULA_Pos)))
#define CKGR_PLLAR_ONE (0x1u << 29)

#define PMC_MCKR_PRES_CLK_2 (0x1u << 4)
#define PMC_MCKR_CSS_PLLA_CLK (0x2u << 0)
#define PMC_MCKR_MDIV_PCK_DIV2 (0x1u << 8)

#define PLLA_MUL 24u
#define PLLA_DIV 3u
#define PLLA_COUNT 60u

#define MEGA_HZ 1000000u
#ifndef MAIN_CRYSTAL_OSCILLATOR_FREQUENCY
#define MAIN_CRYSTAL_OSCILLATOR_FREQUENCY (12 * MEGA_HZ)
#endif

extern void setCoreClockFrequency(uint64_t frequency);

// xdmad.c requires global pmc
Pmc pmc;
static Mpu mpu;

static uint64_t extract_main_oscillator_frequency(void)
{
	Pmc_MainckConfig main_clock_config;
	Pmc_getMainckConfig(&pmc, &main_clock_config);

	if (main_clock_config.src == Pmc_MainckSrc_XOsc) {
		return MAIN_CRYSTAL_OSCILLATOR_FREQUENCY;
	}

	switch (main_clock_config.rcOscFreq) {
	case Pmc_RcOscFreq_4M: {
		return 4 * MEGA_HZ;
	}
	case Pmc_RcOscFreq_8M: {
		return 8 * MEGA_HZ;
	}
#if defined(N7S_TARGET_SAMRH71Q21)
	case Pmc_RcOscFreq_12M: {
		return 12 * MEGA_HZ;
	}
#elif defined(N7S_TARGET_SAMRH71F20) || defined(N7S_TARGET_SAMRH707F18)
	case Pmc_RcOscFreq_10M: {
		return 10 * MEGA_HZ;
	}
	case Pmc_RcOscFreq_12M: {
		return 12 * MEGA_HZ;
	}
#endif
	}

	return 0;
}

static void apply_plla_config(Pmc_MasterckConfig *master_clock_config,
			      uint64_t *mck_frequency)
{
	if (master_clock_config->src == Pmc_MasterckSrc_Pllack) {
		Pmc_PllConfig pll_config;
		Pmc_getPllConfig(&pmc, &pll_config);

		if (pll_config.pllaDiv > 0 && pll_config.pllaMul > 0) {
			*mck_frequency = (*mck_frequency / pll_config.pllaDiv) *
					 (pll_config.pllaMul + 1);
		} else if (pll_config.pllaDiv == 0 && pll_config.pllaMul > 0) {
			*mck_frequency =
				*mck_frequency * (pll_config.pllaMul + 1);
		} else if (pll_config.pllaDiv > 0 && pll_config.pllaMul == 0) {
			*mck_frequency = *mck_frequency / pll_config.pllaDiv;
		}
	}
}

uint64_t SamRH71Core_GetProcessorClockFrequency(void)
{
	Pmc_MasterckConfig master_clock_config;
	Pmc_getMasterckConfig(&pmc, &master_clock_config);

	uint64_t mck_frequency = extract_main_oscillator_frequency();
	apply_plla_config(&master_clock_config, &mck_frequency);

	switch (master_clock_config.presc) {
	case Pmc_MasterckPresc_1: {
		break;
	}
	case Pmc_MasterckPresc_2: {
		mck_frequency = mck_frequency / 2;
		break;
	}
	case Pmc_MasterckPresc_4: {
		mck_frequency = mck_frequency / 4;
		break;
	}
	case Pmc_MasterckPresc_8: {
		mck_frequency = mck_frequency / 8;
		break;
	}
	case Pmc_MasterckPresc_16: {
		mck_frequency = mck_frequency / 16;
		break;
	}
	case Pmc_MasterckPresc_32: {
		mck_frequency = mck_frequency / 32;
		break;
	}
	case Pmc_MasterckPresc_64: {
		mck_frequency = mck_frequency / 64;
		break;
	}
#if defined(N7S_TARGET_SAMV71Q21)
	case Pmc_MasterckPresc_3: {
		mck_frequency = mck_frequency / 7;
		break;
	}
#endif
	}
}

// TODO add option to disable PLLA
#if defined(SAMRH71_PLLA_MULTIPLIER)
#undef SAMRH71_PLLA_MULTIPLIER
#endif

#if defined(SAMRH71_PLLA_DIVIDER)
#undef SAMRH71_PLLA_DIVIDER
#endif

#if defined(SAMRH71_RCOSC_FREQUENCY)
#undef SAMRH71_RCOSC_FREQUENCY
#endif

#if defined(SAMRH71_RCOSC_FREQUENCY_4)
#define SAMRH71_RCOSC_FREQUENCY Pmc_RcOscFreq_4M
#elif defined(SAMRH71_RCOSC_FREQUENCY_8)
#define SAMRH71_RCOSC_FREQUENCY Pmc_RcOscFreq_8M
#elif defined(SAMRH71_RCOSC_FREQUENCY_10)
#define SAMRH71_RCOSC_FREQUENCY Pmc_RcOscFreq_10M
#elif defined(SAMRH71_RCOSC_FREQUENCY_12)
#define SAMRH71_RCOSC_FREQUENCY Pmc_RcOscFreq_12M
#else
#define SAMRH71_RCOSC_FREQUENCY Pmc_RcOscFreq_12M
#endif

#if defined(SAMRH71_MAIN_CLOCK_SOURCE)
#undef SAMRH71_MAIN_CLOCK_SOURCE
#endif

#if defined(SAMRH71_MAIN_CLOCK_SOURCE_RCOSC)
#define SAMRH71_MAIN_CLOCK_SOURCE Pmc_MainckSrc_RcOsc
#elif defined(SAMRH81_MAIN_CLOCK_SOURCE_XOSC)
#define SAMRH71_MAIN_CLOCK_SOURCE Pmc_MainckSrc_XOsc
#elif defined(SAMRH81_MAIN_CLOCK_SOURCE_XOSC_BYPASSED)
#define SAMRH71_MAIN_CLOCK_SOURCE Pmc_MainckSrc_XOscBypassed
#else
#define SAMRH71_MAIN_CLOCK_SOURCE Pmc_MainckSrc_RcOsc
#endif

#if defined(SAMRH71_MASTER_CLOCK_SOURCE)
#undef SAMRH71_MASTER_CLOCK_SOURCE
#endif

#if defined(SAMRH71_MASTER_CLOCK_SOURCE_SLCK)
#define SAMRH71_MASTER_CLOCK_SOURCE Pmc_MasterckSrc_Slck
#elif defined(SAMRH71_MASTER_CLOCK_SOURCE_MAINCK)
#define SAMRH71_MASTER_CLOCK_SOURCE Pmc_MasterckSrc_Mainck
#elif defined(SAMRH71_MASTER_CLOCK_SOURCE_PLLACK)
#define SAMRH71_MASTER_CLOCK_SOURCE Pmc_MasterckSrc_Pllack
#else
#define SAMRH71_MASTER_CLOCK_SOURCE Pmc_MasterckSrc_Pllack
#endif

#if defined(SAMRH71_MASTER_CLOCK_PRESCALER)
#undef SAMRH71_MASTER_CLOCK_PRESCALER
#endif

#if defined(SAMRH71_MASTER_CLOCK_PRESCALER_1)
#define SAMRH71_MASTER_CLOCK_PRESCALER Pmc_MasterckPresc_1
#elif defined(SAMRH71_MASTER_CLOCK_PRESCALER_2)
#define SAMRH71_MASTER_CLOCK_PRESCALER Pmc_MasterckPresc_2
#elif defined(SAMRH71_MASTER_CLOCK_PRESCALER_4)
#define SAMRH71_MASTER_CLOCK_PRESCALER Pmc_MasterckPresc_4
#elif defined(SAMRH71_MASTER_CLOCK_PRESCALER_8)
#define SAMRH71_MASTER_CLOCK_PRESCALER Pmc_MasterckPresc_8
#elif defined(SAMRH71_MASTER_CLOCK_PRESCALER_16)
#define SAMRH71_MASTER_CLOCK_PRESCALER Pmc_MasterckPresc_16
#elif defined(SAMRH71_MASTER_CLOCK_PRESCALER_32)
#define SAMRH71_MASTER_CLOCK_PRESCALER Pmc_MasterckPresc_32
#elif defined(SAMRH71_MASTER_CLOCK_PRESCALER_64)
#define SAMRH71_MASTER_CLOCK_PRESCALER Pmc_MasterckPresc_64
#else
#define SAMRH71_MASTER_CLOCK_PRESCALER Pmc_MasterckPresc_1
#endif

#if defined(SAMRH71_MASTER_CLOCK_DIVIDER)
#undef SAMRH71_MASTER_CLOCK_DIVIDER
#endif

#if defined(SAMRH71_MASTER_CLOCK_DIVIDER_1)
#define SAMRH71_MASTER_CLOCK_DIVIDER Pmc_MasterckDiv_1
#elif defined(SAMRH71_MASTER_CLOCK_DIVIDER_2)
#define SAMRH71_MASTER_CLOCK_DIVIDER Pmc_MasterckDiv_2
#else
#define SAMRH71_MASTER_CLOCK_DIVIDER Pmc_MasterckDiv_2
#endif

void SamRH71Core_Init(void)
{
	Pmc_init(&pmc, Pmc_getDeviceRegisterStartAddress());
	Mpu_init(&mpu);
	Mpu_Config mpuConf = { .isEnabled = true,
			       .isDefaultMemoryMapEnabled = true,
			       .isMpuEnabledInHandlers = true };
	Mpu_setConfig(&mpu, &mpuConf);

#ifndef RT_RTOS_NO_INIT
	// Configure RC Oscillator as source for main clock.
	// Configure PLLA and master clock.
	// This is default setting, unless RT_RTOS_NO_INIT is enabled.
	const Pmc_Config pmcConfig = {
		.mainck = { .src = SAMRH71_MAIN_CLOCK_SOURCE,
			    .rcOscFreq = SAMRH71_RCOSC_FREQUENCY,
			    .xoscStartupTime = 0 }, // TODO configure also startup time, if xosc was selected
		.pll = { .pllaMul = PLLA_MUL,
			 .pllaDiv = PLLA_DIV,
			 .pllaStartupTime = PLLA_COUNT },
		.masterck = { .src = SAMRH71_MASTER_CLOCK_SOURCE,
			      .presc = SAMRH71_MASTER_CLOCK_PRESCALER,
			      .divider = SAMRH71_MASTER_CLOCK_DIVIDER },
		.pck = {},
	};

	ErrorCode errCode = ErrorCode_NoError;
	const bool isSettingConfigSuccessful =
		Pmc_setConfig(&pmc, &pmcConfig, 1000000u, &errCode);
	assert(isSettingConfigSuccessful && "Cannot configure PMC");
#endif

	uint64_t coreFrequency = SamRH71Core_GetProcessorClockFrequency();
	setCoreClockFrequency(coreFrequency);

	uint32_t systickReloadValue =
		(uint32_t)((coreFrequency *
			    rtems_configuration_get_microseconds_per_tick()) /
			   1000000u);

	Systick systick;
	Systick_init(&systick, Systick_getDeviceRegisterStartAddress());
	Systick_Config systickConfig;
	Systick_getConfig(&systick, &systickConfig);
	systickConfig.reloadValue = systickReloadValue;

	Systick_setConfig(&systick, &systickConfig);
}

void SamRH71Core_EnablePeripheralClock(const Pmc_PeripheralId peripheralId)
{
	Pmc_enablePeripheralClk(&pmc, peripheralId);
	const bool is_enabled = Pmc_isPeripheralClkEnabled(&pmc, peripheralId);
	assert(is_enabled && "Peripheral clock not enabled");
}

uint64_t SamRH71Core_GetMainClockFrequency(void)
{
	Pmc_MasterckConfig master_clock_config;
	Pmc_getMasterckConfig(&pmc, &master_clock_config);

	uint64_t mck_frequency = SamRH71Core_GetProcessorClockFrequency();

	switch (master_clock_config.divider) {
	case Pmc_MasterckDiv_1: {
		break;
	}
	case Pmc_MasterckDiv_2: {
		mck_frequency = mck_frequency / 2;
		break;
	}
	}

	return mck_frequency;
}

void SamRH71Core_InterruptSubscribe(const rtems_vector_number vector,
				    const char *info,
				    rtems_interrupt_handler handler,
				    void *handler_arg)
{
	rtems_interrupt_handler_install(vector, info, RTEMS_INTERRUPT_UNIQUE,
					handler, handler_arg);
	rtems_interrupt_vector_enable(vector);
}

rtems_name SamRH71Core_GenerateNewSemaphoreName(void)
{
	static rtems_name name = rtems_build_name('C', 0, 0, 0);
	return name++;
}

rtems_name SamRH71Core_GenerateNewTaskName(void)
{
	static rtems_name name = rtems_build_name('D', 0, 0, 0);
	return name++;
}

bool SamRH71Core_SetPckConfig(const Pmc_PckId id,
			      const Pmc_PckConfig *const config,
			      const uint32_t timeout, ErrorCode *const errCode)
{
	return Pmc_setPckConfig(&pmc, id, config, timeout, errCode);
}

void SamRH71Core_DisableDataCacheInRegion(void *address, size_t sizeExponent)
{
	// At this moment it is used by can driver,
	// where the Mpu_RegionMemoryType_StronglyOrdered breaks the driver:
	// - the BSP uses memcpy from newlib to copy data from/to message ram
	// - if the memcpy has to copy 3 bytes, it copies one byte and then two next bytes
	//    using only one arm instruction `ldrh`, but the address of two next bytes is odd,
	// - the processor raises exception due to unaligned access.
	assert(((uint32_t)address & (~MPU_RBAR_ADDR_MASK)) ==
	       0); // verify proper alignment of address
	assert(sizeExponent >= 4); // exponents less than 4 are reserved
	assert(sizeExponent <=
	       31); // maximum exponent is 31 which defines 4GB region size

	// The Mpu allows to define 16 regions, where the higher region number has
	// higher priority. Regions can overlap, therefore the small regions with
	// disabled cache shall have higher priority. The default memory map defines
	// 11 regions, from 0 to 11 so these region shall be left intact.
	static uint8_t region = 15; // start from with highest priority
	assert(region >
	       11); // verify if region does not overwrite default memory map
	Mpu_RegionConfig mpuRegionConf = {
		.address = (uint32_t)address,
		.isEnabled = true,
		.size = sizeExponent,
		.subregionDisableMask = 0x00,
		.isShareable = false,
		.isExecutable = false,
		.memoryType = Mpu_RegionMemoryType_Normal,
		.innerCachePolicy = Mpu_RegionCachePolicy_NonCacheable,
		.outerCachePolicy = Mpu_RegionCachePolicy_NonCacheable,
		.privilegedAccess = Mpu_RegionAccess_ReadWrite,
		.unprivilegedAccess = Mpu_RegionAccess_ReadWrite,
	};
	Mpu_setRegionConfig(&mpu, region, &mpuRegionConf);
	--region; // decrease region number, so next call will define a new one
}
