#ifndef __MOUSE_PARSER
#define __MOUSE_PARSER

#include <stdlib.h>
#include <stdio.h>

#include <stdint.h>
#include <inttypes.h>
#include <string.h>

#include "macro.h"

#define PARSER_BUFFER 256

//format:
/*
 *
Mouse 5 to save a file in emacs
[5] C-x C-s
 *
 */

extern uint8_t* illuminationArray;
extern uint8_t* CPIArray;

//parses a string. This function maintains the macro info itself
int parse(char* string);

#endif
