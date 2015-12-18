/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_FakeVirtualFileSystemService_h
#define mozilla_dom_FakeVirtualFileSystemService_h

#include "mozilla/dom/FileSystemProviderBinding.h"
#include "mozilla/StaticPtr.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsTArray.h"
#include "nsThreadUtils.h"

class nsIArray;
class nsIVirtualFileSystemCallback;
class nsIVirtualFileSystemInfo;
class nsIVirtualFileSystemOpenedFileInfo;

namespace mozilla {
namespace dom {
namespace virtualfilesystem {

class nsVirtualFileSystemRequestManager;

class BaseVirtualFileSystemService {
public:
  NS_INLINE_DECL_REFCOUNTING(BaseVirtualFileSystemService);

  virtual nsresult Mount(uint32_t aRequestId,
                         const MountOptions& aOptions,
                         nsVirtualFileSystemRequestManager* aRequestManager,
                         nsIVirtualFileSystemCallback* aCallback) = 0;
  virtual nsresult Unmount(uint32_t aRequestId,
                           const UnmountOptions& aOptions,
                           nsIVirtualFileSystemCallback* aCallback) = 0;
  virtual nsresult GetVirtualFileSystemById(const nsAString& aFileSystemId,
                                            nsIVirtualFileSystemInfo** aInfo) = 0;
  virtual nsresult GetAllVirtualFileSystem(nsIArray** aFileSystems) = 0;

protected:
  virtual ~BaseVirtualFileSystemService() = default;

  class VirtualFileSystem final : public nsISupports {
  public:
    NS_DECL_ISUPPORTS

    explicit VirtualFileSystem(const nsAString& aFileSystemId,
                               const nsAString& aDisplayName,
                               bool aWritable,
                               uint32_t aOpenedFilesLimit,
                               nsVirtualFileSystemRequestManager* aRequestManager)
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

    already_AddRefed<nsVirtualFileSystemRequestManager> RequestManager() const
    {
      RefPtr<nsVirtualFileSystemRequestManager> requestManager = mRequestManager;
      return requestManager.forget();
    }

    bool AddOpenedFile(nsIVirtualFileSystemOpenedFileInfo* aFile);

    void RemoveOpenedFile(uint32_t aOpenRequestId);

    already_AddRefed<nsIVirtualFileSystemInfo> ConvertToVirtualFileSystemInfo() const;

  private:
    ~VirtualFileSystem() = default;

    nsString mFileSystemId;
    nsString mDisplayName;
    bool mWritable;
    uint32_t mOpenedFilesLimit;
    RefPtr<nsVirtualFileSystemRequestManager> mRequestManager;
    nsTArray<nsCOMPtr<nsIVirtualFileSystemOpenedFileInfo>> mOpenedFiles;
  };
  struct VirtualFileSystemComparator {
    bool Equals(const RefPtr<VirtualFileSystem>& aA,
                const RefPtr<VirtualFileSystem>& aB) const {
      return aA->FileSystemId() == aB->FileSystemId();
    }
  };
  bool FindVirtualFileSystemById(const nsAString& aFileSystemId, uint32_t& aIndex);
  bool MountInternal(const MountOptions& aOptions,
                     nsVirtualFileSystemRequestManager* aRequestManager,
                     uint32_t* aErrorCode);
  bool UnmountInternal(const UnmountOptions& aOptions,
                       uint32_t* aErrorCode);

  nsTArray<RefPtr<VirtualFileSystem>> mVirtualFileSystems;
};

template <class Derived>
class BaseVirtualFileSystemServiceWrapper : public BaseVirtualFileSystemService
{
public:
  static already_AddRefed<Derived> GetSingleton();

protected:
  BaseVirtualFileSystemServiceWrapper () = default;
  virtual ~BaseVirtualFileSystemServiceWrapper() = default;

  static StaticRefPtr<Derived> sSingleton;
};

class FakeVirtualFileSystemService final
  : public BaseVirtualFileSystemServiceWrapper<FakeVirtualFileSystemService>
{
public:
  virtual nsresult Mount(uint32_t aRequestId,
                         const MountOptions& aOptions,
                         nsVirtualFileSystemRequestManager* aRequestManager,
                         nsIVirtualFileSystemCallback* aCallback) override;
  virtual nsresult Unmount(uint32_t aRequestId,
                           const UnmountOptions& aOptions,
                           nsIVirtualFileSystemCallback* aCallback) override;
  virtual nsresult GetVirtualFileSystemById(const nsAString& aFileSystemId,
                                            nsIVirtualFileSystemInfo** aInfo) override;
  virtual nsresult GetAllVirtualFileSystem(nsIArray** aFileSystems) override;

private:
  friend class BaseVirtualFileSystemServiceWrapper<FakeVirtualFileSystemService>;
  FakeVirtualFileSystemService() = default;
  virtual ~FakeVirtualFileSystemService() = default;

  already_AddRefed<nsIVirtualFileSystemOpenedFileInfo>
    MockOpenedFileInfo(const nsAString& aFilePath,
                       uint32_t aMode,
                       uint32_t aOpenRequestId);
};

} // namespace virtualfilesystem
} // namespace dom
} // namespace mozilla

#endif // mozilla_dom_FakeVirtualFileSystemService_h