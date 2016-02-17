/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/dom/FileSystemProviderCommon.h"
#include "mozilla/Move.h"
#include "mozilla/unused.h"
#include "nsThreadUtils.h"

namespace mozilla {
namespace dom {

NS_IMPL_ISUPPORTS0(BaseFileSystemProviderEventDispatcher)

NS_IMPL_ISUPPORTS(BaseMountUnmountResultCallback,
                  nsIVirtualFileSystemCallback)


template <class T>
NS_IMETHODIMP
MountUnmountResultCallback<T>::OnSuccess(uint32_t aRequestId,
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

template <class T>
NS_IMETHODIMP
MountUnmountResultCallback<T>::OnError(uint32_t aRequestId, uint32_t aErrorCode)
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

NS_IMPL_ISUPPORTS(VirtualFileSystemCallbackWrapper, nsIVirtualFileSystemCallback)

VirtualFileSystemCallbackWrapper::VirtualFileSystemCallbackWrapper(
  nsIVirtualFileSystemCallback* aCallback,
  mozilla::Function<void(uint32_t)> aOnSuccess,
  mozilla::Function<void(uint32_t)> aOnError)
    : mSavedCallback(aCallback)
    , mOnSuccess(Move(aOnSuccess))
    , mOnError(Move(aOnError))
{
}

NS_IMETHODIMP
VirtualFileSystemCallbackWrapper::OnSuccess(uint32_t aRequestId,
                                            nsIVirtualFileSystemRequestValue* aValue,
                                            bool aHasMore)
{
  mOnSuccess(aRequestId);

  if (!mSavedCallback) {
    return NS_OK;
  }

  return mSavedCallback->OnSuccess(aRequestId, aValue, aHasMore);
}

NS_IMETHODIMP
VirtualFileSystemCallbackWrapper::OnError(uint32_t aRequestId, uint32_t aErrorCode)
{
  mOnError(aRequestId);

  if (!mSavedCallback) {
    return NS_OK;
  }

  return mSavedCallback->OnError(aRequestId, aErrorCode);
}

} // namespace dom
} // namespace mozilla