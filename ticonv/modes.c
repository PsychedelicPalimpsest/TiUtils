#include "modes.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "../libti/libti.h"
#include <errno.h>
#include <string.h>


extern char append_asmprog_token;
extern char skip_asmprog_token;


static int handleGenericSingleInputFile(struct ModeInfo* mode_info, char** buff, size_t* buff_size) {
    if (mode_info->in_file_count != 1) {
        LIBTI_ERROR("ERROR: unexpected number of input files in ExtractIntellHex\n");
        return EXIT_FAILURE;
    }

    if (0 != access(mode_info->input_files[0], R_OK)) {
        LIBTI_ERROR("ERROR: input file %s does not exist or cannot be read. Os error: %s\n", mode_info->input_files[0], strerror(errno));
        return EXIT_FAILURE;
    }
    FILE* f = fopen(mode_info->input_files[0], "r");
    if (f == NULL) {LIBTI_ERROR("Cannot open input\n"); return EXIT_FAILURE;}

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* buffer = malloc(size);
    fread(buffer, size, 1, f);
    fclose(f);

    *buff_size = (size_t) size;
    *buff = buffer;
    return EXIT_SUCCESS;
}


int handleGenericExtraJson(struct ModeInfo* mode_info, cJSON** dst) {
    if (mode_info->extra == NULL) {
        *dst = cJSON_CreateObject();
        return EXIT_SUCCESS;
    }
    FILE* f = fopen(mode_info->extra, "r");
    if (f == NULL) {LIBTI_ERROR("Cannot open input\n"); return EXIT_FAILURE;}

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* buffer = alloca(size + 1);
    fread(buffer, size, 1, f);
    buffer[size] = '\0';
    fclose(f);

    *dst = cJSON_ParseWithLength(buffer, size);
    if (*dst == NULL) {
        LIBTI_ERROR("Cannot parse input json\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

/* MODE_EXT_INTEL_HEX */
int handleExtractIntellHex(struct ModeInfo* mode_info) {
    char* buffer = NULL;
    size_t size = 0;
    if (handleGenericSingleInputFile(mode_info, &buffer, &size))
        return EXIT_FAILURE;

    struct FlashFile* flash_file = NULL;

    if (parse_flash(&flash_file, buffer, size)) {
        /* Libti prints its own errors */
        return EXIT_FAILURE;
    }


    FILE* f = fopen(mode_info->output_file, "w");
    if (f == NULL) {
        LIBTI_ERROR("ERROR: output file %s cannot be written too. Os error: %s\n", mode_info->output_file, strerror(errno)); return EXIT_FAILURE;
    }
    fwrite(flash_file->intellHexData, flash_file->dataLength, 1, f);
    fclose(f);

    if (mode_info->extra) {
        f = fopen(mode_info->extra, "w");
        if (f == NULL) {
            LIBTI_ERROR("ERROR: extra output file %s cannot be written too. Os error: %s\n", mode_info->extra, strerror(errno));
        }
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
int handlePackIntellHex(struct ModeInfo* mode_info) {
    char* buffer;
    size_t size;

    if (handleGenericSingleInputFile(mode_info, &buffer, &size))
        return EXIT_FAILURE;
    cJSON* json = NULL;
    if (handleGenericExtraJson(mode_info, &json))
        return EXIT_FAILURE;

    /* Only if a value in the JSON exists where 'Force Convert Footer' == true AND a footer is not already present */
    char doAddTiFooter =
                json
                && cJSON_HasObjectItem(json, "Force Convert Footer")
                && cJSON_IsTrue(cJSON_GetObjectItem(json, "Force Convert Footer"))
                /* Check if footer is already present */
                && 0 != memcmp(buffer + size - sizeof(TI_FOOTER) + 1, TI_FOOTER, sizeof(TI_FOOTER));

    int diff = sizeof(TI_FOOTER) * doAddTiFooter;

    struct FlashFile* flash_file = malloc(sizeof(struct FlashFile) + size + diff);
    if (flash_file_from_json(flash_file, json))
            return EXIT_FAILURE;
    flash_file->dataLength = size + diff;
    memcpy(flash_file->intellHexData, buffer, size);
    if (doAddTiFooter)
        memcpy(flash_file->intellHexData + size, TI_FOOTER, diff);

    char* out_buff = NULL;
    size_t delta;
    if (write_flash(&out_buff, flash_file, &delta))
        return EXIT_FAILURE;

    FILE* f = fopen(mode_info->output_file, "w");
    if (f == NULL) {
        LIBTI_ERROR("Cannot open output\n");
        return EXIT_FAILURE;
    }
    fwrite(out_buff, delta, 1, f);
    fclose(f);

    free(buffer);
    free(out_buff);




    return EXIT_SUCCESS;
}

/* MODE_PCK_PROGRAM_BIN */
int handlePackProgramBin(struct ModeInfo* mode_info) {
    char* buffer;
    size_t size;

    if (handleGenericSingleInputFile(mode_info, &buffer, &size))
        return EXIT_FAILURE;
    cJSON* json = NULL;
    if (handleGenericExtraJson(mode_info, &json))
        return EXIT_FAILURE;


    int diff = 2 + 2 * !!append_asmprog_token;

    struct VariableEntry* entry = alloca(sizeof(struct VariableEntry) + size + diff);

    if (variable_entry_from_json(entry, json))
        return EXIT_FAILURE;
    /* Append the "Token Length" field */
    *(uint16_t*) entry->data = htole16(size + diff - 2);



    if (append_asmprog_token) {
        entry->data[2] = '\xbb';
        entry->data[3] = '\x6d';
    }
    memcpy(entry->data + diff, buffer, size);
    entry->variableLength = size + diff;

    char* var_bytes = NULL;
    size_t delta;
    if (write_variable_entry(&var_bytes, entry, &delta))
        return EXIT_FAILURE;

    struct VariableFile* vf = alloca(sizeof(struct VariableFile) + delta);
    variable_from_json(vf, json);
    vf->dataLength = delta;
    memcpy(vf->data, var_bytes, delta);

    char* out_bytes = NULL;
    write_variable(&out_bytes, vf, &delta);

    FILE* f = fopen(mode_info->output_file, "w");
    if (f == NULL) {
        LIBTI_ERROR("Cannot open output\n");
        return EXIT_FAILURE;
    }
    fwrite(out_bytes, delta, 1, f);
    fclose(f);

    free(out_bytes);
    free(var_bytes);
    free(buffer);
    return EXIT_SUCCESS;
}
/* MODE_EXT_PROGRAM_BIN */
int handleExtractProgramBin(struct ModeInfo* mode_info) {
    char* buffer = NULL;
    size_t size = 0;
    if (handleGenericSingleInputFile(mode_info, &buffer, &size))
        return EXIT_FAILURE;



    struct VariableFile* file  = NULL;
    struct VariableEntry* prog = NULL;
    if (parse_variable(&file, buffer, size)) {
        return EXIT_FAILURE;
    }
    size_t ignored_delta = 0;
	if (parse_variable_entry(&prog, file->data, file->dataLength, &ignored_delta)) {
	    return EXIT_FAILURE;
	}

    FILE* f = fopen(mode_info->output_file, "w");
    if (f == NULL) {
        LIBTI_ERROR("ERROR: output file %s cannot be written too. Os error: %s\n", mode_info->output_file, strerror(errno)); return EXIT_FAILURE;
    }
    /*
     * In programs we skip the first two bytes, that's the "token count"
     * Then, if the user asks for it, we skip the next two bytes (the asm prog token).
     * This should look like BB 6D
     */
    int diff = 2  + 2 * !!skip_asmprog_token;

    if (skip_asmprog_token && !test_asm_prog_tok(prog->data + 2))
        LIBTI_ERROR("WARNING: Invalid asm prog token, likely NOT a asm program. Meaning the file was incorrectly truncated!");

    fwrite(prog->data + diff, prog->variableLength - diff, 1, f);
    fclose(f);

    if (mode_info->extra) {
        f = fopen(mode_info->extra, "w");
        if (f == NULL) {
            LIBTI_ERROR("ERROR: extra output file %s cannot be written too. Os error: %s\n", mode_info->extra, strerror(errno));
        }
        cJSON* json = NULL;
        variable_to_json(&json, file);
        variable_entry_to_json(&json, prog);
        char* str = cJSON_Print(json);

        fprintf(f, "%s\n", str);

        free(str);
        cJSON_Delete(json);
    }

    free(buffer);
    return EXIT_SUCCESS;
}





int handleModes(struct ModeInfo* mode_info) {
    switch (mode_info->mode) {
        case MODE_EXT_INTEL_HEX:
            return handleExtractIntellHex(mode_info);
        case MODE_PCK_INTEL_HEX:
            return handlePackIntellHex(mode_info);

        case MODE_EXT_PROGRAM_BIN:
            return handleExtractProgramBin(mode_info);
        case MODE_PCK_PROGRAM_BIN:
            return handlePackProgramBin(mode_info);
        default:
            printf("Error: Unexpected mode code %d\n", mode_info->mode);
            return EXIT_FAILURE;
    }
}
