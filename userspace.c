#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <libusb-1.0/libusb.h>

//Default: Set l-click, r-click and wheel but nothing else
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
                       -1, -1, -1, -1, -1
                       -1, -1, -1, -1, -1
                       -1, -1, -1, -1 };
char* macroArray = NULL;

//These update macroArray and macroRegister
int addMacro(int num, char* macro, int len);
int delMacro(int num);
//This is a convenience function to compare entries in macroRegister
int sizeMacro(int num);

//send updated info to the Mouse
int updateMapping(libusb_device_handle* hnd);
int updateMacros(libusb_device_handle* hnd);

char otherArray[] = { 0x61, 0xe3, 0x61, 0x1a, 0x68, 0x01, 0x62, 0x1a, 0x68, 0x01, 0x62, 0xe3, 0x68, 0x01,
                      0x61, 0xe3, 0x61, 0x12, 0x68, 0x01, 0x62, 0x12, 0x68, 0x01, 0x62, 0xe3, 0x68, 0x01, 0x2b,
                      0x61, 0xe3, 0x61, 0x15, 0x68, 0x01, 0x62, 0x15, 0x68, 0x01, 0x62, 0xe3, 0x68, 0x01,
                      0x61, 0xe3, 0x61, 0x15, 0x68, 0x01, 0x62, 0x15, 0x68, 0x01, 0x62, 0xe3, 0x68, 0x01, 0x17, 0x3a,
                      0x61, 0xe1, 0x61, 0x10, 0x62, 0x10, 0x62, 0xe1, 0x09, 0x05, 0x10 };

/* This array is the data that should set most buttons to AAAA C-A M-A Win-A LShift-A*/
char myArray[] = {0x61, 0x04, 0x62, 0x04, 0x68, 0x01, 0x61, 0x04, 0x68, 0x01, 0x62, 0x04, 0x68, 0x01, 0x61, 0x04,
                  0x68, 0x01, 0x62, 0x04, 0x68, 0x01, 0x61, 0x04, 0x68, 0x01, 0x62, 0x04, 0x68, 0x01, 0x61, 0x04,
                  0x68, 0x01, 0x62, 0x04, 0x68, 0x01, //aaaa

                  //C-a
                  0x61, 0xe0, 0x68, 0x01, 0x61, 0x04, 0x68, 0x01, 0x62, 0x04, 0x68, 0x01, 0x62, 0xe0, 0x68, 0x01,
                  //M-a
                  0x61, 0xe3, 0x68, 0x01, 0x61, 0x04, 0x68, 0x01, 0x62, 0xe3, 0x68, 0x01, 0x62, 0x04, 0x68, 0x01,
                  //Win-a
                  0x61, 0xe1, 0x68, 0x01, 0x61, 0x04, 0x68, 0x01, 0x62, 0xe1, 0x68, 0x01, 0x62, 0x04, 0x68, 0x01,
                  //LShift-a
                  0x61, 0xe2, 0x68, 0x01, 0x61, 0x04, 0x68, 0x01, 0x62, 0x04, 0x68, 0x01, 0x62, 0xe2, 0x68, 0x01,
};

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

int main(){
    uint16_t vendor_id = 0x1038; //8087
    uint16_t product_id = 0x1320; //0024

    int err;
    int ret;
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
    char bMacro[] = {0x61, 0x05, 0x62, 0x05, 0x68, 0x01};
    char winaMacro[] = {0x61, 0xe1, 0x68, 0x01, 0x61, 0x04, 0x68, 0x01, 0x62, 0xe1, 0x68, 0x01, 0x62, 0x04, 0x68, 0x01};
    addMacro(5, aMacro, sizeof(aMacro));
    addMacro(6, bMacro, sizeof(bMacro));
    addMacro(9, winaMacro, sizeof(winaMacro));

    updateMapping(epicMouse);
    updateMacros(epicMouse);

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
    if((num > 14) || (num < 3)){
        printf("Sizing invalid macro");
        return 0;
    }
    if(num = 14){
        return macroRegister[0] - macroRegister[14];
    }
    return macroRegister[num+1] - macroRegister[num];
}

//These update macroArray and macroRegister
int addMacro(int num, char* macro, int len){
    int newMacroArraySize = macroRegister[0] - macroRegister[num] + len;

    //allocate new array
    char* buildingArray = (char*) malloc(sizeof(char) * newMacroArraySize);

    //copy macros up to new macro
    int i, j; //j counts how far into the building array we are
    for(i = 0; i < macroRegister[num]; i++, j++){
        buildingArray[j] = macroArray[i];
    }

    //copy macros after new macro
    for(i = macroRegister[num+1]; i < macroRegister[0]; i++, j++){
        buildingArray[j] = macroArray[i];
    }

    //copy new macro
    for(i = 0; i < len; i++, j++){
        buildingArray[j] = macro[i];
    }

    //update register
    int offset = sizeMacro(num);
    for(i = num+1; i < 15; i++){
        macroRegister[i] = macroRegister[i] - offset;
    }
    macroRegister[num] = newMacroArraySize - len - 1;
    macroRegister[0] = newMacroArraySize;

    if(macroArray != NULL) free(macroArray);
    macroArray = buildingArray;

    for(i = 4; i < 15; i++){
        char n[] = {0x00, 0x00, 0x00, 0x00, 0x00};
        char m[] = {0x20, 0x00, 0x00, 0x00, 0x06};
        if(sizeMacro(macroRegister[i]) == 0){
            maintainKeyArray(i, n, 5);
        } else {
            m[3] = macroRegister[i];
            m[5] = sizeMacro(i);
            maintainKeyArray(i, m, 5);
        }

    }

    return 0;
}

int delMacro(int num){
    return addMacro(num, NULL, 0);
}
