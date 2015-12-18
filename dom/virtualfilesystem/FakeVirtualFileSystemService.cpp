/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "FakeVirtualFileSystemService.h"
#include "mozilla/ClearOnShutdown.h"
#include "nsComponentManagerUtils.h"
#include "nsIMutableArray.h"
#include "nsIVirtualFileSystemCallback.h"
#include "nsVirtualFileSystemDataType.h"
#include "nsVirtualFileSystemRequestManager.h"

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

class MountUnmountErrorCallback final : public nsRunnable {
public:
  explicit MountUnmountErrorCallback(uint32_t aRequestId,
                                     nsIVirtualFileSystemCallback* aCallback,
                                     uint32_t aErrorCode)
    : mRequestId(aRequestId)
    , mCallback(aCallback)
    , mErrorCode(aErrorCode)
  {}

  NS_IMETHOD Run()
  {
    mCallback->OnError(mRequestId, mErrorCode);
    return NS_OK;
  }

private:
  uint32_t mRequestId;
  nsCOMPtr<nsIVirtualFileSystemCallback> mCallback;
  uint32_t mErrorCode;
};

template <class Derived> StaticRefPtr<Derived>
BaseVirtualFileSystemServiceWrapper<Derived>::sSingleton;

/* static */ template <class Derived> already_AddRefed<Derived>
BaseVirtualFileSystemServiceWrapper<Derived>::GetSingleton()
{
  if (!sSingleton) {
    sSingleton = new Derived();
    ClearOnShutdown(&sSingleton);
  }

  RefPtr<Derived> service = sSingleton.get();
  return service.forget();
}

namespace {

struct VirtualFileSystemOpenedFileInfoComparator {
  bool Equals(const nsCOMPtr<nsIVirtualFileSystemOpenedFileInfo>& aA,
              const nsCOMPtr<nsIVirtualFileSystemOpenedFileInfo>& aB) const {
    uint32_t idA, idB;
    aA->GetOpenRequestId(&idA);
    aB->GetOpenRequestId(&idB);
    return idA == idB;
  }
};

}

NS_IMPL_ISUPPORTS0(BaseVirtualFileSystemService::VirtualFileSystem)

bool
BaseVirtualFileSystemService::FindVirtualFileSystemById(const nsAString& aFileSystemId,
                                                      uint32_t& aIndex)
{
  RefPtr<VirtualFileSystem> storage =
    new VirtualFileSystem(aFileSystemId,
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

bool
BaseVirtualFileSystemService::VirtualFileSystem::AddOpenedFile(
  nsIVirtualFileSystemOpenedFileInfo* aFile)
{
  if (NS_WARN_IF(!aFile)) {
    return false;
  }

  uint32_t openRequestId;
  aFile->GetOpenRequestId(&openRequestId);
  size_t index =
    mOpenedFiles.IndexOf(aFile, 0, VirtualFileSystemOpenedFileInfoComparator());
  if (index == mOpenedFiles.NoIndex) {
    return false;
  }

  mOpenedFiles.AppendElement(aFile);
  return true;
}

void
BaseVirtualFileSystemService::VirtualFileSystem::RemoveOpenedFile(
  uint32_t aOpenRequestId)
{
  nsCOMPtr<nsIVirtualFileSystemOpenedFileInfo> openedFile =
    new virtualfilesystem::nsVirtualFileSystemOpenedFileInfo();
  openedFile->SetOpenRequestId(aOpenRequestId);
  size_t index =
    mOpenedFiles.IndexOf(openedFile, 0, VirtualFileSystemOpenedFileInfoComparator());
  if (index != mOpenedFiles.NoIndex) {
    mOpenedFiles.RemoveElementAt(index);
  }
}

already_AddRefed<nsIVirtualFileSystemInfo>
BaseVirtualFileSystemService::VirtualFileSystem::ConvertToVirtualFileSystemInfo() const
{
  nsCOMPtr<nsIVirtualFileSystemInfo> info = new nsVirtualFileSystemInfo();
  info->SetFileSystemId(mFileSystemId);
  info->SetDisplayName(mDisplayName);
  info->SetWritable(mWritable);
  info->SetOpenedFilesLimit(mOpenedFilesLimit);
  for (auto it : mOpenedFiles) {
    info->AppendOpenedFile(it);
  }
  return info.forget();
}

bool
BaseVirtualFileSystemService::MountInternal(
  const MountOptions& aOptions,
  nsVirtualFileSystemRequestManager* aRequestManager,
  uint32_t* aErrorCode)
{
  if (!aErrorCode) {
    return false;
  }

  uint32_t index;
  if (FindVirtualFileSystemById(aOptions.mFileSystemId, index)) {
    *aErrorCode = nsIVirtualFileSystemCallback::ERROR_EXISTS;
    return false;
  }

  bool writable = false;
  if (aOptions.mWritable.WasPassed() && !aOptions.mWritable.Value().IsNull()) {
    writable = aOptions.mWritable.Value().Value();
  }

  uint32_t openedFilesLimit = 0;
  if (aOptions.mOpenedFilesLimit.WasPassed() &&
      !aOptions.mOpenedFilesLimit.Value().IsNull()) {
    openedFilesLimit = aOptions.mOpenedFilesLimit.Value().Value();
  }
  RefPtr<VirtualFileSystem> storage = new VirtualFileSystem(aOptions.mFileSystemId,
                                                            aOptions.mDisplayName,
                                                            writable,
                                                            openedFilesLimit,
                                                            aRequestManager);

  mVirtualFileSystems.AppendElement(storage);
  return true;
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
  if (!FindVirtualFileSystemById(aOptions.mFileSystemId, index)) {
    *aErrorCode = nsIVirtualFileSystemCallback::ERROR_NOT_FOUND;
    return false;
  }

  mVirtualFileSystems.RemoveElementAt(index);
  return true;
}

nsresult
FakeVirtualFileSystemService::Mount(uint32_t aRequestId,
                                    const MountOptions& aOptions,
                                    nsVirtualFileSystemRequestManager* aRequestManager,
                                    nsIVirtualFileSystemCallback* aCallback)
{
  if (NS_WARN_IF(!(aRequestManager && aCallback))) {
    return NS_ERROR_INVALID_ARG;
  }

  uint32_t errorCode;
  if (!MountInternal(aOptions, aRequestManager, &errorCode)) {
    aCallback->OnError(aRequestId, errorCode);
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIRunnable> callback =
    new MountUnmountSuccessCallback(aRequestId, aCallback);
  return NS_DispatchToCurrentThread(callback);
}

nsresult
FakeVirtualFileSystemService::Unmount(
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

  nsCOMPtr<nsIRunnable> callback =
    new MountUnmountSuccessCallback(aRequestId, aCallback);
  return NS_DispatchToCurrentThread(callback);
}

nsresult
FakeVirtualFileSystemService::GetVirtualFileSystemById(const nsAString& aFileSystemId,
                                                       nsIVirtualFileSystemInfo** aInfo)
{
  uint32_t index;
  if (!FindVirtualFileSystemById(aFileSystemId, index)) {
    *aInfo = nullptr;
    return NS_ERROR_INVALID_ARG;
  }

  RefPtr<VirtualFileSystem> fileSystem = mVirtualFileSystems[index];

  nsCOMPtr<nsIVirtualFileSystemOpenedFileInfo> mockFile =
    MockOpenedFileInfo(NS_LITERAL_STRING("/dummy/path/"),
                       nsIVirtualFileSystemOpenFileRequestedOptions::OPEN_MODE_READ,
                       1);
  fileSystem->AddOpenedFile(mockFile);

  nsCOMPtr<nsIVirtualFileSystemInfo> info = fileSystem->ConvertToVirtualFileSystemInfo();
  info.forget(aInfo);
  return NS_OK;
}

nsresult
FakeVirtualFileSystemService::GetAllVirtualFileSystem(nsIArray** aFileSystems)
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

} // namespace virtualfilesystem
} // namespace dom
} // namespace mozilla
