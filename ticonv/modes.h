#pragma once


enum Mode {
    MODE_UNKNOWN,

    MODE_EXT_INTEL_HEX,
    MODE_PCK_INTEL_HEX,
};


enum KnownFormat {
    FMT_UNKNOWN,

    FMT_BIN,
    FMT_ASM,
    FMT_TXT,
    FMT_IHX,

    FMT_8XK,
    FMT_8XU,
    FMT_8XQ
};

struct ModeInfo {
    enum Mode mode;

    enum KnownFormat inputFormat;
    enum KnownFormat outputFormat;

    int in_file_count;
    char **input_files;
    char *output_file;

    /* Nullable */
    const char *extra;
};



int handleModes(struct ModeInfo* mode_info);


