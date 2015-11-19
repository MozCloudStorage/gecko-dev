/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "FuseManager.h"
#include "FuseMounter.h"
#include "FuseRequestMonitor.h"
#include "FuseResponseHandler.h"

#ifdef VIRTUAL_FILE_SYSTEM_LOG_TAG
#undef VIRTUAL_FILE_SYSTEM_LOG_TAG
#endif
#define VIRTUAL_FILE_SYSTEM_LOG_TAG "FuseManager"
#include "VirtualFileSystemLog.h"

namespace mozilla {
namespace dom {
namespace virtualfilesystem {

void
SetupFuseDevice(nsVirtualFileSystem* aVFS,
                     const nsAString& aMountPoint,
                     const nsAString& aFileSystemId,
                     const nsAString& aDisplayName,
                     const uint32_t aRequestId,
                     nsIVirtualFileSystemCallback* aCallback)
{
  RefPtr<FuseHandler> fh;
  sFuseHandlerTable.Get(aFileSystemId, getter_AddRefs(fh));
  if (fh) {
    ERR("the corresponding FUSE device for '%s' was constructed.",
         NS_ConvertUTF16toUTF8(aFileSystemId).get());
    aCallback->OnError(aRequestId, nsIVirtualFileSystemCallback::ERROR_FAILED);
    return;
  }

  fh = new FuseHandler(aFileSystemId, aMountPoint, aDisplayName);
  sFuseHandlerTable.Put(aFileSystemId, fh);

  RefPtr<FuseMounter> mounter = new FuseMounter(fh);
  RefPtr<nsIVirtualFileSystemResponseHandler> responsehandler =
                                         new FuseResponseHandler(fh);
  RefPtr<FuseRequestMonitor> monitor = new FuseRequestMonitor(fh);

  aVFS->SetResponseHandler(responsehandler);
  RefPtr<nsIVirtualFileSystem> vfs = aVFS;

  mounter->Mount(aCallback, aRequestId);
  monitor->Monitor(vfs);
}

void
ShotdownFuseDevice(const nsAString& aFileSystemId,
                   const uint32_t aRequestId,
                   nsIVirtualFileSystemCallback* aCallback)
{
  RefPtr<FuseHandler> fh;
  if(!sFuseHandlerTable.Remove(aFileSystemId,getter_AddRefs(fh))) {
    ERR("The corresponding FUSE device for '%s' does not exist.",
         NS_ConvertUTF16toUTF8(aFileSystemId).get());
    aCallback->OnError(aRequestId, nsIVirtualFileSystemCallback::ERROR_FAILED);
    return;
  }

  RefPtr<FuseRequestMonitor> monitor = new FuseRequestMonitor(fh);
  monitor->Stop();

  RefPtr<FuseMounter> mounter = new FuseMounter(fh);
  mounter->Unmount(aCallback, aRequestId);
}

} //end namespace virtualfilesystem
} //end namespace dom
} //end namespace mozilla
