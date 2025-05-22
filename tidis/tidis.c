#include <getopt.h>
#include <stdio.h>
#include <stddef.h>



static const char* input_json = NULL;
static const char* input_file= NULL;
static const char* output_folder= NULL;

static char force = 0;


const struct option long_options[] = {
    {"input-json", required_argument, NULL, 'j'},
    {"input-file", required_argument, NULL, 'f'},
    {"output-folder", required_argument, NULL, 'o'},
    {"force", no_argument, NULL, 'F'},
    {0, 0, 0, 0}
};


int main(int argc, char *argv[]){
    int opt;
    while ((opt = getopt_long(argc, argv, "j:f:o:F:", long_options, NULL)) != -1) {
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
           default:
                fprintf(stderr, "Unknown option\n");
                return -1;
        }
    }
    if (input_json == NULL || input_file == NULL || output_folder == NULL){
        fprintf(stderr, "Please use required args\n");
        return -1;
    }


    return 0;
}