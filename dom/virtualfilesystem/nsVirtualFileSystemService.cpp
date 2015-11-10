/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "nsVirtualFileSystemService.h"
#include "nsVirtualFileSystem.h"
#include "nsVirtualFileSystemDataType.h"
#include "nsIMutableArray.h"
#include "nsISupportsPrimitives.h"
#include "nsISupportsUtils.h"
#include "nsServiceManagerUtils.h"
#include "mozilla/Services.h"
#include "FuseManager.h"
#ifdef VIRTUAL_FILE_SYSTEM_LOG_TAG
#undef VIRTUAL_FILE_SYSTEM_LOG_TAG
#endif
#define VIRTUAL_FILE_SYSTEM_LOG_TAG "VirtualFileSystemService"

#include "VirtualFileSystemLog.h"

#define MOUNTROOT "/data/vfs"

namespace mozilla {
namespace dom {
namespace virtualfilesystem {

// nsVirtualFileSystemService

NS_IMPL_ISUPPORTS(nsVirtualFileSystemService, nsIVirtualFileSystemService)

StaticRefPtr<nsIVirtualFileSystemService> nsVirtualFileSystemService::sService;

nsVirtualFileSystemService::nsVirtualFileSystemService()
  : mArrayMonitor("nsVirtualFileSystemService"),
    mVirtualFileSystemArray()
{
}

nsVirtualFileSystemService::~nsVirtualFileSystemService()
{
}

//static
already_AddRefed<nsIVirtualFileSystemService>
nsVirtualFileSystemService::GetSingleton()
{
  if (!sService) {
    sService = new nsVirtualFileSystemService();
  }
  RefPtr<nsIVirtualFileSystemService> service = sService.get();
  return service.forget();
}

// nsIVirtualFileSystemService interface implementation

NS_IMETHODIMP
nsVirtualFileSystemService::Mount(nsIVirtualFileSystemMountRequestedOptions* aOption,
                                  nsIVirtualFileSystemRequestManager* aRequestMgr,
                                  nsIVirtualFileSystemCallback* aCallback)
{
  nsString fileSystemId;
  nsString displayName;
  bool     writable;
  uint32_t openedLimit;
  uint32_t requestId;
  aOption->GetFileSystemId(fileSystemId);
  aOption->GetDisplayName(displayName);
  aOption->GetWritable(&writable);
  aOption->GetOpenedFilesLimit(&openedLimit);
  aOption->GetRequestId(&requestId);

  if (fileSystemId.IsEmpty()) {
    ERR("Empty file system ID.");
    aCallback->OnError(requestId, nsIVirtualFileSystemCallback::ERROR_FAILED);
    return NS_ERROR_FAILURE;
  }

  if (displayName.IsEmpty()) {
    ERR("Empty display name.");
    aCallback->OnError(requestId, nsIVirtualFileSystemCallback::ERROR_FAILED);
    return NS_ERROR_FAILURE;
  }

  RefPtr<nsIVirtualFileSystem> vfs = FindVirtualFileSystemById(fileSystemId);
  if (vfs) {
    LOG("The virtual file system '%s' had already created.",
         NS_ConvertUTF16toUTF8(fileSystemId).get());
    aCallback->OnError(requestId, nsIVirtualFileSystemCallback::ERROR_EXISTS);
    return NS_ERROR_FAILURE;
  }

  nsString mountPoint = nsVirtualFileSystemService::CreateMountPoint(fileSystemId);

  RefPtr<nsIVirtualFileSystemInfo> info = new nsVirtualFileSystemInfo();
  info->SetFileSystemId(fileSystemId);
  info->SetDisplayName(displayName);
  info->SetWritable(writable);
  info->SetOpenedFilesLimit(openedLimit);

  RefPtr<nsVirtualFileSystem> rawstorage = new nsVirtualFileSystem();
  rawstorage->SetInfo(info);
  rawstorage->SetRequestManager(aRequestMgr);

  SetupFuseDevice(rawstorage,
                  mountPoint,
                  fileSystemId,
                  displayName,
                  requestId,
                  aCallback);

  vfs = rawstorage.forget();

  MonitorAutoLock lock(mArrayMonitor);
  mVirtualFileSystemArray.AppendElement(vfs);

  return NS_OK;
}

NS_IMETHODIMP
nsVirtualFileSystemService::Unmount(nsIVirtualFileSystemUnmountRequestedOptions* aOption,
                                    nsIVirtualFileSystemCallback* aCallback)
{
  nsString fileSystemId;
  uint32_t requestId;
  aOption->GetFileSystemId(fileSystemId);
  aOption->GetRequestId(&requestId);

  if (fileSystemId.IsEmpty()) {
    ERR("Empty file system ID.");
    aCallback->OnError(requestId, nsIVirtualFileSystemCallback::ERROR_FAILED);
    return NS_ERROR_FAILURE;
  }

  RefPtr<nsIVirtualFileSystem> vfs = FindVirtualFileSystemById(fileSystemId);
  if (vfs == nullptr) {
    ERR("The cloud storgae '%s' does not exist.",
         NS_ConvertUTF16toUTF8(fileSystemId).get());
    aCallback->OnError(requestId, nsIVirtualFileSystemCallback::ERROR_FAILED);
    return NS_ERROR_FAILURE;
  }

  ShotdownFuseDevice(fileSystemId, requestId, aCallback);

  MonitorAutoLock lock(mArrayMonitor);
  mVirtualFileSystemArray.RemoveElement(vfs);
  return NS_OK;
}

NS_IMETHODIMP
nsVirtualFileSystemService::GetVirtualFileSystemById(const nsAString& aFileSystemId,
                                                     nsIVirtualFileSystemInfo** aInfo)
{
  MonitorAutoLock lock(mArrayMonitor);
  RefPtr<nsIVirtualFileSystem> vfs = FindVirtualFileSystemById(aFileSystemId);
  if (vfs == nullptr) {
    ERR("The cloud storage '%s' does not exist.",
         NS_ConvertUTF16toUTF8(aFileSystemId).get());
    return NS_ERROR_NOT_AVAILABLE;
  }
  RefPtr<nsIVirtualFileSystemInfo> info;
  vfs->GetInfo(getter_AddRefs(info));
  info.forget(aInfo);
  return NS_OK;
}

NS_IMETHODIMP
nsVirtualFileSystemService::GetAllVirtualFileSystemIds(nsIArray** aCloudNames)
{
  NS_ENSURE_ARG_POINTER(aCloudNames);
  MonitorAutoLock lock(mArrayMonitor);
  *aCloudNames = nullptr;
  nsresult rv;
  nsCOMPtr<nsIMutableArray> cloudNames =
    do_CreateInstance(NS_ARRAY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  VirtualFileSystemArray::size_type numClouds = mVirtualFileSystemArray.Length();
  VirtualFileSystemArray::index_type cloudIndex;
  for (cloudIndex = 0; cloudIndex < numClouds; cloudIndex++) {
    RefPtr<nsIVirtualFileSystem> vfs = mVirtualFileSystemArray[cloudIndex];
    nsCOMPtr<nsISupportsString> isupportsString =
      do_CreateInstance(NS_SUPPORTS_STRING_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    RefPtr<nsIVirtualFileSystemInfo> info;
    rv = vfs->GetInfo(getter_AddRefs(info));
    NS_ENSURE_SUCCESS(rv, rv);
    nsString fileSystemId;
    rv = info->GetFileSystemId(fileSystemId);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = isupportsString->SetData(fileSystemId);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = cloudNames->AppendElement(isupportsString, false);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  cloudNames.forget(aCloudNames);
  return NS_OK;
}

NS_IMETHODIMP
nsVirtualFileSystemService::GetRequestManagerById(const nsAString& aFileSystemId,
                                                  nsIVirtualFileSystemRequestManager** aMgr)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}
/////////////////////////////////////////////////////////////////
already_AddRefed<nsIVirtualFileSystem>
nsVirtualFileSystemService::FindVirtualFileSystemById(const nsAString& aFileSystemId)
{
  MonitorAutoLock lock(mArrayMonitor);
  VirtualFileSystemArray::size_type  numVirtualFileSystems =
                                mVirtualFileSystemArray.Length();
  VirtualFileSystemArray::index_type vfsIndex;
  for (vfsIndex = 0; vfsIndex < numVirtualFileSystems; vfsIndex++) {
    RefPtr<nsIVirtualFileSystem> vfs = mVirtualFileSystemArray[vfsIndex];
    RefPtr<nsIVirtualFileSystemInfo> info;
    nsresult rv = vfs->GetInfo(getter_AddRefs(info));
    if (NS_FAILED(rv)) {
      ERR("Fail to get the cloud storage info");
      return nullptr;
    }
    nsString fileSystemId;
    rv = info->GetFileSystemId(fileSystemId);
    if (NS_FAILED(rv)) {
      ERR("Fail to get the cloud storage file system id");
      return nullptr;
    }
    if (fileSystemId.Equals(aFileSystemId)) {
      return vfs.forget();
    }
  }
  return nullptr;
}

nsString
nsVirtualFileSystemService::CreateMountPoint(const nsAString& aFileSystemId)
{
  nsString mountPoint = NS_LITERAL_STRING(MOUNTROOT);
  mountPoint.Append(NS_LITERAL_STRING("/"));
  mountPoint.Append(aFileSystemId);
  return mountPoint;
}

} // end namespace virtualfilesystem
} // end namespace dom
} // end namespace mozilla
