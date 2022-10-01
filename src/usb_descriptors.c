/* 
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 * Copyright (c) 2022 TinyVision.ai Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "tusb.h"

#ifndef USB_VID
#define USB_VID 0x1209  // InterBiometrics - https://pid.codes/
#endif

#ifndef USB_PID
#define USB_PID 0x0001  // TEST ONLY, DO NOT USE IN PRODUCTION
#endif

#ifndef USB_MANUFACTURER
#define USB_MANUFACTURER "TinyVision.ai Inc."
#endif

#ifndef USB_PRODUCT
#define USB_PRODUCT "pico-ice"
#endif

#ifndef USB_SERIAL_NUMBER
#define USB_SERIAL_NUMBER "123456"
#endif

enum string_desc {
    STRID_LANGID = 0,
    STRID_MANUFACTURER,
    STRID_PRODUCT,
    STRID_SERIAL_NUMBER,
    STRID_CDC_0,
    STRID_CDC_1,
    STRID_MSC,
    STRID_VENDOR,
};

//--------------------------------------------------------------------+
// Device Descriptors
//--------------------------------------------------------------------+
tusb_desc_device_t const desc_device = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,

    // The class 0xEF means to lookup the class of the Interface instead.
    .bDeviceClass       = TUSB_CLASS_MISC,
    .bDeviceSubClass    = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol    = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,

    .idVendor           = USB_VID,
    .idProduct          = USB_PID,
    .bcdDevice          = 0x0100,

    .iManufacturer      = STRID_MANUFACTURER,
    .iProduct           = STRID_PRODUCT,
    .iSerialNumber      = STRID_SERIAL_NUMBER,

    .bNumConfigurations = 0x01
};

// Invoked when received GET DEVICE DESCRIPTOR
// Application return pointer to descriptor
uint8_t const *
tud_descriptor_device_cb(void)
{
    return (uint8_t const *) &desc_device;
}

//--------------------------------------------------------------------+
// USB HID
//--------------------------------------------------------------------+

uint8_t const desc_hid_report[] = {
    TUD_HID_REPORT_DESC_GENERIC_INOUT(CFG_TUD_HID_BUFSIZE)
};

// Invoked when received GET HID REPORT DESCRIPTOR
// Application return pointer to descriptor
// Descriptor contents must exist long enough for transfer to complete
uint8_t const * tud_hid_descriptor_report_cb(uint8_t itf)
{
    (void) itf;
    return desc_hid_report;
}

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen)
{
    // TODO not Implemented
    (void) itf;
    (void) report_id;
    (void) report_type;
    (void) buffer;
    (void) reqlen;

    return 0;
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize)
{
    // This example doesn't use multiple report and report ID
    (void) itf;
    (void) report_id;
    (void) report_type;
    (void) buffer;
    (void) bufsize;
}

//--------------------------------------------------------------------+
// Configuration Descriptor
//--------------------------------------------------------------------+


enum {
    // CDC UART for Pico
    ITF_NUM_CDC_0 = 0,
    ITF_NUM_CDC_0_DATA,

    // CDC UART for FPGA
    ITF_NUM_CDC_1,
    ITF_NUM_CDC_1_DATA,

    // MSC virtual FAT for UF2
    ITF_NUM_MSC,

    ITF_NUM_TOTAL
};

#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN + TUD_CDC_DESC_LEN + TUD_MSC_DESC_LEN)

// [7] Direction (0=Out 1=In) [6:4] Reserved [3:0] Endpoint Number

#define EPNUM_CDC_0_NOTIF 0x81
#define EPNUM_CDC_0_OUT 0x02
#define EPNUM_CDC_0_IN 0x82

#define EPNUM_CDC_1_NOTIF 0x83
#define EPNUM_CDC_1_OUT 0x04
#define EPNUM_CDC_1_IN 0x84

#define EPNUM_MSC_OUT 0x05
#define EPNUM_MSC_IN 0x85

uint8_t const desc_fs_configuration[] = {
    // Config number, interface count, string index, total length, attribute, power in mA
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0x00, 500),

    // CDC UART PICO: Interface number, string index, EP notification address and size, EP data address (out, in) and size.
    TUD_CDC_DESCRIPTOR(ITF_NUM_CDC_0, STRID_CDC_0, EPNUM_CDC_0_NOTIF, 8, EPNUM_CDC_0_OUT, EPNUM_CDC_0_IN, 64),

    // CDC UART FPGA: Interface number, string index, EP notification address and size, EP data address (out, in) and size.
    TUD_CDC_DESCRIPTOR(ITF_NUM_CDC_1, STRID_CDC_1, EPNUM_CDC_1_NOTIF, 8, EPNUM_CDC_1_OUT, EPNUM_CDC_1_IN, 64),

    // MSC FAT filesystem: Interface number, string index, EP Out & EP In address, EP size
    TUD_MSC_DESCRIPTOR(ITF_NUM_MSC, STRID_MSC, EPNUM_MSC_OUT, EPNUM_MSC_IN, 64),
};

// Per USB specs: high speed capable device must report device_qualifier and other_speed_configuration

uint8_t const desc_hs_configuration[] = {
    // Config number, interface count, string index, total length, attribute, power in mA
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0x00, 100),

    // CDC UART PICO: Interface number, string index, EP notification address and size, EP data address (out, in) and size.
    TUD_CDC_DESCRIPTOR(ITF_NUM_CDC_0, 4, EPNUM_CDC_0_NOTIF, 8, EPNUM_CDC_0_OUT, EPNUM_CDC_0_IN, 512),

    // CDC UART FPGA: Interface number, string index, EP notification address and size, EP data address (out, in) and size.
    TUD_CDC_DESCRIPTOR(ITF_NUM_CDC_1, 4, EPNUM_CDC_1_NOTIF, 8, EPNUM_CDC_1_OUT, EPNUM_CDC_1_IN, 512),

    // MSC FAT filesystem: Interface number, string index, EP Out & EP In address, EP size
    TUD_MSC_DESCRIPTOR(ITF_NUM_MSC, 5, EPNUM_MSC_OUT, EPNUM_MSC_IN, 512),
};

// device qualifier is mostly similar to device descriptor since we don't change configuration based on speed
tusb_desc_device_qualifier_t const desc_device_qualifier = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,

    .bDeviceClass       = TUSB_CLASS_MISC,
    .bDeviceSubClass    = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol    = MISC_PROTOCOL_IAD,

    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
    .bNumConfigurations = 0x01,
    .bReserved          = 0x00
};

// Invoked when received GET DEVICE QUALIFIER DESCRIPTOR request
// Application return pointer to descriptor, whose contents must exist long enough for transfer to complete.
// device_qualifier descriptor describes information about a high-speed capable device that would
// change if the device were operating at the other speed. If not highspeed capable stall this request.
uint8_t const *
tud_descriptor_device_qualifier_cb(void)
{
    return (uint8_t const*) &desc_device_qualifier;
}

// Invoked when received GET OTHER SEED CONFIGURATION DESCRIPTOR request
// Application return pointer to descriptor, whose contents must exist long enough for transfer to complete
// Configuration descriptor in the other speed e.g if high speed then this is for full speed and vice versa
uint8_t const *
tud_descriptor_other_speed_configuration_cb(uint8_t index)
{
    (void) index; // for multiple configurations

    // if link speed is high return fullspeed config, and vice versa
    return (tud_speed_get() == TUSB_SPEED_HIGH) ? desc_fs_configuration : desc_hs_configuration;
}

// Invoked when received GET CONFIGURATION DESCRIPTOR
// Application return pointer to descriptor
// Descriptor contents must exist long enough for transfer to complete
uint8_t const *
tud_descriptor_configuration_cb(uint8_t index)
{
    (void) index; // for multiple configurations

    // Although we are highspeed, host may be fullspeed.
    return (tud_speed_get() == TUSB_SPEED_HIGH) ? desc_hs_configuration : desc_fs_configuration;
}

//--------------------------------------------------------------------+
// String Descriptors
//--------------------------------------------------------------------+

// array of pointer to string descriptors
char const *string_desc_arr[] = {
    [STRID_LANGID]        = (const char[]) { 0x09, 0x04 }, // 0: Supported language is English (0x0409)
    [STRID_MANUFACTURER]  = USB_MANUFACTURER,
    [STRID_PRODUCT]       = USB_PRODUCT,
    [STRID_SERIAL_NUMBER] = USB_SERIAL_NUMBER,
    [STRID_CDC_0]         = "UART serial (rp2040)",
    [STRID_CDC_1]         = "UART serial (ice40)",
    [STRID_MSC]           = "UF2 flashing (ice40)",
    [STRID_VENDOR]        = "TinyVision.ai Inc",
};

// Invoked when received GET STRING DESCRIPTOR request
// Application return pointer to descriptor, whose contents must exist long enough for transfer to complete
uint16_t const *
tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
    static uint16_t utf16[32];
    uint8_t len;

    (void) langid;

    if (index == 0) {
        memcpy(&utf16[1], string_desc_arr[0], 2);
        len = 1;
    } else {
	const char *str;

        // Note: the 0xEE index string is a Microsoft OS 1.0 Descriptors.
        // https://docs.microsoft.com/en-us/windows-hardware/drivers/usbcon/microsoft-defined-usb-descriptors
        if (index >= sizeof(string_desc_arr) / sizeof(*string_desc_arr))
            return NULL;

        str = string_desc_arr[index];

        len = strlen(str);
        if (len > 31)
            len = 31;

        for (uint8_t i = 0; i < len; i++)
            utf16[i + 1] = str[i];
    }

    // first byte is length (including header), second byte is string type
    utf16[0] = (TUSB_DESC_STRING << 8) | (2 * len + 2);

    return utf16;
}