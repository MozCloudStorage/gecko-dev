/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "FileSystemProvider.h"
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
#include "nsArrayUtils.h"
#include "nsVirtualFileSystem.h"
#include "nsVirtualFileSystemService.h"
#include "nsVirtualFileSystemRequestManager.h"
#include "VirtualFileSystemServiceFactory.h"

namespace mozilla {
namespace dom {

using virtualfilesystem::VirtualFileSystemServiceFactory;
using virtualfilesystem::BaseVirtualFileSystemService;

NS_IMPL_ISUPPORTS0(FileSystemProviderProxy)

NS_IMPL_ISUPPORTS_INHERITED0(FileSystemProviderEventDispatcher,
                             FileSystemProviderProxy)

nsresult
FileSystemProviderEventDispatcher::DispatchFileSystemProviderEvent(
  uint32_t aRequestId,
  const nsAString& aFileSystemId,
  const virtualfilesystem::VirtualFileSystemIPCRequestedOptions& aOptions)
{
  return mFileSystemProvider->DispatchFileSystemProviderEventInternal(
    aRequestId,
    aFileSystemId,
    aOptions);
}

class MountUnmountResultCallback final : public FileSystemProviderProxy
                                       , public nsIVirtualFileSystemCallback
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIVIRTUALFILESYSTEMCALLBACK

  explicit MountUnmountResultCallback(FileSystemProvider* aProvider)
    : FileSystemProviderProxy(aProvider) {}

private:
  virtual ~MountUnmountResultCallback() {}
};

NS_IMPL_ISUPPORTS_INHERITED(MountUnmountResultCallback,
                            FileSystemProviderProxy,
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
  , mEventDispatcher(new FileSystemProviderEventDispatcher(this))
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
  nsCOMPtr<nsIGlobalObject> global = do_QueryInterface(GetOwner());
  MOZ_ASSERT(global);

  RefPtr<Promise> promise = Promise::Create(global, aRv);
  if (NS_WARN_IF(aRv.Failed())) {
    return nullptr;
  }
  mPendingRequestPromises[++sRequestId] = promise;

  nsCOMPtr<nsIVirtualFileSystemCallback> callback =
    new MountUnmountResultCallback(this);
  nsresult rv = mVirtualFileSystemService->Mount(sRequestId,
                                                 aOptions,
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
  nsCOMPtr<nsIGlobalObject> global = do_QueryInterface(GetOwner());
  MOZ_ASSERT(global);

  RefPtr<Promise> promise = Promise::Create(global, aRv);
  if (NS_WARN_IF(aRv.Failed())) {
    return nullptr;
  }
  mPendingRequestPromises[++sRequestId] = promise;

  nsCOMPtr<nsIVirtualFileSystemCallback> callback =
    new MountUnmountResultCallback(this);
  nsresult rv = mVirtualFileSystemService->Unmount(sRequestId, aOptions, callback);
  if (NS_FAILED(rv)) {
    aRv.Throw(rv);
    return nullptr;
  }

  return promise.forget();
}

void
FileSystemProvider::Get(const nsAString& aFileSystemId,
                        Nullable<FileSystemInfo>& aInfo,
                        ErrorResult& aRv)
{
  nsresult rv = mVirtualFileSystemService->GetFileSysetmInfoById(aFileSystemId,
                                                                 aInfo.SetValue());
  if (NS_FAILED(rv)) {
    aInfo.SetNull();
    aRv.Throw(rv);
  }
}

void
FileSystemProvider::GetAll(Nullable<nsTArray<FileSystemInfo>>& aRetVal, ErrorResult& aRv)
{
  aRetVal.SetValue().TruncateLength(0);
  mVirtualFileSystemService->GetAllFileSystemInfo(aRetVal.SetValue());
}

nsresult
FileSystemProvider::DispatchFileSystemProviderEventInternal(
  uint32_t aRequestId,
  const nsAString& aFileSystemId,
  const virtualfilesystem::VirtualFileSystemIPCRequestedOptions& aOptions)
{
  RefPtr<FileSystemProviderEvent> event;
  switch (aOptions.type()) {
    case VirtualFileSystemIPCRequestedOptions::TVirtualFileSystemAbortRequestedOptions:
      event = new FileSystemProviderAbortEvent(this, mRequestManager);
      break;
    case VirtualFileSystemIPCRequestedOptions::TVirtualFileSystemCloseFileRequestedOptions:
      event = new FileSystemProviderCloseFileEvent(this, mRequestManager);
      break;
    case VirtualFileSystemIPCRequestedOptions::TVirtualFileSystemGetMetadataRequestedOptions:
      event = new FileSystemProviderGetMetadataEvent(this, mRequestManager);
      break;
    case VirtualFileSystemIPCRequestedOptions::TVirtualFileSystemOpenFileRequestedOptions:
      event = new FileSystemProviderOpenFileEvent(this, mRequestManager);
      break;
    case VirtualFileSystemIPCRequestedOptions::TVirtualFileSystemReadDirectoryRequestedOptions:
      event = new FileSystemProviderReadDirectoryEvent(this, mRequestManager);
      break;
    case VirtualFileSystemIPCRequestedOptions::TVirtualFileSystemReadFileRequestedOptions:
      event = new FileSystemProviderReadFileEvent(this, mRequestManager);
      break;
    case VirtualFileSystemIPCRequestedOptions::TVirtualFileSystemUnmountRequestedOptions:
      event = new FileSystemProviderUnmountEvent(this, mRequestManager);
      break;
    default:
      MOZ_ASSERT(false);
      return NS_ERROR_INVALID_ARG;
  }

  nsresult rv = event->InitFileSystemProviderEvent(aRequestId, aFileSystemId, aOptions);
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
