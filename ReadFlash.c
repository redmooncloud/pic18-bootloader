#include "flash.h"
#undef _OMNI_CODE_
#if defined (FLASH_V1_1) || defined (FLASH_V1_2) || defined (FLASH_V1_3) || defined (FLASH_V1_4) \
	|| defined (FLASH_V1_5) || defined (FLASH_V1_6) || defined (FLASH_V2_1) || defined (FLASH_V3_1)

 /*********************************************************************
 Function:        	void ReadFlash(unsigned long startaddr, unsigned int num_bytes, unsigned char *flash_array)

 PreCondition:    	None
                  
 Input:           	startaddr - Strating address from which flash has to be read
			num_bytes - Number of bytes of flash to be read
			*flash_array - Pointer to array to which the flash has be read
 
 Output:          	Reads the flash content to array passed as pointer
 
 Side Effects:    	None
 
 Overview:        	The function reads flash for number of bytes passed as parameter from starting address 
                   
 Note:            	Non zero number of bytes has to be passed as parameter for num_bytes
 ********************************************************************/
void ReadFlash(unsigned long startaddr, unsigned int num_bytes, unsigned char *flash_array)
{
	DWORD_VAL flash_addr;
#ifdef _OMNI_CODE_
	DWORD_VAL tblptr_save;
#endif

	flash_addr.Val = startaddr;
#ifdef _OMNI_CODE_
	// save the current tblptr
	tblptr_save.byte.UB = TBLPTRU;
	tblptr_save.byte.HB = TBLPTRH;
	tblptr_save.byte.LB = TBLPTRL;
#endif
	TBLPTRU = flash_addr.byte.UB; //Load the address to Address pointer registers
	TBLPTRH = flash_addr.byte.HB;	
	TBLPTRL	= flash_addr.byte.LB;

	while(num_bytes--)
	{
		//*********** Table read sequence ******************************
#ifndef _OMNI_CODE_
		//_asm	TBLRDPOSTINC _endasm
        asm("TBLRDPOSTINC");
#else
		asm("TBLRDPOSTINC");
#endif
		*flash_array++ = TABLAT;
	}	
#ifdef _OMNI_CODE_
	// restore the tblptr
	TBLPTRU = tblptr_save.byte.UB;
	TBLPTRH = tblptr_save.byte.HB;	
	TBLPTRL	= tblptr_save.byte.LB;
#endif
}


#endif

