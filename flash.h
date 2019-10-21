/*
 * flash.h
 *
 *  Created on: Oct 17, 2019
 *      Author: Anton
 *
 *  This lib realize storage on internal flash memory.
 *  Lib use two pages - 0 and 1 for storing and backuping data
 *
 *  Every time data will be store in new page cycling. First time - page 0,  second page 1 next page 0
 *  If data was corrupted during write processes data will be restored from previous page (HAL_TIMEOUT ret value)
 *
 *  Depend from chip You will get 1k or 2k storage for internal variables.
 *  All data must be written during 1 operation (its relate to page status change procedure)
 *
 */

#ifndef FLASH_FLASH_H_
#define FLASH_FLASH_H_

#include "stm32f1xx_hal.h"

#define FLASH_RESERVE_N_PAGES    2  // Reserve N pages at the end of memory

//Page status
typedef enum  {
  page_clean       = ((uint32_t)0xFFFFFFFF),	// Page Erased and ready to program
  page_spgm       = ((uint32_t)0xFFFFEEEE),		// Program was started but still in progress
  page_valid       = ((uint32_t)0xFFFF0000),	// Data on page was programmed and correct. Ready for use
  page_outdate     = ((uint32_t)0x00000000),	// Data on page correct but outdate. Use for backup
} PageStatusTypeDef;

/**
  * @brief	Change page status. This function change status only from clean to outdate direction.
  *
  * @param	status new page status value
  *
  * @param	page	page number 0 or 1
  *
  * @retval	HAL_StatusTypeDef	HAL Status
  */
HAL_StatusTypeDef flash_set_page_status(PageStatusTypeDef status, uint8_t page);

/**
  * @brief	Read current page status.
  *
  * @param	page	page number 0 or 1
  *
  * @retval	PageStatusTypeDef	Page status
  */
PageStatusTypeDef flash_get_page_status(uint8_t page);

/**
  * @brief	Erase page. Page status automatically changed to page_clean
  *
  * @param	page	page number 0 or 1
  *
  * @retval	HAL_StatusTypeDef	HAL Status
  */
HAL_StatusTypeDef flash_erase_page(uint8_t page);

/**
  * @brief	Write buff to flash memory. MAX_SIZE is (FLASH_PAGE_SIZE - 2). At the start currant page status change to spgm and
  * 		previous page change to outdate.
  * 		In case of both page corrupted then pages will be erased and data written to page 0
  * 		If operation complete successfully current page status will be change to valid
  *
  * @param	buff	data as uint16_t array
  *
  * @param	address	virtual address of variable 0 translate to PAGEx_START_ADDRESS+2
  *
  * @param	length	data length (FLASH_PAGE_SIZE-2) max
  *
  * @retval	HAL_StatusTypeDef	HAL Status
  */
HAL_StatusTypeDef flash_write_bufer(uint16_t* buff, uint32_t address, uint32_t length);

/**
  * @brief	Try to read data from Flash memory. If page valid - data will be read and return HAL_OK
  * 		If data was corrupted function will read data from previous page and return HAL_TIMEOUT
  * 		If data corrupted on both page - function will return HAL_ERROR data will not be read
  *
  * @param	buff	buffer as uint16_t array
  *
  * @param	address	virtual address of variable 0 translate to PAGEx_START_ADDRESS+2
  *
  * @param	length	data length (FLASH_PAGE_SIZE-2) max
  *
  * @retval	HAL_StatusTypeDef	HAL Status
  */
HAL_StatusTypeDef flash_read_bufer(uint16_t* buff, uint32_t address, uint32_t length);

/**
  * @brief	Calculate and setup internal library variables such as PAGEx_START_ADDRESS
  *
  */
void flash_init();

#endif /* FLASH_FLASH_H_ */
