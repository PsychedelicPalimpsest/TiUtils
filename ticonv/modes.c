#include "modes.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "libti.h"
#include <errno.h>
#include <string.h>
#include "../libti/libti.h"


/* MODE_EXT_INTEL_HEX */
int handleExtractIntellHex(struct ModeInfo* mode_info) {
    if (mode_info->in_file_count != 1) {LIBTI_ERROR("ERROR: unexpected number of input files in ExtractIntellHex\n"); return EXIT_FAILURE;}

    if (0 != access(mode_info->input_files[0], R_OK)) {
        LIBTI_ERROR("ERROR: input file %s does not exist or cannot be read. Os error: %s\n", mode_info->input_files[0], strerror(errno));
        return EXIT_FAILURE;
    }
/*
    if (0 != access(mode_info->output_file, W_OK)) {
        LIBTI_ERROR("ERROR: input file %s cannot be written too. Os error: %s\n", mode_info->output_file, strerror(errno));
        return EXIT_FAILURE;
    }
    if (mode_info->extra && 0 != access(mode_info->extra, W_OK)) {
        LIBTI_ERROR("ERROR: input file %s cannot be written too. Os error: %s\n", mode_info->extra, strerror(errno));
        return EXIT_FAILURE;
    }
*/
    FILE* f = fopen(mode_info->input_files[0], "r");
    if (f == NULL) {LIBTI_ERROR("Cannot open input\n"); return EXIT_FAILURE;}

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* buffer = malloc(size);
    fread(buffer, size, 1, f);
    fclose(f);

    struct FlashFile* flash_file = NULL;

    if (parse_flash(&flash_file, buffer, size)) {
        /* Libti prints its own errors */
        return EXIT_FAILURE;
    }


    f = fopen(mode_info->output_file, "w");
    if (f == NULL) {LIBTI_ERROR("Cannot open output\n"); return EXIT_FAILURE;}
    fwrite(flash_file->intellHexData, flash_file->dataLength, 1, f);
    fclose(f);

    if (mode_info->extra) {
        f = fopen(mode_info->extra, "w");\
        cJSON* json = NULL;
        flash_file_to_json(&json, flash_file);
        char* str = cJSON_Print(json);

        fprintf(f, "%s\n", str);

        free(str);
        cJSON_Delete(json);

    }



    printf("Success\n");

    free(buffer);
    return EXIT_SUCCESS;
}


int handleModes(struct ModeInfo* mode_info) {
    switch (mode_info->mode) {
        case MODE_EXT_INTEL_HEX:
            return handleExtractIntellHex(mode_info);
        default:
            printf("Error: Unexpected mode code %d\n", mode_info->mode);
            return EXIT_FAILURE;
    }
}
