#pragma once




enum FileFormat {
    FMT_UNKNOWN,

    FMT_BIN,
    FMT_ASM,
    FMT_TXT,
    FMT_IHX,

    FMT_8XK,
    FMT_8XU,
    FMT_8XQ
};

struct FormatMap {
    enum FileFormat format;
    char force_format_name[16];
    char format_id[16][16];
};


static const struct FormatMap FORMATS[] = {
    {FMT_IHX, "ihx", ".ihx", ".IHX"},
    {FMT_ASM, "asm", ".asm", ".ASM", ".z80", ".Z80"},
    {FMT_TXT, "txt", ".txt", ".TXT"},
    {FMT_BIN, "bin", ".bin", ".BIN"},

    {FMT_8XK, "8xk", ".8Xk", ".8xk"},
    {FMT_8XU, "8xu", ".8Xu", ".8xu"},
};

char endswith(const char *str, const char *substr);
enum FileFormat lookup_format(const char *filename);