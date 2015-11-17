/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_virtualfilesystem_nsVirtualFileSystemRequestManager_h
#define mozilla_dom_virtualfilesystem_nsVirtualFileSystemRequestManager_h

#include <map>
#include <vector>
#include "nsCOMPtr.h"
#include "nsIVirtualFileSystemRequestManager.h"

class nsIFileSystemProviderEventDispatcher;

namespace mozilla {
namespace dom {
namespace virtualfilesystem {

class nsVirtualFileSystemRequestManager final : public nsIVirtualFileSystemRequestManager
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIVIRTUALFILESYSTEMREQUESTMANAGER

  nsVirtualFileSystemRequestManager();
  explicit nsVirtualFileSystemRequestManager(
    nsIFileSystemProviderEventDispatcher* dispatcher);

private:
  class nsVirtualFileSystemRequest final : public nsISupports {
  public:
    NS_DECL_ISUPPORTS

    explicit nsVirtualFileSystemRequest(uint32_t aRequestType,
                                        uint32_t aRequestId,
                                        nsIVirtualFileSystemCallback* aCallback);

    const uint32_t mRequestType;
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
  nsCOMPtr<nsIFileSystemProviderEventDispatcher> mDispatcher;
  uint32_t mRequestId;
};

} // end of namespace virtualfilesystem
} // end of namespace dom
} // end of namespace mozilla
#endif