/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "fatfs.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "sd_spi.h"
#include <string.h>
#include <stdio.h>
#include "ff.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

SPI_HandleTypeDef hspi1;
SPI_HandleTypeDef hspi2;

UART_HandleTypeDef huart2;
UART_HandleTypeDef huart3;

/* USER CODE BEGIN PV */
uint8_t spi_tx;
uint8_t spi_rx;
uint8_t sd_buffer[512];

FATFS image_fs;
FIL image_file;

FRESULT sd_result;
UINT bytes_written;
UINT bytes_read;

char sd_test_data[] = "STM32 SD TEST\r\n";

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
static void MX_USART3_UART_Init(void);
static void MX_SPI2_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */


/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
#define PACKET_SIZE 210

uint8_t spi2_rx_buffer[PACKET_SIZE];
uint16_t crc16_ccitt(const uint8_t *data, uint32_t length)
{
    uint16_t crc = 0xFFFF;

    for (uint32_t i = 0; i < length; i++)
    {
        crc ^= ((uint16_t)data[i] << 8);

        for (uint8_t j = 0; j < 8; j++)
        {
            if (crc & 0x8000)
            {
                crc = (crc << 1) ^ 0x1021;
            }
            else
            {
                crc <<= 1;
            }
        }
    }

    return crc;
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_SPI1_Init();
  MX_USART3_UART_Init();
  MX_FATFS_Init();
  MX_SPI2_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */


  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	    FRESULT fres;
	    UINT bytes_written;

	    uint8_t packet_buffer[PACKET_SIZE];

	    uint8_t transfer_ok = 1;

	    /*
	     * ------------------------------------------------------------
	     * MOUNT FILESYSTEM
	     * ------------------------------------------------------------
	     */

	    fres = f_mount(&USERFatFS, USERPath, 1);

	    if (fres != FR_OK)
	    {
	        char msg[80];

	        int len = sprintf(
	            msg,
	            "FATFS MOUNT ERROR: %d\r\n",
	            fres
	        );

	        HAL_UART_Transmit(
	            &huart3,
	            (uint8_t *)msg,
	            len,
	            HAL_MAX_DELAY
	        );

	        break;
	    }

	    HAL_UART_Transmit(
	        &huart3,
	        (uint8_t *)"FATFS MOUNT OK\r\n",
	        strlen("FATFS MOUNT OK\r\n"),
	        HAL_MAX_DELAY
	    );


	    /*
	     * ------------------------------------------------------------
	     * OPEN image.bin
	     *
	     * The file remains open for the entire transfer.
	     * Each packet payload is appended to the file.
	     * ------------------------------------------------------------
	     */

	    fres = f_open(
	        &USERFile,
	        "image.bin",
	        FA_CREATE_ALWAYS | FA_WRITE
	    );

	    if (fres != FR_OK)
	    {
	        char msg[80];

	        int len = sprintf(
	            msg,
	            "image.bin OPEN ERROR: %d\r\n",
	            fres
	        );

	        HAL_UART_Transmit(
	            &huart3,
	            (uint8_t *)msg,
	            len,
	            HAL_MAX_DELAY
	        );

	        break;
	    }

	    HAL_UART_Transmit(
	        &huart3,
	        (uint8_t *)"image.bin OPEN OK\r\n",
	        strlen("image.bin OPEN OK\r\n"),
	        HAL_MAX_DELAY
	    );


	    /*
	     * ------------------------------------------------------------
	     * TELL RPi TO START IMAGE TRANSFER
	     * ------------------------------------------------------------
	     */

	    uint8_t tx_msg[] = "SEND IMAGE\r\n";

	    HAL_UART_Transmit(
	        &huart2,
	        tx_msg,
	        sizeof(tx_msg) - 1,
	        HAL_MAX_DELAY
	    );

	    HAL_UART_Transmit(
	        &huart3,
	        (uint8_t *)"UART2 TX: SEND IMAGE\r\n",
	        strlen("UART2 TX: SEND IMAGE\r\n"),
	        HAL_MAX_DELAY
	    );


	    /*
	     * ------------------------------------------------------------
	     * RECEIVE PACKETS 0 TO 9
	     * ------------------------------------------------------------
	     */

	    for (uint32_t expected_packet_id = 0;
	         expected_packet_id < 10;
	         expected_packet_id++)
	    {
	        uint8_t ready_msg[] = "SPI READY\r\n";

	        /*
	         * --------------------------------------------------------
	         * TELL RPi THAT STM32 IS READY FOR THIS PACKET
	         * --------------------------------------------------------
	         */

	        HAL_UART_Transmit(
	            &huart2,
	            ready_msg,
	            sizeof(ready_msg) - 1,
	            HAL_MAX_DELAY
	        );

	        /*
	         * IMPORTANT:
	         *
	         * DO NOT PUT DEBUG UART TRANSMISSIONS HERE.
	         *
	         * SPI READY must be followed immediately by
	         * HAL_SPI_Receive().
	         */

	        HAL_StatusTypeDef spi_status;

	        spi_status = HAL_SPI_Receive(
	            &hspi2,
	            packet_buffer,
	            PACKET_SIZE,
	            5000
	        );

	        if (spi_status != HAL_OK)
	        {
	            uint32_t error = HAL_SPI_GetError(&hspi2);

	            char error_msg[100];

	            int len = sprintf(
	                error_msg,
	                "PACKET %lu SPI ERROR: 0x%08lX\r\n",
	                expected_packet_id,
	                error
	            );

	            HAL_UART_Transmit(
	                &huart3,
	                (uint8_t *)error_msg,
	                len,
	                HAL_MAX_DELAY
	            );

	            transfer_ok = 0;

	            break;
	        }


	        /*
	         * --------------------------------------------------------
	         * PARSE PACKET
	         * --------------------------------------------------------
	         */

	        uint16_t sync =
	            ((uint16_t)packet_buffer[0] << 8) |
	            packet_buffer[1];

	        uint32_t packet_id =
	            ((uint32_t)packet_buffer[2] << 24) |
	            ((uint32_t)packet_buffer[3] << 16) |
	            ((uint32_t)packet_buffer[4] << 8) |
	            packet_buffer[5];

	        uint16_t payload_length =
	            ((uint16_t)packet_buffer[6] << 8) |
	            packet_buffer[7];

	        uint16_t received_crc =
	            ((uint16_t)packet_buffer[8] << 8) |
	            packet_buffer[9];


	        /*
	         * --------------------------------------------------------
	         * VALIDATE PAYLOAD LENGTH BEFORE COPYING
	         *
	         * Maximum payload = 200 bytes.
	         * --------------------------------------------------------
	         */

	        if (payload_length > 200)
	        {
	            char error_msg[100];

	            int len = sprintf(
	                error_msg,
	                "PACKET %lu INVALID LENGTH: %u\r\n",
	                expected_packet_id,
	                payload_length
	            );

	            HAL_UART_Transmit(
	                &huart3,
	                (uint8_t *)error_msg,
	                len,
	                HAL_MAX_DELAY
	            );

	            transfer_ok = 0;

	            break;
	        }


	        /*
	         * --------------------------------------------------------
	         * CALCULATE CRC
	         *
	         * CRC covers:
	         *
	         *   SYNC
	         *   PACKET ID
	         *   LENGTH
	         *   PAYLOAD
	         *
	         * CRC field itself is NOT included.
	         * --------------------------------------------------------
	         */

	        uint8_t crc_buffer[208];

	        memcpy(
	            crc_buffer,
	            packet_buffer,
	            8
	        );

	        memcpy(
	            &crc_buffer[8],
	            &packet_buffer[10],
	            payload_length
	        );

	        uint16_t calculated_crc =
	            crc16_ccitt(
	                crc_buffer,
	                8 + payload_length
	            );


	        /*
	         * --------------------------------------------------------
	         * PRINT PACKET INFORMATION
	         * --------------------------------------------------------
	         */

	        char msg[200];

	        snprintf(
	            msg,
	            sizeof(msg),
	            "\r\nPACKET ID: %lu\r\n"
	            "EXPECTED ID: %lu\r\n"
	            "SYNC: 0x%04X\r\n"
	            "PAYLOAD LENGTH: %u\r\n"
	            "RECEIVED CRC: 0x%04X\r\n"
	            "CALCULATED CRC: 0x%04X\r\n",
	            packet_id,
	            expected_packet_id,
	            sync,
	            payload_length,
	            received_crc,
	            calculated_crc
	        );

	        HAL_UART_Transmit(
	            &huart3,
	            (uint8_t *)msg,
	            strlen(msg),
	            HAL_MAX_DELAY
	        );


	        /*
	         * --------------------------------------------------------
	         * VALIDATE PACKET
	         * --------------------------------------------------------
	         */

	        if (sync != 0xAA55 ||
	            packet_id != expected_packet_id ||
	            received_crc != calculated_crc)
	        {
	            char error_msg[120];

	            int len = sprintf(
	                error_msg,
	                "PACKET %lu VALIDATION FAILED\r\n",
	                expected_packet_id
	            );

	            HAL_UART_Transmit(
	                &huart3,
	                (uint8_t *)error_msg,
	                len,
	                HAL_MAX_DELAY
	            );

	            transfer_ok = 0;

	            break;
	        }


	        /*
	         * --------------------------------------------------------
	         * CRC PASSED
	         * --------------------------------------------------------
	         */

	        char crc_ok_msg[80];

	        int crc_ok_len = sprintf(
	            crc_ok_msg,
	            "PACKET %lu CRC OK\r\n",
	            expected_packet_id
	        );

	        HAL_UART_Transmit(
	            &huart3,
	            (uint8_t *)crc_ok_msg,
	            crc_ok_len,
	            HAL_MAX_DELAY
	        );


	        /*
	         * --------------------------------------------------------
	         * WRITE PAYLOAD DIRECTLY TO microSD
	         *
	         * packet_buffer contains ONLY ONE packet.
	         *
	         * The complete image is NEVER stored in STM32 RAM.
	         *
	         * Only:
	         *
	         *     packet_buffer[10 ... 10+payload_length-1]
	         *
	         * is written to image.bin.
	         * --------------------------------------------------------
	         */

	        fres = f_write(
	            &USERFile,
	            &packet_buffer[10],
	            payload_length,
	            &bytes_written
	        );

	        if (fres != FR_OK ||
	            bytes_written != payload_length)
	        {
	            char write_msg[120];

	            int len = sprintf(
	                write_msg,
	                "PACKET %lu WRITE ERROR: result=%d bytes=%u\r\n",
	                expected_packet_id,
	                fres,
	                bytes_written
	            );

	            HAL_UART_Transmit(
	                &huart3,
	                (uint8_t *)write_msg,
	                len,
	                HAL_MAX_DELAY
	            );

	            transfer_ok = 0;

	            break;
	        }


	        /*
	         * --------------------------------------------------------
	         * WRITE SUCCESS
	         * --------------------------------------------------------
	         */

	        char write_ok_msg[100];

	        int write_ok_len = sprintf(
	            write_ok_msg,
	            "PACKET %lu WRITE OK: %u BYTES\r\n",
	            expected_packet_id,
	            bytes_written
	        );

	        HAL_UART_Transmit(
	            &huart3,
	            (uint8_t *)write_ok_msg,
	            write_ok_len,
	            HAL_MAX_DELAY
	        );
	    }


	    /*
	     * ------------------------------------------------------------
	     * CLOSE image.bin
	     * ------------------------------------------------------------
	     */

	    fres = f_close(&USERFile);

	    if (fres == FR_OK)
	    {
	        HAL_UART_Transmit(
	            &huart3,
	            (uint8_t *)"image.bin CLOSE OK\r\n",
	            strlen("image.bin CLOSE OK\r\n"),
	            HAL_MAX_DELAY
	        );
	    }
	    else
	    {
	        char close_msg[80];

	        int len = sprintf(
	            close_msg,
	            "image.bin CLOSE ERROR: %d\r\n",
	            fres
	        );

	        HAL_UART_Transmit(
	            &huart3,
	            (uint8_t *)close_msg,
	            len,
	            HAL_MAX_DELAY
	        );

	        transfer_ok = 0;
	    }


	    /*
	     * ------------------------------------------------------------
	     * FINAL RESULT
	     * ------------------------------------------------------------
	     */

	    if (transfer_ok)
	    {
	        HAL_UART_Transmit(
	            &huart3,
	            (uint8_t *)"PACKETS 0-9 TEST COMPLETE\r\n",
	            strlen("PACKETS 0-9 TEST COMPLETE\r\n"),
	            HAL_MAX_DELAY
	        );
	    }
	    else
	    {
	        HAL_UART_Transmit(
	            &huart3,
	            (uint8_t *)"PACKETS 0-9 TEST FAILED\r\n",
	            strlen("PACKETS 0-9 TEST FAILED\r\n"),
	            HAL_MAX_DELAY
	        );
	    }

	    break;
  }
}
  /* USER CODE END 3 */


/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Supply configuration update enable
  */
  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 10;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_3;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOMEDIUM;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV1;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 0x0;
  hspi1.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
  hspi1.Init.NSSPolarity = SPI_NSS_POLARITY_LOW;
  hspi1.Init.FifoThreshold = SPI_FIFO_THRESHOLD_01DATA;
  hspi1.Init.TxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
  hspi1.Init.RxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
  hspi1.Init.MasterSSIdleness = SPI_MASTER_SS_IDLENESS_00CYCLE;
  hspi1.Init.MasterInterDataIdleness = SPI_MASTER_INTERDATA_IDLENESS_00CYCLE;
  hspi1.Init.MasterReceiverAutoSusp = SPI_MASTER_RX_AUTOSUSP_DISABLE;
  hspi1.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_DISABLE;
  hspi1.Init.IOSwap = SPI_IO_SWAP_DISABLE;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief SPI2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI2_Init(void)
{

  /* USER CODE BEGIN SPI2_Init 0 */

  /* USER CODE END SPI2_Init 0 */

  /* USER CODE BEGIN SPI2_Init 1 */

  /* USER CODE END SPI2_Init 1 */
  /* SPI2 parameter configuration*/
  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_SLAVE;
  hspi2.Init.Direction = SPI_DIRECTION_2LINES;
  hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi2.Init.NSS = SPI_NSS_HARD_INPUT;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 0x0;
  hspi2.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
  hspi2.Init.NSSPolarity = SPI_NSS_POLARITY_LOW;
  hspi2.Init.FifoThreshold = SPI_FIFO_THRESHOLD_01DATA;
  hspi2.Init.TxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
  hspi2.Init.RxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
  hspi2.Init.MasterSSIdleness = SPI_MASTER_SS_IDLENESS_00CYCLE;
  hspi2.Init.MasterInterDataIdleness = SPI_MASTER_INTERDATA_IDLENESS_00CYCLE;
  hspi2.Init.MasterReceiverAutoSusp = SPI_MASTER_RX_AUTOSUSP_DISABLE;
  hspi2.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_DISABLE;
  hspi2.Init.IOSwap = SPI_IO_SWAP_DISABLE;
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI2_Init 2 */

  /* USER CODE END SPI2_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart2, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart2, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */

  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */

  /* USER CODE END USART3_Init 1 */
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  huart3.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart3.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart3.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart3, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart3, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */

  /* USER CODE END USART3_Init 2 */

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
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin : SD_CS_Pin */
  GPIO_InitStruct.Pin = SD_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(SD_CS_GPIO_Port, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
