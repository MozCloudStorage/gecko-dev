/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <algorithm>
#include "mozilla/dom/FileSystemProvider.h"
#include "mozilla/dom/virtualfilesystem/PVirtualFileSystem.h"
#include "nsIVirtualFileSystemDataType.h"
#include "nsIVirtualFileSystemCallback.h"
#include "nsVirtualFileSystemRequestManager.h"
#include "nsQueryObject.h"
#include "nsThreadUtils.h"

namespace mozilla {
namespace dom {
namespace virtualfilesystem {

class RunVirtualFileSystemSuccessCallback : public nsRunnable
{
public:
  explicit RunVirtualFileSystemSuccessCallback(nsIVirtualFileSystemCallback* aCallback,
                                               uint32_t aRequestId,
                                               nsIVirtualFileSystemRequestValue* aValue,
                                               bool aHasMore)
    : mCallback(aCallback)
    , mRequestId(aRequestId)
    , mValue(aValue)
    , mHasMore(aHasMore)
  {}

  NS_IMETHOD Run()
  {
    mCallback->OnSuccess(mRequestId, mValue, mHasMore);
    return NS_OK;
  }

private:
  nsCOMPtr<nsIVirtualFileSystemCallback> mCallback;
  uint32_t mRequestId;
  nsCOMPtr<nsIVirtualFileSystemRequestValue> mValue;
  bool mHasMore;
};

class RunVirtualFileSystemErrorCallback : public nsRunnable
{
public:
  explicit RunVirtualFileSystemErrorCallback(nsIVirtualFileSystemCallback* aCallback,
                                             uint32_t aRequestId,
                                             uint32_t aErrorCode)
    : mCallback(aCallback)
    , mRequestId(aRequestId)
    , mErrorCode(aErrorCode)
  {}

  NS_IMETHOD Run()
  {
    mCallback->OnError(mRequestId, mErrorCode);
    return NS_OK;
  }

private:
  nsCOMPtr<nsIVirtualFileSystemCallback> mCallback;
  uint32_t mRequestId;
  uint32_t mErrorCode;
};

class DispatchRequestTask : public nsRunnable
{
public:
  explicit DispatchRequestTask(uint32_t aRequestId,
                               const nsAString& aFileSystemId,
                               const VirtualFileSystemIPCRequestedOptions& aOptions,
                               nsFileSystemProviderEventDispatcher* aDispatcher)
    : mRequestId(aRequestId)
    , mFileSystemId(aFileSystemId)
    , mOptions(aOptions)
    , mDispatcher(aDispatcher)
  {}

  NS_IMETHOD Run()
  {
    mDispatcher->DispatchFileSystemProviderEvent(mRequestId, mFileSystemId, mOptions);
    return NS_OK;
  }

private:
  uint32_t mRequestId;
  nsString mFileSystemId;
  VirtualFileSystemIPCRequestedOptions mOptions;
  RefPtr<nsFileSystemProviderEventDispatcher> mDispatcher;
};

NS_IMPL_ISUPPORTS0(nsVirtualFileSystemRequestManager::nsVirtualFileSystemRequest)

nsVirtualFileSystemRequestManager::nsVirtualFileSystemRequest
  ::nsVirtualFileSystemRequest(uint32_t aRequestId,
                               nsIVirtualFileSystemCallback* aCallback)
  : mRequestId(aRequestId)
  , mCallback(aCallback)
  , mIsCompleted(false)
{
}

NS_IMPL_CYCLE_COLLECTION_CLASS(nsVirtualFileSystemRequestManager)

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsVirtualFileSystemRequestManager)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsVirtualFileSystemRequestManager)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsVirtualFileSystemRequestManager)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsVirtualFileSystemRequestManager)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsVirtualFileSystemRequestManager)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

nsVirtualFileSystemRequestManager::nsVirtualFileSystemRequestManager()
  : mRequestId(0)
{
}

nsVirtualFileSystemRequestManager::nsVirtualFileSystemRequestManager(
  nsFileSystemProviderEventDispatcher* dispatcher)
  : mDispatcher(dispatcher)
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

  *aRequestId = 0;

  if (NS_WARN_IF(!(aCallback))) {
    return NS_ERROR_INVALID_ARG;
  }

  if (NS_WARN_IF(!mDispatcher)) {
    return NS_ERROR_NOT_INITIALIZED;
  }

  RefPtr<nsVirtualFileSystemRequest> request =
    new nsVirtualFileSystemRequest(++mRequestId, aCallback);
  mRequestMap[mRequestId] = request;
  mRequestIdQueue.push_back(mRequestId);
  nsCOMPtr<nsIRunnable> dispatchTask = new DispatchRequestTask(mRequestId,
                                                               aFileSystemId,
                                                               aOptions,
                                                               mDispatcher);

  nsresult rv = NS_DispatchToCurrentThread(dispatchTask);
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
    nsCOMPtr<nsIRunnable> callback =
      new RunVirtualFileSystemSuccessCallback(req->mCallback,
                                              req->mRequestId,
                                              req->mValue,
                                              false);
    NS_DispatchToCurrentThread(callback);
    mRequestMap.erase(req->mRequestId);
    mRequestIdQueue.erase(it);
  }

  return NS_OK;
}

nsresult
nsVirtualFileSystemRequestManager::RejectRequest(uint32_t aRequestId, uint32_t aErrorCode)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (mRequestMap.find(aRequestId) == mRequestMap.end()) {
    return NS_ERROR_FAILURE;
  }

  RefPtr<nsVirtualFileSystemRequest> request = mRequestMap[aRequestId];
  nsCOMPtr<nsIRunnable> callback =
    new RunVirtualFileSystemErrorCallback(request->mCallback,
                                          aRequestId,
                                          aErrorCode);
  NS_DispatchToCurrentThread(callback);
  DestroyRequest(aRequestId);
  return NS_OK;
}

nsresult
nsVirtualFileSystemRequestManager::SetRequestDispatcher(
  nsFileSystemProviderEventDispatcher* aDispatcher)
{
  MOZ_ASSERT(NS_IsMainThread());

  mDispatcher = aDispatcher;
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