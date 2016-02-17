/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <algorithm>
#include "mozilla/dom/virtualfilesystem/PVirtualFileSystem.h"
#include "mozilla/Preferences.h"
#include "nsComponentManagerUtils.h"
#include "nsIVirtualFileSystemCallback.h"
#include "nsVirtualFileSystemRequestManager.h"
#include "nsQueryObject.h"
#include "nsThreadUtils.h"

namespace mozilla {
namespace dom {
namespace virtualfilesystem {

namespace {

uint32_t RequestTimeoutValue()
{
  const uint32_t kDefaultTimeoutValue = 30000;
  static uint32_t sRequestTimeoutValue = kDefaultTimeoutValue;
  static bool sRequestTimeoutValueInitialized = false;

  if (!sRequestTimeoutValueInitialized) {
    sRequestTimeoutValueInitialized = true;
    Preferences::AddUintVarCache(&sRequestTimeoutValue,
                                 "dom.filesystemprovider.request_timeout",
                                 kDefaultTimeoutValue);
  }

  return sRequestTimeoutValue;
}

} // anonymous namespace

NS_IMPL_ISUPPORTS(nsVirtualFileSystemRequestManager::nsVirtualFileSystemRequest,
                  nsITimerCallback)

nsVirtualFileSystemRequestManager::nsVirtualFileSystemRequest
  ::nsVirtualFileSystemRequest(uint32_t aRequestId,
                               nsIVirtualFileSystemCallback* aCallback,
                               nsVirtualFileSystemRequestManager* aManager)
  : mRequestId(aRequestId)
  , mCallback(aCallback)
  , mIsCompleted(false)
  , mManager(aManager)
{
}

nsresult
nsVirtualFileSystemRequestManager::nsVirtualFileSystemRequest::StartTimer()
{
  if (mTimer) {
    return NS_OK;
  }

  nsresult rv;
  mTimer = do_CreateInstance("@mozilla.org/timer;1", &rv);
  if (NS_FAILED(rv)) {
    return rv;
  }

  rv = mTimer->InitWithCallback(this, RequestTimeoutValue(), nsITimer::TYPE_ONE_SHOT);
  return rv;
}

nsVirtualFileSystemRequestManager::nsVirtualFileSystemRequest
  ::~nsVirtualFileSystemRequest()
{
  if (mTimer) {
    mTimer->Cancel();
    mTimer = nullptr;
  }
}

NS_IMETHODIMP
nsVirtualFileSystemRequestManager::nsVirtualFileSystemRequest
  ::Notify(nsITimer* aTimer)
{
  MOZ_ASSERT(NS_IsMainThread());

  RefPtr<nsVirtualFileSystemRequest> self = this;
  nsCOMPtr<nsIRunnable> r =
    NS_NewRunnableFunction([self] () -> void {
      self->mCallback->OnError(self->mRequestId,
                               nsIVirtualFileSystemCallback::ERROR_TIME_OUT);
      self->mManager->DestroyRequest(self->mRequestId);
    });
  NS_DispatchToCurrentThread(r);
  return NS_OK;
}

nsVirtualFileSystemRequestManager::nsVirtualFileSystemRequestManager(
  BaseFileSystemProviderEventDispatcher* aEventDispatcher)
  : mEventDispatcher(aEventDispatcher)
  , mRequestId(0)
{
}

nsresult
nsVirtualFileSystemRequestManager::CreateRequest(const nsAString& aFileSystemId,
                                                 const VirtualFileSystemIPCRequestedOptions& aOptions,
                                                 nsIVirtualFileSystemCallback* aCallback,
                                                 uint32_t* aRequestId)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (NS_WARN_IF(!aRequestId)) {
    return NS_ERROR_INVALID_ARG;
  }

  if (NS_WARN_IF(!mEventDispatcher)) {
    return NS_ERROR_NOT_INITIALIZED;
  }

  *aRequestId = 0;

  if (NS_WARN_IF(!(aCallback))) {
    return NS_ERROR_INVALID_ARG;
  }

  RefPtr<nsVirtualFileSystemRequest> request =
    new nsVirtualFileSystemRequest(++mRequestId, aCallback, this);
  mRequestMap[mRequestId] = request;
  mRequestIdQueue.append(mRequestId);

  RefPtr<nsVirtualFileSystemRequestManager> self = this;
  nsString fileSystemId(aFileSystemId);
  nsCOMPtr<nsIRunnable> dispatchTask =
    NS_NewRunnableFunction([self, fileSystemId, aOptions] () -> void {
      self->mEventDispatcher->DispatchFileSystemProviderEvent(self->mRequestId,
                                                              fileSystemId,
                                                              aOptions,
                                                              self);
    });
  nsresult rv = NS_DispatchToCurrentThread(dispatchTask);
  if (NS_FAILED(rv)) {
    DestroyRequest(mRequestId);
    return rv;
  }

  rv = request->StartTimer();
  if (NS_FAILED(rv)) {
    DestroyRequest(mRequestId);
    return rv;
  }

  *aRequestId = mRequestId;
  return NS_OK;
}

nsresult
nsVirtualFileSystemRequestManager::FufillRequest(uint32_t aRequestId,
                                                 nsIVirtualFileSystemRequestValue* aValue,
                                                 bool aHasMore)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (NS_WARN_IF(aHasMore && !aValue)) {
    return NS_ERROR_INVALID_ARG;
  }

  if (mRequestMap.find(aRequestId) == mRequestMap.end()) {
    return NS_ERROR_FAILURE;
  }

  RefPtr<nsVirtualFileSystemRequest> request = mRequestMap[aRequestId];

  if (!request->mValue) {
    request->mValue = aValue;
  }
  else {
    request->mValue->Concat(aValue);
  }

  if (aHasMore) {
    return NS_OK;
  }

  request->mIsCompleted = true;

  for (auto it = mRequestIdQueue.begin(); it != mRequestIdQueue.end();) {
    MOZ_ASSERT(mRequestMap.find(*it) != mRequestMap.end());

    RefPtr<nsVirtualFileSystemRequest> req = mRequestMap[*it];
    if (!req->mIsCompleted) {
      break;
    }
    nsCOMPtr<nsIRunnable> r =
      NS_NewRunnableFunction([req] () -> void {
        req->mCallback->OnSuccess(req->mRequestId, req->mValue, false);
      });
    NS_DispatchToCurrentThread(r);
    mRequestMap.erase(req->mRequestId);
    mRequestIdQueue.erase(it);
  }

  return NS_OK;
}

nsresult
nsVirtualFileSystemRequestManager::RejectRequest(uint32_t aRequestId,
                                                 uint32_t aErrorCode)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (mRequestMap.find(aRequestId) == mRequestMap.end()) {
    return NS_ERROR_FAILURE;
  }

  RefPtr<nsVirtualFileSystemRequest> req = mRequestMap[aRequestId];
  nsCOMPtr<nsIRunnable> r =
    NS_NewRunnableFunction([req, aErrorCode] () -> void {
      req->mCallback->OnError(req->mRequestId, aErrorCode);
    });
  NS_DispatchToCurrentThread(r);
  DestroyRequest(aRequestId);
  return NS_OK;
}

void
nsVirtualFileSystemRequestManager::DestroyRequest(uint32_t aRequestId)
{
  MOZ_ASSERT(NS_IsMainThread());

  mRequestMap.erase(aRequestId);
  auto it = std::find(mRequestIdQueue.begin(), mRequestIdQueue.end(), aRequestId);
  if (it != mRequestIdQueue.end()) {
    mRequestIdQueue.erase(it);
  }
}

} // end of namespace virtualfilesystem
} // end of namespace dom
} // end of namespace mozilla