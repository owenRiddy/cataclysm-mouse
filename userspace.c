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
        printf("Error - SteelSeries World of Warcraft: Cataclysm MMO Gaming Mouse not found\n");
        return 1;
    } else {
        printf("SteelSeries World of Warcraft: Cataclysm MMO Gaming Mouse was located\n");
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
            printf("Changes will not be written to the mouse\n");
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

    libusb_device_handle* mouse;
    if(opt_dryRun == 0){
        //libusb_context* context = NULL;
        ret = libusb_init(NULL);

        //libusb_set_debug(NULL, LIBUSB_LOG_LEVEL_DEBUG);
        libusb_set_debug(NULL, LIBUSB_LOG_LEVEL_WARNING);

        mouse = libusb_open_device_with_vid_pid(NULL, vendor_id, product_id);
        err = diagnoseDevice(mouse);
        if(err) printf("Error printing out diagnostic info");

        printf("Detaching Kernel driver. Good luck\n");
        libusb_set_auto_detach_kernel_driver(mouse, 1);

        printf("Claiming the interface...\n");
        libusb_claim_interface(mouse, 0);
    }

    if(opt_file == 1){
        FILE* fp;
        char line[PARSER_BUFFER];
        int rVal;
        fp = fopen(filename, "r");
        while(fgets(line, PARSER_BUFFER, fp) != NULL){
            parse(line);
        }
    } else {
        printf("No config file, resetting mouse (maybe try running this with --help?)\n");
    }

    if(opt_dryRun == 0){
        updateMapping(mouse);
        sleep(2);
        updateMacros(mouse);

        printf("Releasing the interface (return control to kernel)\n");
        libusb_release_interface(mouse, 0);

        printf("Packing up and ending the program\n");
        libusb_close(mouse);
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
