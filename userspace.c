#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <libusb-1.0/libusb.h>
#include <unistd.h>
#include <getopt.h>

//Default: Set l-click, r-click and wheel but nothing else
//When setting a macro, use 0x20 (macro code?) 0x00 0x13 (start position) 0x00 0x06 *length of macro in bytes)
char keySetArray[] = { 0x0e, 0x00, 0x4b, 0x00, 0x42,//0
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
char* macroArray = NULL;
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
int addMacro(int num, char* macro, int len);
int delMacro(int num);
//This is a convenience function to compare entries in macroRegister
int sizeMacro(int num);

//send updated info to the Mouse
int updateMapping(libusb_device_handle* hnd);
int updateMacros(libusb_device_handle* hnd);

/* Codes:
 *
 * Alphabet starts at 0x04  (a)
 * Numberals start at 0x30 (0)
 *
 * */

//key - which mouse key (4-14) to set. Stay away from 1-3 as they are the 'normal' mouse keys
//stanza - the 5 byte sequence to load up
//len - the length of 5 bytes
//eg maintainKeyArray(14, {0x20, 0x00, 0x00, 0x00, 0x06}, 5)
int maintainKeyArray(int key, char * stanza, int len){
    if(key <= 3){
        printf("Setting keys 1-3 is disabled to prevent accidently overwriting left-click");
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

int diagnoseDevice(libusb_device_handle* hnd){

    if(hnd == NULL) {
        printf("Error - epic mouse not found!!\n");
        return 1;
    } else {
        printf("Epic Mouse was located\n");
    }

    struct libusb_device_descriptor des;
    libusb_device *device = libusb_get_device(hnd);

    libusb_get_device_descriptor(device, &des);

    printf("\tidVendor:\t0x%" PRIx16 "\n", des.idVendor);
    printf("\tidProduct:\t0x%" PRIx16 "\n", des.idProduct);
    printf ("\tiManufacturer:\t%d\n", des.iManufacturer);

    if (des.iManufacturer > 0)
    {
        libusb_device_handle *hnd;

        int err = libusb_open(device, &hnd);
        char strDesc[256];

        libusb_get_string_descriptor_ascii(hnd, des.iManufacturer, strDesc, 256);
        printf ("\tstring:\t\t%s\n",  strDesc);
        libusb_close(hnd);
    }

    return 0;
}

//If set, do not send any data to the mouse.
int opt_dryRun = 0;
//If set print the key array
int opt_printKeyArray = 0;
//If set print the macros bound to each key out
int opt_printMacroArray = 0;
//Verbosity flag
int opt_verbose = 0;

int main(int argc, char** argv){
    uint16_t vendor_id = 0x1038; //8087
    uint16_t product_id = 0x1320; //0024

    int err;
    int ret;

    int c;
    while (1) {
        static struct option long_options[] = {
            /* These options set a flag. */
            //{"verbose", no_argument,       &verbose_flag, 1},
            //{"brief",   no_argument,       &verbose_flag, 0},
            //{"add",     no_argument,       0, 'a'},
            //{"append",  no_argument,       0, 'b'},
            //{"delete",  required_argument, 0, 'd'},
            {"verbose",     no_argument, 0, 'v'},
            {"verbose-keyarray",     no_argument, 0, 'k'},
            {"verbose-macros",     no_argument, 0, 'm'},
            {"dry",     no_argument, 0, 'd'},
            {"file",    required_argument, 0, 'f'},
            {0, 0, 0, 0}
        };
        /* getopt_long stores the option index here. */
        int option_index = 0;

        c = getopt_long (argc, argv, "vdkmf:",
                         long_options, &option_index);

        /* Detect the end of the options. */
        if (c == -1) break;

        switch (c) {
        case 0:
            /* If this option set a flag, do nothing else now. */
            if (long_options[option_index].flag != 0)
                break;
            printf ("option %s", long_options[option_index].name);
            if (optarg)
                printf (" with arg %s", optarg);
            printf ("\n");
            break;

        case 'd':
            opt_dryRun = 1;
            break;

        case 'v':
            opt_verbose = 1;
            break;

        case 'f':
            printf ("option -f with value `%s'\n", optarg);
            break;

        case 'k':
            opt_printKeyArray = 1;
            break;

        case 'm':
            opt_printMacroArray = 1;
            break;

        default:
            exit(0);
        }
    }

    //libusb_context* context = NULL;
    ret = libusb_init(NULL);

    //libusb_set_debug(NULL, LIBUSB_LOG_LEVEL_DEBUG);
    libusb_set_debug(NULL, LIBUSB_LOG_LEVEL_WARNING);

    libusb_device_handle* epicMouse = libusb_open_device_with_vid_pid(NULL, vendor_id, product_id);
    err = diagnoseDevice(epicMouse);
    if(err) printf("Error printing out diagnostic info");

    printf("Detaching Kernel driver. Good luck\n");
    libusb_set_auto_detach_kernel_driver(epicMouse, 1);

    printf("Claiming the interface...\n");
    libusb_claim_interface(epicMouse, 0);

    char aMacro[] = {0x61, 0x04, 0x62, 0x04, 0x68, 0x01};
    //char bMacro[] = {0x61, 0x05, 0x61, 0x06, 0x62, 0x05, 0x62, 0x06, 0x68, 0x01};
    //char bMacro[] = {0x61, 0x05, 0x62, 0x05, 0x68, 0x01};
    //ar winaMacro[] = {0x61, 0xe1, 0x68, 0x01, 0x61, 0x04, 0x68, 0x01, 0x62, 0xe1, 0x68, 0x01, 0x62, 0x04, 0x68, 0x01};
    int i;
    for(i = 4; i < 15; i++){
        aMacro[1] = i;
        aMacro[3] = i;
        addMacro(i, aMacro, sizeof(aMacro));
    }

    //Dota bindings - r
    aMacro[1] = 0x15;
    aMacro[3] = 0x15;
    addMacro(7, aMacro, sizeof(aMacro));

    //char altTemplate[] = {0x61, 0xe2, 0x61, 0x04, 0x68, 0x01, 0x62, 0xe2, 0x62, 0x04, 0x68, 0x01};
    //Got rid of alt!
    char altTemplate[] = {0x61, 0x04, 0x68, 0x01, 0x62, 0x04, 0x68, 0x01};
    //Dota bindings - M-u
    altTemplate[1] = 0x18;
    altTemplate[5] = 0x18;
    addMacro(8, altTemplate, sizeof(altTemplate));

    //Dota bindings - M-i
    altTemplate[1] = 0x0c;
    altTemplate[5] = 0x0c;
    addMacro(9, altTemplate, sizeof(altTemplate));

    //Dota bindings - M-g
    altTemplate[1] = 0x0a;
    altTemplate[5] = 0x0a;
    addMacro(10, altTemplate, sizeof(altTemplate));

    //Dota bindings - M-h
    altTemplate[1] = 0x0b;
    altTemplate[5] = 0x0b;
    addMacro(11, altTemplate, sizeof(altTemplate));

    //Dota bindings - M-j
    altTemplate[1] = 0x0d;
    altTemplate[5] = 0x0d;
    addMacro(12, altTemplate, sizeof(altTemplate));


    //char mod[] = {
    //C-a
    //0x61, 0xe0, 0x61, 0x26, 0x68, 0x01, 0x62, 0x26, 0x62, 0xe0, 0x68, 0x01
    //Win-a
    //0x61, 0xe3, 0x68, 0x01, 0x61, 0x04, 0x68, 0x01, 0x62, 0xe3, 0x68, 0x01, 0x62, 0x04, 0x68, 0x01,
    //LShift-a
    //0x61, 0xe1, 0x68, 0x01, 0x61, 0x04, 0x68, 0x01, 0x62, 0xe1, 0x68, 0x01, 0x62, 0x04, 0x68, 0x01,
    //M-a
    //0x61, 0xe2, 0x68, 0x01, 0x61, 0x04, 0x68, 0x01, 0x62, 0x04, 0x68, 0x01, 0x62, 0xe2, 0x68, 0x01,
    //};
    //addMacro(10, mod, sizeof(mod));

    /*int noOfKeys = 5;
    int startConst = 39;
    char* modkey = (char*) malloc(noOfKeys * 6);
    for(i = 0; i < noOfKeys; i++){
        modkey[i * 6 + 0] = 0x61;
        modkey[i * 6 + 1] = i+startConst;
        modkey[i * 6 + 2] = 0x62;
        modkey[i * 6 + 3] = i+startConst;
        modkey[i * 6 + 4] = 0x68;
        modkey[i * 6 + 5] = 0x01;
    }
    addMacro(4, modkey, noOfKeys * 6);
    free(modkey);

    //addMacro(9, winaMacro, sizeof(winaMacro));
    */
    if(opt_dryRun != 1){
        updateMapping(epicMouse);
        sleep(2);
        updateMacros(epicMouse);
    }

    printf("Releasing the interface\n");
    libusb_release_interface(epicMouse, 0);

    printf("Packing up and ending the program\n");
    libusb_close(epicMouse);
    libusb_exit(NULL);

    return 0;
}

//send updated info to the Mouse
int updateMapping(libusb_device_handle* hnd){
    int ret;

    printf("Mapping mouse keys\n");
    ret = libusb_control_transfer(hnd,
                                  0x40, //bmRequestType - 0x40 = Host -> Device, Vendor, Device recieves
                                  16, //bRequest - Set key maps
                                  0, //wValue - Could mean anything
                                  0, //wIndex - Could mean anything
                                  keySetArray, //data
                                  sizeof(keySetArray), //wLength
                                  5000); //timeout - set

    if(ret < 1) {
        printf("Error in mapping: %d", ret);
    } else {
        printf("Mapping keys complete (error: %d, positive = no error)...\n", ret);
    }

    return ret;
}

int updateMacros(libusb_device_handle* hnd){
    if(macroRegister[0] <= 0){
        printf("Trying to transfer no macros!\n");
        return -1;
    }

    int ret;

    printf("Transferring macros\n");
    ret = libusb_control_transfer(hnd,
                                  0x40, //bmRequestType - 0x40 = Host -> Device, Vendor, Device recieves
                                  18, //bRequest - upload macros to mouse
                                  0, //wValue - Could mean anything
                                  0, //wIndex - Could mean anything
                                  macroArray, //data
                                  macroRegister[0], //wLength
                                  5000); //timeout - set
    if (ret < 1) {
        printf("Error in transfer: %d", ret);
    } else {
        printf("Completed transfer (%d bytes)\n", ret);
    }

    return ret;
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
int addMacro(int num, char* macro, int len){
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
    char* buildingArray = (char*) malloc(sizeof(char) * newMacroArraySize);
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
    keySet[num] = 1;

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
