/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/unused.h"
#include "nsVirtualFileSystemRequestManager.h"
#include "nsVirtualFileSystemService.h"
#include "VirtualFileSystemParent.h"
#include "VirtualFileSystemServiceFactory.h"

namespace mozilla {
namespace dom {
namespace virtualfilesystem {

class MountUnmountResultCallback final : public nsIVirtualFileSystemCallback
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIVIRTUALFILESYSTEMCALLBACK

  explicit MountUnmountResultCallback(VirtualFileSystemParent* aParent)
    : mVirtualFileSystemParent(aParent)
  {}
  void Forget() { mVirtualFileSystemParent = nullptr; }

private:
  ~MountUnmountResultCallback() = default;

  VirtualFileSystemParent* MOZ_NON_OWNING_REF mVirtualFileSystemParent;
};

NS_IMPL_ISUPPORTS(MountUnmountResultCallback,
                  nsIVirtualFileSystemCallback)

NS_IMETHODIMP
MountUnmountResultCallback::OnSuccess(uint32_t aRequestId,
                                      nsIVirtualFileSystemRequestValue* aValue,
                                      bool aHasMore)
{
  Unused << aValue;
  Unused << aHasMore;

  Unused << mVirtualFileSystemParent->SendNotifyMountUnmountResult(aRequestId,
                                                                   true);
  return NS_OK;
}

NS_IMETHODIMP
MountUnmountResultCallback::OnError(uint32_t aRequestId, uint32_t aErrorCode)
{
  Unused << aErrorCode;

  Unused << mVirtualFileSystemParent->SendNotifyMountUnmountResult(aRequestId,
                                                                   false);
  return NS_OK;
}

NS_IMPL_ISUPPORTS0(VirtualFileSystemParent)

VirtualFileSystemParent::VirtualFileSystemParent()
{
  MOZ_COUNT_CTOR(VirtualFileSystemParent);
}

VirtualFileSystemParent::~VirtualFileSystemParent()
{
  MOZ_COUNT_DTOR(VirtualFileSystemParent);
}

bool
VirtualFileSystemParent::Init()
{
  MOZ_ASSERT(!mVirtualFileSystemService);

  mVirtualFileSystemService =
    VirtualFileSystemServiceFactory::AutoCreateVirtualFileSystemService();
  if (!mVirtualFileSystemService) {
    return false;
  }

  return true;
}

bool
VirtualFileSystemParent::Recv__delete__()
{
  return true;
}

bool
VirtualFileSystemParent::RecvMount(
  const uint32_t& aRequestId,
  const MountOptions& aOptions)
{
  RefPtr<nsVirtualFileSystemRequestManager> manager =
    new nsVirtualFileSystemRequestManager(nullptr);
  nsCOMPtr<nsIVirtualFileSystemCallback> callback =
    new MountUnmountResultCallback(this);
  if (NS_FAILED(mVirtualFileSystemService->Mount(aRequestId,
                                                 aOptions,
                                                 manager,
                                                 callback))) {
    return false;
  }
  return true;
}

bool
VirtualFileSystemParent::RecvUnmount(
  const uint32_t& aRequestId,
  const UnmountOptions& aOptions)
{
  nsCOMPtr<nsIVirtualFileSystemCallback> callback =
    new MountUnmountResultCallback(this);
  if (NS_FAILED(mVirtualFileSystemService->Unmount(aRequestId, aOptions, callback))) {
    return false;
  }
  return true;
}

PVirtualFileSystemRequestParent*
VirtualFileSystemParent::AllocPVirtualFileSystemRequestParent(
  const uint32_t& aRequestId,
  const uint32_t& aRequestType,
  const nsString& aFileSystemId,
  const VirtualFileSystemIPCRequestedOptions& aOptions)
{
  return nullptr;
}

bool
VirtualFileSystemParent::DeallocPVirtualFileSystemRequestParent(
  PVirtualFileSystemRequestParent* aActor)
{
  return true;
}

void
VirtualFileSystemParent::ActorDestroy(ActorDestroyReason aWhy)
{

}

VirtualFileSystemRequestParent::VirtualFileSystemRequestParent()
{
  MOZ_COUNT_CTOR(VirtualFileSystemRequestParent);
}

VirtualFileSystemRequestParent::~VirtualFileSystemRequestParent()
{
  MOZ_COUNT_DTOR(VirtualFileSystemRequestParent);
}

void
VirtualFileSystemRequestParent::ActorDestroy(ActorDestroyReason aWhy)
{

}

} // namespace virtualfilesystem
} // namespace dom
} // namespace mozilla