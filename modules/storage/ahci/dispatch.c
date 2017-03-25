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
 * MollenOS MCore - Advanced Host Controller Interface Driver
 * TODO:
 *	- Port Multiplier Support
 *	- Power Management
 */

/* Includes
 * - System */
#include <os/mollenos.h>
#include <os/thread.h>
#include <os/utils.h>
#include "manager.h"

/* Includes
 * - Library */
#include <stddef.h>
#include <stdlib.h>

/* AhciCommandDispatch 
 * Dispatches a FIS command on a given port 
 * This function automatically allocates everything neccessary
 * for the transfer */
OsStatus_t
AhciCommandDispatch(
	_In_ AhciTransaction_t *Transaction,
	_In_ Flags_t Flags,
	_In_ void *Command, _In_ size_t CommandLength,
	_In_ void *AtapiCmd, _In_ size_t AtapiCmdLength)
{
	// Variables
	AHCICommandTable_t *CommandTable = NULL;
	size_t BytesLeft = Transaction->SectorCount * Transaction->Device->SectorSize;
	uintptr_t BufferPointer = 0;
	ListNode_t *tNode = NULL;
	DataKey_t Key, SubKey;
	int PrdtIndex = 0;

	// Assert that buffer is DWORD aligned, this must be true
	if (((Addr_t)Transaction->Address & 0x3) != 0) {
		MollenOSSystemLog("AhciCommandDispatch::Buffer was not dword aligned (0x%x)", 
			Transaction->Device->Port->Id, Transaction->Address);
		goto Error;
	}

	// Assert that buffer length is an even byte-count requested
	if ((BytesLeft & 0x1) != 0) {
		MollenOSSystemLog("AhciCommandDispatch::BufferLength is odd, must be even",
			Transaction->Device->Port->Id);
		goto Error;
	}

	// Get a reference to the command slot and reset
	// the data in the command table
	CommandTable = (AHCICommandTable_t*)
		((uint8_t*)Transaction->Device->Port->CommandTable
			+ (AHCI_COMMAND_TABLE_SIZE * Transaction->Slot));
	memset(CommandTable, 0, AHCI_COMMAND_TABLE_SIZE);

	// Sanitizie packet lenghts
	if (CommandLength > 64
		|| AtapiCmdLength > 16) {
		MollenOSSystemLog("AHCI::Commands are exceeding the allowed length, FIS (%u), ATAPI (%u)",
			CommandLength, AtapiCmdLength);
		goto Error;
	}

	// Copy data over into the packets based on type
	if (Command != NULL) {
		memcpy(&CommandTable->FISCommand[0], Command, CommandLength);
	}
	if (AtapiCmd != NULL) {
		memcpy(&CommandTable->FISAtapi[0], AtapiCmd, AtapiCmdLength);
	}

	// Build PRDT entries
	BufferPointer = Transaction->Address;
	while (BytesLeft > 0) {
		AHCIPrdtEntry_t *Prdt = &CommandTable->PrdtEntry[PrdtIndex];
		size_t TransferLength = MIN(AHCI_PRDT_MAX_LENGTH, BytesLeft);

		// Set buffer information and transfer sizes
		Prdt->DataBaseAddress = LODWORD(BufferPointer);
		Prdt->DataBaseAddressUpper = (sizeof(void*) > 4) ? HIDWORD(BufferPointer) : 0;
		Prdt->Descriptor = (TransferLength - 1); // N - 1

		// Adjust counters
		BufferPointer += TransferLength;
		BytesLeft -= TransferLength;
		PrdtIndex++;

		// If this is the last PRDT packet, set IOC
		if (BytesLeft == 0) {
			Prdt->Descriptor |= AHCI_PRDT_IOC;
		}
	}

	// Update command table to the new command
	Transaction->Device->Port->CommandList->Headers[Transaction->Slot].TableLength = (uint16_t)PrdtIndex;
	Transaction->Device->Port->CommandList->Headers[Transaction->Slot].Flags = (uint16_t)(CommandLength / 4);

	// Update transfer with the dispatch flags
	if (Flags & DISPATCH_ATAPI) {
		Transaction->Device->Port->CommandList->Headers[Transaction->Slot].Flags |= (1 << 5);
	}
	if (Flags & DISPATCH_WRITE) {
		Transaction->Device->Port->CommandList->Headers[Transaction->Slot].Flags |= (1 << 6);
	}
	if (Flags & DISPATCH_PREFETCH) {
		Transaction->Device->Port->CommandList->Headers[Transaction->Slot].Flags |= (1 << 7);
	}
	if (Flags & DISPATCH_CLEARBUSY) {
		Transaction->Device->Port->CommandList->Headers[Transaction->Slot].Flags |= (1 << 10);
	}

	// Set the port multiplier
	Transaction->Device->Port->CommandList->Headers[Transaction->Slot].Flags
		|= (DISPATCH_MULTIPLIER(Flags) << 12);

	// Setup key and sort key
	Key.Value = Transaction->Slot;
	SubKey.Value = (int)DISPATCH_MULTIPLIER(Flags);

	// Add transaction to list
	tNode = ListCreateNode(Key, SubKey, Transaction);
	ListAppend(Transaction->Device->Port->Transactions, tNode);

	// Enable command 
	AhciPortStartCommandSlot(Transaction->Device->Port, Transaction->Slot);
	return OsNoError;

Error:
	return OsError;
}

/* AhciVerifyRegisterFIS
 * Verifies a recieved fis result on a port/slot */
OsStatus_t
AhciVerifyRegisterFIS(
	_In_ AhciTransaction_t *Transaction)
{
	// Variables
	AHCIFis_t *Fis = NULL;
	size_t Offset = Transaction->Slot * AHCI_RECIEVED_FIS_SIZE;

	// Get a pointer to the FIS
	Fis = (AHCIFis_t*)((uint8_t*)Transaction->Device->Port->RecievedFisTable + Offset);

	// Is the error bit set?
	if (Fis->RegisterD2H.Status & ATA_STS_DEV_ERROR) {
		if (Fis->RegisterD2H.Error & ATA_ERR_DEV_EOM) {
			MollenOSSystemLog("AHCI::Port (%i): Transmission Error, Invalid LBA(sector) range given, end of media.",
				Transaction->Device->Port->Id, (size_t)Fis->RegisterD2H.Error);
		}
		else {
			MollenOSSystemLog("AHCI::Port (%i): Transmission Error, error 0x%x",
				Transaction->Device->Port->Id, (size_t)Fis->RegisterD2H.Error);
		}
		return OsError;
	}

	// Is the fault bit set?
	if (Fis->RegisterD2H.Status & ATA_STS_DEV_FAULT) {
		MollenOSSystemLog("AHCI::Port (%i): Device Fault, error 0x%x",
			Transaction->Device->Port->Id, (size_t)Fis->RegisterD2H.Error);
		return OsError;
	}

	// If we reach here, all checks has been 
	// passed succesfully, and we return no err
	return OsNoError;
}

/* AhciCommandRegisterFIS 
 * Builds a new AHCI Transaction based on a register FIS */
OsStatus_t 
AhciCommandRegisterFIS(
	_In_ AhciTransaction_t *Transaction,
	_In_ ATACommandType_t Command, 
	_In_ uint64_t SectorLBA, 
	_In_ int Device, 
	_In_ int Write)
{
	// Variables
	FISRegisterH2D_t Fis;
	OsStatus_t Status;
	uint32_t Flags;

	// Reset the fis structure as we have it on stack
	memset((void*)&Fis, 0, sizeof(FISRegisterH2D_t));

	// Fill out initial information
	Fis.Type = LOBYTE(FISRegisterH2D);
	Fis.Flags |= FIS_HOST_TO_DEVICE;
	Fis.Command = LOBYTE(Command);
	Fis.Device = 0x40 | ((LOBYTE(Device) & 0x1) << 4);

	// Handle LBA to CHS translation if disk uses
	// the CHS scheme
	if (Transaction->Device->AddressingMode == 0) {
		//uint16_t Head = 0, Cylinder = 0, Sector = 0;

		// Step 1 -> Transform LBA into CHS

		// Set CHS params

		// Set count
		Fis.Count = LOBYTE(Transaction->SectorCount);
	}
	else if (Transaction->Device->AddressingMode == 1
		|| Transaction->Device->AddressingMode == 2) {
		// Set LBA 28 parameters
		Fis.SectorNo = LOBYTE(SectorLBA);
		Fis.CylinderLow = (uint8_t)((SectorLBA >> 8) & 0xFF);
		Fis.CylinderHigh = (uint8_t)((SectorLBA >> 16) & 0xFF);
		Fis.SectorNoExtended = (uint8_t)((SectorLBA >> 24) & 0xFF);

		// If it's an LBA48, set LBA48 params as well
		if (Transaction->Device->AddressingMode == 2) {
			Fis.CylinderLowExtended = (uint8_t)((SectorLBA >> 32) & 0xFF);
			Fis.CylinderHighExtended = (uint8_t)((SectorLBA >> 40) & 0xFF);

			// Count is 16 bit here
			Fis.Count = LOWORD(Transaction->SectorCount);
		}
		else {
			// COunt is 8 bit in lba28
			Fis.Count = LOBYTE(Transaction->SectorCount);
		}
	}

	// Start out by building dispatcher flags here
	Flags = DISPATCH_MULTIPLIER(0);
	
	// Atapi device?
	if (Transaction->Device->Port->Registers->Signature == SATA_SIGNATURE_ATAPI) {
		Flags |= DISPATCH_ATAPI;
	}

	// Determine direction of operation
	if (Write != 0) {
		Flags |= DISPATCH_WRITE;
	}

	// Allocate a command slot for this transaction
	if (AhciPortAcquireCommandSlot(Transaction->Device->Controller,
		Transaction->Device->Port, &Transaction->Slot) != OsNoError) {
		MollenOSSystemLog("AHCI::Port (%i): Failed to allocate a command slot",
			Transaction->Device->Port->Id);
		return OsError;
	}

	// Execute command - we do this asynchronously
	// so we must handle the rest of this later on
	Status = AhciCommandDispatch(Transaction, Flags, 
		&Fis, sizeof(FISRegisterH2D_t), NULL, 0);

	// Sanitize return, if it didn't start then handle right now
	if (Status != OsNoError) {
		AhciPortReleaseCommandSlot(Transaction->Device->Port, Transaction->Slot);
	}
	
	// Return the success
	return Status;
}

/* AhciCommandFinish
 * Verifies and cleans up a transaction made by dispatch */
OsStatus_t 
AhciCommandFinish(
	_In_ AhciTransaction_t *Transaction)
{
	// Variables
	MRemoteCall_t Rpc;
	OsStatus_t Status;

	// Verify the command execution
	Status = AhciVerifyRegisterFIS(Transaction);

	// Release the allocated slot
	AhciPortReleaseCommandSlot(Transaction->Device->Port, Transaction->Slot);

	// If this was an internal request we need to notify manager
	if (Transaction->Requester == UUID_INVALID) {
		AhciManagerCreateDeviceCallback(Transaction->Device);
	}
	else {
		// Write the result back to the requester
		Rpc.Sender = Transaction->Requester;
		Rpc.ResponsePort = Transaction->Pipe;
		RPCRespond(&Rpc, &Status, sizeof(OsStatus_t));
	}

	// Cleanup the transaction
	free(Transaction);

	// Return the status
	return Status;
}