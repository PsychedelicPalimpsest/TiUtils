#pragma once
#include <stddef.h>
#include <stdint.h>
#include "libti.h"


/*
 * Terminology: For TiHex, and only TiHex, the terms 'Page' and 'Part'
 *              take on special meanings. A 'Part' contains multiple
 *              'Pages'. These 'Pages' typically correspond to actual
 *              pages on the OS/App, but not always.
 *
 *              For example, Apps only have 1 'Part', with the potential
 *              for multiple pages.
 *
 *              Whereas an OS has 3 'Parts', the first being some header
 *              data, the second being the actual OS, and the third being
 *              a signature.
 */


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