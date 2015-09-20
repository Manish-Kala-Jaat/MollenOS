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
* MollenOS Synchronization
* Mutexes
*/

#ifndef _MCORE_MUTEX_H_
#define _MCORE_MUTEX_H_

/* Includes */
#include <Arch.h>
#include <crtdefs.h>
#include <stdint.h>

/* Structures */
typedef struct _Mutex
{
	/* Task that is blocking */
	TId_t Blocker;

	/* Total amout of blocking */
	uint32_t Blocks;

	/* Implemented using spinlock untill we have synchronization 
	 * support in scheduler */
	Spinlock_t Lock;

} Mutex_t;

/* Prototypes */
_CRT_EXTERN Mutex_t *MutexCreate(void);
_CRT_EXTERN void MutexConstruct(Mutex_t *Mutex);
_CRT_EXTERN void MutexDestruct(Mutex_t *Mutex);
_CRT_EXTERN void MutexLock(Mutex_t *Mutex);
_CRT_EXTERN void MutexUnlock(Mutex_t *Mutex);

#endif // !_MCORE_MUTEX_H_