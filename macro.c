#include "macro.h"

//This is a convenience function to compare entries in macroRegister
int sizeMacro(int num);

//key - which mouse key (4-14) to set. Stay away from 1-3 as they are the 'normal' mouse keys
//stanza - the 5 byte sequence to load up
//len - the length of 5 bytes
//eg maintainKeyArray(14, {0x20, 0x00, 0x00, 0x00, 0x06}, 5)
int maintainKeyArray(int key, uint8_t * stanza, int len);

//Default: Set l-click, r-click and wheel but nothing else
//When setting a macro, use 0x20 (macro code?) 0x00 0x13 (start position) 0x00 0x06 *length of macro in bytes)
uint8_t keySetArray[] = { 0x0e, 0x00, 0x4b, 0x00, 0x42,//0
                       0x01, 0x00, 0x00, 0x00, 0x00,//1
                       0x02, 0x00, 0x00, 0x00, 0x00,//2
                       0x03, 0x00, 0x00, 0x00, 0x00,//3
                       0x00, 0x00, 0x00, 0x00, 0x00,//4
                       0x00, 0x00, 0x00, 0x00, 0x00,//5
                       0x00, 0x00, 0x00, 0x00, 0x00,//6
                       0x00, 0x00, 0x00, 0x00, 0x00,//7
                       0x00, 0x00, 0x00, 0x00, 0x00,//8
                       0x00, 0x00, 0x00, 0x00, 0x00,//9
                       0x00, 0x00, 0x00, 0x00, 0x00,//10
                       0x00, 0x00, 0x00, 0x00, 0x00,//11
                       0x00, 0x00, 0x00, 0x00, 0x00,//12
                       0x00, 0x00, 0x00, 0x00, 0x00,//13
                       0x00, 0x00, 0x00, 0x00, 0x00 }; //14
size_t keySetArray_size = sizeof(keySetArray);

//macroRegister[0] = sizeof(macroArray)
//macroRegister[i] = starting position of the ith macro
int macroRegister[] = {0,
                       0, 0, 0, 0, 0,
                       0, 0, 0, 0, 0,
                       0, 0, 0, 0 };
//Array used for sizeMacro functionality
int keySet[] = {0,
                0, 0, 0, 0, 0,
                0, 0, 0, 0, 0,
                0, 0, 0, 0 };
uint8_t* macroArray = NULL;

int maintainKeyArray(int key, uint8_t* stanza, int len){
    if(key <= 3){
        printf("Setting keys 1-3 is disabled to prevent accidently overwriting left-click\n");
        return -1;
    }

    if(len != 5){
        printf("Len is not 5 - only setting one key at a time presently");
        return -2;
    }

    int i;
    for(i = 0; i < len; i++){
        keySetArray[key * 5 + i] = stanza[i];
    }
    return len;
}

//size of the macro num in bytes
int sizeMacro(int num){
    //Clear impossible values
    //We don't set macros for keys 1-3
    if((num > 14) || (num < 3)){
        return 0;
    }

    if(keySet[num] == 0){
        return 0;
    }

    //Compare the macro to the next non-0 macro
    int i, closest = macroRegister[0];
    for(i = 1; i < 15; i++){
        if((keySet[i] != 0) && (macroRegister[i] > macroRegister[num])){
            if(macroRegister[i] < closest){
                closest = macroRegister[i];
            }
        }
    }
    return closest - macroRegister[num];
}

//These update macroArray and macroRegister
int addMacro(int num, uint8_t* macro, int len){
    int key = num;

    int map[] = {0, 0, 0, 0,
                 12, 13, 14,
                 6, 5, 4, 8, 7, 9,
                 10, 11};
    // If I set key 5 according to keySetArray, I set map[5] == key 6 on the mouse

    num = map[num];

    if(opt_verbose) printf("\n----------\nAdding macro to key: %d (%d)\nMacro: ", key, num);

    int i, j, k; //j counts how far into the building array we are
    for(i = 0; i < len; i++){
        if(opt_verbose) printf("%#04hh" PRIx8 " ", macro[i]);
    }
    if(opt_verbose) printf("(%d)\n", len);

    int newMacroArraySize = macroRegister[0] - sizeMacro(num) + len;
    if(opt_verbose) printf("Macro array size from %d => %d\n", macroRegister[0], newMacroArraySize);

    //allocate new array
    uint8_t* buildingArray = (uint8_t*) malloc(sizeof(uint8_t) * newMacroArraySize);
    if(buildingArray == NULL){
        printf("Error no mem!\n");
        exit(1);
    }

    int registerSize = sizeof(macroRegister) / sizeof(macroRegister[0]);
    int* newRegister = (int*) malloc(sizeof(macroRegister));
    if(newRegister == NULL){
        printf("I can't find enough memory :'(");
        exit(1);
    }

    newRegister[0] = newMacroArraySize;
    newRegister[1] = 0;
    newRegister[2] = 0;
    newRegister[3] = 0;

    for(i = 4, j = 0; i < registerSize; i++){
        if(i == num) continue;
        if(keySet[i] != 0){
            newRegister[i] = j;
            for(k = macroRegister[i]; k < macroRegister[i] + sizeMacro(i); k++, j++){
                buildingArray[j] = macroArray[k];
            }
        } else {
            newRegister[i] = 0;
        }
    }

    //copy new macro
    for(i = 0; i < len; i++, j++){
        buildingArray[j] = macro[i];
    }

    for(i = 0; i < registerSize; i++){
        macroRegister[i] = newRegister[i];
    }
    free(newRegister);

    macroRegister[num] = newMacroArraySize - len;
    if(macro == NULL){
        keySet[num] = 0;
    } else {
        keySet[num] = 1;
    }

    if(opt_verbose) printf("Macro array & Register reshuffled\n");

    if(macroArray != NULL) free(macroArray);
    macroArray = buildingArray;

    if(opt_printMacroArray == 1){
        printf("Macro Register (%d items): \n", registerSize - 1); //ignore entry 0
        printf("(no) Start\tSize\tMacro\n");
        for(i = 1; i < registerSize; i++){
            printf("(%02d) %#04hh" PRIx8 "\t%d\t", i, macroRegister[i], sizeMacro(i));
            if(sizeMacro(i) != 0) {
                for(j = 0; j < sizeMacro(i); j++){
                    printf("%#04hh" PRIx8 " ", macroArray[macroRegister[i] + j]);
                }
            }
            printf("\n");
        }
        printf("\nTotal Macro Size: %d\n", macroRegister[0]);
    }

    for(i = 4; i < 15; i++){
        char n[] = {0x00, 0x00, 0x00, 0x00, 0x00};
        char m[] = {0x20, 0x00, 0x00, 0x00, 0x06};
        if(sizeMacro(i) == 0){
            maintainKeyArray(i, n, 5);
        } else {
            m[2] = macroRegister[i];
            m[4] = sizeMacro(i);
            maintainKeyArray(i, m, 5);
        }
    }

    if(opt_printKeyArray == 1){
        printf("Key Array: ");
        for(i = 0; i < 15 * 5; i++){
            if(i % 5 == 0) printf("\n");
            printf("%#04hh" PRIx8 " ", keySetArray[i]);
        }
        printf("\n");
    }

    return 0;
}

int delMacro(int num){
    return addMacro(num, NULL, 0);
}
