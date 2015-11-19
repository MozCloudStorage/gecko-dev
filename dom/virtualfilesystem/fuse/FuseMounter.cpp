/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/mount.h>
#include "FuseMounter.h"
#include "nsServiceManagerUtils.h"
#include "nsIVolumeService.h"
#include "mozilla/FileUtils.h"
#include "nsIArray.h"
#include "nsISupportsPrimitives.h"
#include "nsArrayUtils.h"

#ifdef VIRTUAL_FILE_SYSTEM_LOG_TAG
#undef VIRTUAL_FILE_SYSTEM_LOG_TAG
#endif
#define VIRTUAL_FILE_SYSTEM_LOG_TAG "FuseMounter"
#include "VirtualFileSystemLog.h"

namespace mozilla {
namespace dom {
namespace virtualfilesystem {

// VirtualFileSystemVolumeRequest

VirtualFileSystemVolumeRequest::VirtualFileSystemVolumeRequest(
  const VirtualFileSystemVolumeRequest::eRequestType aType,
  const nsAString& aName,
  const nsAString& aMountPoint)
  : mType(aType),
    mVolumeName(aName),
    mMountPoint(aMountPoint)
{
}

nsresult
VirtualFileSystemVolumeRequest::Run()
{
  nsCOMPtr<nsIVolumeService> volService =
                             do_GetService(NS_VOLUMESERVICE_CONTRACTID);
  if (volService) {
    switch (mType) {
      case CREATEFAKEVOLUME: {
        volService->CreateFakeVolume(mVolumeName, mMountPoint);
        break;
      }
      case REMOVEFAKEVOLUME: {
        volService->RemoveFakeVolume(mVolumeName);
        break;
      }
      default : {
        LOG("Unknown request type [%d]", mType);
      }
    }
    nsCOMPtr<nsIArray> volNames;
    volService->GetVolumeNames(getter_AddRefs(volNames));
    if (volNames) {
      uint32_t numVolumes;
      volNames->GetLength(&numVolumes);
      for (uint32_t idx = 0; idx < numVolumes; ++idx) {
        nsCOMPtr<nsISupportsString> str = do_QueryElementAt(volNames, idx);
        nsAutoString s;
        str->GetData(s);
      }
    } else {
      LOG("No volumes");
    }
  } else {
    ERR("Getting nsVolumeService failed");
  }

  return NS_OK;
}

MounterResponseRunnable::MounterResponseRunnable(nsIVirtualFileSystemCallback* aCallback,
                                                 const uint32_t aRequestId,
                                                 const uint32_t aError,
                                                 const TYPE aType)
 : mCallback(aCallback),
   mRequestId(aRequestId),
   mError(aError),
   mType(aType)
{
}

nsresult
MounterResponseRunnable::Run()
{
  MOZ_ASSERT(NS_IsMainThread());
  switch (mType) {
  case MounterResponseRunnable::SUCCESS: {
    mCallback->OnSuccess(mRequestId, nullptr, false);
  }
  case MounterResponseRunnable::ERROR: {
    mCallback->OnError(mRequestId, mError);
  }
  }
  return NS_OK;
}

// FuseMounter
FuseMounter::FuseMounter(FuseHandler* aFuseHandler)
  : mHandler(aFuseHandler)
{
}

void
FuseMounter::Mount(nsIVirtualFileSystemCallback* aCallback,
                   const uint32_t aRequestId)
{
  MOZ_ASSERT(mHandler);
  RefPtr<FuseMountRunnable> runnable =
                      new FuseMountRunnable(mHandler, aCallback, aRequestId);

  nsresult rv = mHandler->DispatchRunnable(runnable);
  if (NS_FAILED(rv)) {
    LOG("Dispatching mount runnable failed");
    aCallback->OnError(aRequestId, nsIVirtualFileSystemCallback::ERROR_FAILED);
  }
}

void
FuseMounter::Unmount(nsIVirtualFileSystemCallback* aCallback,
                     const uint32_t aRequestId)
{
  MOZ_ASSERT(mHandler);
  RefPtr<FuseUnmountRunnable> runnable =
                     new FuseUnmountRunnable(mHandler, aCallback, aRequestId);

  nsresult rv = mHandler->DispatchRunnable(runnable);
  if (NS_FAILED(rv)) {
    LOG("Dispatching unmount runnable failed");
    aCallback->OnError(aRequestId, nsIVirtualFileSystemCallback::ERROR_FAILED);
  }
}


// FuseMountRunnable
FuseMounter::FuseMountRunnable::FuseMountRunnable(FuseHandler* aFuseHandler,
                            nsIVirtualFileSystemCallback* aCallback,
			    const uint32_t aRequestId)
  : mHandler(aFuseHandler),
    mCallback(aCallback),
    mRequestId(aRequestId)
{
}

bool
FuseMounter::FuseMountRunnable::CheckMountPoint()
{
  MOZ_ASSERT(!NS_IsMainThread());

  nsCString mountPoint = NS_ConvertUTF16toUTF8(mHandler->MountPoint());

  if ( -1 == mkdir(mountPoint.get(), S_IRWXU|S_IRWXG)) {
    switch (errno) {
      case EEXIST: break;
      case ENOTDIR: LOG("Parent is not a directory."); return false;
      case EACCES: LOG("Search permission is denied."); return false;
      case EROFS: LOG("Read-only filesystem."); return false;
      default: {LOG("Create %s failed with errno: %d.", mountPoint.get(), errno);
               return false;}
    }
  }

  // Make sure mount point directory is empty.
  DIR* dir = opendir(mountPoint.get());
  if (!dir) {
    LOG("Cannot open directory '%s' with errno [%s].", mountPoint.get(), strerror(errno));
    return false;
  }

  struct dirent* dirEntry = NULL;
  int32_t numEntries = 0;
  while ((dirEntry = readdir(dir)) != NULL) {
    if (++numEntries > 2) {
      break;
    }
  }
  closedir(dir);
  if (numEntries > 2) {
    // empty directory has entries '.' and '..'
    LOG("'%s' is not an empty directory.", mountPoint.get());
    return false;
  }
  return true;
}

nsresult
FuseMounter::FuseMountRunnable::Run()
{
  MOZ_ASSERT(!NS_IsMainThread());

  MozFuse& fuse = mHandler->GetFuse();

  if (fuse.fuseFd != -1) {
    ERR("FUSE file descriptor [%d], should be -1", fuse.fuseFd);
    DispatchResponse(MounterResponseRunnable::ERROR,
                     nsIVirtualFileSystemCallback::ERROR_FAILED);
    return NS_ERROR_FAILURE;
  }

  if (!CheckMountPoint()) {
    DispatchResponse(MounterResponseRunnable::ERROR,
                     nsIVirtualFileSystemCallback::ERROR_NOT_EMPTY);
    return NS_ERROR_FAILURE;
  }

  int fd;
  int stopfds[2];
  char opts[256];
  int res;

  // Open pipe for finish the request handler thread
  res = MOZ_TEMP_FAILURE_RETRY(pipe2(stopfds, O_DIRECT));
  if (res < 0) {
    ERR("Opening stop channel for FUSE device failed. %s", strerror(errno));
    DispatchResponse(MounterResponseRunnable::ERROR,
                     nsIVirtualFileSystemCallback::ERROR_FAILED);
    return NS_ERROR_FAILURE;
  }

  // unmount the device which mounted on the mount point.
  umount2(mHandler->MountPointStr(), 2);

  // open the fuse device
  fd = MOZ_TEMP_FAILURE_RETRY(open("/dev/fuse", O_RDWR));
  if (fd < 0){
    ERR("Opening FUSE device failed. %s", strerror(errno));
    DispatchResponse(MounterResponseRunnable::ERROR,
                     nsIVirtualFileSystemCallback::ERROR_FAILED);
    close(stopfds[0]);
    close(stopfds[1]);
    return NS_ERROR_FAILURE;
  }

  // setup mount option for fuse
  snprintf(opts, sizeof(opts),
  "fd=%i,rootmode=40000,default_permissions,allow_other,user_id=0,group_id=1015",
  fd);

  // mount fuse device on mount point
  res = mount("/dev/fuse", mHandler->MountPointStr(),
              "fuse", MS_NOSUID | MS_NODEV, opts);
  if (res < 0) {
    ERR("Mounting FUSE device failed. %s", strerror(errno));
    close(fd);
    close(stopfds[0]);
    close(stopfds[1]);
    DispatchResponse(MounterResponseRunnable::ERROR,
                     nsIVirtualFileSystemCallback::ERROR_FAILED);
    return NS_ERROR_FAILURE;
  }

  // Create fake volume for cloud storage
  if (NS_FAILED(NS_DispatchToMainThread(new VirtualFileSystemVolumeRequest(
                        VirtualFileSystemVolumeRequest::CREATEFAKEVOLUME,
                        mHandler->FileSystemId(),
                        mHandler->MountPoint())))) {
    ERR("Dispatching create fake volume '%s' to main thread failed",
         mHandler->FileSystemIdStr());
    umount2(mHandler->MountPointStr(), 2);
    close(fd);
    close(stopfds[0]);
    close(stopfds[1]);
    DispatchResponse(MounterResponseRunnable::ERROR,
                     nsIVirtualFileSystemCallback::ERROR_FAILED);
    return NS_ERROR_FAILURE;
  }

  // setup internal fuse device representation.
  fuse.fuseFd = fd;
  fuse.stopFds[0] = stopfds[0];
  fuse.stopFds[1] = stopfds[1];
  fuse.nextGeneration = 0;
  fuse.token = 0;

  if (NS_FAILED(DispatchResponse(MounterResponseRunnable::SUCCESS,
                                 nsIVirtualFileSystemCallback::ERROR_FAILED))) {
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}

nsresult
FuseMounter::FuseMountRunnable::DispatchResponse(MounterResponseRunnable::TYPE aType,
                                                 const uint32_t aError)
{
  RefPtr<MounterResponseRunnable> runnable =
         new MounterResponseRunnable(mCallback,
                                     mRequestId,
                                     aError,
                                     aType);
  if (NS_FAILED(NS_DispatchToMainThread(runnable))) {
    ERR("Dispatching mounter %s response to main thread failed",
         (aType == MounterResponseRunnable::SUCCESS) ? "success":"error");
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}

// FuseUnmountRunnable
FuseMounter::FuseUnmountRunnable::FuseUnmountRunnable(FuseHandler* aFuseHandler,
                            nsIVirtualFileSystemCallback* aCallback,
                            const uint32_t aRequestId)
  : mHandler(aFuseHandler),
    mCallback(aCallback),
    mRequestId(aRequestId)
{
}

nsresult
FuseMounter::FuseUnmountRunnable::Run()
{
  MOZ_ASSERT(!NS_IsMainThread());

  nsString mountPoint = mHandler->MountPoint();
  nsString fileSystemId = mHandler->FileSystemId();

  // Send remove fake volume job to mainthread.
  if (NS_FAILED(NS_DispatchToMainThread(new VirtualFileSystemVolumeRequest(
                        VirtualFileSystemVolumeRequest::REMOVEFAKEVOLUME,
                        fileSystemId,
                        mountPoint)))) {
    ERR("Dispatching remove fake volume '%s' job to main thread failed",
        NS_ConvertUTF16toUTF8(fileSystemId).get());
    DispatchResponse(MounterResponseRunnable::ERROR,
                     nsIVirtualFileSystemCallback::ERROR_FAILED);
    return NS_ERROR_FAILURE;
  }

  // unmount the device.
  umount2(mHandler->MountPointStr(), 2);
  // Unmount fuse device.
  close(mHandler->GetFuse().fuseFd);
  close(mHandler->GetFuse().stopFds[0]);
  close(mHandler->GetFuse().stopFds[1]);
  mHandler->GetFuse().fuseFd = -1;
  mHandler->GetFuse().stopFds[0] = -1;
  mHandler->GetFuse().stopFds[1] = -1;
  if (NS_FAILED(DispatchResponse(MounterResponseRunnable::SUCCESS,
                                 nsIVirtualFileSystemCallback::ERROR_FAILED))) {
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}

nsresult
FuseMounter::FuseUnmountRunnable::DispatchResponse(MounterResponseRunnable::TYPE aType,
                                                   const uint32_t aError)
{
  RefPtr<MounterResponseRunnable> runnable =
         new MounterResponseRunnable(mCallback,
                                     mRequestId,
                                     aError,
                                     aType);
  if (NS_FAILED(NS_DispatchToMainThread(runnable))) {
    ERR("Dispatching mounter %s response to main thread failed",
         (aType == MounterResponseRunnable::SUCCESS) ? "success":"error");
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}

} //end namespace virtualfilesystem
} //end namespace dom
} //end namespace mozilla
