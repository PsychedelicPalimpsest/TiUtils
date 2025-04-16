#pragma once

/* https://merthsoft.com/linkguide/ti83+/fformat.html */


enum DeviceType{
    DEVICE_TI73 = 0x74,
    DEVICE_TI83p = 0x73,
    DEVICE_TI89 = 0x98,
    DEVICE_TI92 = 0x88
};
enum DataType{
    OS = 0x23,
    APPLICATION = 0x24,
    CERTIFICATE =  0x25,
    LICENSE = 0x3E
};
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


struct VariableFile{
	/* Comment. The comment is either zero-terminated 
		or padded on the right with space characters. */
	char comment[0x2A];
	
	uint16_t checksum;
	uint16_t dataLength;
	/* consists of a number of variable entries */
	char data[];
};

struct BCD_Date {
	uint8_t dd;
	uint8_t mm;
	uint8_t yy1;
	uint8_t yy2;
};


struct FlashFile{
	/* BCD encoded revision numbers */
	uint8_t revMajor;
	uint8_t revMinor;

	/* Usally zero */
	uint8_t flags;
	/* Object type (00h) */
	uint8_t objectType;

	uint8_t nameLength;

	/* TODO: Support is limited, but longer names exist */
	char name[8];

	enum DeviceType deviceType;
	enum DataType dataType;

	uint16_t checksum;
	uint32_t dataLength;
	char intellHexData[];
};


