/* MollenOS
 *
 * Copyright 2011 - 2017, Philip Meulengracht
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
 * MollenOS MCore - Device Manager
 * - Initialization + Event Mechanism
 */

/* Includes
 * - System */
#include <os/driver/device.h>
#include <os/mollenos.h>
#include <ds/list.h>
#include <bus.h>

/* Includes
 * - C-Library */
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>

/* Globals */
List_t *GlbDeviceList = NULL;
UUId_t GlbDeviceIdGen = 0;
int GlbInitialized = 0;
int GlbRun = 0;

/* OnLoad
 * The entry-point of a server, this is called
 * as soon as the server is loaded in the system */
OsStatus_t OnLoad(void)
{
	/* Setup list */
	GlbDeviceList = ListCreate(KeyInteger, LIST_SAFE);

	/* Init variables */
	GlbDeviceIdGen = 0;
	GlbInitialized = 1;
	GlbRun = 1;

	/* Register us with server manager */
	RegisterServer(__DEVICEMANAGER_TARGET);

	/* Enumerate bus controllers/devices */
	BusEnumerate();

	/* No error! */
	return OsNoError;
}

/* OnUnload
 * This is called when the server is being unloaded
 * and should free all resources allocated by the system */
OsStatus_t OnUnload(void)
{
	return OsNoError;
}

/* OnEvent
 * This is called when the server recieved an external evnet
 * and should handle the given event*/
OsStatus_t OnEvent(MRemoteCall_t *Message)
{
	/* Which function is called? */
	switch (Message->Function)
	{
		/* Handles registration of a new device */
		case __DEVICEMANAGER_REGISTERDEVICE: {
			/* Variables for result */
			UUId_t Result;

			/* Evaluate request, but don't free
			* the allocated device storage, we need it */
			Message->Arguments[0].InUse = 0;
			Result = RegisterDevice((MCoreDevice_t*)Message->Arguments[0].Buffer, NULL);

			/* Write the result back to the caller */
			PipeSend(Message->Sender, Message->ResponsePort,
				&Result, sizeof(UUId_t));
		} break;
		case __DEVICEMANAGER_UNREGISTERDEVICE: {

		} break;
		case __DEVICEMANAGER_QUERYDEVICE: {

		} break;
		case __DEVICEMANAGER_IOCTLDEVICE: {

		} break;

		case __DEVICEMANAGER_REGISTERCONTRACT: {

		} break;

		default: {
		} break;
	}

	/* Done! */
	return OsNoError;
}

/* Device Registering
 * Allows registering of a new device in the
 * device-manager, and automatically queries
 * for a driver for the new device */
UUId_t RegisterDevice(MCoreDevice_t *Device, const char *Name)
{
	/* Variables */
	UUId_t DeviceId = GlbDeviceIdGen++;
	DataKey_t Key;

	/* Update name and print debug information */
	if (Name != NULL) {
		memcpy(&Device->Name[0], Name, strlen(Name));
		MollenOSSystemLog("Registered device %s", Name);
	}

	/* Update id, add to list */
	Key.Value = DeviceId;
	Device->Id = DeviceId;
	ListAppend(GlbDeviceList, ListCreateNode(Key, Key, Device));

	/* Now, we want to try to find a driver
	 * for the new device */
	if (InstallDriver(Device) == OsError) {
	}

	/* Done with processing of the new device */
	return DeviceId;
}
