#ifndef __MOUSE_MACROS
#define __MOUSE_MACROS

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <inttypes.h>

//These options must be set externally to this file

extern int opt_printMacroArray;
extern int opt_printKeyArray;
extern int opt_verbose;

//Useful arrays
extern uint8_t keySetArray[];
extern size_t keySetArray_size;
extern int macroRegister[];

extern uint8_t* macroArray;

/*
 *
 * Macro codes include:
 * 0x61 (Press Key)
 * 0x62 (Release Key)
 * 0x68 (Delay)
 * 0x04 (a key)
 * 0x30 (0 key)
 *
 * */

//These update macroArray and macroRegister
int addMacro(int num, uint8_t* macro, int len);
int delMacro(int num);

/* Codes:
 *
 * Alphabet starts at 0x04  (a)
 * Numberals start at 0x1d (1)
 *
 * */

#endif
