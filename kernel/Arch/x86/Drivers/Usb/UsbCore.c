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

/* Includes */
#include <Arch.h>
#include <Drivers\Usb\Usb.h>
#include <Drivers\Usb\HID\HIDManager.h>
#include <Drivers\Usb\MSD\MSDManager.h>
#include <Semaphore.h>
#include <SysTimers.h>
#include <Heap.h>
#include <List.h>
#include <stdio.h>
#include <string.h>

/* Globals */
list_t *GlbUsbControllers = NULL;
list_t *GlbUsbDevices = NULL;
list_t *GlbUsbEvents = NULL;
Semaphore_t *GlbEventLock = NULL;
volatile uint32_t GlbUsbInitialized = 0;
volatile uint32_t GlbUsbControllerId = 0;

/* Prototypes */
void UsbEventHandler(void*);
void UsbDeviceSetup(UsbHc_t *Hc, int Port);
void UsbDeviceDestroy(UsbHc_t *Hc, int Port);
UsbHcPort_t *UsbPortCreate(int Port);

/* Gets called once a USB controller is registreret */
void UsbCoreInit(void)
{
	GlbUsbInitialized = 0xDEADBEEF;
	GlbUsbDevices = list_create(LIST_SAFE);
	GlbUsbControllers = list_create(LIST_SAFE);
	GlbUsbEvents = list_create(LIST_SAFE);
	GlbUsbControllerId = 0;

	/* Initialize Event Semaphore */
	GlbEventLock = SemaphoreCreate(0);

	/* Start Event Thread */
	ThreadingCreateThread("UsbEventHandler", UsbEventHandler, NULL, 0);
}

/* Registrate an OHCI/UHCI/EHCI/XHCI controller */
UsbHc_t *UsbInitController(void *Data, uint32_t Type, uint32_t Ports)
{
	UsbHc_t *Controller;

	/* Allocate Resources */
	Controller = (UsbHc_t*)kmalloc(sizeof(UsbHc_t));
	memset(Controller, 0, sizeof(UsbHc_t));

	Controller->Hc = Data;
	Controller->Type = Type;
	Controller->NumPorts = Ports;

	return Controller;
}

uint32_t UsbRegisterController(UsbHc_t *Controller)
{
	uint32_t Id;

	/* First call? */
	if (GlbUsbInitialized != 0xDEADBEEF)
	{
		/* Oh shit, put hat on quick! */
		UsbCoreInit();
	}

	/* Get id */
	Id = GlbUsbControllerId;
	GlbUsbControllerId++;

	/* Add to list */
	list_append(GlbUsbControllers, list_create_node(Id, Controller));

	return Id;
}

/* Create Event */
void UsbEventCreate(UsbHc_t *Hc, int Port, uint32_t Type)
{
	UsbEvent_t *Event;

	/* Allocate */
	Event = (UsbEvent_t*)kmalloc(sizeof(UsbEvent_t));
	Event->Controller = Hc;
	Event->Port = Port;
	Event->Type = Type;

	/* Append */
	list_append(GlbUsbEvents, list_create_node((int)Type, Event));

	/* Signal */
	SemaphoreV(GlbEventLock);
}

/* Device Connected */
void UsbDeviceSetup(UsbHc_t *Hc, int Port)
{
	UsbHcDevice_t *Device;
	int i;

	/* Make sure we have the port allocated */
	if (Hc->Ports[Port] == NULL)
		Hc->Ports[Port] = UsbPortCreate(Port);

	/* Create a device */
	Device = (UsbHcDevice_t*)kmalloc(sizeof(UsbHcDevice_t));
	Device->HcDriver = Hc;
	Device->Port = (uint8_t)Port;
	Device->NumEndpoints = 1;

	Device->NumInterfaces = 0;
	for (i = 0; i < X86_USB_CORE_MAX_IF; i++)
		Device->Interfaces[i] = NULL;
	
	/* Initial Address must be 0 */
	Device->Address = 0;

	/* Allocate control endpoint */
	for (i = 0; i < 1; i++)
	{
		Device->Endpoints[i] = (UsbHcEndpoint_t*)kmalloc(sizeof(UsbHcEndpoint_t));
		Device->Endpoints[i]->Address = 0;
		Device->Endpoints[i]->Type = X86_USB_EP_TYPE_CONTROL;
		Device->Endpoints[i]->Toggle = 0;
		Device->Endpoints[i]->Bandwidth = 1;
		Device->Endpoints[i]->MaxPacketSize = 64;
		Device->Endpoints[i]->Direction = X86_USB_EP_DIRECTION_BOTH;
		Device->Endpoints[i]->Interval = 0;
	}

	/* Bind it */
	Hc->Ports[Port]->Device = Device;

	/* Allow 100 ms for insertion to complete */
	StallMs(100);

	/* Setup Port */
	Hc->PortSetup(Hc->Hc, Hc->Ports[Port]);

	/* Set Device Address (Just bind it to the port number + 1 (never set address 0) ) */
	if (!UsbFunctionSetAddress(Hc, Port, (uint32_t)(Port + 1)))
	{
		/* Try again */
		if (!UsbFunctionSetAddress(Hc, Port, (uint32_t)(Port + 1)))
		{
			printf("USB_Handler: (Set_Address) Failed to setup port %u\n", Port);
			return;
		}
	}

	/* After SetAddress device is allowed 2 ms recovery */
	StallMs(2);

	/* Get Device Descriptor */
	if (!UsbFunctionGetDeviceDescriptor(Hc, Port))
	{
		/* Try Again */
		if (!UsbFunctionGetDeviceDescriptor(Hc, Port))
		{
			printf("USB_Handler: (Get_Device_Desc) Failed to setup port %u\n", Port);
			return;
		}
	}
	
	/* Get Config Descriptor */
	if (!UsbFunctionGetConfigDescriptor(Hc, Port))
	{
		/* Try Again */
		if (!UsbFunctionGetConfigDescriptor(Hc, Port))
		{
			printf("USB_Handler: (Get_Config_Desc) Failed to setup port %u\n", Port);
			return;
		}
	}

	/* Set Configuration */
	if (!UsbFunctionSetConfiguration(Hc, Port, Hc->Ports[Port]->Device->Configuration))
	{
		/* Try Again */
		if (!UsbFunctionSetConfiguration(Hc, Port, Hc->Ports[Port]->Device->Configuration))
		{
			printf("USB_Handler: (Set_Configuration) Failed to setup port %u\n", Port);
			return;
		}
	}

	/* Go through interfaces and add them */
	for (i = 0; i < (int)Hc->Ports[Port]->Device->NumInterfaces; i++)
	{
		/* We want to support Hubs, HIDs and MSDs*/
		//uint32_t iface = (uint32_t)i;

		/* Is this an HID Interface? :> */
		if (Hc->Ports[Port]->Device->Interfaces[i]->Class == X86_USB_CLASS_HID)
		{
			/* Registrate us with HID Manager */
			//usb_hid_initialise(Hc->Ports[Port]->Device, iface);
		}

		/* Is this an MSD Interface? :> */
		if (Hc->Ports[Port]->Device->Interfaces[i]->Class == X86_USB_CLASS_MSD)
		{
			/* Registrate us with MSD Manager */
			//usb_msd_initialise(Hc->Ports[Port]->Device, iface);
		}

		/* Is this an HUB Interface? :> */
		if (Hc->Ports[Port]->Device->Interfaces[i]->Class == X86_USB_CLASS_HUB)
		{
			/* Protocol specifies usb interface (high or low speed) */

			/* Registrate us with Hub Manager */
		}
	}

	/* Done */
	printf("UsbCore: Setup of port %u done!\n", Port);
}

void UsbDeviceDestroy(UsbHc_t *Hc, int Port)
{
	/* Destroy device */
	_CRT_UNUSED(Hc);
	_CRT_UNUSED(Port);
}

/* Ports */
UsbHcPort_t *UsbPortCreate(int Port)
{
	UsbHcPort_t *HcPort;

	/* Allocate Resources */
	HcPort = kmalloc(sizeof(UsbHcPort_t));

	/* Get Port Status */
	HcPort->Id = Port;

	/* Done */
	return HcPort;
}

/* USB Events */
void UsbEventHandler(void *args)
{
	UsbEvent_t *Event;
	list_node_t *node;

	/* Unused */
	_CRT_UNUSED(args);

	while (1)
	{
		/* Acquire Semaphore */
		SemaphoreP(GlbEventLock);

		/* Pop Event */
		node = list_pop_front(GlbUsbEvents);

		/* Sanity */
		if (node == NULL)
			continue;

		Event = (UsbEvent_t*)node->data;

		/* Again, sanity */
		if (Event == NULL)
			continue;

		/* Handle Event */
		switch (Event->Type)
		{
			case X86_USB_EVENT_CONNECTED:
			{
				/* Setup Device */
				UsbDeviceSetup(Event->Controller, Event->Port);

			} break;

			case X86_USB_EVENT_DISCONNECTED:
			{
				/* Destroy Device */
				UsbDeviceDestroy(Event->Controller, Event->Port);

			} break;

			case X86_USB_EVENT_ROOTHUB_CHECK:
			{
				/* Check Ports for Activity */
				Event->Controller->RootHubCheck(Event->Controller->Hc);

			} break;

			default:
			{
				printf("Unhandled Event: %u on port %i\n", Event->Type, Event->Port);
			} break;
		}
	}
}

/* Gets */
UsbHc_t *UsbGetHcd(uint32_t ControllerId)
{
	return (UsbHc_t*)list_get_data_by_id(GlbUsbControllers, ControllerId, 0);
}

UsbHcPort_t *UsbGetPort(UsbHc_t *Controller, int Port)
{
	/* Sanity */
	if (Controller == NULL || Port >= (int)Controller->NumPorts)
		return NULL;

	return Controller->Ports[Port];
}