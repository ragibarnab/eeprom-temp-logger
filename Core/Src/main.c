/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include <string.h>
#include <stdio.h>

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

SPI_HandleTypeDef hspi3;

UART_HandleTypeDef huart2;

// tmp75b references
static const uint8_t TMP75B_ADDR = 0x48 << 1; // 7-bit address: 1001000
static const uint8_t GEN_CALL_ADDR = 0x00;
static const uint8_t REG_TEMP = 0x00;
static const uint8_t REG_CONFIG = 0x01;
static const uint8_t REG_RESET = 0x06;
static const uint8_t SD_MODE = 0x01;
static const uint8_t OS_MODE = 0x80;

// eeprom references
static const uint8_t EEPROM_READ = 0b00000011;
static const uint8_t EEPROM_WRITE = 0b00000010;
static const uint8_t EEPROM_WRDI = 0b00000100;
static const uint8_t EEPROM_WREN = 0b00000110;
static const uint8_t EEPROM_RDSR = 0b00000101;
static const uint8_t EEPROM_WRSR = 0b00000001;
static const uint8_t EEPROM_PAGE_SIZE = 32;			 // 32 bytes per page
static const unsigned int EEPROM_MAX_BYTES = 0x2000; // 8192 bytes max

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_SPI3_Init(void);
static void MX_USART2_UART_Init(void);

static void TMP75B_OS_Read(uint8_t *i2c_buf, char *uart_buf);
static void TMP75B_Reset(uint8_t *i2c_buf, char *uart_buf);
static void TMP75B_ConfigRead(uint8_t *i2c_buf, char *uart_buf);
static void TMP75B_ShutdownEnable(uint8_t *i2c_buf, char *uart_buf);

static void EEPROM_WriteEnable(char *uart_buf);
static void EEPROM_WIP(uint8_t *spi_buf, char *uart_buf);
static void EEPROM_Write(uint16_t addr, uint8_t *spi_buf, uint8_t size,
						 char *uart_buf);
static void EEPROM_ReadTemp(uint16_t addr, uint8_t *spi_buf, char *uart_buf);
static void EEPROM_LogTemp(uint32_t period_ms);

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void)
{

	/* MCU Configuration--------------------------------------------------------*/

	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();

	/* Configure the system clock */
	SystemClock_Config();

	/* Initialize all configured peripherals */
	MX_GPIO_Init();
	MX_I2C1_Init();
	MX_SPI3_Init();
	MX_USART2_UART_Init();

	/*
		function below samples from the tmp75b sensor at a period of 100 ms
		and will log it into the eeprom from the very beginning to the end
		of its memory. It will light the LED on PA5 when done. The period
		could be much longer (HAL_MAX_DELAY)
	*/
	EEPROM_LogTemp(100);

	// tries to read the last byte of the eeprom to make sure it finished
	uint8_t spi_buf[16];
	char uart_buf[64];
	EEPROM_ReadTemp(EEPROM_MAX_BYTES - 2, spi_buf, uart_buf); 

	/* Infinite loop */

	while (1)
	{
	}
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void)
{
	RCC_OscInitTypeDef RCC_OscInitStruct = {0};
	RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

	/** Configure the main internal regulator output voltage
	 */
	__HAL_RCC_PWR_CLK_ENABLE();
	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);
	/** Initializes the RCC Oscillators according to the specified parameters
	 * in the RCC_OscInitTypeDef structure.
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
	{
		Error_Handler();
	}
	/** Initializes the CPU, AHB and APB buses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
	{
		Error_Handler();
	}
}

/**
 * @brief I2C1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_I2C1_Init(void)
{

	hi2c1.Instance = I2C1;
	hi2c1.Init.ClockSpeed = 100000;
	hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
	hi2c1.Init.OwnAddress1 = 0;
	hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
	hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
	hi2c1.Init.OwnAddress2 = 0;
	hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
	hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
	if (HAL_I2C_Init(&hi2c1) != HAL_OK)
	{
		Error_Handler();
	}
}

/**
 * @brief SPI3 Initialization Function
 * @param None
 * @retval None
 */
static void MX_SPI3_Init(void)
{

	/* SPI3 parameter configuration*/
	hspi3.Instance = SPI3;
	hspi3.Init.Mode = SPI_MODE_MASTER;
	hspi3.Init.Direction = SPI_DIRECTION_2LINES;
	hspi3.Init.DataSize = SPI_DATASIZE_8BIT;
	hspi3.Init.CLKPolarity = SPI_POLARITY_LOW;
	hspi3.Init.CLKPhase = SPI_PHASE_1EDGE;
	hspi3.Init.NSS = SPI_NSS_SOFT;
	hspi3.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
	hspi3.Init.FirstBit = SPI_FIRSTBIT_MSB;
	hspi3.Init.TIMode = SPI_TIMODE_DISABLE;
	hspi3.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
	hspi3.Init.CRCPolynomial = 10;
	if (HAL_SPI_Init(&hspi3) != HAL_OK)
	{
		Error_Handler();
	}
}

/**
 * @brief USART2 Initialization Function
 * @param None
 * @retval None
 */
static void MX_USART2_UART_Init(void)
{

	huart2.Instance = USART2;
	huart2.Init.BaudRate = 115200;
	huart2.Init.WordLength = UART_WORDLENGTH_8B;
	huart2.Init.StopBits = UART_STOPBITS_1;
	huart2.Init.Parity = UART_PARITY_NONE;
	huart2.Init.Mode = UART_MODE_TX_RX;
	huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart2.Init.OverSampling = UART_OVERSAMPLING_16;
	if (HAL_UART_Init(&huart2) != HAL_OK)
	{
		Error_Handler();
	}
}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	/* GPIO Ports Clock Enable */
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5 | GPIO_PIN_10, GPIO_PIN_RESET);

	/*Configure GPIO pins : PA5 PA10 */
	GPIO_InitStruct.Pin = GPIO_PIN_5 | GPIO_PIN_10;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

static void EEPROM_LogTemp(uint32_t period_ms)
{
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
	uint8_t i2c_buf[16];
	uint8_t spi_buf[16];
	uint16_t addr;
	char uart_buf[64];

	TMP75B_Reset(i2c_buf, uart_buf);
	TMP75B_ShutdownEnable(i2c_buf, uart_buf);

	for (addr = 0x0000; addr < EEPROM_MAX_BYTES; addr += 2)
	{
		TMP75B_OS_Read(i2c_buf, uart_buf);
		spi_buf[0] = i2c_buf[0]; // temp msb
		spi_buf[1] = i2c_buf[1]; // temp lsb
		EEPROM_Write(addr, spi_buf, 2, uart_buf);
		if (addr + 2 < EEPROM_MAX_BYTES)
			HAL_Delay(period_ms);
	}
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET); // indicate finish
}

static void TMP75B_OS_Read(uint8_t *i2c_buf, char *uart_buf)
{
	i2c_buf[0] = REG_CONFIG;
	i2c_buf[1] = OS_MODE | SD_MODE;
	HAL_StatusTypeDef ret = HAL_I2C_Master_Transmit(&hi2c1, TMP75B_ADDR, i2c_buf, 2, HAL_MAX_DELAY);
	if (ret != HAL_OK)
	{
		strcpy(uart_buf, "OS Tx Error\r\n");
		HAL_UART_Transmit(&huart2, (uint8_t *)uart_buf, strlen(uart_buf), HAL_MAX_DELAY);
	}
	else
		HAL_Delay(50); // allow time for conversion

	i2c_buf[0] = REG_TEMP;
	ret = HAL_I2C_Master_Transmit(&hi2c1, TMP75B_ADDR, i2c_buf, 1, HAL_MAX_DELAY);
	if (ret != HAL_OK)
		strcpy(uart_buf, "TEMP Tx Error\r\n");
	else
	{
		ret = HAL_I2C_Master_Receive(&hi2c1, TMP75B_ADDR, i2c_buf, 2, HAL_MAX_DELAY);
		if (ret != HAL_OK)
			strcpy(uart_buf, "TEMP Rx Error\r\n");
		else
		{
			uint16_t val = (i2c_buf[0] << 4) + (i2c_buf[1] >> 4);
			float temp_c = 0.0625 * val * 100;
			sprintf(uart_buf, "ABS TEMP READ: %u.%02u C\r\n", 
			((unsigned int)temp_c / 100), 
			((unsigned int)temp_c % 100));
		}
	}
	HAL_UART_Transmit(&huart2, (uint8_t *)uart_buf, strlen(uart_buf), HAL_MAX_DELAY);
}

static void TMP75B_Reset(uint8_t *i2c_buf, char *uart_buf)
{
	i2c_buf[0] = REG_RESET;
	HAL_StatusTypeDef ret = HAL_I2C_Master_Transmit(&hi2c1, GEN_CALL_ADDR, i2c_buf, 1, HAL_MAX_DELAY);
	if (ret != HAL_OK)
		strcpy(uart_buf, "RESET Error\r\n");
	else
		strcpy(uart_buf, "RESET Success\r\n");
	HAL_UART_Transmit(&huart2, (uint8_t *)uart_buf, strlen(uart_buf), HAL_MAX_DELAY);
}

static void TMP75B_ConfigRead(uint8_t *i2c_buf, char *uart_buf)
{
	i2c_buf[0] = REG_CONFIG;
	HAL_StatusTypeDef ret = HAL_I2C_Master_Transmit(&hi2c1, TMP75B_ADDR, i2c_buf, 1, HAL_MAX_DELAY);
	if (ret != HAL_OK)
		strcpy(uart_buf, "CONFIG Tx Error\r\n");
	else
	{
		ret = HAL_I2C_Master_Receive(&hi2c1, TMP75B_ADDR, i2c_buf, 2, HAL_MAX_DELAY);
		if (ret != HAL_OK)
			strcpy(uart_buf, "CONFIG Rx Error\r\n");
		else
			sprintf(uart_buf, "CONFIG READ: 0x%02x 0x%02x\r\n", 
			(unsigned int)i2c_buf[0], (unsigned int)i2c_buf[1]);
	}
	HAL_UART_Transmit(&huart2, (uint8_t *)uart_buf, strlen(uart_buf), HAL_MAX_DELAY);
}

static void TMP75B_ShutdownEnable(uint8_t *i2c_buf, char *uart_buf)
{
	i2c_buf[0] = REG_CONFIG;
	i2c_buf[1] = SD_MODE;
	HAL_StatusTypeDef ret = HAL_I2C_Master_Transmit(&hi2c1, TMP75B_ADDR, i2c_buf, 2, HAL_MAX_DELAY);
	if (ret != HAL_OK)
		strcpy(uart_buf, "CONFIG Tx Error\r\n");
	else
		strcpy(uart_buf, "SD Mode ON\r\n");
	HAL_UART_Transmit(&huart2, (uint8_t *)uart_buf, strlen(uart_buf), HAL_MAX_DELAY);
}

static void EEPROM_WriteEnable(char *uart_buf)
{
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_RESET);
	HAL_StatusTypeDef ret = HAL_SPI_Transmit(&hspi3, (uint8_t *)&EEPROM_WREN, 1, HAL_MAX_DELAY);
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_SET);
	if (ret != HAL_OK)
	{
		strcpy(uart_buf, "WREN Error\r\n");
		HAL_UART_Transmit(&huart2, (uint8_t *)uart_buf, strlen(uart_buf), HAL_MAX_DELAY);
	}
}

static void EEPROM_WIP(uint8_t *spi_buf, char *uart_buf)
{
	uint8_t WIP;
	do
	{
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_RESET);
		HAL_SPI_Transmit(&hspi3, (uint8_t *)&EEPROM_RDSR, 1, HAL_MAX_DELAY);
		HAL_SPI_Receive(&hspi3, (uint8_t *)spi_buf, 1, HAL_MAX_DELAY);
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_SET);

		WIP = spi_buf[0] & 0b00000001;
	} while (WIP);
}

static void EEPROM_Write(uint16_t addr, uint8_t *spi_buf, uint8_t size,
						 char *uart_buf)
{
	EEPROM_WriteEnable(uart_buf);
	uint8_t addr_buf[] = {(uint8_t)(addr >> 8), (uint8_t)(addr)};
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_RESET);
	HAL_StatusTypeDef ret = HAL_SPI_Transmit(&hspi3, (uint8_t *)&EEPROM_WRITE, 1, HAL_MAX_DELAY);
	if (ret != HAL_OK)
	{
		strcpy(uart_buf, "WRITE Error\r\n");
		HAL_UART_Transmit(&huart2, (uint8_t *)uart_buf, strlen(uart_buf), HAL_MAX_DELAY);
	}
	else
	{
		ret = HAL_SPI_Transmit(&hspi3, addr_buf, 2, HAL_MAX_DELAY);
		if (ret != HAL_OK)
		{
			strcpy(uart_buf, "WRITE Error\r\n");
			HAL_UART_Transmit(&huart2, (uint8_t *)uart_buf, strlen(uart_buf), HAL_MAX_DELAY);
		}
		else
		{
			ret = HAL_SPI_Transmit(&hspi3, spi_buf, size, HAL_MAX_DELAY);
			if (ret != HAL_OK)
			{
				strcpy(uart_buf, "WRITE Error\r\n");
				HAL_UART_Transmit(&huart2, (uint8_t *)uart_buf, strlen(uart_buf), HAL_MAX_DELAY);
			}
		}
	}
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_SET);
	EEPROM_WIP(spi_buf, uart_buf);
}

static void EEPROM_ReadTemp(uint16_t addr, uint8_t *spi_buf, char *uart_buf)
{
	uint8_t addr_buf[] = {(uint8_t)(addr >> 8), (uint8_t)(addr)};
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_RESET);
	HAL_SPI_Transmit(&hspi3, (uint8_t *)&EEPROM_READ, 1, HAL_MAX_DELAY);
	HAL_SPI_Transmit(&hspi3, addr_buf, 2, HAL_MAX_DELAY);
	HAL_SPI_Receive(&hspi3, spi_buf, 2, HAL_MAX_DELAY);
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_SET);
	sprintf(uart_buf, "Offset 0x%04x: 0x%02x 0x%02x\r\n", addr, (uint8_t)spi_buf[0], (uint8_t)spi_buf[1]);
	HAL_UART_Transmit(&huart2, (uint8_t *)uart_buf, strlen(uart_buf), HAL_MAX_DELAY);
}

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{

	__disable_irq();
	while (1)
	{
	}
}

#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif /* USE_FULL_ASSERT */
