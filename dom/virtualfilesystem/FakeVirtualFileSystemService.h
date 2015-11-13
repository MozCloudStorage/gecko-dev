/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_FakeVirtualFileSystemService_h
#define mozilla_dom_FakeVirtualFileSystemService_h

#include "nsCOMPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsIVirtualFileSystemService.h"
#include "nsTArray.h"
#include "nsString.h"

#define FAKE_VIRTUALFILESYSTEM_SERVICE_CONTRACTID \
  "@mozilla.org/tv/fakevirtualfilesystemservice;1"
#define FAKEVIRTUALFILESYSTEMSERVICE_CID \
  { 0xb643b0d9, 0x2492, 0x49ea, { 0xbf, 0xd6, 0x51, 0xa0, 0xb7, 0x34, 0xba, 0x5b } }

class nsIVirtualFileSystemOpenedFileInfo;

namespace mozilla {
namespace dom {

class FakeVirtualFileSystemService final : public nsIVirtualFileSystemService
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIVIRTUALFILESYSTEMSERVICE

  FakeVirtualFileSystemService() = default;

  static FakeVirtualFileSystemService* GetSingleton();

private:
  class VirtualFileSystem final : public nsISupports {
  public:
    NS_DECL_ISUPPORTS

    explicit VirtualFileSystem(const nsAString& aFileSystemId,
                               const nsAString& aDisplayName,
                               bool aWritable,
                               uint32_t aOpenedFilesLimit,
                               nsIVirtualFileSystemRequestManager* aRequestManager)
      : mFileSystemId(aFileSystemId)
      , mDisplayName(aDisplayName)
      , mWritable(aWritable)
      , mOpenedFilesLimit(aOpenedFilesLimit)
      , mRequestManager(aRequestManager)
    {
    }

    const nsString& FileSystemId() const
    {
      return mFileSystemId;
    }

    const nsString& DisplayName() const
    {
      return mDisplayName;
    }

    bool Writable() const
    {
      return mWritable;
    }

    uint32_t OpenedFilesLimit() const
    {
      return mOpenedFilesLimit;
    }

    already_AddRefed<nsIVirtualFileSystemRequestManager> RequestManager() const
    {
      nsCOMPtr<nsIVirtualFileSystemRequestManager> requestManager = mRequestManager;
      return requestManager.forget();
    }

  private:
    ~VirtualFileSystem() = default;

    nsString mFileSystemId;
    nsString mDisplayName;
    bool mWritable;
    uint32_t mOpenedFilesLimit;
    nsCOMPtr<nsIVirtualFileSystemRequestManager> mRequestManager;
  };

   ~FakeVirtualFileSystemService() = default;
  struct VirtualFileSystemComparator {
    bool Equals(const RefPtr<VirtualFileSystem>& aA, const RefPtr<VirtualFileSystem>& aB) const {
      return aA->FileSystemId() == aB->FileSystemId();
    }
  };
  bool FindVirtualFileSystemById(const nsAString& aFileSystemId, uint32_t& aIndex);
  already_AddRefed<nsIVirtualFileSystemOpenedFileInfo>
    MockOpenedFileInfo(const nsAString& aFilePath,
                       uint32_t aMode,
                       uint32_t aOpenRequestId);

  nsTArray<RefPtr<VirtualFileSystem>> mVirtualFileSystems;
};

} // namespace dom
} // namespace mozilla

#endif // mozilla_dom_FakeVirtualFileSystemService_h