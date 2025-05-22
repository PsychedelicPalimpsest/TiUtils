#include "tihex.h"
#include "libti.h"
#include "hex.h"

#include <stdlib.h>
#include <endian.h>

/*
 * Mostly just https://en.wikipedia.org/wiki/Intel_HEX, but the TI-84 has
 * multiple "parts" and "pages" that are all stuck together.
 */


TiError decode_page_data(struct IH_Part* dst, char* src, size_t length, char expectedPageCount){
	char* extent = src + length;
	struct IH_Part* current_part = dst;
	struct IH_Part* last_part = NULL;
	char was_last_eof = 0;
	char has_set_addr = 0;
	
	struct IH_Page** page_loc;
	struct IH_Page* page;
	char has_set_extaddr = 0;
	size_t realloc_size = 2048;

	SETUP_PAGE:
	current_part->next = NULL;
	current_part->first_page = calloc(sizeof(struct IH_Page) + realloc_size, 1);

	page_loc = &current_part->first_page;
	page = *page_loc;



	while (extent > src && expectedPageCount){
		/* Seek to a colon */
		while(extent > src && *src != ':') src++;
		if (src + 9 >= extent) break;
		was_last_eof = 0;

		/* Skip colon */
		src++;

		uint8_t byte_count;
		uint16_t address;
		uint8_t record_type;
		unsigned char expected_sum;
		unsigned char found_sum;

		/* Load in first three value */
		hex2bin((char*) &byte_count, src, 2);
		src+=2;
		hex2bin((char*) &address,  src, 4);
		src+=4;
		address = be16toh(address);

		hex2bin((char*) &record_type, src, 2);
		src+=2;

		expected_sum = byte_count;
		expected_sum += address & 0xFF;
		expected_sum += address >> 8;
		expected_sum += record_type;

		switch (record_type) {
			/* Data */
			case 0x0:
				while (realloc_size <= page->dataLength + byte_count) {
					realloc_size *= 2;
					*page_loc = realloc(*page_loc, sizeof(struct IH_Page) + realloc_size);
				}
				page = *page_loc;

				if (!has_set_addr) {
					has_set_addr = 1;
					page->startAddress = address;
				}


				hex2bin(page->data + page->dataLength, src, byte_count*2);
				src += byte_count*2;


				for (int i = 0; i < byte_count; i++){
					expected_sum += page->data[page->dataLength + i];
				}

				hex2bin((char*) &found_sum, src, 2);
				src += 2;
				expected_sum = 0 - expected_sum;

				if (found_sum != expected_sum){
					LIBTI_ERROR("ERROR: ti hex (intel hex) checksum failure! Expected %2X found %2X with record type %2X\n", expected_sum, found_sum, record_type);
					return ERR_TI_HEX_FORMAT;
				}


				page->dataLength += byte_count;
				break;

				/** End Of File:
				 *  This markes the boundery between "parts"
				 */
			case 0x1:
				was_last_eof = 1;
				has_set_addr = 0;
				last_part = current_part;

				current_part->next = calloc(1, sizeof(struct IH_Part));
				current_part = current_part->next;

				/* Ignore checksum */
				src += 2;
				expectedPageCount--;

				goto SETUP_PAGE;
				break;

			/** Extended Segment Address:
			 *  This specifies what page a page should contain
			 */
			case 0x2:
				if (byte_count != 2){
					LIBTI_ERROR("ERROR: the Extended Segment Address needs a byte count of two");
					return ERR_TI_HEX_FORMAT;
				}
				has_set_addr = 0;
				if (has_set_extaddr){
					realloc_size = 2048;
					page->next = calloc(sizeof(struct IH_Page) + realloc_size, 1);
					page_loc = &page->next;
					page = *page_loc;
				}
				hex2bin((char*) &page->extAddress, src, 4);
				src += 4;

				page->extAddress = be16toh(page->extAddress);

				hex2bin((char*) &found_sum, src, 2);
				src += 2;

				page->hasExtAddress = 1;

				expected_sum += (page->extAddress & 0xFF) + (page->extAddress >> 8);
				expected_sum = 0 - expected_sum;

				if (found_sum != expected_sum){
					LIBTI_ERROR("ERROR: ti hex (intel hex) checksum failure! Expected %2X found %2X with record type %2X\n", expected_sum, found_sum, record_type);
					return ERR_TI_HEX_FORMAT;
				}
				has_set_extaddr = 1;
				break;


			default: LIBTI_ERROR("WARNING: Unknown record type: %02X\n", record_type); break;
		}
	}
	if (was_last_eof && last_part) {
		free(last_part->next);
		last_part->next = NULL;
	}

	if (expectedPageCount) {
		LIBTI_ERROR("WARNING: page count seems incomplete, %d\n", expectedPageCount);
	}

	return ERR_OK;
}




struct intel_hex_internal_data {
	size_t realloc_size;
	size_t offset;
	char** dst;
};
TiError encode_single_line(struct intel_hex_internal_data* state, char bytes, char record_type, uint16_t addr, char* byte_data) {
	char head[4] = {bytes, 0, 0, record_type};
	*(uint16_t*)&head[1] = htobe16(addr);

	size_t diff = 1 + 8 + ((size_t) bytes)*2 + 2 + 1;
	while (state->offset + diff >= state->realloc_size) {
		state->realloc_size = state->realloc_size * 2;
		*state->dst = realloc(*state->dst, state->realloc_size);
	}

	(*state->dst)[state->offset++] = ':';
	bin2hex(*state->dst + state->offset, head, 4);
	state->offset += 8;

	if (bytes) {
		bin2hex(*state->dst + state->offset, byte_data, bytes);
		state->offset += bytes * 2;
	}

	char sum = 0;
#	pragma unroll
	for (int i = 0; i < 4; i++)
		sum += head[i];
	for (int i = 0; i < bytes; i++)
		sum += byte_data[i];
	sum = 0 - sum;
	bin2hex(*state->dst + state->offset, &sum, 1);
	state->offset += 2;

	(*state->dst)[state->offset++] = '\n';
}


TiError encode_page_data(char** dst, struct IH_Part* src, char lineLength) {
	*dst = realloc(*dst, 2048);
	struct intel_hex_internal_data internal_data = {2048, 0, dst};


	TOP:
	struct IH_Page* curr_page = src->first_page;

	while (curr_page) {
		uint16_t address = curr_page->startAddress;
		if (curr_page->hasExtAddress) {
			uint16_t page_data = htobe16(curr_page->extAddress);
			encode_single_line(&internal_data, 2, 2, 0, (char*)&page_data);
		}

		size_t len = curr_page->dataLength;
		char* data_loc = curr_page->data;
		while (len) {
			char byte_len = (len < lineLength) ? (char) len : lineLength;
			len -= byte_len;

			encode_single_line(&internal_data, byte_len, 0, address, data_loc);

			address += byte_len;
			data_loc += byte_len;
		}


		curr_page = curr_page->next;
	}

	encode_single_line(&internal_data, 0, 1, 0, NULL);

	if (src->next) {
		src = src->next;
		goto TOP;
	}



	return ERR_OK;
}