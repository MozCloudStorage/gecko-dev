/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/ClearOnShutdown.h"
#include "mozilla/dom/FileSystemProviderBinding.h"
#include "nsComponentManagerUtils.h"
#include "nsIMutableArray.h"
#include "nsIVirtualFileSystemCallback.h"
#include "nsVirtualFileSystem.h"
#include "nsVirtualFileSystemRequestManager.h"
#include "nsVirtualFileSystemService.h"
#include "nsThreadUtils.h"
#include "nsXULAppAPI.h"

#ifdef MOZ_WIDGET_GONK
#include "FuseManager.h"
#define MOUNTROOT "/data/vfs"
#endif

namespace mozilla {
namespace dom {
namespace virtualfilesystem {

class MountUnmountSuccessCallback final : public nsRunnable
{
public:
  explicit MountUnmountSuccessCallback(uint32_t aRequestId,
                                       nsIVirtualFileSystemCallback* aCallback)
    : mRequestId(aRequestId)
    , mCallback(aCallback)
  {
  }

  NS_IMETHOD Run()
  {
    mCallback->OnSuccess(mRequestId, nullptr, false);
    return NS_OK;
  }

private:
  uint32_t mRequestId;
  nsCOMPtr<nsIVirtualFileSystemCallback> mCallback;
};

NS_IMPL_ISUPPORTS0(BaseVirtualFileSystemService)

namespace {

struct FileSystemInfoComparator {
  bool Equals(const RefPtr<FileSystemInfoWrapper>& aA,
              const RefPtr<FileSystemInfoWrapper>& aB) const {
    return aA->FileSystemId() == aB->FileSystemId();
  }
};

} // anonymous namespace

bool
BaseVirtualFileSystemService::FindFileSystemInfoById(const nsAString& aFileSystemId,
                                                     uint32_t& aIndex)
{
  MountOptions options;
  options.mFileSystemId = aFileSystemId;
  RefPtr<FileSystemInfoWrapper> info = new FileSystemInfoWrapper(options);

  size_t index =
    mVirtualFileSystems.IndexOf(info, 0, FileSystemInfoComparator());
  if (index == mVirtualFileSystems.NoIndex) {
    return false;
  }

  aIndex = index;
  return true;
}

nsresult
BaseVirtualFileSystemService::GetFileSysetmInfoById(const nsAString& aFileSystemId,
                                                    FileSystemInfo& aInfo)
{
  uint32_t index;
  if (!FindFileSystemInfoById(aFileSystemId, index)) {
    return NS_ERROR_INVALID_ARG;
  }

  RefPtr<FileSystemInfoWrapper> info = mVirtualFileSystems[index];

  info->GetFileSystemInfo(aInfo);
  return NS_OK;
}

void
BaseVirtualFileSystemService::GetAllFileSystemInfo(
  nsTArray<FileSystemInfo>& aArray)
{
  aArray.Clear();
  for (const auto& it : mVirtualFileSystems) {
    FileSystemInfo info;
    it->GetFileSystemInfo(info);
    aArray.AppendElement(info);
  }
}

already_AddRefed<FileSystemInfoWrapper>
BaseVirtualFileSystemService::MountInternal(
  const MountOptions& aOptions,
  nsVirtualFileSystemRequestManager* aRequestManager,
  uint32_t* aErrorCode)
{
  if (!aErrorCode) {
    return nullptr;
  }

  uint32_t index;
  if (FindFileSystemInfoById(aOptions.mFileSystemId, index)) {
    *aErrorCode = nsIVirtualFileSystemCallback::ERROR_EXISTS;
    return nullptr;
  }

  RefPtr<FileSystemInfoWrapper> info = new FileSystemInfoWrapper(aOptions);

  mVirtualFileSystems.AppendElement(info);
  return info.forget();
}

bool
BaseVirtualFileSystemService::UnmountInternal(
  const UnmountOptions& aOptions,
  uint32_t* aErrorCode)
{
  if (!aErrorCode) {
    return false;
  }

  uint32_t index;
  if (!FindFileSystemInfoById(aOptions.mFileSystemId, index)) {
    *aErrorCode = nsIVirtualFileSystemCallback::ERROR_NOT_FOUND;
    return false;
  }

  mVirtualFileSystems.RemoveElementAt(index);
  return true;
}

StaticRefPtr<nsVirtualFileSystemService>
nsVirtualFileSystemService::sSingleton;

/* static */ already_AddRefed<nsVirtualFileSystemService>
nsVirtualFileSystemService::GetSingleton()
{
  if (!sSingleton) {
    sSingleton = new nsVirtualFileSystemService();
    ClearOnShutdown(&sSingleton);
  }

  RefPtr<nsVirtualFileSystemService> service = sSingleton.get();
  return service.forget();
}

NS_IMPL_ISUPPORTS_INHERITED(nsVirtualFileSystemService,
                            BaseVirtualFileSystemService,
                            nsIVirtualFileSystemService)

#ifdef MOZ_WIDGET_GONK
namespace {

nsString
CreateMountPoint(const nsAString& aFileSystemId)
{
  nsString mountPoint = NS_LITERAL_STRING(MOUNTROOT);
  mountPoint.Append(NS_LITERAL_STRING("/"));
  mountPoint.Append(aFileSystemId);
  return mountPoint;
}

} // anonymous namespace
#endif

nsresult
nsVirtualFileSystemService::Mount(uint32_t aRequestId,
                                  const MountOptions& aOptions,
                                  nsVirtualFileSystemRequestManager* aRequestManager,
                                  nsIVirtualFileSystemCallback* aCallback)
{
  if (NS_WARN_IF(!(aRequestManager && aCallback))) {
    return NS_ERROR_INVALID_ARG;
  }

  uint32_t errorCode;
  RefPtr<FileSystemInfoWrapper> info =
    MountInternal(aOptions, aRequestManager, &errorCode);
  if (!info) {
    aCallback->OnError(aRequestId, errorCode);
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIVirtualFileSystem> fileSystem =
    new nsVirtualFileSystem(info, aRequestManager);
  mVirtualFileSystemMap[aOptions.mFileSystemId] = fileSystem;
#ifdef MOZ_WIDGET_GONK
  nsString mountPoint = nsVirtualFileSystemService::CreateMountPoint(fileSystemId);
  SetupFuseDevice(fileSystem,
                  mountPoint,
                  aOptions.mFileSystemId,
                  aOptions.mDisplayName,
                  aRequestId,
                  aCallback);
#else
  nsCOMPtr<nsIVirtualFileSystemCallback> callback = aCallback;
  nsCOMPtr<nsIRunnable> runnable =
    NS_NewRunnableFunction([callback, aRequestId] () -> void
      {
        callback->OnSuccess(aRequestId, nullptr, false);
      });
  nsresult rv = NS_DispatchToCurrentThread(runnable);
  if (NS_FAILED(rv)) {
    return rv;
  }
#endif
  return NS_OK;
}

nsresult
nsVirtualFileSystemService::Unmount(
  uint32_t aRequestId,
  const UnmountOptions& aOptions,
  nsIVirtualFileSystemCallback* aCallback)
{
  if (NS_WARN_IF(!aCallback)) {
    return NS_ERROR_INVALID_ARG;
  }

  uint32_t errorCode;
  if (!UnmountInternal(aOptions, &errorCode)) {
    aCallback->OnError(aRequestId, errorCode);
    return NS_ERROR_FAILURE;
  }

#ifdef MOZ_WIDGET_GONK
  ShotdownFuseDevice(aOptions.mFileSystemId, aRequestId, aCallback);
#else
  nsCOMPtr<nsIVirtualFileSystemCallback> callback = aCallback;
  nsCOMPtr<nsIRunnable> runnable =
    NS_NewRunnableFunction([callback, aRequestId] () -> void
      {
        callback->OnSuccess(aRequestId, nullptr, false);
      });
  nsresult rv = NS_DispatchToCurrentThread(runnable);
  if (NS_FAILED(rv)) {
    return rv;
  }
#endif
  return NS_OK;
}

NS_IMETHODIMP
nsVirtualFileSystemService::GetVirtualFileSystemById(
  const nsAString& aFileSystemId,
  nsIVirtualFileSystem** aRetVal)
{
  MOZ_ASSERT(XRE_IsParentProcess());

  if (NS_WARN_IF(!aRetVal)) {
    return NS_ERROR_INVALID_ARG;
  }

  VirtualFileSystemMapType::iterator it = mVirtualFileSystemMap.find(nsString(aFileSystemId));
  if (it == mVirtualFileSystemMap.end()) {
    return NS_ERROR_INVALID_ARG;
  }

  it->second.forget(aRetVal);
  return NS_OK;
}

} // namespace virtualfilesystem
} // namespace dom
} // namespace mozilla
