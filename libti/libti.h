#pragma once
#include <stdio.h>



/* This library allows you to define your own error logging, 
   but as this is just a console application most of the time
   just use stderr */
#ifndef LIBTI_ERROR

/* By default just dump it into stderr */
#define LIBTI_ERROR(ERR, ...) fprintf(stderr, ERR, ##__VA_ARGS__);

#endif



struct TiSettings {
	char add_ti_footer;

};


static inline void init_default_settings(struct TiSettings* settings) {
	settings->add_ti_footer = 0;
}



typedef enum {
	ERR_OK = 0,

	/* Check errno for more details */
	ERR_SYS,


	/* An issue with the file format */
	ERR_TI_LINK_FORMAT,
	ERR_TI_HEX_FORMAT,

	ERR_JSON_FORMAT_ERROR
} TiError;


static const char* tiError2str(TiError err){
	switch (err){
		case ERR_OK: return "Success";
		case ERR_SYS: return "System error";
		case ERR_TI_LINK_FORMAT: return "TI format file error";
		case ERR_TI_HEX_FORMAT: return "TI hex (intel hex) error";
		case ERR_JSON_FORMAT_ERROR: return "Json format file error";


		default: return "Unknown error";
	}
}


#include "linkFormats.h"