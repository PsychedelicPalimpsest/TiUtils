#include <getopt.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "fileFormats.h"

#include "common.h"

#include "app.h"
#include "asmProg.h"
#include "os.h"

#include "cJSON.h"



const struct option long_options[] = {
    {"input-json", required_argument, NULL, 'j'},
    {"input-file", required_argument, NULL, 'f'},
    {"output-folder", required_argument, NULL, 'o'},
    {"set-assembler", required_argument, NULL, 'A'},
    {"force", no_argument, NULL, 'F'},
    {0, 0, 0, 0}
};


int main(int argc, char *argv[]){
    int opt;
    char* input_assembler = NULL;
    while ((opt = getopt_long(argc, argv, "f:o:j:F", long_options, NULL)) != -1) {
       switch (opt) {
            case 'j':
                input_json = optarg;
                break;
            case 'f':
                input_file = optarg;
                break;
            case 'o':
                output_folder = optarg;
                break;
            case 'F':
                force = 1;
                break;
           case 'A':
               input_assembler = optarg;
               break;
           default:
                fprintf(stderr, "Error: Unknown option\n");
                return EXIT_FAILURE;
        }
    }

    if (input_file == NULL || output_folder == NULL){
        fprintf(stderr, "Error: Please use required args\n");
        return EXIT_FAILURE;
    }
    if (input_json != NULL && !endswith(input_json, ".json")){
        fprintf(stderr, "Error:Input JSON must be .json\n");
        return EXIT_FAILURE;
    }
    input_json_obj = cJSON_CreateObject();
    if (input_json) {
        FILE* input = fopen(input_json, "r");
        if (input == NULL) {
            fprintf(stderr, "Warning: Could not open %s, assuming you mean to create it.\n", input_json);
        }else {
            fseek(input, 0, SEEK_END);
            size_t size = ftell(input);
            fseek(input, 0, SEEK_SET);
            char* buffer = malloc(size + 1);
            fread(buffer, size, 1, input);
            fclose(input);

            cJSON_Delete(input_json_obj);
            input_json_obj = cJSON_Parse(buffer);
            if (input_json_obj == NULL) {
                fprintf(stderr, "Error: The json input file is invalid!");
                return EXIT_FAILURE;
            }
            free(buffer);
        }
    }



    if (cJSON_HasObjectItem(input_json_obj, "Assembler")) {
        cJSON* as = cJSON_GetObjectItem(input_json_obj, "Assembler");
        if (!cJSON_IsString(as)) {
            fprintf(stderr, "Error: 'Assembler' in json MUST be a string");
            return EXIT_FAILURE;
        }
        if (input_assembler)
            fprintf(stderr, "Warning: 'Assembler' is set in json and the command line, ignoring the json.\n");

        user_assembler = as->valuestring;
    }
    if (input_assembler)
        user_assembler = input_assembler;

    input_file_format = lookup_format(input_file);

    switch (input_file_format){
        case FMT_8XP:
            return handle_asm_prog();
        default:
            fprintf(stderr, "Input file format is not supported\n");
            return EXIT_FAILURE;
    }



    return 0;
}