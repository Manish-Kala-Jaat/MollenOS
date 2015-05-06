/* MollenOS
*
* Copyright 2011 - 2014, Philip Meulengracht
*
* This program is free software : you can redistribute it and / or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation ? , either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.If not, see <http://www.gnu.org/licenses/>.
*
*
* MollenOS X86-32 USB Core Driver
*/

#ifndef X86_USB_H_
#define X86_USB_H_

/* Includes */
#include <crtdefs.h>
#include <stdint.h>

/* Definitions */
#define X86_USB_CORE_MAX_PORTS	16
#define X86_USB_CORE_MAX_IF		4
#define X86_USB_CORE_MAX_EP		16

#define X86_USB_TYPE_OHCI		0
#define X86_USB_TYPE_UHCI		1
#define X86_USB_TYPE_EHCI		2
#define X86_USB_TYPE_XHCI		3

/* Usb Devices Class Codes */
#define X86_USB_CLASS_IF			0x00	/* Get from Interface */
#define X86_USB_CLASS_AUDIO			0x01
#define X86_USB_CLASS_CDC			0x02
#define X86_USB_CLASS_HID			0x03
#define X86_USB_CLASS_PHYSICAL		0x05
#define X86_USB_CLASS_IMAGE			0x06
#define X86_USB_CLASS_PRINTER		0x07
#define X86_USB_CLASS_MSD			0x08
#define X86_USB_CLASS_HUB			0x09
#define X86_USB_CLASS_CDC_DATA		0x0A
#define X86_USB_CLASS_SMART_CARD	0x0B
#define X86_USB_CLASS_SECURITY		0x0D
#define X86_USB_CLASS_VIDEO			0x0E
#define X86_USB_CLASS_HEALTHCARE	0x0F
#define X86_USB_CLASS_DIAGNOSIS		0xDC
#define X86_USB_CLASS_WIRELESS		0xE0
#define X86_USB_CLASS_MISC			0xEF
#define X86_USB_CLASS_APP_SPECIFIC	0xFE
#define X86_USB_CLASS_VENDOR_SPEC	0xFF

/* Packet Request Targets */

/* Bit 7 */
#define X86_USB_REQ_DIRECTION_IN		0x80
#define X86_USB_REQ_DIRECTION_OUT		0x0

/* Bit 5-6 */
#define X86_USB_REQ_TARGET_CLASS		0x20

/* Bits 0-4 */
#define X86_USB_REQ_TARGET_DEVICE		0x0
#define X86_USB_REQ_TARGET_INTERFACE	0x1
#define X86_USB_REQ_TARGET_ENDPOINT		0x2
#define X86_USB_REQ_TARGET_OTHER		0x3

/* Packet Request Types */
#define X86_USB_REQ_GET_STATUS		0x00
#define X86_USB_REQ_CLR_FEATURE		0x01
#define X86_USB_REQ_SET_FEATURE		0x03
#define X86_USB_REQ_SET_ADDR		0x05
#define X86_USB_REQ_GET_DESC		0x06
#define X86_USB_REQ_SET_DESC		0x07
#define X86_USB_REQ_GET_CONFIG		0x08
#define X86_USB_REQ_SET_CONFIG		0x09
#define X86_USB_REQ_GET_INTERFACE	0x0A
#define X86_USB_REQ_SET_INTERFACE	0x0B
#define X86_USB_REQ_SYNC_FRAME		0x0C

/* Descriptor Types */
#define X86_USB_DESC_TYPE_DEVICE	0x01
#define X86_USB_DESC_TYPE_CONFIG	0x02
#define X86_USB_DESC_TYPE_STRING	0x03
#define X86_USB_DESC_TYPE_INTERFACE	0x04 //Interface
#define X86_USB_DESC_TYPE_ENDP		0x05
#define X86_USB_DESC_TYPE_DEV_QAL	0x06 //DEVICE QUALIFIER
#define X86_USB_DESC_TYPE_OSC		0x07 //Other Speed Config
#define X86_USB_DESC_TYPE_IF_PWR	0x08	//Interface Power

/* Structures */
#pragma pack(push, 1)
typedef struct _UsbPacket
{
	/* Request Direction (Bit 7: 1 -> IN, 0 - OUT) 
	 *                   (Bit 0-4: 0 -> Device, 1 -> Interface, 2 -> Endpoint, 3 -> Other, 4... Reserved) */
	uint8_t Direction;

	/* Request Type (see above) */
	uint8_t Type;

	/* Request Value */
	uint8_t ValueLo;
	uint8_t ValueHi;

	/* Request Index */
	uint16_t Index;

	/* Length */
	uint16_t Length;

} UsbPacket_t;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct _UsbDeviceDescriptor
{
	/* Descriptor Length (Bytes) */
	uint8_t Length;

	/* Descriptor Type */
	uint8_t Type;

	/* USB Release Number in Binary-Coded Decimal (i.e, 2.10 is expressed as 210h) */
	uint16_t UsbRnBcd;

	/* USB Class Code (USB-IF) */
	uint8_t Class;

	/* Usb Subclass Code (USB-IF) */
	uint8_t Subclass;

	/* Device Protocol Code (USB-IF) */
	uint8_t Protocol;

	/* Max packet size for endpoing zero (8, 16, 32, or 64 are the only valid options) */
	uint8_t MaxPacketSize;

	/* Vendor Id */
	uint16_t VendorId;

	/* Product Id */
	uint16_t ProductId;

	/* Device Release Number in Binary-Coded Decimal (i.e, 2.10 is expressed as 210h) */
	uint16_t DeviceRnBcd;

	/* String Descriptor Indexes */
	uint8_t StrIndexManufactor;
	uint8_t StrIndexProduct;
	uint8_t StrIndexSerialNum;

	/* Number of Configuration */
	uint8_t NumConfigurations;

} UsbDeviceDescriptor_t;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct _UsbConfigDescriptor
{
	/* Descriptor Length (Bytes) */
	uint8_t Length;

	/* Descriptor Type */
	uint8_t Type;

	/* The total combined length in bytes of all
	 * the descriptors returned with the request
	 * for this CONFIGURATION descriptor */
	uint16_t TotalLength;

	/* Number of Interfaces */
	uint8_t NumInterfaces;

	/* Configuration */
	uint8_t ConfigurationValue;

	/* String Index */
	uint8_t	StrIndexConfiguration;

	/* Attributes 
	 * Bit 6: 0 - Selfpowered, 1 - Local Power Source 
	 * Bit 7: 1 - Remote Wakeup Support */
	uint8_t Attributes;

	/* Power Consumption 
	 * Expressed in units of 2mA (i.e., a value of 50 in this field indicates 100mA) */
	uint8_t MaxPowerConsumption;

} UsbConfigDescriptor_t;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct _UsbInterfaceDescriptor
{
	/* Descriptor Length (Bytes) */
	uint8_t Length;

	/* Descriptor Type */
	uint8_t Type;

	/* Number of Interface */
	uint8_t NumInterface;

	/* Alternative Setting */
	uint8_t AlternativeSetting;

	/* Number of Endpoints other than endpoint zero */
	uint8_t NumEndpoints;
	
	/* USB Class Code (USB-IF) */
	uint8_t Class;

	/* Usb Subclass Code (USB-IF) */
	uint8_t Subclass;

	/* Device Protocol Code (USB-IF) */
	uint8_t Protocol;

	/* String Index */
	uint8_t StrIndexInterface;

} UsbInterfaceDescriptor_t;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct _UsbEndpointDescriptor
{
	/* Descriptor Length (Bytes) */
	uint8_t Length;

	/* Descriptor Type */
	uint8_t Type;

	/* Address 
	 * Bits 0-3: Endpoint Number
	 * Bit 7: 1 - In, 0 - Out */
	uint8_t Address;

	/* Attributes 
	 * Bits 0-1: Xfer Type (00 control, 01 isosync, 10 bulk, 11 interrupt) 
	 * Bits 2-3: Sync Type (00 no sync, 01 async, 10 adaptive, 11 sync) 
	 * Bits 4-5: Usage Type (00 data, 01 feedback) */
	uint8_t Attributes;

	/* Maximum Packet Size (Bits 0-10) (Bits 11-15: 0) */
	uint16_t MaxPacketSize;

	/* Interval */
	uint8_t Interval;

} UsbEndpointDescriptor_t;
#pragma pack(pop)

/* The Abstract Usb Endpoint */
typedef struct _UsbHcEndpoint
{
	/* Type */
	uint32_t Type;

	/* Address */
	uint32_t Address;

	/* Direction (IN, OUT) */
	uint32_t Direction;

	/* Max Packet Size (Always 64 bytes, almost) */
	uint32_t MaxPacketSize;

	/* Bandwidth */
	uint32_t Bandwidth;

	/* Data Toggle */
	uint32_t Toggle;

	/* Poll Interval */
	uint32_t Interval;

} UsbHcEndpoint_t;

#define X86_USB_EP_DIRECTION_IN		0x0
#define X86_USB_EP_DIRECTION_OUT	0x1
#define X86_USB_EP_DIRECTION_BOTH	0x2

#define X86_USB_EP_TYPE_CONTROL		0x0
#define X86_USB_EP_TYPE_ISOCHRONOUS	0x1
#define X86_USB_EP_TYPE_BULK		0x2
#define X86_USB_EP_TYPE_INTERRUPT	0x3

/* The Abstract Usb Interface */
typedef struct _UsbHcInterface
{
	/* Interface Type */
	uint32_t Id;
	uint32_t Class;
	uint32_t Subclass;
	uint32_t Protocol;

	/* Ep Numbers */
	uint32_t Endpoints;

} UsbHcInterface_t;

/* The Abstract Device */
#pragma pack(push, 1)
typedef struct _UsbHcDevice
{
	/* Device Information */
	uint8_t Class;
	uint8_t Subclass;
	uint8_t Protocol;

	/* Device Id's */
	uint16_t VendorId;
	uint16_t ProductId;

	/* Configuration */
	uint8_t NumConfigurations;
	uint16_t ConfigMaxLength;
	uint16_t MaxPowerConsumption;
	uint8_t Configuration;

	/* String Ids */
	uint8_t StrIndexProduct;
	uint8_t StrIndexManufactor;
	uint8_t StrIndexSerialNum;

	/* Host Driver (UsbHc_t) & Port Number */
	void *HcDriver;
	uint8_t Port;

	/* Device Address */
	uint32_t Address;

	/* Device Descriptors (Config,Interface,Endpoint,Hid,etc) */
	void *Descriptors;
	uint32_t DescriptorsLength;

	/* Device Interfaces */
	uint32_t NumInterfaces;
	UsbHcInterface_t *Interfaces[X86_USB_CORE_MAX_IF];

	/* Device Endpoints */
	uint32_t NumEndpoints;
	UsbHcEndpoint_t *Endpoints[X86_USB_CORE_MAX_EP];

	/* Driver Data */
	void *DriverData;

} UsbHcDevice_t;
#pragma pack(pop)

/* The Abstract Transaction 
 * A request consists of several transactions */
typedef struct _UsbHcTransaction
{
	/* Type */
	uint32_t Type;

	/* A Transfer Descriptor Ptr */
	void *TransferDescriptor;

	/* Transfer Descriptor Buffer */
	void *TransferBuffer;

	/* Target/Source Buffer */
	void *IoBuffer;

	/* Target/Source Buffer Length */
	size_t IoLength;

	/* Next Transaction */
	struct _UsbHcTransaction *Link;

} UsbHcTransaction_t;

/* Types */
#define X86_USB_TRANSACTION_SETUP	1
#define X86_USB_TRANSACTION_IN		2
#define X86_USB_TRANSACTION_OUT		3

/* The Abstract Transfer Request */
typedef struct _UsbHcRequest
{
	/* Bulk or Control? */
	uint32_t Type;
	uint32_t LowSpeed;

	/* Transfer Specific Information */
	void *Data;
	UsbHcDevice_t *Device;

	/* Endpoint */
	uint32_t Endpoint;

	/* Length */
	uint32_t Length;

	/* Transaction Information */
	uint32_t TokenBytes;
	uint32_t Toggle;

	/* Buffer Information */
	void *IoBuffer;
	size_t IoLength;

	/* Packet */
	UsbPacket_t Packet;

	/* The Transaction List */
	UsbHcTransaction_t *Transactions;

	/* Is it done? */
	uint32_t Completed;

} UsbHcRequest_t;

/* Type Definitions */
#define X86_USB_REQUEST_TYPE_CONTROL	0x0
#define X86_USB_REQUEST_TYPE_BULK		0x1
#define X86_USB_REQUEST_TYPE_INTERRUPT	0x2
#define X86_USB_REQUEST_TYPE_ISOCHR		0x3

/* The Abstract Port */
typedef struct _UsbHcPort
{
	/* Port Number */
	uint32_t Id;

	/* Connection Status */
	uint32_t Connected;

	/* Enabled Status */
	uint32_t Enabled;

	/* Speed */
	uint32_t FullSpeed;

	/* Device Connected */
	UsbHcDevice_t *Device;

} UsbHcPort_t;

/* The Abstract Controller */
typedef struct _UsbHc
{
	/* Controller Type */
	uint32_t Type;

	/* Controller Data */
	void *Hc;

	/* Controller Info */
	uint32_t NumPorts;

	/* Ports */
	UsbHcPort_t *Ports[X86_USB_CORE_MAX_PORTS];

	/* Port Functions */
	void (*RootHubCheck)(void*);
	void (*PortSetup)(void*, UsbHcPort_t*);

	/* Transaction Functions */
	void (*TransactionInit)(void*, UsbHcRequest_t*);
	UsbHcTransaction_t *(*TransactionSetup)(void*, UsbHcRequest_t*);
	UsbHcTransaction_t *(*TransactionIn)(void*, UsbHcRequest_t*);
	UsbHcTransaction_t *(*TransactionOut)(void*, UsbHcRequest_t*);
	void (*TransactionSend)(void*, UsbHcRequest_t*);

	/* Install Interrupt EP */
	void (*InstallInterrupt)(void*, UsbHcDevice_t*, UsbHcEndpoint_t*,
		void*, size_t, void(*Callback)(void*, size_t), void*);

} UsbHc_t;

/* Usb Event */
typedef struct _UsbEvent
{
	/* Event Type */
	uint32_t Type;

	/* Controller */
	UsbHc_t *Controller;

	/* Port */
	int Port;

} UsbEvent_t;

/* Event Types */
#define X86_USB_EVENT_CONNECTED		0
#define X86_USB_EVENT_DISCONNECTED	1
#define X86_USB_EVENT_TRANSFER		2
#define X86_USB_EVENT_ROOTHUB_CHECK	3

/* Prototypes */

/* Returns an controller ID for used with identification */
_CRT_EXTERN UsbHc_t *UsbInitController(void *Data, uint32_t Type, uint32_t Ports);
_CRT_EXTERN uint32_t UsbRegisterController(UsbHc_t *Controller);

/* Transfer Utilities */
_CRT_EXTERN void UsbTransactionInit(UsbHc_t *Hc, UsbHcRequest_t *Request, uint32_t Type,
										UsbHcDevice_t *Device, uint32_t Endpoint, uint32_t MaxLength);
_CRT_EXTERN void UsbTransactionSetup(UsbHc_t *Hc, UsbHcRequest_t *Request, uint32_t PacketSize);
_CRT_EXTERN void UsbTransactionIn(UsbHc_t *Hc, UsbHcRequest_t *Request, uint32_t Handshake, void *Buffer, uint32_t Length);
_CRT_EXTERN void UsbTransactionOut(UsbHc_t *Hc, UsbHcRequest_t *Request, uint32_t Handshake, void *Buffer, uint32_t Length);
_CRT_EXTERN void UsbTransactionSend(UsbHc_t *Hc, UsbHcRequest_t *Request);

/* Functions */
_CRT_EXTERN int UsbFunctionSetAddress(UsbHc_t *Hc, int Port, uint32_t Address);
_CRT_EXTERN int UsbFunctionGetDeviceDescriptor(UsbHc_t *Hc, int Port);
_CRT_EXTERN int UsbFunctionGetConfigDescriptor(UsbHc_t *Hc, int Port);
_CRT_EXTERN int UsbFunctionSetConfiguration(UsbHc_t *Hc, int Port, uint32_t Configuration);
_CRT_EXTERN int UsbFunctionGetDescriptor(UsbHc_t *Hc, int Port, void *Buffer, uint8_t Direction,
											uint8_t DescriptorType, uint8_t SubType, 
											uint8_t DescriptorIndex, uint16_t DescriptorLength);
/* Send packet */
_CRT_EXTERN int UsbFunctionSendPacket(UsbHc_t *Hc, int Port, void *Buffer, uint8_t RequestType,
											uint8_t Request, uint8_t ValueHi, uint8_t ValueLo, 
											uint16_t Index, uint16_t Length);

/* Events */
_CRT_EXTERN void UsbEventCreate(UsbHc_t *Hc, int Port, uint32_t Type);

/* Gets */
_CRT_EXTERN UsbHc_t *UsbGetHcd(uint32_t ControllerId);
_CRT_EXTERN UsbHcPort_t *UsbGetPort(UsbHc_t *Controller, int Port);

#endif // !X86_USB_H_
