#pragma once
#include "fileFormats.h"
#include "cJSON.h"

extern const char* input_json;;
extern const char* input_file;
extern const char* output_folder;

extern char force;
extern enum FileFormat input_file_format;

extern cJSON* input_json_obj;

extern const char* user_assembler;


/* The following only exist if setup_output_dir() was called */

extern const char* output_original_bins_folder;


/* {x} / {y} */
char* join(const char* x, const char* y);
int setup_output_dir();

int save_input_json();