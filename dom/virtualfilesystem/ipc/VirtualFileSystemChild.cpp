/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "nsIVirtualFileSystemDataType.h"
#include "nsIVirtualFileSystemRequestManager.h"
#include "VirtualFileSystemChild.h"

namespace mozilla {
namespace dom {
namespace virtualfilesystem {

VirtualFileSystemChild::VirtualFileSystemChild(VirtualFileSystemIPCService* aService)
  : mActorDestroyed(false)
  , mService(aService)
{
  MOZ_COUNT_CTOR(VirtualFileSystemChild);
}

VirtualFileSystemChild::~VirtualFileSystemChild()
{
  MOZ_COUNT_DTOR(VirtualFileSystemChild);

  if (!mActorDestroyed) {
    Send__delete__(this);
  }
  mService = nullptr;
}

bool
VirtualFileSystemChild::RecvNotifyMountUnmountResult(const uint32_t& aRequestId,
                                                     const bool& aSucceeded)
{
  if (mService) {
    mService->NotifyMountUnmountResult(aRequestId, aSucceeded);
  }
  return true;
}

PVirtualFileSystemRequestChild*
VirtualFileSystemChild::AllocPVirtualFileSystemRequestChild(
  const uint32_t& aRequestId,
  const uint32_t& aRequestType,
  const nsString& aFileSystemId,
  const VirtualFileSystemIPCRequestedOptions& aOptions)
{
  RefPtr<VirtualFileSystemRequestChild> actor =
    new VirtualFileSystemRequestChild(mService);
  return actor.forget().take();;
}

bool
VirtualFileSystemChild::DeallocPVirtualFileSystemRequestChild(
  PVirtualFileSystemRequestChild* aActor)
{
   RefPtr<VirtualFileSystemRequestChild> actor =
    dont_AddRef(static_cast<VirtualFileSystemRequestChild*>(aActor));
  return true;
}

void
VirtualFileSystemChild::ActorDestroy(ActorDestroyReason aWhy)
{
  mActorDestroyed = true;
  mService->NotifyVirtualFileSystemChildDestroyed();
  mService = nullptr;
}

bool
VirtualFileSystemChild::RecvPVirtualFileSystemRequestConstructor(
  PVirtualFileSystemRequestChild* aActor,
  const uint32_t& aRequestId,
  const uint32_t& aRequestType,
  const nsString& aFileSystemId,
  const VirtualFileSystemIPCRequestedOptions& aOptions)
{
  MOZ_ASSERT(mService);

  PVirtualFileSystemRequestChild* actor =
    static_cast<VirtualFileSystemRequestChild*>(aActor);

  /*nsCOMPtr<nsIVirtualFileSystemRequestManager> manager;
  if (NS_FAILED(mService->GetRequestManagerById(aFileSystemId,
                                                getter_AddRefs(manager)))) {
    return false;
  }

  switch (aOptions.type()) {
    case VirtualFileSystemIPCRequestedOptions::TVirtualFileSystemAbortRequestedOptions:
    {
      VirtualFileSystemAbortRequestedOptions options = aOptions;
      nsCOMPtr<nsIVirtualFileSystemAbortRequestedOptions> options =
        do_CreateInstance(VIRTUAL_FILE_SYSTEM_ABORT_REQUESTED_OPTIONS_CONTRACT_ID);
      options->SetFileSystemId(aFileSystemId);
      options->SetOperationRequestId(options.operationRequestId());

      break;
    }
    case VirtualFileSystemIPCRequestedOptions::TVirtualFileSystemGetMetadataRequestedOptions:
    {
      break;
    }
    case VirtualFileSystemIPCRequestedOptions::TVirtualFileSystemCloseFileRequestedOptions:
    {
      break;
    }
    case VirtualFileSystemIPCRequestedOptions::TVirtualFileSystemOpenFileRequestedOptions:
    {
      break;
    }
    case VirtualFileSystemIPCRequestedOptions::TVirtualFileSystemReadDirectoryRequestedOptions:
    {
      break;
    }
    case VirtualFileSystemIPCRequestedOptions::TVirtualFileSystemReadFileRequestedOptions:
    {
      break;
    }
    case VirtualFileSystemIPCRequestedOptions::TVirtualFileSystemUnmountRequestedOptions:
    {
      break;
    }
    default:
      NS_RUNTIMEABORT("not reached");
      break;
  }*/
  return true;
}

NS_IMPL_ISUPPORTS0(VirtualFileSystemRequestChild)

VirtualFileSystemRequestChild::VirtualFileSystemRequestChild(
  VirtualFileSystemIPCService* aService)
  : mService(aService)
{
  MOZ_COUNT_CTOR(VirtualFileSystemRequestChild);
}

VirtualFileSystemRequestChild::~VirtualFileSystemRequestChild()
{
  MOZ_COUNT_DTOR(VirtualFileSystemRequestChild);
}

bool
VirtualFileSystemRequestChild::Recv__delete__(
  const VirtualFileSystemResponseValue& aResponse)
{
  return true;
}

void
VirtualFileSystemRequestChild::ActorDestroy(ActorDestroyReason aWhy)
{
  mService = nullptr;
}

} // namespace virtualfilesystem
} // namespace dom
} // namespace mozilla