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
* MollenOS C Library - File Write
*/

/* Includes */
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <os/Syscall.h>

/* The fwrite
* writes to a file handle */
size_t fwrite(const void * vptr, size_t size, size_t count, FILE * stream)
{
	/* Variables */
	size_t BytesToWrite = count * size;
	int RetVal = 0;

	/* Sanity */
	if (vptr == NULL
		|| stream == NULL
		|| BytesToWrite == 0)
		return 0;

	/* Syscall */
	RetVal = Syscall3(MOLLENOS_SYSCALL_VFSWRITE, MOLLENOS_SYSCALL_PARAM(stream),
		MOLLENOS_SYSCALL_PARAM(vptr), MOLLENOS_SYSCALL_PARAM(BytesToWrite));

	/* No need to check return
	* the syscall will set error code if any */
	BytesToWrite = (size_t)RetVal;

	/* Gj */
	return BytesToWrite;
}