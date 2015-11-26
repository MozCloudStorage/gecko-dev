/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/dom/FileSystemProviderAbortEvent.h"
#include "mozilla/dom/FileSystemProviderBinding.h"
#include "mozilla/dom/FileSystemProviderCloseFileEvent.h"
#include "mozilla/dom/FileSystemProviderGetMetadataEvent.h"
#include "mozilla/dom/FileSystemProviderOpenFileEvent.h"
#include "mozilla/dom/FileSystemProviderReadDirectoryEvent.h"
#include "mozilla/dom/FileSystemProviderReadFileEvent.h"
#include "mozilla/dom/FileSystemProviderUnmountEvent.h"
#include "mozilla/dom/Promise.h"
#include "mozilla/unused.h"

#include "VirtualFileSystemServiceFactory.h"
#include "FileSystemProvider.h"
#include "nsArrayUtils.h"
#include "nsVirtualFileSystemDataType.h"
#include "nsVirtualFileSystemRequestManager.h"
#include "nsIVirtualFileSystemService.h"

namespace mozilla {
namespace dom {

using virtualfilesystem::nsVirtualFileSystemMountRequestedOptions;
using virtualfilesystem::nsVirtualFileSystemUnmountRequestedOptions;

class nsFileSystemProviderProxy : public nsISupports
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS

  explicit nsFileSystemProviderProxy(FileSystemProvider* aProvider)
    : mFileSystemProvider(aProvider) {}
  void Forget() { mFileSystemProvider = nullptr; }

protected:
  virtual ~nsFileSystemProviderProxy() {}

  FileSystemProvider* MOZ_NON_OWNING_REF mFileSystemProvider;
};

NS_IMPL_ISUPPORTS0(nsFileSystemProviderProxy)

class nsFileSystemProviderEventDispatcher final
  : public nsFileSystemProviderProxy
  , public nsIFileSystemProviderEventDispatcher
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIFILESYSTEMPROVIDEREVENTDISPATCHER

  explicit nsFileSystemProviderEventDispatcher(FileSystemProvider* aProvider)
    : nsFileSystemProviderProxy(aProvider) {}

private:
  virtual ~nsFileSystemProviderEventDispatcher() {}

};

NS_IMPL_ISUPPORTS_INHERITED(nsFileSystemProviderEventDispatcher,
                            nsFileSystemProviderProxy,
                            nsIFileSystemProviderEventDispatcher)

NS_IMETHODIMP
nsFileSystemProviderEventDispatcher::DispatchFileSystemProviderEvent(
  uint32_t aRequestId,
  uint32_t aRequestType,
  nsIVirtualFileSystemRequestedOptions* aOptions)
{
  return mFileSystemProvider->DispatchFileSystemProviderEventInternal(
    aRequestId,
    aRequestType,
    aOptions);
}

class MountUnmountResultCallback final : public nsFileSystemProviderProxy
                                       , public nsIVirtualFileSystemCallback
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIVIRTUALFILESYSTEMCALLBACK

  explicit MountUnmountResultCallback(FileSystemProvider* aProvider)
    : nsFileSystemProviderProxy(aProvider) {}

private:
  virtual ~MountUnmountResultCallback() {}
};

NS_IMPL_ISUPPORTS_INHERITED(MountUnmountResultCallback,
                            nsFileSystemProviderProxy,
                            nsIVirtualFileSystemCallback)

NS_IMETHODIMP
MountUnmountResultCallback::OnSuccess(uint32_t aRequestId,
                                      nsIVirtualFileSystemRequestValue* aValue,
                                      bool aHasMore)
{
  Unused << aValue;
  Unused << aHasMore;

  if (!mFileSystemProvider) {
    return NS_ERROR_NOT_INITIALIZED;
  }

  if (!NS_IsMainThread()) {
    RefPtr<MountUnmountResultCallback> self = this;
    nsCOMPtr<nsIRunnable> r = NS_NewRunnableFunction([self, aRequestId] () -> void
    {
      self->OnSuccess(aRequestId, nullptr, false);
    });
    return NS_DispatchToMainThread(r);
  }

  mFileSystemProvider->NotifyMountUnmountResult(aRequestId, true);
  Forget();
  return NS_OK;
}

NS_IMETHODIMP
MountUnmountResultCallback::OnError(uint32_t aRequestId, uint32_t aErrorCode)
{
  Unused << aErrorCode;

  if (!mFileSystemProvider) {
    return NS_ERROR_NOT_INITIALIZED;
  }

  if (!NS_IsMainThread()) {
    RefPtr<MountUnmountResultCallback> self = this;
    nsCOMPtr<nsIRunnable> r = NS_NewRunnableFunction([self, aRequestId] () -> void
    {
      self->OnError(aRequestId, false);
    });
    return NS_DispatchToMainThread(r);
  }

  mFileSystemProvider->NotifyMountUnmountResult(aRequestId, false);
  Forget();
  return NS_OK;
}

static uint32_t sRequestId = 0;

NS_IMPL_CYCLE_COLLECTION_CLASS(FileSystemProvider)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(FileSystemProvider,
                                                  DOMEventTargetHelper)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(FileSystemProvider,
                                                DOMEventTargetHelper)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_ADDREF_INHERITED(FileSystemProvider, DOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(FileSystemProvider, DOMEventTargetHelper)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(FileSystemProvider)
NS_INTERFACE_MAP_END_INHERITING(DOMEventTargetHelper)

FileSystemProvider::FileSystemProvider(nsPIDOMWindow* aWindow)
  : DOMEventTargetHelper(aWindow)
  , mEventDispatcher(new nsFileSystemProviderEventDispatcher(this))
{
}

FileSystemProvider::~FileSystemProvider()
{
  mEventDispatcher->Forget();
}

bool
FileSystemProvider::Init()
{
  mVirtualFileSystemService =
    VirtualFileSystemServiceFactory::AutoCreateVirtualFileSystemService();
  if (!mVirtualFileSystemService) {
    return false;
  }

  mRequestManager =
    new virtualfilesystem::nsVirtualFileSystemRequestManager(mEventDispatcher);
  return true;
}

JSObject*
FileSystemProvider::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return FileSystemProviderBinding::Wrap(aCx, this, aGivenProto);
}

/* static */ already_AddRefed<FileSystemProvider>
FileSystemProvider::Create(nsPIDOMWindow* aWindow)
{
  RefPtr<FileSystemProvider> provider = new FileSystemProvider(aWindow);
  return (provider->Init()) ? provider.forget() : nullptr;
}

already_AddRefed<Promise>
FileSystemProvider::Mount(const MountOptions& aOptions, ErrorResult& aRv)
{
  nsCOMPtr<nsIVirtualFileSystemMountRequestedOptions> mountOptions =
    new nsVirtualFileSystemMountRequestedOptions();
  mountOptions->SetFileSystemId(aOptions.mFileSystemId);
  mountOptions->SetDisplayName(aOptions.mDisplayName);
  if (aOptions.mWritable.WasPassed() && !aOptions.mWritable.Value().IsNull()) {
    mountOptions->SetWritable(aOptions.mWritable.Value().Value());
  }
  if (aOptions.mOpenedFilesLimit.WasPassed() &&
      !aOptions.mOpenedFilesLimit.Value().IsNull()) {
    mountOptions->SetOpenedFilesLimit(aOptions.mOpenedFilesLimit.Value().Value());
  }
  mountOptions->SetRequestId(++sRequestId);

  nsCOMPtr<nsIGlobalObject> global = do_QueryInterface(GetOwner());
  MOZ_ASSERT(global);

  RefPtr<Promise> promise = Promise::Create(global, aRv);
  if (NS_WARN_IF(aRv.Failed())) {
    return nullptr;
  }

  mPendingRequestPromises[sRequestId] = promise;

  nsCOMPtr<nsIVirtualFileSystemCallback> callback =
    new MountUnmountResultCallback(this);
  nsresult rv = mVirtualFileSystemService->Mount(mountOptions,
                                                 mRequestManager,
                                                 callback);
  if (NS_FAILED(rv)) {
    aRv.Throw(rv);
    return nullptr;
  }

  return promise.forget();
}

already_AddRefed<Promise>
FileSystemProvider::Unmount(const UnmountOptions& aOptions, ErrorResult& aRv)
{
  nsCOMPtr<nsIVirtualFileSystemUnmountRequestedOptions> unmountOptions =
    new nsVirtualFileSystemUnmountRequestedOptions();
  unmountOptions->SetFileSystemId(aOptions.mFileSystemId);
  unmountOptions->SetRequestId(++sRequestId);

  nsCOMPtr<nsIGlobalObject> global = do_QueryInterface(GetOwner());
  MOZ_ASSERT(global);

  RefPtr<Promise> promise = Promise::Create(global, aRv);
  if (NS_WARN_IF(aRv.Failed())) {
    return nullptr;
  }

  mPendingRequestPromises[sRequestId] = promise;

  nsCOMPtr<nsIVirtualFileSystemCallback> callback =
    new MountUnmountResultCallback(this);
  nsresult rv = mVirtualFileSystemService->Unmount(unmountOptions, callback);
  if (NS_FAILED(rv)) {
    aRv.Throw(rv);
    return nullptr;
  }

  return promise.forget();
}

void
FileSystemProvider::ConvertVirtualFileSystemInfo(FileSystemInfo& aRetInfo,
                                                 nsIVirtualFileSystemInfo* aInfo)
{
  if (!aInfo) {
    return;
  }

  nsString fileSystemId;
  aInfo->GetFileSystemId(fileSystemId);
  nsString displayName;
  aInfo->GetDisplayName(displayName);
  bool writable;
  aInfo->GetWritable(&writable);
  uint32_t openedFilesLimit;
  aInfo->GetOpenedFilesLimit(&openedFilesLimit);

  aRetInfo.mFileSystemId.Construct(fileSystemId);
  aRetInfo.mDisplayName.Construct(displayName);
  aRetInfo.mWritable.Construct(writable);
  aRetInfo.mOpenedFilesLimit.Construct(openedFilesLimit);

  nsCOMPtr<nsIArray> opendFiles;
  aInfo->GetOpenedFiles(getter_AddRefs(opendFiles));
  if (opendFiles) {
    Sequence<OpenedFile> openedFileSequence;
    uint32_t len = 0;
    opendFiles->GetLength(&len);
    for (uint32_t i = 0; i < len; i++) {
      nsCOMPtr<nsIVirtualFileSystemOpenedFileInfo> fileInfo
        = do_QueryElementAt(opendFiles, i);
      if (fileInfo) {
        nsString filePath;
        fileInfo->GetFilePath(filePath);
        uint32_t mode;
        fileInfo->GetMode(&mode);
        uint32_t openRequestId;
        fileInfo->GetOpenRequestId(&openRequestId);

        OpenedFile file;
        file.mFilePath.Construct(filePath);
        file.mMode.Construct(static_cast<OpenFileMode>(mode));
        file.mOpenRequestId.Construct(openRequestId);
        openedFileSequence.AppendElement(file,  fallible);
      }
    }
  aRetInfo.mOpenedFiles.Construct(openedFileSequence);
  }
}

void
FileSystemProvider::Get(const nsAString& aFileSystemId,
                        Nullable<FileSystemInfo>& aInfo,
                        ErrorResult& aRv)
{
  aInfo.SetNull();

  nsCOMPtr<nsIVirtualFileSystemInfo> info;
  nsresult rv = mVirtualFileSystemService->GetVirtualFileSystemById(aFileSystemId,
                                                                    getter_AddRefs(info));
  if (NS_FAILED(rv)) {
    aRv.Throw(rv);
    return;
  }

  ConvertVirtualFileSystemInfo(aInfo.SetValue(), info);
}

void
FileSystemProvider::GetAll(Nullable<nsTArray<FileSystemInfo>>& aRetVal, ErrorResult& aRv)
{
  aRetVal.SetNull();

  nsCOMPtr<nsIArray> fileSystemArray;
  nsresult rv = mVirtualFileSystemService->GetAllVirtualFileSystem(
    getter_AddRefs(fileSystemArray));
  if (NS_FAILED(rv)) {
    aRv.Throw(rv);
    return;
  }

  aRetVal.SetValue().TruncateLength(0);

  uint32_t len = 0;
  fileSystemArray->GetLength(&len);
  for (uint32_t i = 0; i < len; i++) {
    nsCOMPtr<nsIVirtualFileSystemInfo> info = do_QueryElementAt(fileSystemArray, i);
    if (info) {
      FileSystemInfo fileSystemInfo;
      ConvertVirtualFileSystemInfo(fileSystemInfo, info);
      aRetVal.SetValue().AppendElement(fileSystemInfo);
    }
  }
}

nsresult
FileSystemProvider::DispatchFileSystemProviderEventInternal(
  uint32_t aRequestId,
  uint32_t aRequestType,
  nsIVirtualFileSystemRequestedOptions* aOptions)
{
  if (NS_WARN_IF(!aOptions)) {
    return NS_ERROR_INVALID_ARG;
  }

  RefPtr<FileSystemProviderEvent> event;
  switch (aRequestType) {
    case nsIVirtualFileSystemRequestManager::REQUEST_ABORT:
      event = new FileSystemProviderAbortEvent(this, mRequestManager);
      break;
    case nsIVirtualFileSystemRequestManager::REQUEST_CLOSEFILE:
      event = new FileSystemProviderCloseFileEvent(this, mRequestManager);
      break;
    case nsIVirtualFileSystemRequestManager::REQUEST_GETMETADATA:
      event = new FileSystemProviderGetMetadataEvent(this, mRequestManager);
      break;
    case nsIVirtualFileSystemRequestManager::REQUEST_OPENFILE:
      event = new FileSystemProviderOpenFileEvent(this, mRequestManager);
      break;
    case nsIVirtualFileSystemRequestManager::REQUEST_READDIRECTORY:
      event = new FileSystemProviderReadDirectoryEvent(this, mRequestManager);
      break;
    case nsIVirtualFileSystemRequestManager::REQUEST_READFILE:
      event = new FileSystemProviderReadFileEvent(this, mRequestManager);
      break;
    case nsIVirtualFileSystemRequestManager::REQUEST_UNMOUNT:
      event = new FileSystemProviderUnmountEvent(this, mRequestManager);
      break;
    default:
      MOZ_ASSERT(false);
      return NS_ERROR_INVALID_ARG;
  }

  nsresult rv = event->InitFileSystemProviderEvent(aOptions);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  return DispatchTrustedEvent(event);
}

void
FileSystemProvider::NotifyMountUnmountResult(uint32_t aRequestId, bool aSucceeded)
{
  auto it = mPendingRequestPromises.find(aRequestId);
  if (NS_WARN_IF(it == mPendingRequestPromises.end())) {
    return;
  }

  if (aSucceeded) {
    it->second->MaybeResolve(JS::UndefinedHandleValue);
  }
  else {
    it->second->MaybeReject(NS_ERROR_FAILURE);
  }
  mPendingRequestPromises.erase(it);
}

} // namespace dom
} // namespace mozilla
