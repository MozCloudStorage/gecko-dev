/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_virtualfilesystem_nsVirtualFileSystemRequestManager_h
#define mozilla_dom_virtualfilesystem_nsVirtualFileSystemRequestManager_h

#include <map>
#include <vector>
#include "nsCycleCollectionParticipant.h"

namespace mozilla {
namespace dom {

class FileSystemProviderEventDispatcher;

namespace virtualfilesystem {

class VirtualFileSystemIPCRequestedOptions;

class nsVirtualFileSystemRequestManager final : public nsISupports
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(nsVirtualFileSystemRequestManager)

  explicit nsVirtualFileSystemRequestManager(
    FileSystemProviderEventDispatcher* aEventDispatcher);

  nsresult CreateRequest(const nsAString& aFileSystemId,
                         const VirtualFileSystemIPCRequestedOptions& aOptions,
                         nsIVirtualFileSystemCallback* aCallback,
                         uint32_t* aRequestId);
  nsresult FufillRequest(uint32_t aRequestId,
                         nsIVirtualFileSystemRequestValue* aValue,
                         bool aHasMore);
  nsresult RejectRequest(uint32_t aRequestId, uint32_t aErrorCode);

private:
  class nsVirtualFileSystemRequest final : public nsISupports {
  public:
    NS_DECL_ISUPPORTS

    explicit nsVirtualFileSystemRequest(uint32_t aRequestId,
                                        nsIVirtualFileSystemCallback* aCallback);

    const uint32_t mRequestId;
    nsCOMPtr<nsIVirtualFileSystemCallback> mCallback;
    bool mIsCompleted;
    nsCOMPtr<nsIVirtualFileSystemRequestValue> mValue;

  private:
    ~nsVirtualFileSystemRequest() = default;
  };

  ~nsVirtualFileSystemRequestManager() = default;
  void DestroyRequest(uint32_t aRequestId);

  typedef std::map<uint32_t, RefPtr<nsVirtualFileSystemRequest>> RequestMapType;
  RequestMapType mRequestMap;
  typedef std::vector<uint32_t> RequestIdQueueType;
  RequestIdQueueType mRequestIdQueue;
  RefPtr<FileSystemProviderEventDispatcher> mEventDispatcher;
  uint32_t mRequestId;
};

} // end of namespace virtualfilesystem
} // end of namespace dom
} // end of namespace mozilla
#endif