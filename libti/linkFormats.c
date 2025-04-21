#include "libti.h"
#include "linkFormats.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include <endian.h>

#include "hex.h"


const char FLASH_MAGIC_NUMBER[8] = "**TIFL**";




TiError parse_flash(struct FlashFile** dst, char* src, size_t length){
	char text[256] = {0};

	if (length < 78){
		LIBTI_ERROR("Error: flash seems corrupt. Must be at least 78 bytes, found %d\n", length);
		return ERR_TI_LINK_FORMAT;
	}
	if (0 != memcmp(src, FLASH_MAGIC_NUMBER, sizeof(FLASH_MAGIC_NUMBER))){
		LIBTI_ERROR("Error: flash file expected, but the magic number does not match!");
		return ERR_TI_LINK_FORMAT;
	}

	uint32_t data_size = le32toh(*((uint32_t*)(src + 74)) );



	/* 10mb max. TI-OS is smaller than a meg, so this is a logical sanity check */
	if (data_size > 10000000llu){
		LIBTI_ERROR("Error: flash file seems to be quite large (%d bytes?), TiUtils refuses any files above 10 megabytes!\n", data_size);
		return ERR_TI_LINK_FORMAT;
	}

	/** 
	 * Calulate the size of the file based of data section
	 * 74 is the data size offset. 4 is the length of the size field.
	 */
	if (data_size+74+4 > length){
		LIBTI_ERROR("ERROR: data size section indicated that the file was truncated. The size of the file is %llu, and the calculated size was %d\n", length, data_size+74+4);
		return ERR_TI_LINK_FORMAT;
	}

	*dst = realloc(*dst, sizeof(struct FlashFile) + data_size);
	struct FlashFile* file = *dst;

	file->dataLength = data_size;

	file->revMajor = src[7];
	file->revMinor = src[8];
	file->flags = src[9];

	file->date.dd = src[12];
	file->date.mm = src[13];
	file->date.yy1 = src[14];
	file->date.yy2 = src[15];

	file->nameLength = src[16];

	/* TODO: Longer name support */
	memcpy(&file->name, src + 17, 8);

	file->deviceType = (enum DeviceType) src[48];
	file->dataType = (enum DataType) src[49];
	file->checksum = le16toh( *((uint16_t*)(src + 76 + data_size)) );

	memcpy(file->intellHexData, src + 78, data_size);


	return ERR_OK;
}