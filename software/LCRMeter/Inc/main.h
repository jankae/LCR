/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f3xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define DISP_IO2_Pin GPIO_PIN_2
#define DISP_IO2_GPIO_Port GPIOE
#define DISP_IO3_Pin GPIO_PIN_3
#define DISP_IO3_GPIO_Port GPIOE
#define DISP_IO4_Pin GPIO_PIN_4
#define DISP_IO4_GPIO_Port GPIOE
#define DISP_IO5_Pin GPIO_PIN_5
#define DISP_IO5_GPIO_Port GPIOE
#define DISP_IO6_Pin GPIO_PIN_6
#define DISP_IO6_GPIO_Port GPIOE
#define DISP_RS_Pin GPIO_PIN_13
#define DISP_RS_GPIO_Port GPIOC
#define DISP_WR_Pin GPIO_PIN_14
#define DISP_WR_GPIO_Port GPIOC
#define DISP_RD_Pin GPIO_PIN_15
#define DISP_RD_GPIO_Port GPIOC
#define TOUCH_CS_Pin GPIO_PIN_9
#define TOUCH_CS_GPIO_Port GPIOF
#define TOUCH_IRQ_Pin GPIO_PIN_0
#define TOUCH_IRQ_GPIO_Port GPIOA
#define DISP_CS_Pin GPIO_PIN_1
#define DISP_CS_GPIO_Port GPIOA
#define DISP_RST_Pin GPIO_PIN_2
#define DISP_RST_GPIO_Port GPIOA
#define SD_CS_Pin GPIO_PIN_3
#define SD_CS_GPIO_Port GPIOA
#define DISP_IO7_Pin GPIO_PIN_7
#define DISP_IO7_GPIO_Port GPIOE
#define DISP_IO8_Pin GPIO_PIN_8
#define DISP_IO8_GPIO_Port GPIOE
#define DISP_IO9_Pin GPIO_PIN_9
#define DISP_IO9_GPIO_Port GPIOE
#define DISP_IO10_Pin GPIO_PIN_10
#define DISP_IO10_GPIO_Port GPIOE
#define DISP_IO11_Pin GPIO_PIN_11
#define DISP_IO11_GPIO_Port GPIOE
#define DISP_IO12_Pin GPIO_PIN_12
#define DISP_IO12_GPIO_Port GPIOE
#define DISP_IO13_Pin GPIO_PIN_13
#define DISP_IO13_GPIO_Port GPIOE
#define DISP_IO14_Pin GPIO_PIN_14
#define DISP_IO14_GPIO_Port GPIOE
#define DISP_IO15_Pin GPIO_PIN_15
#define DISP_IO15_GPIO_Port GPIOE
#define USB_IDENT_Pin GPIO_PIN_8
#define USB_IDENT_GPIO_Port GPIOA
#define AD5941_CS_Pin GPIO_PIN_6
#define AD5941_CS_GPIO_Port GPIOB
#define AD5941_RESET_Pin GPIO_PIN_7
#define AD5941_RESET_GPIO_Port GPIOB
#define SOUND_INT_Pin GPIO_PIN_8
#define SOUND_INT_GPIO_Port GPIOB
#define DISP_IO0_Pin GPIO_PIN_0
#define DISP_IO0_GPIO_Port GPIOE
#define DISP_IO1_Pin GPIO_PIN_1
#define DISP_IO1_GPIO_Port GPIOE
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
