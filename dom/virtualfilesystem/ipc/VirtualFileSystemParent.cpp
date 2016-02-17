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

class VirtualFileSystemParentEventDispatcher final
  : public FileSystemProviderProxy<VirtualFileSystemParent>
  , public BaseFileSystemProviderEventDispatcher
{
public:
  explicit VirtualFileSystemParentEventDispatcher(VirtualFileSystemParent* aProvider)
    : FileSystemProviderProxy(aProvider) {}

  virtual nsresult DispatchFileSystemProviderEvent(
    uint32_t aRequestId,
    const nsAString& aFileSystemId,
    const virtualfilesystem::VirtualFileSystemIPCRequestedOptions& aOptions,
    virtualfilesystem::BaseVirtualFileSystemRequestManager* aRequestManager) override;

private:
  virtual ~VirtualFileSystemParentEventDispatcher() = default;

};

nsresult
VirtualFileSystemParentEventDispatcher::DispatchFileSystemProviderEvent(
  uint32_t aRequestId,
  const nsAString& aFileSystemId,
  const virtualfilesystem::VirtualFileSystemIPCRequestedOptions& aOptions,
  virtualfilesystem::BaseVirtualFileSystemRequestManager* aRequestManager)
{
  PVirtualFileSystemRequestParent* actor =
    new VirtualFileSystemRequestParent(aRequestManager);
  bool res = mFileSystemProvider->SendPVirtualFileSystemRequestConstructor(
    actor,
    aRequestId,
    nsString(aFileSystemId),
    aOptions);
  return res ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMPL_ISUPPORTS0(VirtualFileSystemParent)

VirtualFileSystemParent::VirtualFileSystemParent()
  : mActorDestroyed(false)
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
  nsCOMPtr<nsIVirtualFileSystemCallback> callback =
    new VirtualFileSystemParentMountUnmountCallback(this);
  if (NS_FAILED(mVirtualFileSystemService->Mount(aRequestId,
                                                 aOptions,
                                                 new VirtualFileSystemParentEventDispatcher(this),
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
    new VirtualFileSystemParentMountUnmountCallback(this);
  if (NS_FAILED(mVirtualFileSystemService->Unmount(aRequestId, aOptions, callback))) {
    return false;
  }
  return true;
}

PVirtualFileSystemRequestParent*
VirtualFileSystemParent::AllocPVirtualFileSystemRequestParent(
  const uint32_t& aRequestId,
  const nsString& aFileSystemId,
  const VirtualFileSystemIPCRequestedOptions& aOptions)
{
  return nullptr;
}

bool
VirtualFileSystemParent::DeallocPVirtualFileSystemRequestParent(
  PVirtualFileSystemRequestParent* aActor)
{
  delete aActor;
  return true;
}

void
VirtualFileSystemParent::ActorDestroy(ActorDestroyReason aWhy)
{
  mActorDestroyed = true;
}

void
VirtualFileSystemParent::NotifyMountUnmountResult(uint32_t aRequestId, bool aSucceeded)
{
  if (mActorDestroyed) {
    return;
  }

  Unused << SendNotifyMountUnmountResult(aRequestId, aSucceeded);
}

VirtualFileSystemRequestParent::VirtualFileSystemRequestParent(
  BaseVirtualFileSystemRequestManager* aRequestManager)
  : mActorDestroyed(false)
  , mRequestManager(aRequestManager)
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
  mActorDestroyed = true;
}

bool
VirtualFileSystemRequestParent::RecvResponseData(
  const uint32_t& aRequestId,
  const VirtualFileSystemResponseValue& aResponse)
{
  if (mActorDestroyed) {
    return true;
  }

  nsresult rv = NS_OK;
  switch (aResponse.type()) {
    case VirtualFileSystemResponseValue::TVirtualFileSystemErrorResponse:
    {
      VirtualFileSystemErrorResponse response = aResponse;
      rv = mRequestManager->RejectRequest(aRequestId,
                                          response.error());
      break;
    }
    case VirtualFileSystemResponseValue::TVirtualFileSystemSuccessResponse:
    {
      rv = mRequestManager->FufillRequest(aRequestId,
                                          nullptr,
                                          false);
      break;
    }
    case VirtualFileSystemResponseValue::TVirtualFileSystemGetMetadataResponse:
    {
      VirtualFileSystemGetMetadataResponse response = aResponse;
      nsCOMPtr<nsIVirtualFileSystemGetMetadataRequestValue> value =
        new nsVirtualFileSystemGetMetadataRequestValue(response.metadata());
      rv = mRequestManager->FufillRequest(aRequestId,
                                          value,
                                          response.hasMore());
      break;
    }
    case VirtualFileSystemResponseValue::TVirtualFileSystemReadFileResponse:
    {
      VirtualFileSystemReadFileResponse response = aResponse;
      nsCOMPtr<nsIVirtualFileSystemReadFileRequestValue> value =
        new nsVirtualFileSystemReadFileRequestValue(response.data());
      rv = mRequestManager->FufillRequest(aRequestId,
                                          value,
                                          response.hasMore());
      break;
    }
    case VirtualFileSystemResponseValue::TVirtualFileSystemReadDirectoryResponse:
    {
      VirtualFileSystemReadDirectoryResponse response = aResponse;
      nsCOMPtr<nsIVirtualFileSystemReadDirectoryRequestValue> value =
        new nsVirtualFileSystemReadDirectoryRequestValue(Move(response.entries()));
      rv = mRequestManager->FufillRequest(aRequestId,
                                          value,
                                          response.hasMore());
      break;
    }
    default:
      NS_NOTREACHED("Unexpected response type");
      return false;
  }

  if (NS_WARN_IF(NS_FAILED(rv))) {
    return false;
  }
  return true;
}

bool
VirtualFileSystemRequestParent::Recv__delete__()
{
  return true;
}

} // namespace virtualfilesystem
} // namespace dom
} // namespace mozilla