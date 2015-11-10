/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_virtualfilesystem_fusemanager_h__
#define mozilla_dom_virtualfilesystem_fusemanager_h__

#include "nsCOMPtr.h"
#include "mozilla/AlreadyAddRefed.h"
#include "nsRefPtrHashtable.h"
#include "nsVirtualFileSystem.h"
#include "nsIVirtualFileSystemCallback.h"
#include "nsString.h"
#include "FuseHandler.h"

namespace mozilla {
namespace dom {
namespace virtualfilesystem {

typedef nsRefPtrHashtable<nsStringHashKey, FuseHandler> FuseHandlerHashtable;
static FuseHandlerHashtable sFuseHandlerTable;

void SetupFuseDevice(nsVirtualFileSystem* aVFS,
                     const nsAString& aMountPoint,
                     const nsAString& aFileSystemId,
                     const nsAString& aDisplayName,
                     const uint32_t aRequestId,
                     nsIVirtualFileSystemCallback* aCallback);

void ShotdownFuseDevice(const nsAString& aFileSystemId,
                        const uint32_t aRequestId,
                        nsIVirtualFileSystemCallback* aCallback);

} // end namespace virtualfilesystem
} // end namespace dom
} // end namespace mozilla
#endif
