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

static uint32_t sRequestId = 0;

NS_IMPL_CYCLE_COLLECTION_INHERITED(FileSystemProvider, DOMEventTargetHelper,
                                   mVirtualFileSystemService,
                                   mRequestManager)

NS_IMPL_ADDREF_INHERITED(FileSystemProvider, DOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(FileSystemProvider, DOMEventTargetHelper)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(FileSystemProvider)
  NS_INTERFACE_MAP_ENTRY(nsIVirtualFileSystemCallback)
  NS_INTERFACE_MAP_ENTRY(nsIFileSystemProviderEventDispatcher)
NS_INTERFACE_MAP_END_INHERITING(DOMEventTargetHelper)

FileSystemProvider::FileSystemProvider(nsPIDOMWindow* aWindow)
  : DOMEventTargetHelper(aWindow)
{

}

FileSystemProvider::~FileSystemProvider()
{

}

bool
FileSystemProvider::Init()
{
  mVirtualFileSystemService =
    VirtualFileSystemServiceFactory::AutoCreateVirtualFileSystemService();
  if (!mVirtualFileSystemService) {
    return false;
  }

  mRequestManager = new virtualfilesystem::nsVirtualFileSystemRequestManager(this);
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
  nsCOMPtr<nsIGlobalObject> global = do_QueryInterface(GetOwner());
  MOZ_ASSERT(global);

  RefPtr<Promise> promise = Promise::Create(global, aRv);
  if (NS_WARN_IF(aRv.Failed())) {
    return nullptr;
  }

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

  mPendingRequestPromises[sRequestId] = promise;
  mVirtualFileSystemService->Mount(mountOptions, mRequestManager, this);

  return promise.forget();
}

already_AddRefed<Promise>
FileSystemProvider::Unmount(const UnmountOptions& aOptions, ErrorResult& aRv)
{
  nsCOMPtr<nsIGlobalObject> global = do_QueryInterface(GetOwner());
  MOZ_ASSERT(global);

  RefPtr<Promise> promise = Promise::Create(global, aRv);
  if (NS_WARN_IF(aRv.Failed())) {
    return nullptr;
  }

  nsCOMPtr<nsIVirtualFileSystemUnmountRequestedOptions> unmountOptions =
    new UnmountRequestedOptions();
  unmountOptions->SetFileSystemId(aOptions.mFileSystemId);
  unmountOptions->SetRequestId(++sRequestId);

  mPendingRequestPromises[sRequestId] = promise;
  mVirtualFileSystemService->Unmount(unmountOptions, this);

  return promise.forget();
}

void
FileSystemProvider::Get(const nsAString& aFileSystemId, FileSystemInfo& aInfo, ErrorResult& aRv)
{
  nsCOMPtr<nsIVirtualFileSystemInfo> info;
  nsresult rv = mVirtualFileSystemService->GetVirtualFileSystemById(aFileSystemId,
                                                                    getter_AddRefs(info));
  if (NS_FAILED(rv)) {
     aRv.Throw(rv);
     return;
   }

   nsString fileSystemId;
   info->GetFileSystemId(fileSystemId);
   nsString displayName;
   info->GetDisplayName(displayName);
   bool writable;
   info->GetWritable(&writable);
   uint32_t openedFilesLimit;
   info->GetOpenedFilesLimit(&openedFilesLimit);
   nsCOMPtr<nsIArray> opendFiles;
   info->GetOpenedFiles(getter_AddRefs(opendFiles));

   aInfo.mFileSystemId.Construct(fileSystemId);
   aInfo.mDisplayName.Construct(displayName);
   aInfo.mWritable.Construct(writable);
   aInfo.mOpenedFilesLimit.Construct(openedFilesLimit);
   if (opendFiles) {
     Sequence<OpenedFile> openedFileSequence;
     uint32_t len = 0;
     opendFiles->GetLength(&len);
     for (uint32_t i = 0; i < len; i++) {
       nsCOMPtr<nsIVirtualFileSystemOpenedFileInfo> fileInfo = do_QueryElementAt(opendFiles, i);
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
     aInfo.mOpenedFiles.Construct(openedFileSequence);
   }
}

NS_IMETHODIMP
FileSystemProvider::DispatchFileSystemProviderEvent(uint32_t aRequestId,
                                                    uint32_t aRequestType,
                                                    nsIVirtualFileSystemRequestedOptions* aOption)
{
  if (NS_WARN_IF(!aOption)) {
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

  event->InitFileSystemProviderEvent(aOption);
  return DispatchTrustedEvent(event);;
}

NS_IMETHODIMP
FileSystemProvider::OnSuccess(uint32_t aRequestId,
                              nsIVirtualFileSystemRequestValue* aValue,
                              bool aHasMore)
{
  Unused << aValue;
  Unused << aHasMore;

  auto it = mPendingRequestPromises.find(aRequestId);
  if (NS_WARN_IF(it == mPendingRequestPromises.end())) {
    return NS_ERROR_INVALID_ARG;
  }

  it->second->MaybeResolve(JS::UndefinedHandleValue);
  mPendingRequestPromises.erase(it);
  return NS_OK;
}

NS_IMETHODIMP
FileSystemProvider::OnError(uint32_t aRequestId, uint32_t aErrorCode)
{
  Unused << aErrorCode;

  auto it = mPendingRequestPromises.find(aRequestId);
  if (NS_WARN_IF(it == mPendingRequestPromises.end())) {
    return NS_ERROR_INVALID_ARG;
  }

  it->second->MaybeReject(NS_ERROR_FAILURE);
  mPendingRequestPromises.erase(it);
  return NS_OK;
}

} // namespace dom
} // namespace mozilla
