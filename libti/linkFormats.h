#pragma once
#include <stdint.h>
#include <stdio.h>

/* https://merthsoft.com/linkguide/ti83+/fformat.html */

enum DeviceType{
    DEVICE_TI73 = 0x74,
    DEVICE_TI83p = 0x73,
    DEVICE_TI89 = 0x98,
    DEVICE_TI92 = 0x88
};

static void print_device_type(enum DeviceType type) {
    switch (type) {
        case DEVICE_TI73: printf("TI-73"); break;
        case DEVICE_TI83p: printf("TI-83+"); break;
        case DEVICE_TI89: printf("TI-89"); break;
        case DEVICE_TI92: printf("TI-92"); break;
        default: printf("Unknown(0x%02X)", type);
    }
}

enum DataType{
    OS = 0x23,
    APPLICATION = 0x24,
    CERTIFICATE =  0x25,
    LICENSE = 0x3E
};


static void print_data_type(enum DataType type) {
    switch (type) {
        case OS: printf("OS"); break;
        case APPLICATION: printf("Application"); break;
        case CERTIFICATE: printf("Certificate"); break;
        case LICENSE: printf("License"); break;
    	default: printf("Unknown(0x%02X)", type);
    }
}

enum TypeID{
    TYPE_RealNumber = 0x00,
    TYPE_RealList = 0x01,
    TYPE_Matrix = 0x02,
    TYPE_YVariable = 0x03,
    TYPE_String = 0x04,
    TYPE_Program = 0x05,
    TYPE_EditLockedProgram = 0x06,
    TYPE_Picture = 0x07,
    TYPE_GraphicsDatabase = 0x08,
    TYPE_WindowSettings = 0x0B,
    TYPE_ComplexNumber = 0x0C,
    TYPE_ComplexListNumber = 0x0D,
    TYPE_WindowSettings2 = 0x0F,
    TYPE_SavedWindowSettings = 0x10,
    TYPE_TableSetup = 0x11,
    TYPE_Backup = 0x13,
    TYPE_DeleteFlashApplication = 0x14,
    TYPE_ApplicationVariable = 0x15,
    TYPE_GroupVariable = 0x16,
    TYPE_Directory = 0x19,
    TYPE_OS = 0x23,
    TYPE_IdList = 0x26,
    TYPE_GetCertificate = 0x27,
    TYPE_Clock = 0x29
};

static void print_type_id(enum TypeID type) {
    switch (type) {
        case TYPE_RealNumber: printf("RealNumber"); break;
        case TYPE_RealList: printf("RealList"); break;
        case TYPE_Matrix: printf("Matrix"); break;
        case TYPE_YVariable: printf("YVariable"); break;
        case TYPE_String: printf("String"); break;
        case TYPE_Program: printf("Program"); break;
        case TYPE_EditLockedProgram: printf("EditLockedProgram"); break;
        case TYPE_Picture: printf("Picture"); break;
        case TYPE_GraphicsDatabase: printf("GraphicsDatabase"); break;
        case TYPE_WindowSettings: printf("WindowSettings"); break;
        case TYPE_ComplexNumber: printf("ComplexNumber"); break;
        case TYPE_ComplexListNumber: printf("ComplexListNumber"); break;
        case TYPE_WindowSettings2: printf("WindowSettings2"); break;
        case TYPE_SavedWindowSettings: printf("SavedWindowSettings"); break;
        case TYPE_TableSetup: printf("TableSetup"); break;
        case TYPE_Backup: printf("Backup"); break;
        case TYPE_DeleteFlashApplication: printf("DeleteFlashApplication"); break;
        case TYPE_ApplicationVariable: printf("ApplicationVariable"); break;
        case TYPE_GroupVariable: printf("GroupVariable"); break;
        case TYPE_Directory: printf("Directory"); break;
        case TYPE_OS: printf("OS"); break;
        case TYPE_IdList: printf("IdList"); break;
        case TYPE_GetCertificate: printf("GetCertificate"); break;
        case TYPE_Clock: printf("Clock"); break;
        default: printf("Unknown(0x%02X)", type);
    }
}

struct VariableEntry{
	/* Always has a value of 11 or 13 (Bh or Dh). */
	uint16_t format;
	uint16_t variableLength;

	enum TypeID type;

	/* Variable name, padded with NULL characters*/
	char name[8];

	/* The following feilds only exist if format=Dh */
	
	/* Typically zero */
	uint8_t version;
	
	/* Flag. Set to 80h if variable is archived, 00h else */
	uint8_t flags;

	/* End format specific */

	/* Size of variableLength */
	char data[];
};


static void print_variable_entry(struct VariableEntry* entry) {
    printf("Format: 0x%04X\n", entry->format);
    printf("Length: %u\n", entry->variableLength);
    printf("Type: "); print_type_id(entry->type);
    printf("\nName: %.8s\n", entry->name);
    
    if (entry->format == 0x0D) {
        printf("Version: 0x%02X\n", entry->version);
        printf("Flags: 0x%02X\n", entry->flags);
    }
}



struct VariableFile{
	/* Comment. The comment is either zero-terminated 
		or padded on the right with space characters. */
	char comment[0x2A];
	
	uint16_t checksum;
	uint16_t dataLength;
	/* consists of a number of variable entries */
	char data[];
};

static void print_variable_file(struct VariableFile* file) {
    printf("Comment: %.42s\n", file->comment);
    printf("Checksum: 0x%04X\n", file->checksum);
    printf("Data Length: %u\n", file->dataLength);
}



struct BCD_Date {
	uint8_t dd;
	uint8_t mm;
	uint8_t yy1;
	uint8_t yy2;
};

static void print_bcd_date(struct BCD_Date* date) {
    printf("%02x/%02x/%02x%02x", date->dd, date->mm, date->yy1, date->yy2);
}



struct FlashFile{
	/* BCD encoded revision numbers */
	uint8_t revMajor;
	uint8_t revMinor;

	/* Usally zero */
	uint8_t flags;
	/* Object type (00h) */
	uint8_t objectType;

	uint8_t nameLength;

	struct BCD_Date date;

	/* TODO: Support is limited, but longer names exist */
	char name[8];

	enum DeviceType deviceType;
	enum DataType dataType;

	uint16_t checksum;
	uint32_t dataLength;
	char intellHexData[];
};

static void print_flash_file(struct FlashFile* file) {
    printf("Revision: %x.%x\n", file->revMajor, file->revMinor);
    printf("Flags: 0x%02X\n", file->flags);
    printf("Object Type: 0x%02X\n", file->objectType);
    printf("Name Length: %u\n", file->nameLength);
    printf("Date: "); print_bcd_date(&file->date);
    printf("\nName: %.8s\n", file->name);
    printf("Device Type: "); print_device_type(file->deviceType);
    printf("\nData Type: "); print_data_type(file->dataType);
    printf("\nChecksum: 0x%04X\n", file->checksum);
    printf("Data Length: %u\n", file->dataLength);
}




TiError parse_flash(struct FlashFile** dst, char* src, size_t length);

/* Simple TI checksum alg. */
static uint16_t calculate_sum(const char* in, size_t length){
    uint16_t length16 = (uint16_t) length;
    uint16_t sum = 2*(length16 & 0xFF) + 2*(length16 >> 8);
    while(length--){
        sum += *in;
    }
    return sum;
}


/* TODO: 8xk sum via https://github.com/alberthdev/spasm-ng/blob/master/export.cpp */




