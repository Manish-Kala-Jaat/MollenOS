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
* MollenOS C-Library - Assert
*/

/* Includes */
#include <assert.h>

#ifdef LIBC_KERNEL
extern void kernel_panic(const char* str);
void _assert_panic(const char* str)
{
	kernel_panic(str);
}
#else
#include <os/MollenOS.h>
#include <stdlib.h>

/* Redirect output and exit thread */
void _assert_panic(const char* str)
{
	MollenOSSystemLog(str);
	abort();
}

#endif