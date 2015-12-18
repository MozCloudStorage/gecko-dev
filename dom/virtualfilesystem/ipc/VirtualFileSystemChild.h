/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_VirtualFileSystemChild_h
#define mozilla_dom_VirtualFileSystemChild_h

#include "mozilla/dom/virtualfilesystem/PVirtualFileSystemChild.h"
#include "mozilla/dom/virtualfilesystem/PVirtualFileSystemRequestChild.h"

namespace mozilla {
namespace dom {
namespace virtualfilesystem {

class VirtualFileSystemIPCService;

class VirtualFileSystemChild final : public PVirtualFileSystemChild
{
public:
  VirtualFileSystemChild(VirtualFileSystemIPCService* aService);

  virtual bool RecvNotifyMountUnmountResult(const uint32_t& aRequestId,
                                            const bool& aSucceeded) override;

  virtual PVirtualFileSystemRequestChild* AllocPVirtualFileSystemRequestChild(
    const uint32_t& aRequestId,
    const uint32_t& aRequestType,
    const nsString& aFileSystemId,
    const VirtualFileSystemIPCRequestedOptions& aOptions) override;

  virtual bool DeallocPVirtualFileSystemRequestChild(
    PVirtualFileSystemRequestChild* aActor) override;

  virtual void ActorDestroy(ActorDestroyReason aWhy) override;

  virtual bool RecvPVirtualFileSystemRequestConstructor(
    PVirtualFileSystemRequestChild* aActor,
    const uint32_t& aRequestId,
    const uint32_t& aRequestType,
    const nsString& aFileSystemId,
    const VirtualFileSystemIPCRequestedOptions& aOptions) override;

private:
  virtual ~VirtualFileSystemChild();

  bool mActorDestroyed;
  RefPtr<VirtualFileSystemIPCService> mService;
};

class VirtualFileSystemRequestChild final : public nsISupports
                                          , public PVirtualFileSystemRequestChild
{
public:
  NS_DECL_ISUPPORTS

  VirtualFileSystemRequestChild(VirtualFileSystemIPCService* aService);

  virtual bool Recv__delete__(
    const VirtualFileSystemResponseValue& aResponse) override;

  virtual void ActorDestroy(ActorDestroyReason aWhy) override;

private:
  virtual ~VirtualFileSystemRequestChild();

  RefPtr<VirtualFileSystemIPCService> mService;
};

} // namespace virtualfilesystem
} // namespace dom
} // namespace mozilla

#endif // mozilla_dom_VirtualFileSystemChild_h