#include "libti.h"
#include "linkFormats.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include <endian.h>
#include "cJSON.h"
#include "hex.h"


const char FLASH_MAGIC_NUMBER[8] = "**TIFL**";

/* Not sure why TI adds this, but to be binary identical, we use it */
const char TI_FOOTER[24] = "   -- CONVERT 2.6 --\r\n\x1a";




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


static void cJSON_AddhexToObject(cJSON* dst, const char* name, unsigned char num) {
	char out[2 + 2 + 1] = {0};
	snprintf(out, sizeof(out), "0x%02X", num);

	cJSON_AddStringToObject(dst, name, out);
}
static void cJSON_AddBCDDateToObject(cJSON* dst, const char* name, struct BCD_Date* date) {
	char out[2 + 1 + 2 + 1 + 4 + 1] = {0};
	snprintf(out, sizeof(out), "%02x/%02x/%02x%02x", date->dd, date->mm, date->yy1, date->yy2);

	cJSON_AddStringToObject(dst, name, out);
}




TiError flash_file_to_json(cJSON** dst, struct FlashFile* src) {

	if (!*dst) cJSON_Delete(*dst);
	*dst = cJSON_CreateObject();
	cJSON* root = *dst;

	char rev[6] = {0};
	char name[9] = {0};




	snprintf(rev, sizeof(rev), "%02X.%02X", src->revMajor, src->revMinor);
	snprintf(name, sizeof(name), "%.8s", src->name);


	cJSON_AddStringToObject(root, "__FORMAT", "See here for more: https://merthsoft.com/linkguide/ti83+/fformat.html");

	cJSON_AddStringToObject(root, "Revision", rev);
	cJSON_AddhexToObject(root, "Flags", src->flags);
	cJSON_AddhexToObject(root, "Object type", src->objectType);
	cJSON_AddhexToObject(root, "Device type", src->deviceType);
	cJSON_AddhexToObject(root, "Data type", src->dataType);
	cJSON_AddStringToObject(root, "Name", name);
	cJSON_AddNumberToObject(root, "Name length", src->nameLength);
	cJSON_AddBCDDateToObject(root, "Date", &src->date);


	if (src->dataLength > sizeof(TI_FOOTER)) {
		/* TI OSes typically have a specific footer at the end */
		if (0 == strcmp(src->intellHexData + src->dataLength - sizeof(TI_FOOTER)+1, TI_FOOTER))
			cJSON_AddTrueToObject(root, "Force Convert Footer");
	}


	return ERR_OK;
}


