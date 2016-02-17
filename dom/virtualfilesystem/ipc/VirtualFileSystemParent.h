/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_VirtualFileSystemParent_h
#define mozilla_dom_VirtualFileSystemParent_h

#include "mozilla/dom/FileSystemProviderCommon.h"
#include "mozilla/dom/virtualfilesystem/PVirtualFileSystemParent.h"
#include "mozilla/dom/virtualfilesystem/PVirtualFileSystemRequestParent.h"

namespace mozilla {
namespace dom {
namespace virtualfilesystem {

class BaseVirtualFileSystemService;
class VirtualFileSystemParent;

typedef MountUnmountResultCallback<VirtualFileSystemParent>
VirtualFileSystemParentMountUnmountCallback;

class VirtualFileSystemParent final : public nsISupports
                                    , public PVirtualFileSystemParent
{
public:
  NS_DECL_ISUPPORTS

  VirtualFileSystemParent();

  bool Init();

  virtual bool Recv__delete__() override;

  virtual bool RecvMount(
    const uint32_t& aRequestId,
    const MountOptions& aOptions) override;

  virtual bool RecvUnmount(
    const uint32_t& aRequestId,
    const UnmountOptions& aOptions) override;

  virtual PVirtualFileSystemRequestParent* AllocPVirtualFileSystemRequestParent(
    const uint32_t& aRequestId,
    const nsString& aFileSystemId,
    const VirtualFileSystemIPCRequestedOptions& aOptions) override;

  virtual bool DeallocPVirtualFileSystemRequestParent(
    PVirtualFileSystemRequestParent* aActor) override;

  virtual void ActorDestroy(ActorDestroyReason aWhy) override;

private:
  friend VirtualFileSystemParentMountUnmountCallback;

  virtual ~VirtualFileSystemParent();
  void NotifyMountUnmountResult(uint32_t aRequestId, bool aSucceeded);

  bool mActorDestroyed;
  RefPtr<BaseVirtualFileSystemService> mVirtualFileSystemService;
};

class VirtualFileSystemRequestParent final : public PVirtualFileSystemRequestParent
{
public:
  explicit VirtualFileSystemRequestParent(
    BaseVirtualFileSystemRequestManager* aRequestManager);

  virtual void ActorDestroy(ActorDestroyReason aWhy) override;

  virtual bool RecvResponseData(
    const uint32_t& aRequestId,
    const VirtualFileSystemResponseValue& aResponse) override;

  virtual bool Recv__delete__() override;

private:
  virtual ~VirtualFileSystemRequestParent();

  bool mActorDestroyed;
  RefPtr<BaseVirtualFileSystemRequestManager> mRequestManager;
};

} // namespace virtualfilesystem
} // namespace dom
} // namespace mozilla

#endif // mozilla_dom_VirtualFileSystemParent_h