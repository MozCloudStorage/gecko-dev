/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_VirtualFileSystemIPCService_h
#define mozilla_dom_VirtualFileSystemIPCService_h

#include <map>
#include "nsVirtualFileSystemService.h"

namespace mozilla {
namespace dom {
namespace virtualfilesystem {

class VirtualFileSystemIPCService final : public BaseVirtualFileSystemService
{
public:
  static already_AddRefed<VirtualFileSystemIPCService> GetSingleton();

  virtual nsresult Mount(uint32_t aRequestId,
                         const MountOptions& aOptions,
                         nsVirtualFileSystemRequestManager* aRequestManager,
                         nsIVirtualFileSystemCallback* aCallback) override;
  virtual nsresult Unmount(uint32_t aRequestId,
                           const UnmountOptions& aOptions,
                           nsIVirtualFileSystemCallback* aCallback) override;
  void NotifyVirtualFileSystemChildDestroyed();
  bool NotifyMountUnmountResult(uint32_t aRequestId, bool aSucceeded);

private:
  VirtualFileSystemIPCService();
  ~VirtualFileSystemIPCService();

  static StaticRefPtr<VirtualFileSystemIPCService> sSingleton;
  typedef std::map<uint32_t, nsCOMPtr<nsIVirtualFileSystemCallback>> CallbackMapType;
  CallbackMapType mMountUnmountCallbackMap;
};

} // namespace virtualfilesystem
} // namespace dom
} // namespace mozilla

#endif // mozilla_dom_VirtualFileSystemIPCService_h