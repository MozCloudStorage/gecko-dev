/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/ClearOnShutdown.h"
#include "mozilla/dom/ContentChild.h"
#include "nsIMutableArray.h"
#include "nsIVirtualFileSystemCallback.h"
#include "nsVirtualFileSystemDataType.h"
#include "VirtualFileSystemChild.h"
#include "VirtualFileSystemIPCService.h"

namespace mozilla {
namespace dom {
namespace virtualfilesystem {

namespace {

VirtualFileSystemChild* gVirtualFileSystemChild;

} // anonymous

VirtualFileSystemIPCService::VirtualFileSystemIPCService()
{
  ContentChild* contentChild = ContentChild::GetSingleton();
  if (NS_WARN_IF(!contentChild)) {
    return;
  }
  gVirtualFileSystemChild = new VirtualFileSystemChild(this);
  NS_WARN_IF(!contentChild->SendPVirtualFileSystemConstructor(gVirtualFileSystemChild));
}

VirtualFileSystemIPCService::~VirtualFileSystemIPCService()
{
  gVirtualFileSystemChild = nullptr;
}

nsresult
VirtualFileSystemIPCService::Mount(uint32_t aRequestId,
                                   const MountOptions& aOptions,
                                   nsVirtualFileSystemRequestManager* aRequestManager,
                                   nsIVirtualFileSystemCallback* aCallback)
{
  if (NS_WARN_IF(!(aRequestManager && aCallback))) {
    return NS_ERROR_INVALID_ARG;
  }

  if (!gVirtualFileSystemChild) {
    return NS_ERROR_NOT_INITIALIZED;
  }

  if (mMountUnmountCallbackMap.find(aRequestId) != mMountUnmountCallbackMap.end()) {
    MOZ_ASSERT(false, "RequestId can not be duplicated!");
    return NS_ERROR_FAILURE;
  }

  uint32_t errorCode;
  if (!MountInternal(aOptions, aRequestManager, &errorCode)) {
    aCallback->OnError(aRequestId, errorCode);
    return NS_ERROR_FAILURE;
  }

  if (!gVirtualFileSystemChild->SendMount(aRequestId, aOptions)) {
    return NS_ERROR_FAILURE;
  }

  mMountUnmountCallbackMap[aRequestId] = aCallback;
  return NS_OK;
}

nsresult
VirtualFileSystemIPCService::Unmount(
  uint32_t aRequestId,
  const UnmountOptions& aOptions,
  nsIVirtualFileSystemCallback* aCallback)
{
  if (NS_WARN_IF(!aCallback)) {
    return NS_ERROR_INVALID_ARG;
  }

  if (!gVirtualFileSystemChild) {
    return NS_ERROR_NOT_INITIALIZED;
  }

  if (mMountUnmountCallbackMap.find(aRequestId) != mMountUnmountCallbackMap.end()) {
    MOZ_ASSERT(false, "RequestId can not be duplicated!");
    return NS_ERROR_FAILURE;
  }

  uint32_t errorCode;
  if (!UnmountInternal(aOptions, &errorCode)) {
    aCallback->OnError(aRequestId, errorCode);
    return NS_ERROR_FAILURE;
  }

  if (!gVirtualFileSystemChild->SendUnmount(aRequestId, aOptions)) {
    return NS_ERROR_FAILURE;
  }

  mMountUnmountCallbackMap[aRequestId] = aCallback;
  return NS_OK;
}

nsresult
VirtualFileSystemIPCService::GetVirtualFileSystemById(const nsAString& aFileSystemId,
                                                      nsIVirtualFileSystemInfo** aInfo)
{
  uint32_t index;
  if (!FindVirtualFileSystemById(aFileSystemId, index)) {
    *aInfo = nullptr;
    return NS_ERROR_INVALID_ARG;
  }

  RefPtr<VirtualFileSystem> fileSystem = mVirtualFileSystems[index];

  nsCOMPtr<nsIVirtualFileSystemInfo> info = fileSystem->ConvertToVirtualFileSystemInfo();
  info.forget(aInfo);

  return NS_OK;
}

nsresult
VirtualFileSystemIPCService::GetAllVirtualFileSystem(nsIArray** aFileSystems)
{
  nsresult rv;
  nsCOMPtr<nsIMutableArray> fileSystemArray = do_CreateInstance(NS_ARRAY_CONTRACTID, &rv);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  for (const auto& fileSystem : mVirtualFileSystems) {
    nsCOMPtr<nsIVirtualFileSystemInfo> info;
    if (NS_SUCCEEDED(GetVirtualFileSystemById(fileSystem->FileSystemId(),
                                              getter_AddRefs(info)))) {
      fileSystemArray->AppendElement(info, false);
    }
  }
  fileSystemArray.forget(aFileSystems);
  return NS_OK;
}

void
VirtualFileSystemIPCService::NotifyVirtualFileSystemChildDestroyed()
{
  gVirtualFileSystemChild = nullptr;
}

bool
VirtualFileSystemIPCService::NotifyMountUnmountResult(
  uint32_t aRequestId,
  bool aSucceeded)
{
  CallbackMapType::iterator it = mMountUnmountCallbackMap.find(aRequestId);
  if (it == mMountUnmountCallbackMap.end()) {
    MOZ_ASSERT(false, "RequestId is not found!");
    return false;
  }

  if (aSucceeded) {
    it->second->OnSuccess(aRequestId, nullptr, false);
  }
  else {
    it->second->OnError(aRequestId, 0);
  }

  mMountUnmountCallbackMap.erase(it);
  return true;
}

} // namespace virtualfilesystem
} // namespace dom
} // namespace mozilla