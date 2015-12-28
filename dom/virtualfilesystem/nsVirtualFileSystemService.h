/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_nsVirtualFileSystemService_h
#define mozilla_dom_nsVirtualFileSystemService_h

#include <map>
#include "mozilla/StaticPtr.h"
#include "nsIVirtualFileSystemService.h"
#include "nsString.h"
#include "nsTArray.h"

class nsIVirtualFileSystemCallback;

namespace mozilla {
namespace dom {

struct FileSystemInfo;
struct MountOptions;
struct UnmountOptions;

namespace virtualfilesystem {

class FileSystemInfoWrapper;
class nsVirtualFileSystemRequestManager;

class BaseVirtualFileSystemService : public nsISupports {
public:
  NS_DECL_ISUPPORTS

  virtual nsresult Mount(uint32_t aRequestId,
                         const MountOptions& aOptions,
                         nsVirtualFileSystemRequestManager* aRequestManager,
                         nsIVirtualFileSystemCallback* aCallback) = 0;

  virtual nsresult Unmount(uint32_t aRequestId,
                           const UnmountOptions& aOptions,
                           nsIVirtualFileSystemCallback* aCallback) = 0;

  nsresult GetFileSysetmInfoById(const nsAString& aFileSystemId, FileSystemInfo& aInfo);

  void GetAllFileSystemInfo(nsTArray<FileSystemInfo>& aArray);

protected:
  BaseVirtualFileSystemService() = default;
  virtual ~BaseVirtualFileSystemService() = default;

  bool FindFileSystemInfoById(const nsAString& aFileSystemId, uint32_t& aIndex);
  already_AddRefed<FileSystemInfoWrapper>
  MountInternal(const MountOptions& aOptions,
                nsVirtualFileSystemRequestManager* aRequestManager,
                uint32_t* aErrorCode);
  bool UnmountInternal(const UnmountOptions& aOptions,
                       uint32_t* aErrorCode);

  nsTArray<RefPtr<FileSystemInfoWrapper>> mVirtualFileSystems;
};

class nsVirtualFileSystemService final
  : public nsIVirtualFileSystemService
  , public BaseVirtualFileSystemService
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIVIRTUALFILESYSTEMSERVICE

  static already_AddRefed<nsVirtualFileSystemService> GetSingleton();

  virtual nsresult Mount(uint32_t aRequestId,
                         const MountOptions& aOptions,
                         nsVirtualFileSystemRequestManager* aRequestManager,
                         nsIVirtualFileSystemCallback* aCallback) override;
  virtual nsresult Unmount(uint32_t aRequestId,
                           const UnmountOptions& aOptions,
                           nsIVirtualFileSystemCallback* aCallback) override;

private:
  nsVirtualFileSystemService() = default;
  virtual ~nsVirtualFileSystemService() = default;

  static StaticRefPtr<nsVirtualFileSystemService> sSingleton;
  typedef std::map<nsString, nsCOMPtr<nsIVirtualFileSystem>> VirtualFileSystemMapType;
  VirtualFileSystemMapType mVirtualFileSystemMap;
};

} // namespace virtualfilesystem
} // namespace dom
} // namespace mozilla

#endif // mozilla_dom_nsVirtualFileSystemService_h