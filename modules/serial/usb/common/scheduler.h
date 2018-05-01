/* MollenOS
 *
 * Copyright 2018, Philip Meulengracht
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
 * MollenOS MCore - USB Controller Scheduler
 * - Contains the implementation of a shared controller scheduker
 *   for all the usb drivers
 */

#ifndef __USB_SCHEDULER__
#define __USB_SCHEDULER__

/* Includes
 * - Library */
#include <os/contracts/usbhost.h>
#include <os/spinlock.h>
#include <os/osdefs.h>
#include <os/usb.h>

/* Definitions
 * Generic bandwidth allocation constants/support */
#define FRAME_TIME_USECS                1000L
#define FRAME_TIME_BITS                 12000L
#define FRAME_TIME_MAX_BITS_ALLOC       (90L * FRAME_TIME_BITS / 100L)
#define FRAME_TIME_MAX_USECS_ALLOC      (90L * FRAME_TIME_USECS / 100L)

#define BitTime(ByteCount)              (7 * 8 * ByteCount / 6)
#define NS_TO_US(ns)                    DIVUP(ns, 1000L)

/* Full/low speed bandwidth allocation constants/support. */
#define BW_HOST_DELAY   1000L
#define BW_HUB_LS_SETUP 333L

/* Ceiling [nano/micro]seconds (typical) for that many bytes at high speed
 * ISO is a bit less, no ACK ... from USB 2.0 spec, 5.11.3 (and needed
 * to preallocate bandwidth) */
#define USB2_HOST_DELAY 5       // nsec, guess
#define HS_NSECS(bytes) (((55 * 8 * 2083) \
        + (2083UL * (3 + BitTime(bytes))))/1000 \
        + USB2_HOST_DELAY)
#define HS_NSECS_ISO(bytes) (((38 * 8 * 2083) \
        + (2083UL * (3 + BitTime(bytes))))/1000 \
        + USB2_HOST_DELAY)
#define HS_USECS(bytes)         NS_TO_US(HS_NSECS(bytes))
#define HS_USECS_ISO(bytes)     NS_TO_US(HS_NSECS_ISO(bytes))

/* UsbSchedulerObject
 * The scheduler header information that must be present in all scheduleable
 * objects. The scheduler can then check the element and update accordingly. */
PACKED_TYPESTRUCT(UsbSchedulerObject, {
    uint32_t                Flags;
    uint16_t                Index;
    uint16_t                BreathIndex;
    uint16_t                DepthIndex;
    uint16_t                FrameInterval;
    uint16_t                Bandwidth;
    uint16_t                StartFrame;
    uint16_t                FrameMask;
});

/* UsbSchedulerObject::Flags
 * Contains definitions and bitfield definitions for UsbSchedulerObject::Flags 
 * Bit  0-11: Reserved and should be always set to link-flags for the element. 
 * Bit    12: Allocation status
 * Bit    13: Has bandwidth allocated
 * Bit    14: Isochronous Element
 * Bit 15-31: Available */
#define USB_ELEMENT_LINKFLAGS(Flags)    ((Flags & 0xFFF))
#define USB_ELEMENT_ALLOCATED           (1 << 12)
#define USB_ELEMENT_BANDWIDTH           (1 << 13)
#define USB_ELEMENT_ISOCHRONOUS         (1 << 14)

#define USB_ELEMENT_INDEX_MASK          0x1FFF
#define USB_ELEMENT_POOL_MASK           0x7
#define USB_ELEMENT_POOL_SHIFT          13

#define USB_ELEMENT_CREATE_INDEX(Pool, Index)   (uint16_t)((Index & USB_ELEMENT_INDEX_MASK) | ((Pool & USB_ELEMENT_POOL_MASK) << USB_ELEMENT_POOL_SHIFT))
#define USB_ELEMENT_NO_INDEX                    (uint16_t)0xFFFF

#define USB_ELEMENT_LINK_END            (1 << 0)

#define USB_CHAIN_BREATH                0
#define USB_CHAIN_DEPTH                 1
#define USB_POOL_MAXCOUNT               8

/* UsbSchedulerPool
 * Defines a type of pool for the usb scheduler. It allows for objects
 * with different sizes */
typedef struct _UsbSchedulerPool {
    uint8_t*    ElementPool;                // Pool
    uintptr_t   ElementPoolPhysical;        // Pool Physical
    size_t      ElementBaseSize;            // Size of an element
    size_t      ElementAlignedSize;         // Size of an element
    size_t      ElementCount;               // Number of elements
    size_t      ElementCountReserved;       // Number of reserved elements

    size_t      ElementLinkBreathOffset;    // Offset to the physical breath link member
    size_t      ElementDepthBreathOffset;   // Offset to the physical breath link member
    size_t      ElementObjectOffset;        // Offset to the UsbSchedulerObject
} UsbSchedulerPool_t;

/* UsbSchedulerSettings
 * Available settings for customizing an instance of the usb scheduler.
 * Allows for custom sizes, custom framelist and many others. */
typedef struct _UsbSchedulerSettings {
    Flags_t     Flags;                          // Flags
    size_t      FrameCount;                     // Number of frames
    size_t      SubframeCount;                  // Number of sub-frames
    size_t      MaxBandwidthPerFrame;           // Max bandwidth per frame
    
    reg32_t*    FrameList;                      // Physical frame list
    uintptr_t   FrameListPhysical;              // Physical address of frame list
    
    int         PoolCount;                      // Number of pools in use
    UsbSchedulerPool_t Pools[USB_POOL_MAXCOUNT];// Pools
} UsbSchedulerSettings_t;

#define USB_SCHEDULER_FRAMELIST         (1 << 0) // If set, we should create a framelist
#define USB_SCHEDULER_FL64              (1 << 1) // If set, use the 64 bit framelist
#define USB_SCHEDULER_DELAYED_CLEANUP   (1 << 2) // If set, cleanup of elements happens in a seperate process iteration
#define USB_SCHEDULER_NULL_ELEMENT      (1 << 3) // If set, all chains make use of null-elements

#define USB_SCHEDULER_LINK_BIT_EOL      (1 << 4) // Specify that empty links must be marked with EOL
#define USB_SCHEDULER_LINK_BIT_DEPTH    (1 << 5) // Specify that elements that use their depth link must be marked as such
#define USB_SCHEDULER_LINK_BIT_TYPE     (1 << 6) // Specify that elements use type bits

/* UsbScheduler
 * Contains information neccessary to keep track of scheduling
 * bandwidths and which frames are occupied. As generic as possible
 * to be usuable by all controllers */
typedef struct _UsbScheduler {
    // Meta
    UsbSchedulerSettings_t  Settings;
    Spinlock_t              Lock;

    // Resources
    size_t                  PoolSizeBytes;          // The total number of bytes allocated
    uintptr_t*              VirtualFrameList;       // Virtual frame list
    
    // Bandwidth
    size_t*                 Bandwidth;              // Bandwidth[FrameCount]
    size_t                  TotalBandwidth;         // Total bandwidth
} UsbScheduler_t;

#define USB_ELEMENT_INDEX(Pool, Index)              (uint8_t*)&(Pool->ElementPool[(Index & USB_ELEMENT_INDEX_MASK) * Pool->ElementAlignedSize])
#define USB_ELEMENT_PHYSICAL(Pool, Index)           (Pool->ElementPoolPhysical + ((Index & USB_ELEMENT_INDEX_MASK) * Pool->ElementAlignedSize))
#define USB_ELEMENT_GET_POOL(Scheduler, Index)      &Scheduler->Settings.Pools[(Index >> USB_ELEMENT_POOL_SHIFT) & USB_ELEMENT_POOL_MASK]

#define USB_ELEMENT_LINK(Pool, Element, Direction)  *(reg32_t*)((uint8_t*)Element + ((Direction == USB_CHAIN_BREATH) ? Pool->ElementLinkBreathOffset : Pool->ElementDepthBreathOffset))
#define USB_ELEMENT_OBJECT(Pool, Element)           (UsbSchedulerObject_t*)((uint8_t*)Element + Pool->ElementObjectOffset)

/* UsbSchedulerSettingsCreate
 * Initializes a new instance of the settings to customize the
 * scheduler. */
__EXTERN
void
UsbSchedulerSettingsCreate(
    _In_ UsbSchedulerSettings_t*    Settings,
    _In_ size_t                     FrameCount,
    _In_ size_t                     SubframeCount,
    _In_ size_t                     MaxBandwidthPerFrame,
    _In_ Flags_t                    Flags);

/* UsbSchedulerSettingsConfigureFrameList
 * Configure the framelist settings for the scheduler. This is always
 * neccessary to call if the controller is supplying its own framelist. */
__EXTERN
void
UsbSchedulerSettingsConfigureFrameList(
    _In_ UsbSchedulerSettings_t*    Settings,
    _In_ reg32_t*                   FrameList,
    _In_ uintptr_t                  FrameListPhysical);

/* UsbSchedulerSettingsAddPool
 * Adds a new pool to the scheduler configuration that will get created
 * with the scheduler. */
__EXTERN
void
UsbSchedulerSettingsAddPool(
    _In_ UsbSchedulerSettings_t*    Settings,
    _In_ size_t                     ElementSize,
    _In_ size_t                     ElementAlignment,
    _In_ size_t                     ElementCount,
    _In_ size_t                     ElementCountReserved,
    _In_ size_t                     LinkBreathMemberOffset,
    _In_ size_t                     LinkDepthMemberOffset,
    _In_ size_t                     ObjectMemberOffset);

/* UsbSchedulerInitialize 
 * Initializes a new instance of a scheduler that can be used to
 * keep track of controller bandwidth and which frames are active.
 * MaxBandwidth is usually either 800 or 900. */
__EXTERN
OsStatus_t
UsbSchedulerInitialize(
	_In_  UsbSchedulerSettings_t*   Settings,
    _Out_ UsbScheduler_t**          SchedulerOut);

/* UsbSchedulerDestroy 
 * Cleans up any resources allocated by the scheduler. Any transactions already
 * scheduled by this scheduler will be unreachable and invalid after this call. */
__EXTERN
OsStatus_t
UsbSchedulerDestroy(
	_In_ UsbScheduler_t*            Scheduler);

/* UsbSchedulerResetInternalData
 * Reinitializes all data structures in the scheduler to initial state. 
 * This should never be called unless the associating controller is in a
 * stopped state as the framelist is affected. */
__EXTERN
OsStatus_t
UsbSchedulerResetInternalData(
    _In_ UsbScheduler_t*            Scheduler,
    _In_ int                        ResetElements,
    _In_ int                        ResetFramelist);

/* UsbSchedulerGetPoolElement
 * Retrieves the element at the given pool and index. */
__EXTERN
OsStatus_t
UsbSchedulerGetPoolElement(
    _In_  UsbScheduler_t*           Scheduler,
    _In_  int                       Pool,
    _In_  int                       Index,
    _Out_ uint8_t**                 ElementOut,
    _Out_ uintptr_t*                ElementPhysicalOut);

/* UsbSchedulerGetPoolFromElement
 * Retrieves which pool an element belongs to by only knowing the address. */
__EXTERN
OsStatus_t
UsbSchedulerGetPoolFromElement(
    _In_  UsbScheduler_t*           Scheduler,
    _In_  uint8_t*                  Element,
    _Out_ UsbSchedulerPool_t**      Pool);

/* UsbSchedulerGetPoolFromElementPhysical
 * Retrieves which pool an element belongs to by only knowing the physical address. */
__EXTERN
OsStatus_t
UsbSchedulerGetPoolFromElementPhysical(
    _In_  UsbScheduler_t*           Scheduler,
    _In_  uintptr_t                 ElementPhysical,
    _Out_ UsbSchedulerPool_t**      Pool);

/* UsbSchedulerAllocateElement
 * Allocates a new element for usage with the scheduler. If this returns
 * OsError we are out of elements and we should wait till next transfer. ElementOut
 * will in this case be set to USB_OUT_OF_RESOURCES. */
__EXTERN
OsStatus_t
UsbSchedulerAllocateElement(
    _In_  UsbScheduler_t*           Scheduler,
    _In_  int                       Pool,
    _Out_ uint8_t**                 ElementOut);

/* UsbSchedulerFreeElement
 * Releases the previously allocated element by resetting it. This call automatically
 * frees any bandwidth associated with the element. */
__EXTERN
void
UsbSchedulerFreeElement(
    _In_ UsbScheduler_t*            Scheduler,
    _In_ uint8_t*                   Element);

/* UsbSchedulerAllocateBandwidth
 * Allocates bandwidth for a scheduler element. The bandwidth will automatically
 * be fitted into where is best place on schedule. If there is no more room it will
 * return OsError. */
__EXTERN
OsStatus_t
UsbSchedulerAllocateBandwidth(
    _In_ UsbScheduler_t*            Scheduler,
    _In_ UsbHcEndpointDescriptor_t* Endpoint,
    _In_ size_t                     BytesToTransfer,
	_In_ UsbTransferType_t          Type,
	_In_ UsbSpeed_t                 Speed,
    _In_ uint8_t*                   Element);

/* UsbSchedulerChainElement
 * Chains up a new element to the given element chain. The root element
 * must be specified and the element to append to the chain. Also the
 * chain direction must be specified. */
__EXTERN
OsStatus_t
UsbSchedulerChainElement(
    _In_ UsbScheduler_t*        Scheduler,
    _In_ uint8_t*               ElementRoot,
    _In_ uint8_t*               Element,
    _In_ uint16_t               Marker,
    _In_ int                    Direction);

/* UsbSchedulerUnchainElement
 * Removes an existing element from the given element chain. The root element
 * must be specified and the element to remove from the chain. Also the
 * chain direction must be specified. */
__EXTERN
OsStatus_t
UsbSchedulerUnchainElement(
    _In_ UsbScheduler_t*        Scheduler,
    _In_ uint8_t*               ElementRoot,
    _In_ uint8_t*               Element,
    _In_ int                    Direction);

/* UsbSchedulerLinkPeriodicElement
 * Queue's up a periodic/isochronous transfer. If it was not possible
 * to schedule the transfer with the requested bandwidth, it returns
 * OsError */
__EXTERN
OsStatus_t
UsbSchedulerLinkPeriodicElement(
    _In_ UsbScheduler_t*        Scheduler,
    _In_ uint8_t*               Element);

/* UsbSchedulerUnlinkPeriodicElement
 * Removes an already queued up periodic transfer (interrupt/isoc) from the
 * controllers scheduler. */
__EXTERN
void
UsbSchedulerUnlinkPeriodicElement(
    _In_ UsbScheduler_t*        Scheduler,
    _In_ uint8_t*               Element);

#endif //!__USB_SCHEDULER__
