#include <stdio.h>
#include <stdlib.h>
#include "libti.h"
#include "tihex.h"
#include <string.h>

int main(){
	FILE* f = fopen("/home/mitchell/Documents/tiUtils/bins/ti84plus_2.55.8xu", "r");
	
	fseek(f, 0, SEEK_END);
	long end = ftell(f);
	fseek(f, 0, SEEK_SET);

	char* os = malloc(end);
	fread(os, end, 1, f);

	struct FlashFile* flash = NULL;
	printf("Ret: %d\n", parse_flash(&flash, os, end));

	print_flash_file(flash);
	
	struct IH_Part* p1 = calloc(sizeof(struct IH_Part), 3);
	printf("Ret: %d\n", decode_page_data(p1, flash->intellHexData, flash->dataLength, 3));

	char* out = NULL;
	encode_page_data(&out, p1, 0x20);
	// printf("OUT: %s\n", out);


	FILE* og = fopen("OG.ihx", "w");
	fwrite(flash->intellHexData, flash->dataLength, 1, og);
	fclose(og);
	FILE* my = fopen("MY.ihx", "w");
	fprintf(my, "%s", out);
	fclose(my);


	free(os);
	fclose(f);
}