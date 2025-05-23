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
	if (length < 78){
		LIBTI_ERROR("Error: flash seems corrupt. Must be at least 78 bytes, found %llu\n", length);
		return ERR_TI_LINK_FORMAT;
	}
	if (0 != memcmp(src, FLASH_MAGIC_NUMBER, sizeof(FLASH_MAGIC_NUMBER))){
		LIBTI_ERROR("Error: flash file expected, but the magic number does not match!");
		return ERR_TI_LINK_FORMAT;
	}

	uint32_t data_size = le32toh(*(uint32_t *) (src + 74));



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

	file->revMajor = src[8];
	file->revMinor = src[9];
	file->flags = src[10];
    file->objectType = src[11];

	file->date.dd = src[12];
	file->date.mm = src[13];
	file->date.yy1 = src[14];
	file->date.yy2 = src[15];

	file->nameLength = src[16];

	/* TODO: Longer name support */
	memcpy(&file->name, src + 17, 8);

	file->deviceType = (enum DeviceType) src[48];
	file->dataType = (enum DataType) src[49];
	// file->checksum = le16toh( *(uint16_t *) (src + 76 + data_size));

	memcpy(file->intellHexData, src + 78, data_size);


	return ERR_OK;
}

TiError write_flash(char** dst, struct FlashFile* src, size_t* delta) {
    int size = src->dataLength + 78;

    *dst = realloc( *dst, size );
    char* to = *dst;

    /* A lot of 'filler' sections need null (NOTE: Binary compatibility might challenge this) */
    memset(to, 0, 78);

    memcpy(to, FLASH_MAGIC_NUMBER, sizeof(FLASH_MAGIC_NUMBER));
    to[8] = src->revMajor;
    to[9] = src->revMinor;
    to[10] = src->flags;
    to[11] = src->objectType;

    to[12] = src->date.dd;
    to[13] = src->date.mm;
    to[14] = src->date.yy1;
    to[15] = src->date.yy2;

    to[16] = src->nameLength;
    memcpy(to + 17, &src->name, 8);

    to[48] = (char) src->deviceType;
    to[49] = (char) src->dataType;


    *(uint32_t *) (to + 74) = htole32(src->dataLength);
    memcpy(to + 78, src->intellHexData, src->dataLength);

    // *(uint16_t *) (to + 78 + src->dataLength) = htole16(
    //     calculate_var_sum(src->intellHexData, src->dataLength)
    // );
    *delta = 78 + src->dataLength /* + 2 */;

    return ERR_OK;
}

static void cJSON_AddHexToObject(cJSON* dst, const char* name, unsigned char num) {
	char out[2 + 2 + 1] = {0};
	snprintf(out, sizeof(out), "0x%02X", num);

	cJSON_AddStringToObject(dst, name, out);
}
static TiError cJSON_GetHexFromObject(cJSON* json, const char* name, uint8_t* out) {
    json = cJSON_GetObjectItem(json, name);
    if (!cJSON_IsString(json))
        goto ERR;

	char* str = json->valuestring;
	if (!str || 0 != strncmp(str, "0x", 2) || strlen(str) != 4)
        goto ERR;

	hex2bin((char*) out, str + 2, 2);
	return ERR_OK;

    ERR:
    LIBTI_ERROR("Error: '%s' must be a string, starting with 0x, be valid hex, and a MAXIMUM of 2 digits!\n", name)
    return ERR_JSON_FORMAT_ERROR;
}



static const char* BCD_DATE_FORMAT = "%02x/%02x/%02x%02x";

static void cJSON_AddBCDDateToObject(cJSON* dst, const char* name, struct BCD_Date* date) {
	char out[2 + 1 + 2 + 1 + 4 + 1] = {0};
	snprintf(out, sizeof(out), BCD_DATE_FORMAT, date->dd, date->mm, date->yy1, date->yy2);

	cJSON_AddStringToObject(dst, name, out);
}
static TiError cJSON_GetBCDDateFromObject(cJSON* json, const char* name, struct BCD_Date* date) {
	char* str = cJSON_GetObjectItem(json, name)->valuestring;
	if (4 != sscanf(str, BCD_DATE_FORMAT, &date->dd, &date->mm, &date->yy1, &date->yy2))
		return ERR_JSON_FORMAT_ERROR;

	return ERR_OK;
}


#define handleReadHexJsonField(JSON_OBJ, FIELD, NAME, DEFAULT) {                       \
		if (!(JSON_OBJ) || !cJSON_HasObjectItem((JSON_OBJ), (NAME))){                  \
			(FIELD) = (DEFAULT);	                                                   \
		} else {                                                                       \
			if (cJSON_GetHexFromObject((JSON_OBJ), (NAME), (unsigned char*)&(FIELD)))  \
				return ERR_JSON_FORMAT_ERROR;                                          \
		}                                                                              \
	} while(0);
#define handleReadIntJsonField(JSON_OBJ, FIELD, NAME, DEFAULT) {                       \
		if (!(JSON_OBJ) || !cJSON_HasObjectItem((JSON_OBJ), (NAME))){                  \
			(FIELD) = (DEFAULT);	                                                   \
		} else {                                                                       \
			cJSON* _obj = cJSON_GetObjectItem((JSON_OBJ), (NAME));                     \
			if (!cJSON_IsNumber(_obj)){                                                \
                LIBTI_ERROR("Error: '%s' must be a valid integer!", (NAME));           \
                return ERR_JSON_FORMAT_ERROR;                                          \
			}                                                                          \
			(FIELD) = _obj->valueint;                                                  \
		}                                                                              \
	} while(0);

TiError flash_file_to_json(cJSON** dst, struct FlashFile* src) {
	if (!*dst) *dst = cJSON_CreateObject();
	cJSON* root = *dst;

	char rev[6] = {0};
	char name[9] = {0};




	snprintf(rev, sizeof(rev), "%02X.%02X", src->revMajor, src->revMinor);
	snprintf(name, sizeof(name), "%.8s", src->name);


	cJSON_AddStringToObject(root, "__FORMAT", "See here for more: https://merthsoft.com/linkguide/ti83+/fformat.html");

	cJSON_AddStringToObject(root, "Revision", rev);
	cJSON_AddHexToObject(root, "Flags", src->flags);
	cJSON_AddHexToObject(root, "Object type", src->objectType);
	cJSON_AddHexToObject(root, "Device type", src->deviceType);
	cJSON_AddHexToObject(root, "Data type", src->dataType);
	cJSON_AddStringToObject(root, "Name", name);
	cJSON_AddNumberToObject(root, "Name length", src->nameLength);
	cJSON_AddBCDDateToObject(root, "Date", &src->date);


	if (src->dataLength > sizeof(TI_FOOTER)) {
		/* TI OSes typically have a specific footer at the end */
		if (0 == strcmp(src->intellHexData + src->dataLength - sizeof(TI_FOOTER) + 1, TI_FOOTER))
			cJSON_AddTrueToObject(root, "Force Convert Footer");
	}


	return ERR_OK;
}

TiError flash_file_from_json(struct FlashFile* dst, cJSON* src) {
	if (!src || !cJSON_HasObjectItem(src, "Date"))
		dst->date = (struct BCD_Date) {0, 0, 0x20, 00};
	else if (cJSON_GetBCDDateFromObject(src, "Date", &dst->date))
			return ERR_JSON_FORMAT_ERROR;
    if (!src || !cJSON_HasObjectItem(src, "Revision"))
        dst->revMajor = dst->revMinor = 0;
    else {
        cJSON* r = cJSON_GetObjectItem(src, "Revision");
        if (!cJSON_IsString(r) || 2 != sscanf(r->valuestring, "%02X.%02X", &dst->revMajor, &dst->revMinor)) {
            LIBTI_ERROR("Error: 'Revision' is not valid! Must be a string, with two period seperated BCD digits. Ex: '2A.01'\n");
            return ERR_JSON_FORMAT_ERROR;
        }
    }
    if (!src || !cJSON_HasObjectItem(src, "Name")) {
        memcpy(dst->name, "UNNAMED\0", 8);
    }
    else {
        char* name = cJSON_GetObjectItem(src, "Name")->valuestring;
        size_t len = strlen(name);
        if (len > sizeof(dst->name)) {
            LIBTI_ERROR("Warning: variable names cannot be longer than 8 chars!");
            len = 8;
        }
        memset(dst->name, 0, 8);
        memcpy(dst->name, name, len);
    }

    handleReadHexJsonField(src, dst->flags, "Flags", 0);
    handleReadHexJsonField(src, dst->objectType, "Object type", 0);
    handleReadHexJsonField(src, dst->deviceType, "Device type", DEVICE_TI83p);
    handleReadHexJsonField(src, dst->dataType, "Data type", APPLICATION);
    handleReadIntJsonField(src, dst->nameLength, "Name length", 8);


	return ERR_OK;
}

const char VAR_MAGIC_NUMBER[8 + 3] = "**TI83F*\x1A\x0A\0";

TiError parse_variable(struct VariableFile** dst, char* src, size_t length) {
	if (length < 8 + 3 + 42 + 2 + 2) {
		LIBTI_ERROR("Error: Variable file MUST be at least 57 bytes, found %llu!\n", length)
		return ERR_TI_LINK_FORMAT;
	}
	if (0 != memcmp(src, VAR_MAGIC_NUMBER, sizeof(VAR_MAGIC_NUMBER))) {
		LIBTI_ERROR("Error: Variable file expected, magic number does not match!");
		return ERR_TI_LINK_FORMAT;
	}

	uint16_t data_size = le16toh(*(uint16_t*)(src + 53));
	uint16_t check_sum = le16toh(*(uint16_t*)(src + data_size + 55));

	*dst = (struct VariableFile*) realloc(*dst, sizeof(struct VariableFile) + data_size);

	struct VariableFile* variable = *dst;

	memcpy(variable->comment, src + 11, 42);
	variable->dataLength = data_size;
	variable->checksum = check_sum;

	memcpy(variable->data, src + 55, data_size);

	uint16_t sum = calculate_var_sum(src + 55, data_size);
	if (check_sum != sum) {
		LIBTI_ERROR("Error: Variable file checksum does not match! Expected %x, calculated %x\n", check_sum, sum);
		return ERR_TI_LINK_FORMAT;
	}

	return ERR_OK;
}

TiError write_variable(char** dst, struct VariableFile* src, size_t* delta) {
	const size_t size = src->dataLength + sizeof(VAR_MAGIC_NUMBER) + 42 + 2 + 2;
	*dst = (char*) realloc(*dst, size);

	char* to = *dst;
	memcpy(to, VAR_MAGIC_NUMBER, sizeof(VAR_MAGIC_NUMBER));
	memcpy(to + sizeof(VAR_MAGIC_NUMBER), src->comment, 42);

	*(uint16_t*) &to[sizeof(VAR_MAGIC_NUMBER) + 42] = src->dataLength;
	memcpy(to + sizeof(VAR_MAGIC_NUMBER) + 42 + 2, src->data, src->dataLength);

	*(uint16_t*) &to[size-2] = htole16(calculate_var_sum(src->data, src->dataLength));

	*delta = size;
	return ERR_OK;
}


TiError parse_variable_entry(struct VariableEntry** dst, char* src, size_t length, size_t* delta) {
	if (length < 2 + 2 + 1 + 8 + 2) {
		LIBTI_ERROR("Error: A variable MUST be at least 15 bytes, found %llu!\n", length)
		return ERR_TI_LINK_FORMAT;
	}

	uint16_t data_size = le16toh(*(uint16_t*)(src + 15));
	*dst = (struct VariableEntry*) realloc(*dst, sizeof(struct VariableEntry) + data_size);

	struct VariableEntry* entry = *dst;


	entry->format = le16toh(*(uint16_t*)(src));
	entry->type = src[4];
	memcpy(entry->name, src + 5, 8);
	entry->variableLength = data_size;

	if (entry->format != 11 && entry->format != 13) {
		LIBTI_ERROR("Error: Var format is incorrect");
		return ERR_TI_LINK_FORMAT;
	}


	if (entry->format == 13) {
		entry->version = src[13];
		entry->flags = src[14];
	}

	memcpy(entry->data, src + entry->format + 2 + 2, data_size);
	*delta = entry->format + 2 + 2 + data_size;

	return ERR_OK;
}

TiError write_variable_entry(char** dst, struct VariableEntry* src, size_t* delta) {
	fprintf(stderr, "\nFormat; %d\n", src->format);
	if (src->format != 11 && src->format != 13) {
		LIBTI_ERROR("Error: Var 'format' is incorrect");
		return ERR_TI_LINK_FORMAT;
	}

	/* Metadata length + u16 + u16 + data */
	*delta = src->format + 2 + 2 + src->variableLength;
	*dst = realloc(*dst, *delta);

	char* to = *dst;
	*(uint16_t*) &to[0] = htole16(src->format);
	*(uint16_t*) &to[2] = htole16(src->variableLength);
	to[4] = src->type;
	memcpy(to + 5, src->name, 8);



	if (src->format == 13) {
		to[13] = src->version;
		to[14] = src->flags;
	}

	*(uint16_t*) &to[src->format + 2] = htole16(src->variableLength);
	memcpy(to + src->format + 2 + 2, src->data, src->variableLength);
	return ERR_OK;
}


TiError variable_file_to_json(cJSON** dst, struct VariableFile* src) {
	if (!*dst) *dst = cJSON_CreateObject();
	cJSON* root = *dst;
	char comment[43] = {0};
	memcpy(comment, src->comment, 42);

	if (!cJSON_HasObjectItem(root, "__FORMAT"))
		cJSON_AddStringToObject(root, "__FORMAT", "See here for more: https://merthsoft.com/linkguide/ti83+/fformat.html");

	cJSON_AddStringToObject(root, "Comment", comment);

	return ERR_OK;
}

TiError variable_from_json(struct VariableFile* dst, cJSON* src) {
	memset(dst->comment, 0, sizeof(dst->comment));
	if (!dst || !cJSON_HasObjectItem(src, "Comment")) {
		/* No comment is the default if no extra json is provided */
	} else {
		char* comment = cJSON_GetObjectItem(src, "Comment")->valuestring;
		size_t len = strlen(comment);
		if (len > sizeof(dst->comment)) {
			LIBTI_ERROR("Warning: the comment specified in the extra json is too long, so will be truncated");
			len = 42;
		}
		memcpy(dst->comment, comment, len);
	}



	return ERR_OK;
}


TiError variable_entry_to_json(cJSON** dst, struct VariableEntry* src) {
	if (!*dst) *dst = cJSON_CreateObject();
	cJSON* root = *dst;

	char name[9] = {0};
	memcpy(name, src->name, 8);

	if (!cJSON_HasObjectItem(root, "__FORMAT"))
		cJSON_AddStringToObject(root, "__FORMAT", "See here for more: https://merthsoft.com/linkguide/ti83+/fformat.html");

	cJSON_AddNumberToObject(root, "Metadata format", src->format);
	cJSON_AddStringToObject(root, "Name", name);
	cJSON_AddHexToObject(root, "Type", (unsigned char) src->type);
	if (src->format == 13) {
		cJSON_AddHexToObject(root, "Flags", src->flags);
		cJSON_AddHexToObject(root, "Version", src->version);
	}

	return ERR_OK;
}

TiError variable_entry_from_json(struct VariableEntry* dst, cJSON* src) {
	if (!src || !cJSON_HasObjectItem(src, "Metadata format"))
		dst->format = 11;
	else {
		uint16_t v = (uint16_t) cJSON_GetObjectItem(src, "Metadata format")->valueint;
		if (v != 11 && v != 13) {
			LIBTI_ERROR("Error: 'Metadata format' MUST be 11 or 13\n");
			return ERR_JSON_FORMAT_ERROR;
		}
		dst->format = v;
	}

	if (!src || !cJSON_HasObjectItem(src, "Name")) {
		memcpy(dst->name, "UNNAMED\0", 8);
	} else {
		char* name = cJSON_GetObjectItem(src, "Name")->valuestring;
		size_t len = strlen(name);
		if (len > sizeof(dst->name)) {
			LIBTI_ERROR("Warning: variable names cannot be longer than 8 chars!");
			len = 8;
		}
		memset(dst->name, 0, 8);
		memcpy(dst->name, name, len);
	}

	handleReadHexJsonField(src, dst->type, "Type", TYPE_Program);

	/* These might not even be used depending on Metadata format */
	handleReadHexJsonField(src, dst->flags, "Flags", 0);
	handleReadHexJsonField(src, dst->version, "Version", 0);

	



	return ERR_OK;
}
