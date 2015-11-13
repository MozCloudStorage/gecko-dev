/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_nsvirtualfilesystem_h_
#define mozilla_dom_nsvirtualfilesystem_h_

#include "nsString.h"
#include "nsCOMPtr.h"
#include "nsTArray.h"
#include "nsIVirtualFileSystemService.h"
#include "nsIVirtualFileSystem.h"
#include "nsIVirtualFileSystemCallback.h"
#include "nsIVirtualFileSystemRequestManager.h"
#include "nsIVirtualFileSystemResponseHandler.h"
#include "nsIVirtualFileSystemDataType.h"

namespace mozilla {
namespace dom {
namespace virtualfilesystem {

class nsVirtualFileSystem final : public nsIVirtualFileSystem
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIVIRTUALFILESYSTEM

  nsVirtualFileSystem();

  const char* FileSystemIdStr();
  const char* DisplayNameStr();
  const char* MountPointStr();
  const bool IsWritable();
  const nsString GetFileSystemId();
  const nsString GetMountPoint();

  void SetInfo(nsIVirtualFileSystemInfo* aInfo);
  void SetResponseHandler(nsIVirtualFileSystemResponseHandler* aResponseHandler);
  void SetRequestManager(nsIVirtualFileSystemRequestManager* aRequestManager);

private:
  ~nsVirtualFileSystem() = default;

  RefPtr<nsIVirtualFileSystemInfo> mInfo;
  RefPtr<nsIVirtualFileSystemRequestManager> mRequestManager;
  RefPtr<nsIVirtualFileSystemResponseHandler> mResponseHandler;
  nsString mMountPoint;
};

} // end namespace virtualfilesystem
} // end namespace dom
} // end namespace mozilla
#endif