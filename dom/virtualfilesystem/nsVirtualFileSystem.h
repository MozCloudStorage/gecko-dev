/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_nsvirtualfilesystem_h_
#define mozilla_dom_nsvirtualfilesystem_h_

#include "nsString.h"
#include "nsCOMPtr.h"
#include "nsTArray.h"
#include "nsIVirtualFileSystem.h"

namespace mozilla {
namespace dom {

struct FileSystemInfo;
struct MountOptions;
struct OpenedFile;

namespace virtualfilesystem {

class BaseVirtualFileSystemRequestManager;
class nsVirtualFileSystem;

class FileSystemInfoWrapper final
{
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(FileSystemInfoWrapper)

  FileSystemInfoWrapper(const MountOptions& aOptions);

  void GetFileSystemInfo(FileSystemInfo &aInfo) const;

  const nsString& FileSystemId() const;

  bool Writable() const;

  uint32_t OpenedFilesLimit() const;

  uint32_t OpenedFilesCount() const;

  void AppendOpenedFile(const OpenedFile& aFile);

  void RemoveOpenedFile(uint32_t aOpenRequestId);

private:
  virtual ~FileSystemInfoWrapper() = default;

  FileSystemInfo mFileSystemInfo;
};

class nsVirtualFileSystem final : public nsIVirtualFileSystem
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIVIRTUALFILESYSTEM

  nsVirtualFileSystem(FileSystemInfoWrapper* aFileSysetmInfo,
                      BaseVirtualFileSystemRequestManager* aRequestManager);

private:
  virtual ~nsVirtualFileSystem() = default;

  RefPtr<FileSystemInfoWrapper> mFileSystemInfo;
  RefPtr<BaseVirtualFileSystemRequestManager> mRequestManager;
};

} // end namespace virtualfilesystem
} // end namespace dom
} // end namespace mozilla
#endif