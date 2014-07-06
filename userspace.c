#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <libusb-1.0/libusb.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>

#include "macro.h"
#include "parser.h"

//send updated info to the Mouse
int updateMapping(libusb_device_handle* hnd);
int updateMacros(libusb_device_handle* hnd);

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

//Am I reading a set of macros from a file? Probably, but assume no
int opt_file = 0;
char* filename;
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
            {"help",             no_argument,       0, 'h'},
            {"verbose",          no_argument,       0, 'v'},
            {"verbose-keyarray", no_argument,       0, 'k'},
            {"verbose-macros",   no_argument,       0, 'm'},
            {"dry",              no_argument,       0, 'd'},
            {"file",             required_argument, 0, 'f'},
            {0, 0, 0, 0}
        };
        /* getopt_long stores the option index here. */
        int option_index = 0;

        c = getopt_long (argc, argv, "hvdkmf:",
                         long_options, &option_index);

        /* Detect the end of the options. */
        if (c == -1) break;

        switch (c) {
        case 'h':
            printf("[h]elp, [v]erbose, verbose-[k]eyarray, verbose-[m]acros, [d]ry and [f]ile (needs an argument)\n");
            printf("Verbose implies verbose-macros and verbose-keyarray\n");
            break;

        case 'd':
            opt_dryRun = 1;
            break;

        case 'v':
            opt_verbose = 1;
            opt_printKeyArray = 1;
            opt_printMacroArray = 1;
            break;

        case 'f':
            opt_file = 1;
            filename = strdup(optarg);
            if(filename == NULL) {
                printf("Error parsing file name");
                exit(1);
            }
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

    libusb_device_handle* epicMouse;
    if(opt_dryRun == 0){
        //libusb_context* context = NULL;
        ret = libusb_init(NULL);

        //libusb_set_debug(NULL, LIBUSB_LOG_LEVEL_DEBUG);
        libusb_set_debug(NULL, LIBUSB_LOG_LEVEL_WARNING);

        epicMouse = libusb_open_device_with_vid_pid(NULL, vendor_id, product_id);
        err = diagnoseDevice(epicMouse);
        if(err) printf("Error printing out diagnostic info");

        printf("Detaching Kernel driver. Good luck\n");
        libusb_set_auto_detach_kernel_driver(epicMouse, 1);

        printf("Claiming the interface...\n");
        libusb_claim_interface(epicMouse, 0);
    }

    uint8_t aMacro[] = {0x61, 0x04, 0x62, 0x04, 0x68, 0x01};
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
    uint8_t altTemplate[] = {0x61, 0x04, 0x68, 0x01, 0x62, 0x04, 0x68, 0x01};
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

    if(opt_file == 1){
        FILE* fp;
        char line[PARSER_BUFFER];
        int rVal;
        fp = fopen(filename, "r");
        while(fgets(line, PARSER_BUFFER, fp) != NULL){
            parse(line);
        }
    }

    if(opt_dryRun == 0){
        updateMapping(epicMouse);
        sleep(2);
        updateMacros(epicMouse);

        printf("Releasing the interface\n");
        libusb_release_interface(epicMouse, 0);

        printf("Packing up and ending the program\n");
        libusb_close(epicMouse);
        libusb_exit(NULL);
    }

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
                                  keySetArray_size, //wLength
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
