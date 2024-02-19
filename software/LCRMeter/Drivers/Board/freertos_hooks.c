#include "freertos_hooks.h"

#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "stm.h"
#include "log.h"

#ifdef HARDFAULT_HOOK
void prvGetRegistersFromStack(uint32_t *pulFaultStackAddress)  __attribute__((no_instrument_function));
void prvGetRegistersFromStack(uint32_t *pulFaultStackAddress) {
	/* These are volatile to try and prevent the compiler/linker optimising them
	 away as the variables never actually get used. If the debugger won't show the
	 values of the variables, make them global my moving their declaration outside
	 of this function. */
	volatile uint32_t r0;
	volatile uint32_t r1;
	volatile uint32_t r2;
	volatile uint32_t r3;
	volatile uint32_t r12;
	volatile uint32_t lr; /* Link register. */
	volatile uint32_t pc; /* Program counter. */
	volatile uint32_t psr;/* Program status register. */

	r0 = pulFaultStackAddress[0];
	r1 = pulFaultStackAddress[1];
	r2 = pulFaultStackAddress[2];
	r3 = pulFaultStackAddress[3];

	r12 = pulFaultStackAddress[4];
	lr = pulFaultStackAddress[5];
	pc = pulFaultStackAddress[6];
	psr = pulFaultStackAddress[7];

	/* When the following line is hit, the variables contain the register values. */
	log_force("\r\nHard fault occured:");
	log_force("SCB->HFSR: 0x%08x", SCB->HFSR);
	if (SCB->HFSR & SCB_HFSR_VECTTBL_Msk) {
		log_force("HardFault on vector table read");
	} else if (SCB->HFSR & SCB_HFSR_FORCED_Msk) {
		log_force("Forced hardfault, SCB->CFSR: 0x%08x", SCB->CFSR);
		log_force("BFAR: 0x%08x", SCB->BFAR);
		log_force("MMFAR: 0x%08x", SCB->MMFAR);
#ifdef SCB_CFSR_MLSPERR
		if(SCB->CFSR & SCB_CFSR_M) {
			log_force("Memory manage fault during floating point lazy state preservation");
		}
#endif

#ifdef SCB_CFSR_LSPERR
		if (SCB->CFSR & SCB_CFSR_LSPERR) {
			log_force("BusFault during floating point lazy state preservation");
		}
#endif

	}
	log_force(
			"r0 0x%lx, r1 0x%lx, r2 0x%lx, r3 0x%lx, r12 0x%lx, lr 0x%lx, pc 0x%lx, psr 0x%lx\n",
			r0, r1, r2, r3, r12, lr, pc, psr);

	while (1)
		;
}

void HardFault_Handler( void ) __attribute__((naked,no_instrument_function));
#if defined __GNUC__
void HardFault_Handler()
{
	__asm volatile (
		" tst lr, #4                                                \n"
		" ite eq                                                    \n"
		" mrseq r0, msp                                             \n"
		" mrsne r0, psp                                             \n"
		" ldr r1, [r0, #24]                                         \n"
		" ldr r2, handler2_address_const                            \n"
		" bx r2                                                     \n"
		" handler2_address_const: .word prvGetRegistersFromStack    \n"
		);
}
#elif defined __ICCARM__
void HardFault_Handler()
{
	__asm volatile (
		" tst lr, #4                                                \n"
		" ite eq                                                    \n"
		" mrseq r0, msp                                             \n"
		" mrsne r0, psp                                             \n"
		" ldr r1, [r0, #24]                                         \n"
		" ldr r2, =prvGetRegistersFromStack                         \n"
		" bx r2                                                     \n"
		);
}
#endif
#endif

#if configGENERATE_RUN_TIME_STATS
#define TIM 				2

/* Automatically build register names based on timer selection */
#define TIM_M2(y) 					TIM ## y
#define TIM_M1(y)  					TIM_M2(y)
#define TIM_BASE					TIM_M1(TIM)

#define TIM_DBG_CLK_FREEZE_M2(y) 	__HAL_DBGMCU_FREEZE_TIM ## y
#define TIM_DBG_CLK_FREEZE_M1(y)  	TIM_DBG_CLK_FREEZE_M2(y)
#define TIM_DBG_CLK_FREEZE			TIM_DBG_CLK_FREEZE_M1(TIM)

#define TIM_CLK_EN_M2(y) 			__HAL_RCC_TIM ## y ## _CLK_ENABLE
#define TIM_CLK_EN_M1(y)  			TIM_CLK_EN_M2(y)
#define TIM_CLK_EN					TIM_CLK_EN_M1(TIM)

#define TIM_HANDLER_M2(x) 			TIM ## x ## _IRQHandler
#define TIM_HANDLER_M1(x)  			TIM_HANDLER_M2(x)
#define TIM_HANDLER					TIM_HANDLER_M1(TIM)

#define TIM_NVIC_ISR_M2(x) 			TIM ## x ## _IRQn
#define TIM_NVIC_ISR_M1(x)  		TIM_NVIC_ISR_M2(x)
#define TIM_NVIC_ISR				TIM_NVIC_ISR_M1(TIM)

static uint16_t us_e16;

void configureTimerForRunTimeStats(void)
{
	TIM_DBG_CLK_FREEZE();
	TIM_CLK_EN();
	const uint32_t APB1_freq = HAL_RCC_GetPCLK1Freq();
	const uint32_t AHB_freq = HAL_RCC_GetHCLKFreq();
	uint32_t timerFreq = APB1_freq == AHB_freq ? APB1_freq : APB1_freq * 2;

	TIM_BASE->PSC = (timerFreq / 1000000UL) - 1;

	HAL_NVIC_SetPriority(TIM_NVIC_ISR, 0, 0);
	HAL_NVIC_EnableIRQ(TIM_NVIC_ISR);

	TIM_BASE->DIER = TIM_DIER_UIE;
	TIM_BASE->CR1 |= TIM_CR1_CEN;
}

unsigned long getRunTimeCounterValue(void)
{
	uint32_t timer;
	uint32_t overflows;
	uint8_t MissedOverflow = 0;
	TIM_BASE->DIER &= ~TIM_DIER_UIE;
	timer = TIM_BASE->CNT;
	overflows = us_e16;
	MissedOverflow = (TIM_BASE->SR & TIM_SR_UIF);
	TIM_BASE->DIER |= TIM_DIER_UIE;
	/* No overflow bit mapped into counter value */
	if (timer < (1 << 15) && MissedOverflow) {
		/* Timer overflow has not been handled yet */
		overflows++;
	}
	return (((uint32_t) overflows) << 16) + timer;
}

void TIM_HANDLER(void) {
	if(TIM_BASE->SR & TIM_SR_UIF) {
		/* Clear the flag */
		TIM_BASE->SR = ~TIM_SR_UIF;
		/* 2^20 us have elapsed */
		us_e16++;
	}
}
#endif

#if configUSE_TICKLESS_IDLE == 1
// almost all of the following is directly copied from the FreeRTOS port and overrides weak functions with some small changes

/* For backward compatibility, ensure configKERNEL_INTERRUPT_PRIORITY is
defined.  The value should also ensure backward compatibility.
FreeRTOS.org versions prior to V4.4.0 did not include this definition. */
#ifndef configKERNEL_INTERRUPT_PRIORITY
	#define configKERNEL_INTERRUPT_PRIORITY 255
#endif

#ifndef configSYSTICK_CLOCK_HZ
	#define configSYSTICK_CLOCK_HZ configCPU_CLOCK_HZ
	/* Ensure the SysTick is clocked at the same frequency as the core. */
	#define portNVIC_SYSTICK_CLK_BIT	( 1UL << 2UL )
#else
	/* The way the SysTick is clocked is not modified in case it is not the same
	as the core. */
	#define portNVIC_SYSTICK_CLK_BIT	( 0 )
#endif

/* Constants required to manipulate the core.  Registers first... */
#define portNVIC_SYSTICK_CTRL_REG			( * ( ( volatile uint32_t * ) 0xe000e010 ) )
#define portNVIC_SYSTICK_LOAD_REG			( * ( ( volatile uint32_t * ) 0xe000e014 ) )
#define portNVIC_SYSTICK_CURRENT_VALUE_REG	( * ( ( volatile uint32_t * ) 0xe000e018 ) )
#define portNVIC_SYSPRI2_REG				( * ( ( volatile uint32_t * ) 0xe000ed20 ) )
/* ...then bits in the registers. */
#define portNVIC_SYSTICK_INT_BIT			( 1UL << 1UL )
#define portNVIC_SYSTICK_ENABLE_BIT			( 1UL << 0UL )
#define portNVIC_SYSTICK_COUNT_FLAG_BIT		( 1UL << 16UL )
#define portNVIC_PENDSVCLEAR_BIT 			( 1UL << 27UL )
#define portNVIC_PEND_SYSTICK_CLEAR_BIT		( 1UL << 25UL )

#define portNVIC_PENDSV_PRI					( ( ( uint32_t ) configKERNEL_INTERRUPT_PRIORITY ) << 16UL )
#define portNVIC_SYSTICK_PRI				( ( ( uint32_t ) configKERNEL_INTERRUPT_PRIORITY ) << 24UL )

/* Constants required to check the validity of an interrupt priority. */
#define portFIRST_USER_INTERRUPT_NUMBER		( 16 )
#define portNVIC_IP_REGISTERS_OFFSET_16 	( 0xE000E3F0 )
#define portAIRCR_REG						( * ( ( volatile uint32_t * ) 0xE000ED0C ) )
#define portMAX_8_BIT_VALUE					( ( uint8_t ) 0xff )
#define portTOP_BIT_OF_BYTE					( ( uint8_t ) 0x80 )
#define portMAX_PRIGROUP_BITS				( ( uint8_t ) 7 )
#define portPRIORITY_GROUP_MASK				( 0x07UL << 8UL )
#define portPRIGROUP_SHIFT					( 8UL )

/* Masks off all bits but the VECTACTIVE bits in the ICSR register. */
#define portVECTACTIVE_MASK					( 0xFFUL )

/* Constants required to set up the initial stack. */
#define portINITIAL_XPSR					( 0x01000000UL )

/* The systick is a 24-bit counter. */
#define portMAX_24_BIT_NUMBER				( 0xffffffUL )

/* A fiddle factor to estimate the number of SysTick counts that would have
occurred while the SysTick counter is stopped during tickless idle
calculations. */
#define portMISSED_COUNTS_FACTOR			( 45UL )

/* For strict compliance with the Cortex-M spec the task start address should
have bit-0 clear, as it is loaded into the PC on exit from an ISR. */
#define portSTART_ADDRESS_MASK				( ( StackType_t ) 0xfffffffeUL )

#define MAX_POSSIBLE_SUPPRESSED_TICKS	(UINT16_MAX*1000UL / (RTC_FREQUENCY / 16))
extern RTC_HandleTypeDef hrtc;
__IO uint32_t uwTick;
static volatile uint8_t rtc_wakeup;
static uint16_t pd_sleep = 0;

void pd_prevent_stop() {
	if (stm_in_interrupt()) {
		taskENTER_CRITICAL_FROM_ISR();
	} else {
		taskENTER_CRITICAL();
	}
	pd_sleep++;
	if (stm_in_interrupt()) {
		taskEXIT_CRITICAL_FROM_ISR(0);
	} else {
		taskEXIT_CRITICAL();
	}
}

void pd_allow_stop() {
	if (stm_in_interrupt()) {
		taskENTER_CRITICAL_FROM_ISR();
	} else {
		taskENTER_CRITICAL();
	}
	if (pd_sleep) {
		pd_sleep--;
	}
	if (stm_in_interrupt()) {
		taskEXIT_CRITICAL_FROM_ISR(0);
	} else {
		taskEXIT_CRITICAL();
	}
}

static uint32_t ulTimerCountsForOneTick = 0;

/*
 * The maximum number of tick periods that can be suppressed is limited by the
 * 24 bit resolution of the SysTick timer.
 */
static uint32_t xMaximumPossibleSuppressedTicks = 0;

/*
 * Compensate for the CPU cycles that pass while the SysTick is stopped (low
 * power functionality only.
 */
static uint32_t ulStoppedTimerCompensation = 0;

void vPortSetupTimerInterrupt( void )
{
	/* Calculate the constants required to configure the tick interrupt. */
	#if configUSE_TICKLESS_IDLE == 1
	{
		ulTimerCountsForOneTick = ( configSYSTICK_CLOCK_HZ / configTICK_RATE_HZ );
		xMaximumPossibleSuppressedTicks = portMAX_24_BIT_NUMBER / ulTimerCountsForOneTick;
		ulStoppedTimerCompensation = portMISSED_COUNTS_FACTOR / ( configCPU_CLOCK_HZ / configSYSTICK_CLOCK_HZ );
	}
	#endif /* configUSE_TICKLESS_IDLE */

	/* Configure SysTick to interrupt at the requested rate. */
	portNVIC_SYSTICK_LOAD_REG = ( configSYSTICK_CLOCK_HZ / configTICK_RATE_HZ ) - 1UL;
	portNVIC_SYSTICK_CTRL_REG = ( portNVIC_SYSTICK_CLK_BIT | portNVIC_SYSTICK_INT_BIT | portNVIC_SYSTICK_ENABLE_BIT );
}

__weak void freertos_stop_restore_clocks() {

}

void vPortSuppressTicksAndSleep(TickType_t xExpectedIdleTime) {
	if(pd_sleep) {
		// stop mode is not allowed, use standard FreeRTOS port implementation
		uint32_t ulReloadValue, ulCompleteTickPeriods, ulCompletedSysTickDecrements, ulSysTickCTRL;
		TickType_t xModifiableIdleTime;

			/* Make sure the SysTick reload value does not overflow the counter. */
			if( xExpectedIdleTime > xMaximumPossibleSuppressedTicks )
			{
				xExpectedIdleTime = xMaximumPossibleSuppressedTicks;
			}

			/* Stop the SysTick momentarily.  The time the SysTick is stopped for
			is accounted for as best it can be, but using the tickless mode will
			inevitably result in some tiny drift of the time maintained by the
			kernel with respect to calendar time. */
			portNVIC_SYSTICK_CTRL_REG &= ~portNVIC_SYSTICK_ENABLE_BIT;
			HAL_SuspendTick();

			/* Calculate the reload value required to wait xExpectedIdleTime
			tick periods.  -1 is used because this code will execute part way
			through one of the tick periods. */
			ulReloadValue = portNVIC_SYSTICK_CURRENT_VALUE_REG + ( ulTimerCountsForOneTick * ( xExpectedIdleTime - 1UL ) );
			if( ulReloadValue > ulStoppedTimerCompensation )
			{
				ulReloadValue -= ulStoppedTimerCompensation;
			}

			/* Enter a critical section but don't use the taskENTER_CRITICAL()
			method as that will mask interrupts that should exit sleep mode. */
			__asm volatile( "cpsid i" );
			__asm volatile( "dsb" );
			__asm volatile( "isb" );

			/* If a context switch is pending or a task is waiting for the scheduler
			to be unsuspended then abandon the low power entry. */
			if( eTaskConfirmSleepModeStatus() == eAbortSleep )
			{
				/* Restart from whatever is left in the count register to complete
				this tick period. */
				portNVIC_SYSTICK_LOAD_REG = portNVIC_SYSTICK_CURRENT_VALUE_REG;

				/* Restart SysTick. */
				portNVIC_SYSTICK_CTRL_REG |= portNVIC_SYSTICK_ENABLE_BIT;
				HAL_ResumeTick();

				/* Reset the reload register to the value required for normal tick
				periods. */
				portNVIC_SYSTICK_LOAD_REG = ulTimerCountsForOneTick - 1UL;

				/* Re-enable interrupts - see comments above the cpsid instruction()
				above. */
				__asm volatile( "cpsie i" );
			}
			else
			{
				/* Set the new reload value. */
				portNVIC_SYSTICK_LOAD_REG = ulReloadValue;

				/* Clear the SysTick count flag and set the count value back to
				zero. */
				portNVIC_SYSTICK_CURRENT_VALUE_REG = 0UL;

				/* Restart SysTick. */
				portNVIC_SYSTICK_CTRL_REG |= portNVIC_SYSTICK_ENABLE_BIT;

				/* Sleep until something happens.  configPRE_SLEEP_PROCESSING() can
				set its parameter to 0 to indicate that its implementation contains
				its own wait for interrupt or wait for event instruction, and so wfi
				should not be executed again.  However, the original expected idle
				time variable must remain unmodified, so a copy is taken. */
				xModifiableIdleTime = xExpectedIdleTime;
				__HAL_RCC_PWR_CLK_ENABLE();
				MODIFY_REG(PWR->CR, (PWR_CR_PDDS | PWR_CR_LPSDSR), 0);
//				configPRE_SLEEP_PROCESSING( &xModifiableIdleTime );
				if( xModifiableIdleTime > 0 )
				{
					__asm volatile( "dsb" );
					__asm volatile( "wfi" );
					__asm volatile( "nop" );
					__asm volatile( "nop" );
					__asm volatile( "nop" );
					__asm volatile( "nop" );
					__asm volatile( "nop" );
					__asm volatile( "nop" );
					__asm volatile( "nop" );
					__asm volatile( "nop" );
					__asm volatile( "nop" );
					__asm volatile( "nop" );
					__asm volatile( "nop" );
					__asm volatile( "nop" );
					__asm volatile( "isb" );
				}
//				configPOST_SLEEP_PROCESSING( &xExpectedIdleTime );

				/* Stop SysTick.  Again, the time the SysTick is stopped for is
				accounted for as best it can be, but using the tickless mode will
				inevitably result in some tiny drift of the time maintained by the
				kernel with respect to calendar time. */
				ulSysTickCTRL = portNVIC_SYSTICK_CTRL_REG;
				portNVIC_SYSTICK_CTRL_REG = ( ulSysTickCTRL & ~portNVIC_SYSTICK_ENABLE_BIT );

				/* Re-enable interrupts - see comments above the cpsid instruction()
				above. */
				__asm volatile( "cpsie i" );

				if( ( ulSysTickCTRL & portNVIC_SYSTICK_COUNT_FLAG_BIT ) != 0 )
				{
					uint32_t ulCalculatedLoadValue;

					/* The tick interrupt has already executed, and the SysTick
					count reloaded with ulReloadValue.  Reset the
					portNVIC_SYSTICK_LOAD_REG with whatever remains of this tick
					period. */
					ulCalculatedLoadValue = ( ulTimerCountsForOneTick - 1UL ) - ( ulReloadValue - portNVIC_SYSTICK_CURRENT_VALUE_REG );

					/* Don't allow a tiny value, or values that have somehow
					underflowed because the post sleep hook did something
					that took too long. */
					if( ( ulCalculatedLoadValue < ulStoppedTimerCompensation ) || ( ulCalculatedLoadValue > ulTimerCountsForOneTick ) )
					{
						ulCalculatedLoadValue = ( ulTimerCountsForOneTick - 1UL );
					}

					portNVIC_SYSTICK_LOAD_REG = ulCalculatedLoadValue;

					/* The tick interrupt handler will already have pended the tick
					processing in the kernel.  As the pending tick will be
					processed as soon as this function exits, the tick value
					maintained by the tick is stepped forward by one less than the
					time spent waiting. */
					ulCompleteTickPeriods = xExpectedIdleTime - 1UL;
				}
				else
				{
					/* Something other than the tick interrupt ended the sleep.
					Work out how long the sleep lasted rounded to complete tick
					periods (not the ulReload value which accounted for part
					ticks). */
					ulCompletedSysTickDecrements = ( xExpectedIdleTime * ulTimerCountsForOneTick ) - portNVIC_SYSTICK_CURRENT_VALUE_REG;

					/* How many complete tick periods passed while the processor
					was waiting? */
					ulCompleteTickPeriods = ulCompletedSysTickDecrements / ulTimerCountsForOneTick;

					/* The reload value is set to whatever fraction of a single tick
					period remains. */
					portNVIC_SYSTICK_LOAD_REG = ( ( ulCompleteTickPeriods + 1UL ) * ulTimerCountsForOneTick ) - ulCompletedSysTickDecrements;
				}

				/* Restart SysTick so it runs from portNVIC_SYSTICK_LOAD_REG
				again, then set portNVIC_SYSTICK_LOAD_REG back to its standard
				value.  The critical section is used to ensure the tick interrupt
				can only execute once in the case that the reload register is near
				zero. */
				portNVIC_SYSTICK_CURRENT_VALUE_REG = 0UL;
				portENTER_CRITICAL();
				{
					vTaskStepTick( ulCompleteTickPeriods );
					uwTick += ulCompleteTickPeriods;
					portNVIC_SYSTICK_CTRL_REG |= portNVIC_SYSTICK_ENABLE_BIT;
					HAL_ResumeTick();
					portNVIC_SYSTICK_LOAD_REG = ulTimerCountsForOneTick - 1UL;
				}
				portEXIT_CRITICAL();
			}
	} else {
		// more energy saving stop mode is allowed, use custom implementation
		uint32_t ulCompleteTickPeriods;

		/* Make sure the SysTick reload value does not overflow the counter. */
		if (xExpectedIdleTime > MAX_POSSIBLE_SUPPRESSED_TICKS) {
			xExpectedIdleTime = MAX_POSSIBLE_SUPPRESSED_TICKS;
		}

		portNVIC_SYSTICK_CTRL_REG &= ~portNVIC_SYSTICK_ENABLE_BIT;
		HAL_SuspendTick();

		/* Enter a critical section but don't use the taskENTER_CRITICAL()
		 method as that will mask interrupts that should exit sleep mode. */
		__asm volatile( "cpsid i" );
		__asm volatile( "dsb" );
		__asm volatile( "isb" );

		/* If a context switch is pending or a task is waiting for the scheduler
		 to be unsuspended then abandon the low power entry. */
		if (eTaskConfirmSleepModeStatus() == eAbortSleep) {
			portNVIC_SYSTICK_CTRL_REG |= portNVIC_SYSTICK_ENABLE_BIT;
			HAL_ResumeTick();

			/* Re-enable interrupts - see comments above the cpsid instruction()
			 above. */
			__asm volatile( "cpsie i" );
		} else {
			uint32_t subsecond_before = RTC->SSR;
			uint32_t second_before = (uint8_t) (RTC->TR
					& (RTC_TR_ST | RTC_TR_SU));
			RTC->DR;
			uint32_t rtc_ticks = xExpectedIdleTime * (RTC_FREQUENCY / 16)
					/ 1000;
			HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, rtc_ticks,
			RTC_WAKEUPCLOCK_RTCCLK_DIV16);
			rtc_wakeup = 0;

			/* Enter power down Mode */
			__HAL_RCC_PWR_CLK_ENABLE()
			;
			HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);
			// was in stop mode, adjust clocks
			freertos_stop_restore_clocks();

			/* Re-enable interrupts - see comments above the cpsid instruction()
			 above. */
			__asm volatile( "cpsie i" );

			if (rtc_wakeup) {
				// woke up by RTC interrupt, passed time already known
				ulCompleteTickPeriods = xExpectedIdleTime;
			} else {
				// woke up by some other interrupt, calculate passed time with RTC
				uint32_t subsecond_after = RTC->SSR;
				uint32_t second_after = (uint8_t) (RTC->TR
						& (RTC_TR_ST | RTC_TR_SU));
				RTC->DR;
				int16_t seconds_passed = second_after - second_before;
				if (seconds_passed < 0) {
					seconds_passed += 60;
				}
				int32_t subseconds_passed =
						-(subsecond_after - subsecond_before);
				uint16_t prediv_s = (RTC->PRER & 0x7FFF) + 1;
				if (subseconds_passed < 0) {
					subseconds_passed += prediv_s;
				}
				ulCompleteTickPeriods = seconds_passed * 1000
						+ subseconds_passed * 1000 / (prediv_s);
				if (ulCompleteTickPeriods > xExpectedIdleTime) {
					ulCompleteTickPeriods = xExpectedIdleTime;
				}
			}
			HAL_RTCEx_DeactivateWakeUpTimer(&hrtc);

			// adjust passed system time
			portENTER_CRITICAL();
			{
				vTaskStepTick(ulCompleteTickPeriods);
				uwTick += ulCompleteTickPeriods;
			}
			portEXIT_CRITICAL();

			portNVIC_SYSTICK_CTRL_REG |= portNVIC_SYSTICK_ENABLE_BIT;
			HAL_ResumeTick();
		}
	}
}

void HAL_RTCEx_WakeUpTimerEventCallback(RTC_HandleTypeDef *hrtc) {
	/* Clear Wake Up Flag */
	__HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);
	rtc_wakeup = 1;
}
#endif

void *pvPortCalloc(size_t num, size_t xSize) {
	void *ret = pvPortMalloc(num * xSize);
	if (ret) {
		memset(ret, 0, num * xSize);
	}
	return ret;
}

#if configUSE_TRACE_FACILITY == 1
int freertos_print_task_overview() {
	uint16_t nTasks = uxTaskGetNumberOfTasks();
	TaskStatus_t *task_states = pvPortMalloc(nTasks * sizeof(TaskStatus_t));
	if (!task_states) {
		LOG(Log_System, LevelError,
				"Failed to allocate memory for task states");
		return -1;
	}
	uint32_t total_runtime;
	nTasks = uxTaskGetSystemState(task_states, nTasks, &total_runtime);
	for (uint16_t i = 0; i < nTasks; i++) {
		uint8_t runtime_percentage = task_states[i].ulRunTimeCounter
				/ (total_runtime / 100);
		const char state_names[][8] = { "RUNNING", "READY", "BLOCKED",
				"SUSPEND", "DELETED", "INVALID" };
		LOG(Log_System, LevelInfo,
				"Task %d, %.8s: state %.7s, priority %d, runtime %d%%, stack remaining[words] %lu)",
				task_states[i].xTaskNumber, task_states[i].pcTaskName,
				state_names[task_states[i].eCurrentState],
				task_states[i].uxBasePriority, runtime_percentage,
				task_states[i].usStackHighWaterMark);
	}
	LOG(Log_System, LevelInfo, "Free heap[bytes]: %lu/%lu (minimum ever: %lu)",
			xPortGetFreeHeapSize(), configTOTAL_HEAP_SIZE,
			xPortGetMinimumEverFreeHeapSize());
	vPortFree(task_states);
	return 0;
}
#endif
