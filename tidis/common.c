#include "common.h"
#include <stddef.h>

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fileFormats.h"

#include <sys/stat.h>
#include <sys/types.h>

const char* input_json = NULL;
const char* input_file= NULL;
const char* output_folder= NULL;

char force = 0;
enum FileFormat input_file_format;
cJSON* input_json_obj = NULL;



const char* output_original_bins_folder = NULL;
const char* output_json_file = NULL;


const char* user_assembler = "spasm";




/* {x} / {y} */
char* join(const char* x, const char* y) {
    size_t xlen = strlen(x);
    size_t ylen = strlen(y);

    size_t newlen = xlen + 1 + ylen + 1;
    char* result = malloc(newlen);
    memcpy(result, x, xlen);
    result[xlen] = '/';
    memcpy(result + xlen + 1, y, ylen);
    result[newlen - 1] = 0;

    return result;
}

/*
 * Try to either create a folder,
 * or in force mode it is valid for the folder to already exist
 *
 * So return 0 if successful, 1 if something is wrong.
 */
static int verify_folder_correctness(const char* test) {
    int didFail = mkdir(test, 0777);

    DIR* dir = opendir(test);

    /* No directory could be made or found */
    if (didFail && NULL == dir) {
        fprintf(stderr, "Error: Failed to open directory %s\n", test);
        return 1;
    }
    /* The directory exists for sure */

    closedir(dir);

    didFail = didFail && !force;
    if (didFail) fprintf(stderr, "Error: Directory already exists %s\n", test);
    return didFail;
}



int setup_output_dir() {
    if (verify_folder_correctness(output_folder))
        return EXIT_FAILURE;


    output_original_bins_folder = join(output_folder, ".original binary");
    output_json_file = join(output_folder, "meta.json");


    if (verify_folder_correctness(output_original_bins_folder))
        return EXIT_FAILURE;




    return EXIT_SUCCESS;
}

int save_input_json() {
    FILE* f = fopen(output_json_file, "w");
    if (NULL == f) {
        fprintf(stderr, "Error: Failed to open output json file %s\n", output_json_file);
        return EXIT_FAILURE;
    }

    char* out = cJSON_Print(input_json_obj);
    fprintf(f, "%s\n", out);
    free(out);

    fclose(f);

    return EXIT_SUCCESS;
}

