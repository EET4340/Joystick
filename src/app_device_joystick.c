/*******************************************************************************
Copyright 2016 Microchip Technology Inc. (www.microchip.com)

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

To request to license the code under the MLA license (www.microchip.com/mla_license), 
please contact mla_licensing@microchip.com
 *******************************************************************************/

#ifndef USBJOYSTICK_C
#define USBJOYSTICK_C

/** INCLUDES *******************************************************/
#include "usb/usb.h"
#include "usb/usb_device_hid.h"
#include "system.h"
#include "stdint.h"

/** TYPE DEFINITIONS ************************************************/
typedef union _INTPUT_CONTROLS_TYPEDEF {
    struct {
        uint8_t throttle;
        uint16_t X;
        uint16_t Y;
        uint8_t button1 : 1;
        uint8_t button2 : 1;
        uint8_t button3 : 1;
        uint8_t button4 : 1;
        uint8_t button5 : 1;
        uint8_t button6 : 1;
        uint8_t button7 : 1;
        uint8_t button8 : 1;
        
    };
    uint8_t bytes[6];
} INPUT_CONTROLS;


/** VARIABLES ******************************************************/
/* Some processors have a limited range of RAM addresses where the USB module
 * is able to access.  The following section is for those devices.  This section
 * assigns the buffers that need to be used by the USB module into those
 * specific areas.
 */
#if defined(FIXED_ADDRESS_MEMORY)
#if defined(COMPILER_MPLAB_C18)
#pragma udata JOYSTICK_DATA=JOYSTICK_DATA_ADDRESS
INPUT_CONTROLS joystick_input;
#pragma udata
#elif defined(__XC8)
INPUT_CONTROLS joystick_input JOYSTICK_DATA_ADDRESS;
#endif
#else
INPUT_CONTROLS joystick_input;
#endif


unsigned int readAnalogInput(int channel);

USB_VOLATILE USB_HANDLE lastTransmission = 0;

/*********************************************************************
 * Function: void APP_DeviceJoystickInitialize(void);
 *
 * Overview: Initializes the demo code
 *
 * PreCondition: None
 *
 * Input: None
 *
 * Output: None
 *
 ********************************************************************/
void APP_DeviceJoystickInitialize(void) {
    //initialize the variable holding the handle for the last
    // transmission
    lastTransmission = 0;

    //enable the HID endpoint
    USBEnableEndpoint(JOYSTICK_EP, USB_IN_ENABLED | USB_HANDSHAKE_ENABLED | USB_DISALLOW_SETUP);
    //Setup digital pins for buttons
    ANSELB = 0; // all port B digital
    TRISB = 0xff;
    WPUB = 0xff;
    INTCON2bits.RBPU = 0;
    //setup analog pins for pots
    ANSELAbits.ANSA0 = 1;
    ANSELAbits.ANSA1 = 1;
    TRISAbits.RA0 = 1;
    TRISAbits.RA1 = 1;
    //setup ADC
    ADCON2bits.ADFM = 1; //Right justify
    ADCON2bits.ADCS = 0b110; //Fosc / 64 = ~1.3us
    ADCON2bits.ACQT = 0b100; //6 TAD = ~7.8us
    ADCON0bits.CHS = 0;
    ADCON0bits.ADON = 1;
}//end UserInit

/*********************************************************************
 * Function: void APP_DeviceJoystickTasks(void);
 *
 * Overview: Keeps the demo running.
 *
 * PreCondition: The demo should have been initialized and started via
 *   the APP_DeviceJoystickInitialize() and APP_DeviceJoystickStart() demos
 *   respectively.
 *
 * Input: None
 *
 * Output: None
 *
 ********************************************************************/
void APP_DeviceJoystickTasks(void) {
    /* If the USB device isn't configured yet, we can't really do anything
     * else since we don't have a host to talk to.  So jump back to the
     * top of the while loop. */
    if (USBGetDeviceState() < CONFIGURED_STATE) {
        /* Jump back to the top of the while loop. */
        return;
    }

    /* If we are currently suspended, then we need to see if we need to
     * issue a remote wakeup.  In either case, we shouldn't process any
     * keyboard commands since we aren't currently communicating to the host
     * thus just continue back to the start of the while loop. */
    if (USBIsDeviceSuspended() == true) {
        /* Jump back to the top of the while loop. */
        return;
    }

    //If the last transmission is complete
    if (!HIDTxHandleBusy(lastTransmission)) {
        //update controls
        if (PORTBbits.RB5 == 0) {
            joystick_input.button1 = 1;
        } else {
            joystick_input.button1 = 0;
        }
        joystick_input.button2 = 0;
        joystick_input.button3 = 0;
        joystick_input.button4 = 0;
        int position;
        position = readAnalogInput(0);
        joystick_input.X = position - 512;
        joystick_input.Y = 0;
        position = readAnalogInput(1);
        position /= 4;
        position -= 128;
        if (position == -128) {
            position = -127;
        }
        joystick_input.throttle = position;
        //Send the packet over USB to the host.
        lastTransmission = HIDTxPacket(JOYSTICK_EP, (uint8_t*) & joystick_input, sizeof (joystick_input));
    }

}//end ProcessIO

unsigned int readAnalogInput(int channel) {
    ADCON0bits.CHS = channel & 0x1f;
    ADCON0bits.GO = 1;
    while (ADCON0bits.GO == 1);
    return *(unsigned int *)&ADRESL;
}
#endif
