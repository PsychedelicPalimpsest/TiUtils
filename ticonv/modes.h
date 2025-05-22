#pragma once
#include "fileFormats.h"

enum Mode {
    MODE_UNKNOWN,

    MODE_EXT_INTEL_HEX,
    MODE_PCK_INTEL_HEX,
};


struct ModeInfo {
    enum Mode mode;

    enum FileFormat inputFormat;
    enum FileFormat outputFormat;

    int in_file_count;
    char **input_files;
    char *output_file;

    /* Nullable */
    const char *extra;
};



int handleModes(struct ModeInfo* mode_info);


