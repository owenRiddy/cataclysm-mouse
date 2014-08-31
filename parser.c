#include "parser.h"

uint8_t* illuminationArray = NULL;

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

// 0 - not a valid line
// 1-3, unused
// 4 to 14, keynumbers
// 15, RGB illumination
int getParseMode(char* string){
    int retCode = 0;
    int i = 0;

    if(string[i] != '['){
        //This line is not a valid command line
        return 0;
    }

    while(string[++i] != '\0'){
        switch(string[i]){
        case ']':
            return retCode;
            break;
        case '0' ... '9':
            retCode = retCode * 10 + string[i] - '0';
            break;
        case 'L':
            retCode = 15;
            break;
        default:
            //Invalid character
            return 0;
            break;
        }
    }

    return retCode;
}

int parseIllumination(char* string){
    printf("Setting illumination values\n");
    int ret = -1;

    //I hope you have enough memory for this
    if(illuminationArray != NULL){
        free(illuminationArray);
    }

    illuminationArray = malloc(sizeof(uint8_t) * 3);

    ret = sscanf(string, "[L] %" SCNd8" %" SCNd8 " %" SCNd8,
                 &illuminationArray[0],
                 &illuminationArray[1],
                 &illuminationArray[2]);

    if(ret != 3){
        printf("Error reading illumination values from '%s' (%d)\n", string, ret);
        free(illuminationArray);
        illuminationArray = NULL;
    }
}

int parse(char* string){
    if(opt_verbose) printf("\t!parsing %s", string);

    //We construct the macro by interpreting the [whatever] at the start of the line
    //Then moving on to the later part which is the configuration value

    int state = -1;
    int keynum = getParseMode(string);

    if(keynum == 0){
        return 0;
    }

    if(keynum == 15){
        parseIllumination(string);
        return 0;
    }

    //We construct the macro one chord at a time, each chord is a modifier (eg, 'M-') and a key (eg, 'a')
    int size = 0;
    uint8_t* macro = NULL;

    int modifier = 0;
    uint8_t key = 0;


    int i = -1;
    while(string[++i] != '\0'){
        switch(string[i]){
            //This switch assumes gcc extensions
            //A-Z shouldn't be in use because the correct way is LShift a-LShift z
        case 'a' ... 'z':
            key = 0x04 + string[i] - 'a';
            break;

        case ' ':
        case '\n':
            if(opt_verbose){
                printf("processing %" PRIx8 "-%" PRIx8 " (%d)...\n", modifier, key, size);
            }
            size = processArg(&macro, size, key, modifier);
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
            if((strlen(string) < i + 2) || (string[i+1] != '-')){
                printf("parse error char $d, expected; eg; C-x in %s\n", i, string);
                exit(1);
            }
            break;
        case '0' ... '9':
            key = 0x1e + string[i] - '0';
            if(string[i] == '0'){
                //0 requires special handling, ascii = 0..9
                //This mouse = 1..9; 0
                key = key + 9;
            }
            break;
        case '-':
            break;
        case ']':
            state = 1;
            key = 0;
            modifier = 0;
            break;
        default:
            if(state == 1){
                printf("Unexpected character '%c' - aborting\n", string[i]);
                return -1;
            }
            break;
        }
    }

    if(size == 0){
        delMacro(keynum);
    }

    if(size > 0){
        addMacro(keynum, macro, size);
        free(macro);
    }

    return 0;
}
