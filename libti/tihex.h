#pragma once
#include <stddef.h>
#include <stdint.h>
#include "libti.h"

struct IH_Part{
	struct IH_Part* next;
	struct IH_Page* first_page;
};

struct IH_Page{
	uint16_t startAddress;

	/* This is where page information is stashed */
	uint16_t extAddress;

	/* First part of an OS is lacking page info, do to only being metadata */
	char hasExtAddress;

	struct IH_Page* next;

	size_t dataLength;
	char data[];
};

TiError decode_page_data(struct IH_Part* dst, char* src, size_t length, char expectedPageCount);

TiError encode_page_data(char** dst, struct IH_Part* src, char lineLength);