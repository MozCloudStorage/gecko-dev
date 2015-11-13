/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "FakeVirtualFileSystemService.h"
#include "mozilla/StaticPtr.h"
#include "mozilla/ClearOnShutdown.h"
#include "nsCOMPtr.h"
#include "nsComponentManagerUtils.h"
#include "nsIMutableArray.h"
#include "nsIVirtualFileSystemCallback.h"
#include "nsVirtualFileSystemDataType.h"
#include "nsThreadUtils.h"

namespace mozilla {
namespace dom {

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

class MountUnmountErrorCallback final : public nsRunnable
{
public:
  explicit MountUnmountErrorCallback(uint32_t aRequestId,
                                     nsIVirtualFileSystemCallback* aCallback)
    : mRequestId(aRequestId)
    , mCallback(aCallback)
  {
  }

  NS_IMETHOD Run()
  {
    mCallback->OnError(mRequestId, 0);
    return NS_OK;
  }

private:
  uint32_t mRequestId;
  nsCOMPtr<nsIVirtualFileSystemCallback> mCallback;
};

NS_IMPL_ISUPPORTS0(FakeVirtualFileSystemService::VirtualFileSystem)

NS_IMPL_ISUPPORTS(FakeVirtualFileSystemService,
                  nsIVirtualFileSystemService)

StaticRefPtr<FakeVirtualFileSystemService> sSingleton;

/* static */ FakeVirtualFileSystemService*
FakeVirtualFileSystemService::GetSingleton()
{
  if (!sSingleton) {
    sSingleton = new FakeVirtualFileSystemService();
    ClearOnShutdown(&sSingleton);
  }
  return sSingleton;
}

NS_IMETHODIMP
FakeVirtualFileSystemService::Mount(nsIVirtualFileSystemMountRequestedOptions* aOptions,
                                    nsIVirtualFileSystemRequestManager* aRequestManager,
                                    nsIVirtualFileSystemCallback* aCallback)
{
  if (NS_WARN_IF(!(aOptions && aRequestManager && aCallback))) {
    return NS_ERROR_INVALID_ARG;
  }

  uint32_t requestId;
  aOptions->GetRequestId(&requestId);
  nsString fileSystemId;
  aOptions->GetFileSystemId(fileSystemId);

  uint32_t index;
  if (FindVirtualFileSystemById(fileSystemId, index)) {
    nsCOMPtr<nsIRunnable> callback = new MountUnmountErrorCallback(requestId,
                                                                   aCallback);
    return NS_DispatchToCurrentThread(callback);
  }

  nsString displayName;
  aOptions->GetDisplayName(displayName);
  bool writable;
  aOptions->GetWritable(&writable);
  uint32_t openedFilesLimit;
  aOptions->GetOpenedFilesLimit(&openedFilesLimit);

  RefPtr<VirtualFileSystem> storage = new VirtualFileSystem(fileSystemId,
                                                            displayName,
                                                            writable,
                                                            openedFilesLimit,
                                                            aRequestManager);

  mVirtualFileSystems.AppendElement(storage);
  nsCOMPtr<nsIRunnable> callback = new MountUnmountSuccessCallback(requestId,
                                                                   aCallback);
  return NS_DispatchToCurrentThread(callback);
}

NS_IMETHODIMP
FakeVirtualFileSystemService::Unmount(nsIVirtualFileSystemUnmountRequestedOptions* aOptions,
                                      nsIVirtualFileSystemCallback* aCallback)
{
  if (NS_WARN_IF(!(aOptions && aCallback))) {
    return NS_ERROR_INVALID_ARG;
  }

  uint32_t requestId;
  aOptions->GetRequestId(&requestId);
  nsString fileSystemId;
  aOptions->GetFileSystemId(fileSystemId);

  uint32_t index;
  if (!FindVirtualFileSystemById(fileSystemId, index)) {
    nsCOMPtr<nsIRunnable> callback = new MountUnmountErrorCallback(requestId,
                                                                   aCallback);
    return NS_DispatchToCurrentThread(callback);
  }

  mVirtualFileSystems.RemoveElementAt(index);
  nsCOMPtr<nsIRunnable> callback = new MountUnmountSuccessCallback(requestId,
                                                                   aCallback);
  return NS_DispatchToCurrentThread(callback);
}

NS_IMETHODIMP
FakeVirtualFileSystemService::GetVirtualFileSystemById(const nsAString& aFileSystemId,
                                                       nsIVirtualFileSystemInfo** aInfo)
{
  uint32_t index;
  if (!FindVirtualFileSystemById(aFileSystemId, index)) {
    *aInfo = nullptr;
    return NS_ERROR_INVALID_ARG;
  }

  RefPtr<VirtualFileSystem> fileSystem = mVirtualFileSystems[index];
  nsCOMPtr<nsIVirtualFileSystemInfo> info = new virtualfilesystem::nsVirtualFileSystemInfo();
  info->SetFileSystemId(fileSystem->FileSystemId());
  info->SetDisplayName(fileSystem->DisplayName());
  info->SetWritable(fileSystem->Writable());
  info->SetOpenedFilesLimit(fileSystem->OpenedFilesLimit());

  nsCOMPtr<nsIVirtualFileSystemOpenedFileInfo> mockFile =
    MockOpenedFileInfo(NS_LITERAL_STRING("/dummy/path/"),
                       nsIVirtualFileSystemOpenFileRequestedOptions::OPEN_MODE_READ,
                       1);
  info->AppendOpenedFile(mockFile);
  info.forget(aInfo);
  return NS_OK;
}

NS_IMETHODIMP
FakeVirtualFileSystemService::GetAllVirtualFileSystemIds(nsIArray** aFileSystems)
{
  nsresult rv;
  nsCOMPtr<nsIMutableArray> fileSystemArray = do_CreateInstance(NS_ARRAY_CONTRACTID, &rv);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  for (const auto& fileSystem : mVirtualFileSystems) {
    nsCOMPtr<nsIVirtualFileSystemInfo> info;
    if (NS_SUCCEEDED(GetVirtualFileSystemById(fileSystem->FileSystemId(),
                                              getter_AddRefs(info)))) {
      fileSystemArray->AppendElement(info, false);
    }
  }
  fileSystemArray.forget(aFileSystems);
  return NS_OK;
}

NS_IMETHODIMP
FakeVirtualFileSystemService::GetRequestManagerById(const nsAString& aFileSystemId,
                                                    nsIVirtualFileSystemRequestManager** aManager)
{
  uint32_t index;
  if (!FindVirtualFileSystemById(aFileSystemId, index)) {
    *aManager = nullptr;
    return NS_ERROR_INVALID_ARG;
  }

  RefPtr<VirtualFileSystem> fileSystem = mVirtualFileSystems[index];
  nsCOMPtr<nsIVirtualFileSystemRequestManager> manager = fileSystem->RequestManager();
  manager.forget(aManager);
  return NS_OK;
}

already_AddRefed<nsIVirtualFileSystemOpenedFileInfo>
FakeVirtualFileSystemService::MockOpenedFileInfo(const nsAString& aFilePath,
                                                 uint32_t aMode,
                                                 uint32_t aOpenRequestId)
{
  nsCOMPtr<nsIVirtualFileSystemOpenedFileInfo> openedFile =
    new virtualfilesystem::nsVirtualFileSystemOpenedFileInfo();
  openedFile->SetFilePath(aFilePath);
  openedFile->SetMode(aMode);
  openedFile->SetOpenRequestId(aOpenRequestId);
  return openedFile.forget();
}

bool
FakeVirtualFileSystemService::FindVirtualFileSystemById(const nsAString& aFileSystemId,
                                                        uint32_t& aIndex)
{
  RefPtr<VirtualFileSystem> storage = new VirtualFileSystem(aFileSystemId,
                                                            /* aDisplayName */ EmptyString(),
                                                            /* aWritable */ false,
                                                            /* aOpenedFilesLimit */ 0,
                                                            /* aRequestManager */ nullptr);

  size_t index = mVirtualFileSystems.IndexOf(storage, 0, VirtualFileSystemComparator());
  if (index == mVirtualFileSystems.NoIndex) {
    return false;
  }

  aIndex = index;
  return true;
}

} // namespace dom
} // namespace mozilla
