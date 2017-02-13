/* MollenOS
 *
 * Copyright 2011 - 2016, Philip Meulengracht
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
 * MollenOS - Process Functions
 */

/* Includes */
#include <os/MollenOS.h>
#include <os/Syscall.h>

/* Process Spawn
 * This function spawns a new process 
 * in it's own address space, and returns
 * the new process id 
 * If startup is failed, the returned value 
 * is 0xFFFFFFFF */
UUId_t ProcessSpawn(const char *Path, const char *Arguments)
{
	/* Variables */
	int RetVal = 0;

	/* Syscall! */
	RetVal = Syscall2(SYSCALL_PROCSPAWN, 
		SYSCALL_PARAM(Path), SYSCALL_PARAM(Arguments));

	/* Done */
	return (UUId_t)RetVal;
}

/* Process Join 
 * Attaches to a running process 
 * and waits for the process to quit
 * the return value is the return code
 * from the target process */
int ProcessJoin(UUId_t Process)
{
	/* Redirect call */
	return Syscall1(SYSCALL_PROCJOIN, SYSCALL_PARAM(Process));
}

/* Process Kill 
 * Kills target process id 
 * On error, returns -1, or if the 
 * kill was succesful, returns 0 */
OsStatus_t ProcessKill(UUId_t Process)
{
	/* Sanity -- Who the 
	 * fuck would try to kill 
	 * window server */
	if (Process == 0)
		return OsError;

	/* Done */
	return (OsStatus_t)Syscall1(SYSCALL_PROCKILL, SYSCALL_PARAM(Process));
}

/* Process Query
 * Queries information about the
 * given process id, or use 0
 * to query information about current
 * process */
int ProcessQuery(UUId_t Process, ProcessQueryFunction_t Function, void *Buffer, size_t Length)
{
	/* Prep for syscall */
	return Syscall4(SYSCALL_PROCQUERY, SYSCALL_PARAM(Process),
		SYSCALL_PARAM(Function), SYSCALL_PARAM(Buffer),
		SYSCALL_PARAM(Length));
}
