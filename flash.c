/*
 * flash.c
 *
 *  Created on: Oct 17, 2019
 *      Author: Anton
 */


#include "flash.h"

#ifdef STM32F10X_XL
  #define PAGE_BANK		   FLASH_BANK_2
#else
  #define PAGE_BANK		   FLASH_BANK_1
#endif


#define PAGE_HEADER_SIZE   sizeof(PageStatusTypeDef)

volatile uint32_t PAGE0_START_ADDRESS = 0x00;
volatile uint32_t PAGE1_START_ADDRESS = 0x00;

void flash_init()
{
	 uint32_t flash_end = FLASH_BASE + (*(__IO uint16_t*) (FLASHSIZE_BASE)) * 1024; // get End flash address
	 PAGE0_START_ADDRESS = (flash_end - FLASH_RESERVE_N_PAGES*FLASH_PAGE_SIZE - 2*FLASH_PAGE_SIZE);
	 PAGE1_START_ADDRESS = (flash_end - FLASH_RESERVE_N_PAGES*FLASH_PAGE_SIZE -   FLASH_PAGE_SIZE);
}

_Bool flash_ready(void)
{
	return !(FLASH->SR & FLASH_SR_BSY);
}


HAL_StatusTypeDef erase_page(uint32_t adress)
{
	HAL_StatusTypeDef res = HAL_OK;
	uint32_t error;
	FLASH_EraseInitTypeDef EraseInit;
	EraseInit.Banks = PAGE_BANK;
	EraseInit.NbPages = 1;
	EraseInit.PageAddress = adress;
	EraseInit.TypeErase = FLASH_TYPEERASE_PAGES;
	if (flash_ready()){
		res += HAL_FLASH_Unlock();
		if (HAL_OK == res)
			res += HAL_FLASHEx_Erase(&EraseInit, &error);
		if (HAL_OK == res)
			res += HAL_FLASH_Lock();
		else
			HAL_FLASH_Lock();
		return res;
	}
	else return HAL_BUSY;
}


HAL_StatusTypeDef write_page_status(PageStatusTypeDef status, uint32_t address)
{
	HAL_StatusTypeDef res = HAL_OK;
	if (flash_ready()){
		res += HAL_FLASH_Unlock();
		if (HAL_OK == res)
			res += HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, address, status);
		if (HAL_OK == res)
			res += HAL_FLASH_Lock();
		else
			HAL_FLASH_Lock();
		return res;
	}
	else return HAL_BUSY;
}

HAL_StatusTypeDef flash_set_page_status(PageStatusTypeDef status, uint8_t page)
{
	if (0 == page)
		return write_page_status(status, PAGE0_START_ADDRESS);
	else
		return write_page_status(status, PAGE1_START_ADDRESS);
}

PageStatusTypeDef flash_get_page_status(uint8_t page)
{
	if (0 == page)
		return (*(__IO uint32_t*) PAGE0_START_ADDRESS);
	else
		return (*(__IO uint32_t*) PAGE1_START_ADDRESS);
}

HAL_StatusTypeDef flash_erase_page(uint8_t page)
{
	if (0 == page)
		return erase_page(PAGE0_START_ADDRESS);
	else
		return erase_page(PAGE1_START_ADDRESS);
}

HAL_StatusTypeDef write_bufer(uint16_t* buff, uint8_t page, uint32_t address, uint32_t length)
{
	HAL_StatusTypeDef res = HAL_OK;
	uint32_t page_address;
	if (0 == page)
		page_address = PAGE0_START_ADDRESS;
	else
		page_address = PAGE1_START_ADDRESS;

	if (flash_ready())
	{
		res += HAL_FLASH_Unlock();
		if (HAL_OK == res)
		{
			for(uint32_t i=0; i<length; i++)
				res += HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, page_address+PAGE_HEADER_SIZE+address+i*2U, buff[i]);
		}
		if (HAL_OK == res)
			res += HAL_FLASH_Lock();
		else
			HAL_FLASH_Lock();
	}
	return res;
}

HAL_StatusTypeDef flash_write_bufer(uint16_t* buff, uint32_t address, uint32_t length)
{
	HAL_StatusTypeDef res = HAL_OK;
	// Both pages are clean - write to page 0
	if (page_clean == flash_get_page_status(0) && page_clean == flash_get_page_status(1))
	{
		res += flash_set_page_status(page_spgm, 0);
		if (HAL_OK == res)
			res += write_bufer(buff, 0, address, length);
		if (HAL_OK == res)
			res += flash_set_page_status(pageVALID, 0);

	}
	// Page 0 VALID, page 1 - target
	else if (pageVALID == flash_get_page_status(0) && pageVALID != flash_get_page_status(1))
	{
		if (page_clean != flash_get_page_status(1)) //if page 1 not clean
		{
			res += flash_erase_page(1); //erase it
			if (HAL_OK == res)
				res += flash_set_page_status(page_spgm, 1); //programm it
			if (HAL_OK == res)
				res += write_bufer(buff, 1, address, length);
			if (HAL_OK == res)
				res += flash_set_page_status(page_outdate, 0);
			if (HAL_OK == res)
				res += flash_set_page_status(pageVALID, 1);
		}
		else										//if page 1 clean then just programm it
		{
			res += flash_set_page_status(page_spgm, 1);
			if (HAL_OK == res)
				res += write_bufer(buff, 1, address, length);
			if (HAL_OK == res)
				res += flash_set_page_status(page_outdate, 0);
			if (HAL_OK == res)
				res += flash_set_page_status(pageVALID, 1);
		}
	}
	// Page 1 VALID, page 0 - target
	else if (pageVALID == flash_get_page_status(1) && pageVALID != flash_get_page_status(0))
		{
			if (page_clean != flash_get_page_status(0)) // if page 0 not clean
			{
				res += flash_erase_page(0);	//erase it
				if (HAL_OK == res)
					res += flash_set_page_status(page_spgm, 0); // programm it
				if (HAL_OK == res)
					res += write_bufer(buff, 0, address, length);
				if (HAL_OK == res)
					res += flash_set_page_status(page_outdate, 1);
				if (HAL_OK == res)
					res += flash_set_page_status(pageVALID, 0);
			}
			else								// if page 0 clean then just programm it
			{
				res += flash_set_page_status(page_spgm, 0);
				if (HAL_OK == res)
					res += write_bufer(buff, 0, address, length);
				if (HAL_OK == res)
					res += flash_set_page_status(page_outdate, 1);
				if (HAL_OK == res)
					res += flash_set_page_status(pageVALID, 0);
			}
		}
	// Both page are not VALID - ALRAM ALRAM - some things goes wrong - ERASE ALL
	// Write data to page0
	else if (pageVALID != flash_get_page_status(1) && pageVALID != flash_get_page_status(0))
		{
			res += flash_erase_page(0);
			res += flash_erase_page(1);
			if (HAL_OK == res)
				res += flash_set_page_status(page_spgm, 0);
			if (HAL_OK == res)
				res += write_bufer(buff, 0, address, length);
			if (HAL_OK == res)
				res += flash_set_page_status(pageVALID, 0);
		}

	return res;
}


HAL_StatusTypeDef flash_read_bufer(uint16_t* buff, uint32_t address, uint32_t length)
{
	HAL_StatusTypeDef res = HAL_OK;
	// page 0 VALID or page 0 INVALID and page 1 broken - target page 0
	if (pageVALID == flash_get_page_status(0) || (page_outdate == flash_get_page_status(0) && pageVALID != flash_get_page_status(1)))
	{
		for(uint32_t i=0; i<length; i++)
			buff[i] = (*(__IO uint32_t*) (PAGE0_START_ADDRESS+PAGE_HEADER_SIZE+address+i*2U));
		if (page_outdate == flash_get_page_status(0) && pageVALID != flash_get_page_status(1))
			res = HAL_TIMEOUT;
		return res;
	}
	// page 1 VALID or page 1 INVALID and page 0 broken - target page 1
	else if (pageVALID == flash_get_page_status(1) || (page_outdate == flash_get_page_status(1) && pageVALID != flash_get_page_status(0)))
	{
		for(uint32_t i=0; i<length; i++)
			buff[i] = (*(__IO uint32_t*) (PAGE1_START_ADDRESS+PAGE_HEADER_SIZE+address+i*2U));
		if (page_outdate == flash_get_page_status(1) && pageVALID != flash_get_page_status(0))
			res = HAL_TIMEOUT;
		return res;
	}
	// both page broken
	else
		res = HAL_ERROR;
	return res;
}

