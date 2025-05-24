#include "asmProg.h"

#include <stdlib.h>

#include "common.h"
#include "libti.h"
#include "linkFormats.h"

#include "makefile.h"


int get_variable_obj(struct VariableFile** file_dst, struct VariableEntry** entry_dst) {
    FILE *f = fopen(input_file, "r");
    if (f == NULL) {
        fprintf(stderr, "Error opening input file: %s\n", input_file);
        return EXIT_FAILURE;
    }
    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *buffer = malloc(file_size);
    fread(buffer, file_size, 1, f);
    fprintf(stderr, "File size: %llu\n", (size_t) file_size);


    if (parse_variable(file_dst, buffer, file_size)) {
        return EXIT_FAILURE;
    }
    size_t ignored_delta = 0;
    if (parse_variable_entry(entry_dst, (*file_dst)->data, (*file_dst)->dataLength, &ignored_delta)) {
        return EXIT_FAILURE;
    }


    fclose(f);
    free(buffer);
    return EXIT_SUCCESS;
}


int handle_asm_prog() {
    if (setup_output_dir()) return EXIT_FAILURE;

    struct StrList* strlist = strlist_new();


    for (int i = 0; i < 16; i++) {
        char* filen = malloc(16);
        sprintf(filen, "%d.bin", i);
        strlist_append(&strlist, filen);
    }
    char* out = strlist_join(strlist, ", ");
    printf("%s\n", out);
    free(out);
    strlist_free(strlist);


    return EXIT_SUCCESS;

    struct VariableFile* file = NULL;
    struct VariableEntry* entry = NULL;

    if (get_variable_obj(&file, &entry)) return EXIT_FAILURE;

    if (variable_to_json(&input_json_obj, file)) return EXIT_FAILURE;
    if (variable_entry_to_json(&input_json_obj, entry)) return EXIT_FAILURE;

    char* bin_file = join(output_original_bins_folder, "bin.bin");
    FILE* f = fopen(bin_file, "w");

    if (f == NULL) {
        fprintf(stderr, "Error opening output file: %s\n", bin_file);
        return EXIT_FAILURE;
    }

    fwrite(entry->data, entry->variableLength, 1, f);
    fclose(f);
    free(bin_file);



    if (save_input_json()) return EXIT_FAILURE;
    return EXIT_SUCCESS;
}
