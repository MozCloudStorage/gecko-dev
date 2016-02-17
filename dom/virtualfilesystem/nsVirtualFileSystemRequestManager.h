/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_virtualfilesystem_nsVirtualFileSystemRequestManager_h
#define mozilla_dom_virtualfilesystem_nsVirtualFileSystemRequestManager_h

#include <map>
#include "mozilla/dom/FileSystemProviderCommon.h"
#include "mozilla/Vector.h"
#include "nsITimer.h"

namespace mozilla {
namespace dom {
namespace virtualfilesystem {

class nsVirtualFileSystemRequestManager final : public BaseVirtualFileSystemRequestManager
{
public:
  explicit nsVirtualFileSystemRequestManager(
    BaseFileSystemProviderEventDispatcher* aEventDispatcher);

  virtual nsresult CreateRequest(const nsAString& aFileSystemId,
                                 const VirtualFileSystemIPCRequestedOptions& aOptions,
                                 nsIVirtualFileSystemCallback* aCallback,
                                 uint32_t* aRequestId) override;
  virtual nsresult FufillRequest(uint32_t aRequestId,
                                 nsIVirtualFileSystemRequestValue* aValue,
                                 bool aHasMore) override;
  virtual nsresult RejectRequest(uint32_t aRequestId, uint32_t aErrorCode) override;

private:
  class nsVirtualFileSystemRequest final : public nsITimerCallback {
  public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSITIMERCALLBACK

    explicit nsVirtualFileSystemRequest(uint32_t aRequestId,
                                        nsIVirtualFileSystemCallback* aCallback,
                                        nsVirtualFileSystemRequestManager* aManager);
    nsresult StartTimer();

    const uint32_t mRequestId;
    nsCOMPtr<nsIVirtualFileSystemCallback> mCallback;
    bool mIsCompleted;
    nsCOMPtr<nsIVirtualFileSystemRequestValue> mValue;

  private:
    virtual ~nsVirtualFileSystemRequest();

    nsCOMPtr<nsITimer> mTimer;
    nsVirtualFileSystemRequestManager* mManager;
  };
  friend class nsVirtualFileSystemRequest;
  virtual ~nsVirtualFileSystemRequestManager() = default;
  void DestroyRequest(uint32_t aRequestId);

  RefPtr<BaseFileSystemProviderEventDispatcher> mEventDispatcher;
  typedef std::map<uint32_t, RefPtr<nsVirtualFileSystemRequest>> RequestMapType;
  RequestMapType mRequestMap;
  mozilla::Vector<uint32_t> mRequestIdQueue;
  uint32_t mRequestId;
};

} // end of namespace virtualfilesystem
} // end of namespace dom
} // end of namespace mozilla
#endif