#include "parser.h"

size_t prependByteToString(uint8_t** array, uint8_t byte, size_t arraySize){
    //I am optomistic, I have enough memory
    uint8_t* newArray = (uint8_t *) malloc((arraySize + 1) * sizeof(uint8_t));

    int i;
    for(i = 0; i < arraySize; i++){
        //These brackets took a lot of headscratching
        newArray[i + 1] = (*array)[i];
    }
    newArray[0] = byte;
    free(*array);
    *array = newArray;

    return arraySize + 1;
}

size_t appendByteToString(uint8_t** array, uint8_t byte, size_t arraySize){
    //I am optomistic, I have enough memory
    *array = (uint8_t *) realloc(*array, (arraySize + 1) * sizeof(uint8_t));
    (*array)[arraySize] = byte;
    return arraySize + 1;
}

int processArg(uint8_t** array, size_t arraySize, uint8_t key, int modifier){
    uint8_t* newArray = NULL;
    size_t newArraySize = 0;
    newArraySize = prependByteToString(&newArray, key, newArraySize);
    newArraySize = prependByteToString(&newArray, 0x61, newArraySize);
    newArraySize = appendByteToString(&newArray, 0x62, newArraySize);
    newArraySize = appendByteToString(&newArray, key, newArraySize);
    if(modifier > 0){
        newArraySize = prependByteToString(&newArray, modifier, newArraySize);
        newArraySize = prependByteToString(&newArray, 0x61, newArraySize);
        newArraySize = appendByteToString(&newArray, 0x62, newArraySize);
        newArraySize = appendByteToString(&newArray, modifier, newArraySize);
    }
    int i;
    *array = (uint8_t *) realloc(*array, arraySize + newArraySize * sizeof(uint8_t));
    for(i = 0; i < newArraySize; i++){
        (*array)[arraySize + i] = newArray[i];
    }
    free(newArray);
    return arraySize + newArraySize;
}

int parse(char* string){
    if(opt_verbose) printf("\t!parsing %s", string);

    /*
     *
     * States
     * -1 - not yet on a valid line
     * 0  - reading the key number to set
     * 1  - reading a macro
     * 2  - unknown state
     *
     * */

    //These two variables record information about the macro we are constructing
    int size = 0;
    uint8_t* macro = NULL;
    int keynum = 0;

    //data used while parsing
    int state = -1;
    uint8_t key = 0;
    int modifier = 0;

    int i = -1;
    while(string[++i] != '\0'){
        switch(string[i]){
            //This switch assumes gcc extensions
            //A-Z shouldn't be in use because the correct way is LShift a-LShift z
        case ' ':
        case '\n':
            if((state == 1) && (key > 0)){
                if(opt_verbose) printf("processing %" PRIx8 "-%" PRIx8 " (%d)...\n", modifier, key, size);
                size = processArg(&macro, size, key, modifier);
            }
            modifier = 0;
            key = 0;
            break;
        case 'a' ... 'z':
            key = 0x04 + string[i] - 'a';
            break;
        case 'S':
            //S- -> Windows key -> 0xe3
            modifier = modifier + 1;
        case 'M':
            //M- -> 0xe2
            modifier = modifier + 1;
        case 'L':
            //L- -> Left Shift -> 0xe1
            modifier = modifier + 1;
        case 'C':
            //C- -> 0xe0
            modifier = modifier + 0xe0;
            if((state == 1) && ((strlen(string) < i + 2) || (string[i+1] != '-'))){
                printf("parse error char $d, expected; eg; C-x in %s\n", i, string);
                exit(1);
            }
            break;
        case '0' ... '9':
            if(state == 0){
                keynum = keynum * 10 + string[i] - '0';
            } else if(state == 1) {
                key = 0x1e + string[i] - '0';
                if(string[i] == '0'){
                    //0 requires special handling, ascii = 0..9
                    //This mouse = 1..9; 0
                    key = key + 9;
                }
            }
            break;
        case '-':
            break;
        case '[':
            printf("%i\n", i);
            if(i == 0){
                state = 0;
                key = 0;
                modifier = 0;
            }
            break;
        case ']':
            printf("%i\n", state);
            if(state == 0){
                state = 1;
                key = 0;
                modifier = 0;
            }
            break;
        default:
            break;
        }
    }

    if((state == 1) && (size == 0)){
        delMacro(keynum);
    }
    if(size > 0){
        addMacro(keynum, macro, size);
        free(macro);
    }
    return 0;
}
