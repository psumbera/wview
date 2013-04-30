/*---------------------------------------------------------------------------
 
  FILENAME:
        hidapi-solaris.c
 
  PURPOSE:
        Provide an adaptation of the hidapi utilities.
 
  NOTES:
        Modified version of hidapi-linux.c just to accommodate minimal use with
        Solaris and wview daemon (tested with WMR88A). It begs for cleaning up
        and finishing.
 
  LICENSE:
        Copyright (c) 2011, Mark S. Teel (mteel2005@gmail.com)
  
        This source code is released for free distribution under the terms 
        of the GNU General Public License.
  
----------------------------------------------------------------------------*/
/* ******************************************************
 HIDAPI - Multi-Platform library for
 communication with HID devices.
 
 Alan Ott
 Signal 11 Software
 
 8/22/2009
 Linux Version - 6/2/2010
 Libusb Version - 8/13/2010
 
 Copyright 2009, All Rights Reserved.
 
 This software may be used by anyone for any reason so
 long as this copyright notice remains intact.
 ********************************************************/

#if defined(__linux__) || defined(__FreeBSD__)
  #ifdef __FreeBSD__
    #if __FreeBSD__ >= 8
      #define INCLUDE_SOURCE
    #else
      #undef INCLUDE_SOURCE
    #endif
  #else
    #define INCLUDE_SOURCE
  #endif
#else
  #undef INCLUDE_SOURCE
#endif

#define INCLUDE_SOURCE

#ifdef INCLUDE_SOURCE
// libusb supported, proceed:


/* C */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <locale.h>
#include <errno.h>

/* Unix */
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/utsname.h>
#include <fcntl.h>
#include <pthread.h>

/* GNU / LibUSB */
#ifdef __FreeBSD__
#include "libusb.h"
#else
#include "usb.h"
#endif
#include "iconv.h"

#include "hidapi.h"

/* Linked List of input reports received from the device. */
struct input_report {
    uint8_t     *data;
    size_t      len;
    struct input_report *next;
};


struct hid_device_ {
    /* Handle to the actual device. */
    usb_dev_handle *device_handle;

    /* Endpoint information */
    int input_endpoint;
    int output_endpoint;
    int input_ep_max_packet_size;

    /* The interface number of the HID */
    int interface;

    /* Indexes of Strings */
    int manufacturer_index;
    int product_index;
    int serial_index;

    /* Whether blocking reads are used */
    int blocking; /* boolean */

    /* List of received input reports. */
    struct input_report *input_reports;
};

static int initialized = 0;

static int return_data(hid_device *dev, unsigned char *data, size_t length);

hid_device *new_hid_device()
{
    hid_device *dev = calloc(1, sizeof(hid_device));
    dev->device_handle = NULL;
    dev->input_endpoint = 0;
    dev->output_endpoint = 0;
    dev->input_ep_max_packet_size = 0;
    dev->interface = 0;
    dev->manufacturer_index = 0;
    dev->product_index = 0;
    dev->serial_index = 0;
    dev->blocking = 0;
    dev->input_reports = NULL;

    return dev;
}

static void register_error(hid_device *device, const char *op)
{

}

/* Get the first language the device says it reports. This comes from
   USB string #0. */
static uint16_t get_first_language(usb_dev_handle *dev)
{
    uint16_t    buf[32];
    int         len;

    return 0;
}


static char *make_path(struct usb_device *dev, int interface_number)
{
    char str[64];
    snprintf(str, sizeof(str), "%04x:%04x:%02x",
             0 /* XXX libusb_get_bus_number(dev)*/,
             0 /* XXX libusb_get_device_address(dev)*/,
             interface_number);
    str[sizeof(str)-1] = '\0';

    return strdup(str);
}

struct hid_device_info  HID_API_EXPORT *hid_enumerate(uint16_t vendor_id, uint16_t product_id)
{
    return NULL;
}

void  HID_API_EXPORT hid_free_enumeration(struct hid_device_info *devs)
{
    struct hid_device_info *d = devs;
    while (d)
    {
        struct hid_device_info *next = d->next;
        if (d->path)
            free(d->path);
        free(d);
        d = next;
    }
}

hid_device * hid_open(uint16_t vendor_id, uint16_t product_id)
{
    struct hid_device_info *devs, *cur_dev;
    hid_device *handle = NULL;

    struct usb_bus *busses;
    struct usb_bus *bus;
    struct usb_device *dev;
    int c, i, res;
    char data[337];
    char idata[10];

    usb_init();
    usb_find_busses();
    usb_find_devices();
    busses = usb_get_busses();
    for (bus = busses; bus; bus = bus->next) {

        for (dev = bus->devices; dev; dev = dev->next) {

           if(dev->descriptor.idProduct == product_id && dev->descriptor.idVendor == vendor_id) {

              handle = calloc(1, sizeof(hid_device));

              handle->device_handle  = usb_open( dev);
	      handle->output_endpoint = 0;
	      handle->input_endpoint = 1;

              if( NULL == handle->device_handle ){
                printf( "usb_open(): no handle to device\n" );
		return NULL;
              }
              res = usb_claim_interface(handle->device_handle, 0);
              if(res  < 0) {
                printf("Device interface not available to be claimed! \n");
		return NULL;
              }
	   }
        }
    }

    return handle;
}

hid_device * HID_API_EXPORT hid_open_path(const char *path)
{
#if 0 
    hid_device *dev = NULL;

    dev = new_hid_device();

    libusb_device **devs;
    libusb_device *usb_dev;
    ssize_t num_devs;
    int res;
    int i = 0;
    int good_open = 0;

    setlocale(LC_ALL,"");

    if (!initialized)
    {
        libusb_init(NULL);
        initialized = 1;
    }

    num_devs = libusb_get_device_list(NULL, &devs);
    while ((usb_dev = devs[i++]) != NULL)
    {
        struct libusb_device_descriptor desc;
        struct libusb_config_descriptor *conf_desc = NULL;
        int i,j,k;
        libusb_get_device_descriptor(usb_dev, &desc);

        libusb_get_active_config_descriptor(usb_dev, &conf_desc);
        for (j = 0; j < conf_desc->bNumInterfaces; j++)
        {
            const struct libusb_interface *intf = &conf_desc->interface[j];
            for (k = 0; k < intf->num_altsetting; k++)
            {
                const struct libusb_interface_descriptor *intf_desc;
                intf_desc = &intf->altsetting[k];
                if (intf_desc->bInterfaceClass == LIBUSB_CLASS_HID)
                {
                    char *dev_path = make_path(usb_dev, intf_desc->bInterfaceNumber);
                    if (!strcmp(dev_path, path))
                    {
                        /* Matched Paths. Open this device */

                        // OPEN HERE //
                        res = libusb_open(usb_dev, &dev->device_handle);
                        if (res < 0)
                        {
                            printf("can't open device\n");
                            break;
                        }
                        good_open = 1;
                        res = libusb_detach_kernel_driver(dev->device_handle, intf_desc->bInterfaceNumber);
                        if (res < 0)
                        {
                            //printf("Unable to detach. Maybe this is OK\n");
                        }

                        res = libusb_claim_interface(dev->device_handle, intf_desc->bInterfaceNumber);
                        if (res < 0)
                        {
                            printf("can't claim interface %d: %d\n", intf_desc->bInterfaceNumber, res);
                            libusb_close(dev->device_handle);
                            good_open = 0;
                            break;
                        }

                        /* Store off the string descriptor indexes */
                        dev->manufacturer_index = desc.iManufacturer;
                        dev->product_index      = desc.iProduct;
                        dev->serial_index       = desc.iSerialNumber;

                        /* Store off the interface number */
                        dev->interface = intf_desc->bInterfaceNumber;

                        /* Find the INPUT and OUTPUT endpoints. An
                           OUTPUT endpoint is not required. */
                        for (i = 0; i < intf_desc->bNumEndpoints; i++)
                        {
                            const struct libusb_endpoint_descriptor *ep
                                        = &intf_desc->endpoint[i];

                            /* Determine the type and direction of this
                               endpoint. */
                            int is_interrupt =
                                (ep->bmAttributes & LIBUSB_TRANSFER_TYPE_MASK)
                                == LIBUSB_TRANSFER_TYPE_INTERRUPT;
                            int is_output =
                                (ep->bEndpointAddress & LIBUSB_ENDPOINT_DIR_MASK)
                                == LIBUSB_ENDPOINT_OUT;
                            int is_input =
                                (ep->bEndpointAddress & LIBUSB_ENDPOINT_DIR_MASK)
                                == LIBUSB_ENDPOINT_IN;

                            /* Decide whether to use it for intput or output. */
                            if (dev->input_endpoint == 0 &&
                                    is_interrupt && is_input)
                            {
                                /* Use this endpoint for INPUT */
                                dev->input_endpoint = ep->bEndpointAddress;
                                dev->input_ep_max_packet_size = ep->wMaxPacketSize;
                            }
                            if (dev->output_endpoint == 0 &&
                                    is_interrupt && is_output)
                            {
                                /* Use this endpoint for OUTPUT */
                                dev->output_endpoint = ep->bEndpointAddress;
                            }
                        }

                        pthread_create(&dev->thread, NULL, read_thread, dev);

                    }
                    free(dev_path);
                }
            }
            intf++;
        }
        libusb_free_config_descriptor(conf_desc);

    }

    libusb_free_device_list(devs, 1);

    // If we have a good handle, return it.
    if (good_open)
    {
        return dev;
    }
    else
    {
        // Unable to open any devices.
        free(dev);
        return NULL;
    }
#endif
}


int HID_API_EXPORT hid_write(hid_device *dev, const unsigned char *data, size_t length)
{
    int res;
    int report_number = data[0];
    int skipped_report_id = 0;

    if (report_number == 0x0)
    {
        data++;
        length--;
        skipped_report_id = 1;
    }


    if (dev->output_endpoint <= 0)
    {
        /* No interrput out endpoint. Use the Control Endpoint */
        res = usb_control_msg(dev->device_handle,
                                      USB_TYPE_CLASS|USB_RECIP_INTERFACE,
                                      0x09/*HID Set_Report*/,
                                      (2/*HID output*/ << 8) | report_number,
                                      dev->interface,
                                      (unsigned char *)data, length,
                                      1000/*timeout millis*/);

        if (res < 0)
            return -1;

        if (skipped_report_id)
            length++;

        return length;
    }
    else
    {
#if 0
        /* Use the interrupt out endpoint */
        int actual_length;
        res = usb_interrupt_write(dev->device_handle,
                                        dev->output_endpoint,
                                        (unsigned char*)data,
                                        length,
                                        &actual_length, 1000);

        if (res < 0)
            return -1;

        if (skipped_report_id)
            actual_length++;

        return actual_length;
#endif
    }
}

int HID_API_EXPORT hid_read(hid_device *dev, unsigned char *data, size_t length)
{
    int bytes_read = -1;

    bytes_read = usb_interrupt_read(dev->device_handle, 0x81, data, length, -1);

    return bytes_read;
}

int HID_API_EXPORT hid_set_nonblocking(hid_device *dev, int nonblock)
{
    dev->blocking = !nonblock;

    return 0;
}


int HID_API_EXPORT hid_send_feature_report(hid_device *dev, const unsigned char *data, size_t length)
{
    int res = -1;
    int skipped_report_id = 0;
    int report_number = data[0];
    
    if (report_number == 0x0)
    {
        data++;
        length--;
        skipped_report_id = 1;
    }

    res = usb_control_msg(dev->device_handle,
                               USB_TYPE_CLASS|USB_RECIP_INTERFACE,
                               0x09/*HID set_report*/,
                               (3/*HID feature*/ << 8) | report_number,
                               dev->interface,
                               (unsigned char *)data, length,
                               1000/*timeout millis*/);

    if (res < 0)
        return -1;

    /* Account for the report ID */
    if (skipped_report_id)
        length++;

    return length;
}

int HID_API_EXPORT hid_get_feature_report(hid_device *dev, unsigned char *data, size_t length)
{
     int res = -1;
     int skipped_report_id = 0;
     int report_number = data[0];
    
     if (report_number == 0x0)
     {
        /* Offset the return buffer by 1, so that the report ID
           will remain in byte 0. */
        data++;
        length--;
        skipped_report_id = 1;
     }
     res = usb_control_msg(dev->device_handle,
                               USB_TYPE_CLASS|USB_RECIP_INTERFACE,
                               0x01/*HID get_report*/,
                               (3/*HID feature*/ << 8) | report_number,
                               dev->interface,
                               (unsigned char *)data, length,
                               1000/*timeout millis*/);

     if (res < 0)
         return -1;

     if (skipped_report_id)
         res++;

     return res;
}


void HID_API_EXPORT hid_close(hid_device *dev)
{
    if (!dev)
        return;

    /* Close the handle */
    usb_close(dev->device_handle);

    free(dev);
}


HID_API_EXPORT const wchar_t * HID_API_CALL  hid_error(hid_device *dev)
{
    return NULL;
}


#endif  // INCLUDE_SOURCE

