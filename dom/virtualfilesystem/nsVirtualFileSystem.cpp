/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/dom/FileSystemProviderBinding.h"
#include "mozilla/dom/virtualfilesystem/PVirtualFileSystem.h"
#include "mozilla/Function.h"
#include "mozilla/Move.h"
#include "nsVirtualFileSystem.h"
#include "nsArrayUtils.h"
#include "nsThreadUtils.h"

namespace mozilla {
namespace dom {
namespace virtualfilesystem {

class CallbackWrapper final : public nsIVirtualFileSystemCallback
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIVIRTUALFILESYSTEMCALLBACK

  explicit CallbackWrapper(nsIVirtualFileSystemCallback* aCallback,
                           mozilla::Function<void(uint32_t)> aFunction)
    : mSavedCallback(aCallback)
    , mFunction(Move(aFunction))
  {}

private:
  ~CallbackWrapper() = default;

  nsCOMPtr<nsIVirtualFileSystemCallback> mSavedCallback;
  mozilla::Function<void(uint32_t)> mFunction;
};

NS_IMPL_ISUPPORTS(CallbackWrapper, nsIVirtualFileSystemCallback)

NS_IMETHODIMP
CallbackWrapper::OnSuccess(uint32_t aRequestId,
                           nsIVirtualFileSystemRequestValue* aValue,
                           bool aHasMore)
{
  mFunction(aRequestId);
  return mSavedCallback->OnSuccess(aRequestId, aValue, aHasMore);
}

NS_IMETHODIMP
CallbackWrapper::OnError(uint32_t aRequestId, uint32_t aErrorCode)
{
  return mSavedCallback->OnError(aRequestId, aErrorCode);
}

FileSystemInfoWrapper::FileSystemInfoWrapper(const MountOptions& aOptions)
{
  mFileSystemInfo.mFileSystemId.Construct(aOptions.mFileSystemId);
  mFileSystemInfo.mDisplayName.Construct(aOptions.mDisplayName);

  bool writable = false;
  if (aOptions.mWritable.WasPassed() && !aOptions.mWritable.Value().IsNull()) {
    writable = aOptions.mWritable.Value().Value();
  }
  mFileSystemInfo.mWritable.Construct(writable);

  uint32_t openedFilesLimit = 0;
  if (aOptions.mOpenedFilesLimit.WasPassed() &&
      !aOptions.mOpenedFilesLimit.Value().IsNull()) {
    openedFilesLimit = aOptions.mOpenedFilesLimit.Value().Value();
  }
  mFileSystemInfo.mOpenedFilesLimit.Construct(openedFilesLimit);

  mFileSystemInfo.mOpenedFiles.Construct();
}

void
FileSystemInfoWrapper::GetFileSystemInfo(FileSystemInfo &aInfo) const
{
  aInfo = mFileSystemInfo;
}

const nsString&
FileSystemInfoWrapper::FileSystemId() const
{
  return mFileSystemInfo.mFileSystemId.Value();
}

bool
FileSystemInfoWrapper::Writable() const
{
  return mFileSystemInfo.mWritable.Value();
}

uint32_t
FileSystemInfoWrapper::OpenedFilesLimit() const
{
  return mFileSystemInfo.mOpenedFilesLimit.Value();
}

uint32_t
FileSystemInfoWrapper::OpenedFilesCount() const
{
  return mFileSystemInfo.mOpenedFiles.Value().Length();
}

namespace {

struct OpenedFileComparator {
  bool Equals(const OpenedFile& aA,
              const OpenedFile& aB) const {
    return aA.mOpenRequestId == aB.mOpenRequestId;
  }
};

}

void
FileSystemInfoWrapper::AppendOpenedFile(const OpenedFile& aFile)
{
  Sequence<OpenedFile>& openedFiles = mFileSystemInfo.mOpenedFiles.Value();
  size_t index = openedFiles.IndexOf(aFile, 0, OpenedFileComparator());
  if (index == openedFiles.NoIndex) {
    openedFiles.AppendElement(aFile, fallible);
  }
}

void
FileSystemInfoWrapper::RemoveOpenedFile(uint32_t aOpenRequestId)
{
  Sequence<OpenedFile>& openedFiles = mFileSystemInfo.mOpenedFiles.Value();
  OpenedFile openedFile;
  openedFile.mOpenRequestId.Construct(aOpenRequestId);
  size_t index = openedFiles.IndexOf(openedFile, 0, OpenedFileComparator());
  if (index != openedFiles.NoIndex) {
    openedFiles.RemoveElementAt(index);
  }
}

NS_IMPL_ISUPPORTS(nsVirtualFileSystem, nsIVirtualFileSystem)

nsVirtualFileSystem::nsVirtualFileSystem(
  FileSystemInfoWrapper* aFileSysetmInfo,
  nsVirtualFileSystemRequestManager* aRequestManager)
  : mFileSystemInfo(aFileSysetmInfo)
  , mRequestManager(aRequestManager)
{
  MOZ_ASSERT(XRE_IsParentProcess());
}

NS_IMETHODIMP
nsVirtualFileSystem::Abort(const uint32_t aOperationId,
                           nsIVirtualFileSystemCallback* aCallback,
                           uint32_t* aRequestId)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mRequestManager);

  printf_stderr("############### abort\n");
  if (NS_WARN_IF(!(aCallback && aRequestId))) {
    return NS_ERROR_INVALID_ARG;
  }

  VirtualFileSystemAbortRequestedOptions options(aOperationId);
  printf_stderr("############### abort 1\n");
  return mRequestManager->CreateRequest(
    mFileSystemInfo->FileSystemId(),
    VirtualFileSystemIPCRequestedOptions(options),
    aCallback,
    aRequestId);
}

NS_IMETHODIMP
nsVirtualFileSystem::OpenFile(const nsAString& aPath,
                              uint32_t aMode,
                              nsIVirtualFileSystemCallback* aCallback,
                              uint32_t* aRequestId)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mRequestManager);

  if (NS_WARN_IF(!(aCallback && aRequestId))) {
    return NS_ERROR_INVALID_ARG;
  }

  if (aMode != nsIVirtualFileSystem::OPEN_FILE_MODE_READ ||
      aMode != nsIVirtualFileSystem::OPEN_FILE_MODE_WRITE) {
    return NS_ERROR_INVALID_ARG;
  }

  if (!mFileSystemInfo->Writable() &&
      aMode == nsIVirtualFileSystem::OPEN_FILE_MODE_WRITE) {
    aCallback->OnError(0, nsIVirtualFileSystemCallback::ERROR_ACCESS_DENIED);
    return NS_OK;
  }

  if (mFileSystemInfo->OpenedFilesCount() == mFileSystemInfo->OpenedFilesLimit()) {
    aCallback->OnError(0, nsIVirtualFileSystemCallback::ERROR_TOO_MANY_OPENED);
    return NS_OK;
  }

  VirtualFileSystemOpenFileRequestedOptions options(aMode, nsString(aPath));
  RefPtr<nsVirtualFileSystem> self = this;
  nsString path(aPath);
  auto func = [path, aMode, self] (uint32_t aId) -> void {
    OpenedFile file;
    file.mFilePath.Construct(path);
    file.mMode.Construct(static_cast<OpenFileMode>(aMode));
    file.mOpenRequestId.Construct(aId);
    self->mFileSystemInfo->AppendOpenedFile(file);
  };
  nsCOMPtr<nsIVirtualFileSystemCallback> callback =
    new CallbackWrapper(aCallback, func);

  return mRequestManager->CreateRequest(
    mFileSystemInfo->FileSystemId(),
    VirtualFileSystemIPCRequestedOptions(options),
    callback,
    aRequestId);
}

NS_IMETHODIMP
nsVirtualFileSystem::CloseFile(uint32_t aOpenFileRequestId,
                               nsIVirtualFileSystemCallback* aCallback,
                               uint32_t* aRequestId)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mRequestManager);

  if (NS_WARN_IF(!(aCallback && aRequestId))) {
    return NS_ERROR_INVALID_ARG;
  }

  VirtualFileSystemCloseFileRequestedOptions options(aOpenFileRequestId);
  RefPtr<nsVirtualFileSystem> self = this;
  auto func = [self] (uint32_t aId) -> void {
    self->mFileSystemInfo->RemoveOpenedFile(aId);
  };
  nsCOMPtr<nsIVirtualFileSystemCallback> callback =
    new CallbackWrapper(aCallback, func);

  return mRequestManager->CreateRequest(
    mFileSystemInfo->FileSystemId(),
    VirtualFileSystemIPCRequestedOptions(options),
    callback,
    aRequestId);
}

NS_IMETHODIMP
nsVirtualFileSystem::GetMetadata(const nsAString& aEntryPath,
                                 nsIVirtualFileSystemCallback* aCallback,
                                 uint32_t* aRequestId)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mRequestManager);

  if (NS_WARN_IF(!(aCallback && aRequestId))) {
    return NS_ERROR_INVALID_ARG;
  }

  VirtualFileSystemGetMetadataRequestedOptions options((nsString(aEntryPath)));

  return mRequestManager->CreateRequest(
    mFileSystemInfo->FileSystemId(),
    VirtualFileSystemIPCRequestedOptions(options),
    aCallback,
    aRequestId);
}

NS_IMETHODIMP
nsVirtualFileSystem::ReadDirectory(const nsAString& aDirPath,
                                   nsIVirtualFileSystemCallback* aCallback,
                                   uint32_t* aRequestId)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mRequestManager);

  if (NS_WARN_IF(!(aCallback && aRequestId))) {
    return NS_ERROR_INVALID_ARG;
  }

  nsString dirPath(aDirPath);
  VirtualFileSystemReadDirectoryRequestedOptions options((nsString(aDirPath)));

  return mRequestManager->CreateRequest(
    mFileSystemInfo->FileSystemId(),
    VirtualFileSystemIPCRequestedOptions(options),
    aCallback,
    aRequestId);
}

NS_IMETHODIMP
nsVirtualFileSystem::ReadFile(const uint32_t aOpenFileRequestId,
                              const uint64_t aOffset,
                              const uint64_t aLength,
                              nsIVirtualFileSystemCallback* aCallback,
                              uint32_t* aRequestId)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mRequestManager);

  if (NS_WARN_IF(!(aCallback && aRequestId))) {
    return NS_ERROR_INVALID_ARG;
  }

  VirtualFileSystemReadFileRequestedOptions options(aOpenFileRequestId, aLength, aOffset);

  return mRequestManager->CreateRequest(
    mFileSystemInfo->FileSystemId(),
    VirtualFileSystemIPCRequestedOptions(options),
    aCallback,
    aRequestId);
}

NS_IMETHODIMP
nsVirtualFileSystem::Unmount(nsIVirtualFileSystemCallback* aCallback, uint32_t* aRequestId)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mRequestManager);

  if (NS_WARN_IF(!(aCallback && aRequestId))) {
    return NS_ERROR_INVALID_ARG;
  }

  VirtualFileSystemUnmountRequestedOptions options;

  return mRequestManager->CreateRequest(
    mFileSystemInfo->FileSystemId(),
    VirtualFileSystemIPCRequestedOptions(options),
    aCallback,
    aRequestId);
}

} // end namespace virtualfilesystem
} // end namespace dom
} // end namespace mozilla