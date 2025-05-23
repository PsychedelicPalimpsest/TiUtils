#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>

/* See for a full list of modes and formats */
#include "modes.h"
#include "fileFormats.h"



#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))



struct ModeDef {
    enum Mode mode;
    char mode_str[128];
};
struct ModeIdentification {
    enum Mode mode;
    char has_multi_input;
    enum FileFormat input_id[16];
    enum FileFormat output_id[16];
};


struct ModeDef MODES[] = {
    {MODE_EXT_INTEL_HEX, "=ext_intel_hex"},
    {MODE_PCK_INTEL_HEX, "=pck_intel_hex"},

    {MODE_EXT_PROGRAM_BIN, "=ext_program_bin"},
    {MODE_PCK_PROGRAM_BIN, "=pck_program_bin"}
};

/* What modes are autodetected/allowed for certain file formats */
static const struct ModeIdentification MODE_ID[] = {
    {MODE_EXT_INTEL_HEX, 0, {FMT_8XK, FMT_8XU, FMT_8XQ, 0}, {FMT_IHX, 0} },
    {MODE_PCK_INTEL_HEX, 0, {FMT_IHX, 0}, {FMT_8XK, FMT_8XU, FMT_8XQ, 0} },

    {MODE_EXT_PROGRAM_BIN, 0, {FMT_8XP, 0}, {FMT_BIN, 0}},
    {MODE_PCK_PROGRAM_BIN, 0, {FMT_BIN, 0}, {FMT_8XP, 0}}
};



static enum Mode autodetect_mode(int input_file_count, enum FileFormat input_format, enum FileFormat output_format) {
    for (int i = 0; i < ARRAY_SIZE(MODE_ID); i++) {
        char conforms_to_input = 0;
        char conforms_to_output = 0;

        /* Make sure that the input file count, conforms to what we are expecting */
        if (MODE_ID[i].has_multi_input != input_file_count > 1) continue;

        for (int j = 0; j < 16 && MODE_ID[i].input_id[j]; j++) {
            conforms_to_input |= input_format == MODE_ID[i].input_id[j];
        }
        for (int j = 0; j < 16 && MODE_ID[i].output_id[j]; j++) {
            conforms_to_output |= output_format == MODE_ID[i].output_id[j];
        }
        if (conforms_to_input && conforms_to_output)
            return MODE_ID[i].mode;
    }


    return MODE_UNKNOWN;
}


static void print_help(const char *progname) {
    fprintf(stderr, "Usage: %s [OPTION...] INPUT... OUTPUT\n", progname);
    fprintf(stderr, "Convert TI-84 specific files between formats.\n\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -I, --input-format=FORMAT    Set input format\n");
    fprintf(stderr, "                               \tDefault: inferred from input file extension\n");
    fprintf(stderr, "  -O, --output-format=FORMAT   Set output format (BIN, ASM, INTEL_HEX, etc.)\n");
    fprintf(stderr, "                               \tDefault: inferred from output file extension\n");
    fprintf(stderr, "  -m, --mode=MODE              Conversion mode\n");
    fprintf(stderr, "                               \tDefault: inferred from format combination\n");
    fprintf(stderr, "  -e, --extra-json file.json   Usage depends on the mode, but is a file used for metadata.\n");
    fprintf(stderr, "  -h, --help                   Display this help information\n");
    fprintf(stderr, "  -F, --list-formats           Display supported formats and exit\n\n");
    fprintf(stderr, "  -M, --list-modes             Display supported modes and exit\n\n");
}
static void print_formats() {
    fprintf(stderr, "Formats supported for conversion:\n\n");
    fprintf(stderr, "\tExt\tName\t\tDescription\n");
    fprintf(stderr, "\t8xp\tApp\t\tTi-8x application\n");
    fprintf(stderr, "\t8xk\tApp\t\tTi-84 flash app\n");
    fprintf(stderr, "\t8xu\tOS\t\tTi-84 operating system\n");
    fprintf(stderr, "\t8xq\tOS\t\tTi-84 certificate\n");
    fprintf(stderr, "\t8xk\tProgram\t\tTi-84 program file\n");

}
static void print_modes() {
    fprintf(stderr, "Formats modes for conversion:\n\n");
    fprintf(stderr, "\tName\t\t\tInput File extension(s)\t\tOutput file extension(s)\t\tDescription\n");
    fprintf(stderr, "\t-m=ext_intel_hex\t8xk, 8xu, 8xq\t\t\tihx\t\t\tExtracts the intel hex from a flash file. If the --extra-json is set, will write the metadata to that file\n");
    fprintf(stderr, "\t-m=pck_intel_hex\tihx\t\t\t\t8xk, 8xu, 8xq\t\tPacks the intel hex for a flash file. If the --extra-json is set, will read the metadata from that file\n");
    fprintf(stderr, "\t-m=ext_program_bin\tbin\t\t\t\t8xp\t\t\tExtracts the binary from a program file. If the --extra-json is set, will read the metadata from that file\n");
}

/* Some state, yes it is evil globals */
static enum Mode current_mode = MODE_UNKNOWN;
static enum FileFormat current_input_format = FMT_UNKNOWN;
static enum FileFormat current_output_format = FMT_UNKNOWN;

char append_asmprog_token = 0;
char skip_asmprog_token = 0;


/* Final checks to make sure all is right before running the actual program */
static void verify_mode_and_formats() {
    if (current_mode == MODE_UNKNOWN) {
        fprintf(stderr, "Error: Mode not set successfully (Reason unknown, likely a bug)\n");
        exit(EXIT_SUCCESS);
    }
    if (current_input_format == FMT_UNKNOWN) {
        fprintf(stderr, "Error: Input file format not set successfully (Reason unknown, likely a bug)\n");
        exit(EXIT_SUCCESS);
    }
    if (current_output_format == FMT_UNKNOWN) {
        fprintf(stderr, "Error: Input file format not set successfully (Reason unknown, likely a bug)\n");
        exit(EXIT_SUCCESS);
    }
}

int main(int argc, char *argv[]) {
    const char *input_format = NULL;
    const char *output_format = NULL;
    const char *mode = NULL;
    const char* extra = NULL;

    struct option long_options[] = {
        {"input-format",  required_argument, 0, 'I'},
        {"output-format", required_argument, 0, 'O'},
        {"mode",          required_argument, 0, 'm'},
        {"extra-json",    required_argument, 0, 'e'},
        {"help",           no_argument,       0, 'h'},
        {"list-formats",no_argument,       0, 'F'},
        {"list-modes",no_argument,       0, 'M'},
        {"append-asmprog-token", no_argument, 0, 'P'},
        {"skip-asmprog-token", no_argument, 0, 'S'},
        {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "I:O:m:hFMPSe:", long_options, NULL)) != -1) {
        switch (opt) {
        case 'I':
            input_format = optarg;
            break;
        case 'O':
            output_format = optarg;
            break;
        case 'm':
            mode = optarg;
            break;
        case 'e':
            extra = optarg;
            break;
        case 'M':
            print_modes();
            return EXIT_SUCCESS;
        case 'F':
            print_formats();
            return EXIT_SUCCESS;
        case 'h':
            print_help(argv[0]);
            return EXIT_SUCCESS;
        case 'P':
            append_asmprog_token = 1;
            break;
        case 'S':
            skip_asmprog_token = 1;
            break;
        default:
            print_help(argv[0]);
            return EXIT_FAILURE;
        }
    }

    // Handle positional arguments
    if (argc - optind < 2) {
        fprintf(stderr, "Error: Requires at least one input file and one output file\n");
        print_help(argv[0]);
        return EXIT_FAILURE;
    }

    int input_count = argc - optind - 1;
    char **input_files = &argv[optind];
    char *output_file = argv[argc - 1];



    /* If the user forces a format, look it up, and use it */
    if (input_format || output_format)
        for (int i = 0; i < ARRAY_SIZE(FORMATS); i++) {
            if (input_format && 0 == strcmp(FORMATS[i].force_format_name, input_format)) {
                current_input_format = FORMATS[i].format;
            }
            if (output_format && 0 == strcmp(FORMATS[i].force_format_name, output_format)) {
                current_output_format = FORMATS[i].format;
            }
        }
    if (input_format && current_input_format == FMT_UNKNOWN) {
        fprintf(stderr, "Error: Cannot unknown format \"%s\"\n", input_format);
        print_formats();
        exit(EXIT_FAILURE);
    }
    if (output_format && current_output_format == FMT_UNKNOWN) {
        fprintf(stderr, "Error: Cannot unknown format \"%s\"\n", output_format);
        print_formats();
        exit(EXIT_FAILURE);
    }

    if (!input_format) {
        current_input_format = lookup_format(input_files[0]);
        if (current_input_format == FMT_UNKNOWN) {
            fprintf(stderr, "Error: Unknown/unsupported file type \"%s\"\n", input_files[0]);
            exit(EXIT_FAILURE);
        }
    }
    if (!output_format) {
        current_output_format = lookup_format(output_file);
        if (current_output_format == FMT_UNKNOWN) {
            fprintf(stderr, "Error: Unknown/unsupported file type \"%s\"\n", output_file);
            exit(EXIT_FAILURE);
        }
    }

    /* If the user forces a mode, look it up and use it */
    if (mode != NULL) {
        #pragma unroll
        for (int i = 0; i < ARRAY_SIZE(MODES); i++) {
            if (0 == strcmp(MODES[i].mode_str, mode)) {
                current_mode = MODES[i].mode;
                break;
            }
        }
        if (current_mode == MODE_UNKNOWN) {
            fprintf(stderr, "Error: Cannot mode \"%s\" non-existent!\n", mode);
            print_modes();
            exit(EXIT_FAILURE);
        }
    }else {
        current_mode = autodetect_mode(input_count, current_input_format, current_output_format);
        if (current_mode == MODE_UNKNOWN) {
            fprintf(stderr, "Error: Cannot auto-detect mode! Please specify what mode you are using with the -m flag.");
            print_modes();
            exit(EXIT_FAILURE);
        }
    }

    verify_mode_and_formats();
    struct ModeInfo info = (struct ModeInfo) {
        current_mode,
        current_input_format,
        current_output_format,
        input_count,
        input_files,
        output_file,
        extra
    };


    return handleModes(&info);
}
