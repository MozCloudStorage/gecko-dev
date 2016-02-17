/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/dom/FileSystemProviderCommon.h"
#include "mozilla/dom/virtualfilesystem/VirtualFileSystemIPCService.h"
#include "mozilla/unused.h"
#include "nsMemory.h"
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
  mEventDispatcherMap.clear();
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
  const nsString& aFileSystemId,
  const VirtualFileSystemIPCRequestedOptions& aOptions)
{
  RefPtr<VirtualFileSystemRequestChild> actor =
    new VirtualFileSystemRequestChild(aOptions.type());
  return actor.forget().take();
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
  const nsString& aFileSystemId,
  const VirtualFileSystemIPCRequestedOptions& aOptions)
{
  MOZ_ASSERT(mService);

  nsString fileSystemId = nsString(aFileSystemId);
  EventDispatcherMapType::iterator it =
    mEventDispatcherMap.find(fileSystemId);
  if (it == mEventDispatcherMap.end()) {
    return false;
  }

  auto actor = static_cast<VirtualFileSystemRequestChild*>(aActor);
  if (NS_FAILED(it->second->DispatchFileSystemProviderEvent(aRequestId,
                                                            aFileSystemId,
                                                            aOptions,
                                                            actor))) {
    return false;
  }

  return true;
}

void
VirtualFileSystemChild::RegisterEventDispatcher(
  const nsAString& aFileSystemId,
  BaseFileSystemProviderEventDispatcher* aDispatcher)
{
  if (mActorDestroyed) {
    return;
  }

  nsString fileSystemId = nsString(aFileSystemId);
  EventDispatcherMapType::iterator it =
    mEventDispatcherMap.find(fileSystemId);
  if (it == mEventDispatcherMap.end()) {
    mEventDispatcherMap[fileSystemId] = aDispatcher;
  }
}

void
VirtualFileSystemChild::UnregisterEventDispatcher(
  const nsAString& aFileSystemId)
{
  if (mActorDestroyed) {
    return;
  }

  nsString fileSystemId = nsString(aFileSystemId);
  mEventDispatcherMap.erase(fileSystemId);
}

void
VirtualFileSystemRequestChild::ActorDestroy(ActorDestroyReason aWhy)
{
  mActorDestroyed = true;
}

nsresult
VirtualFileSystemRequestChild::CreateRequest(
  const nsAString& aFileSystemId,
  const VirtualFileSystemIPCRequestedOptions& aOptions,
  nsIVirtualFileSystemCallback* aCallback,
  uint32_t* aRequestId)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

namespace {

  void ConvertToEntryMetadata(nsIEntryMetadata* aMetaData,
                              EntryMetadata& aResult)
  {
    if (!aMetaData) {
      return;
    }

    bool isDirectory;
    nsString name;
    uint64_t size;
    DOMTimeStamp modificationTime;
    nsString mimeType;
    aMetaData->GetIsDirectory(&isDirectory);
    aMetaData->GetName(name);
    aMetaData->GetSize(&size);
    aMetaData->GetModificationTime(&modificationTime);
    aMetaData->GetMimeType(mimeType);

    aResult.mIsDirectory = isDirectory;
    aResult.mModificationTime = modificationTime;
    aResult.mName = name;
    aResult.mSize = size;
    aResult.mMimeType.Construct(mimeType);
  }

} // anonymous namespace

nsresult
VirtualFileSystemRequestChild::FufillRequest(
  uint32_t aRequestId,
  nsIVirtualFileSystemRequestValue* aValue,
  bool aHasMore)
{
  if (mActorDestroyed) {
    return NS_ERROR_FAILURE;
  }

  switch (mType) {
    case VirtualFileSystemIPCRequestedOptions::TVirtualFileSystemAbortRequestedOptions:
    case VirtualFileSystemIPCRequestedOptions::TVirtualFileSystemCloseFileRequestedOptions:
    case VirtualFileSystemIPCRequestedOptions::TVirtualFileSystemOpenFileRequestedOptions:
    case VirtualFileSystemIPCRequestedOptions::TVirtualFileSystemUnmountRequestedOptions:
    {
      VirtualFileSystemSuccessResponse response;
      Unused << SendResponseData(aRequestId, response);
    }
    case VirtualFileSystemIPCRequestedOptions::TVirtualFileSystemGetMetadataRequestedOptions:
    {
      nsCOMPtr<nsIVirtualFileSystemGetMetadataRequestValue> value =
        do_QueryInterface(aValue);
      if (!value) {
        return NS_ERROR_FAILURE;
      }

      nsCOMPtr<nsIEntryMetadata> metadata;
      value->GetMetadata(getter_AddRefs(metadata));
      EntryMetadata result;
      ConvertToEntryMetadata(metadata, result);

      VirtualFileSystemGetMetadataResponse response(result, aHasMore);
      Unused << SendResponseData(aRequestId, response);
    }
    case VirtualFileSystemIPCRequestedOptions::TVirtualFileSystemReadDirectoryRequestedOptions:
    {
      nsCOMPtr<nsIVirtualFileSystemReadDirectoryRequestValue> value =
        do_QueryInterface(aValue);
      if (!value) {
        return NS_ERROR_FAILURE;
      }

      uint32_t length;
      nsIEntryMetadata** entries;
      nsresult rv = value->GetEntries(&length, &entries);
      if (NS_FAILED(rv)) {
        return rv;
      }

      nsTArray<EntryMetadata> metadataArray;
      for (uint32_t i = 0; i < length; i++) {
        EntryMetadata result;
        ConvertToEntryMetadata(entries[i], result);
        metadataArray.AppendElement(result);
      }
      NS_FREE_XPCOM_ISUPPORTS_POINTER_ARRAY(length, entries);

      VirtualFileSystemReadDirectoryResponse response(metadataArray, aHasMore);
      Unused << SendResponseData(aRequestId, response);
      break;
    }
    case VirtualFileSystemIPCRequestedOptions::TVirtualFileSystemReadFileRequestedOptions:
    {
      nsCOMPtr<nsIVirtualFileSystemReadFileRequestValue> value =
        do_QueryInterface(aValue);
      if (!value) {
        return NS_ERROR_FAILURE;
      }

      nsAutoCString data;
      value->GetData(data);
      VirtualFileSystemReadFileResponse response(data, aHasMore);
      Unused << SendResponseData(aRequestId, response);
      break;
    }
    default:
       MOZ_ASSERT_UNREACHABLE("unexpected options type");
      return NS_ERROR_UNEXPECTED;
  }

  if (!aHasMore) {
    Unused << Send__delete__(this);
  }

  return NS_OK;
}

nsresult
VirtualFileSystemRequestChild::RejectRequest(
  uint32_t aRequestId,
  uint32_t aErrorCode)
{
  if (mActorDestroyed) {
    return NS_ERROR_FAILURE;
  }

  VirtualFileSystemErrorResponse response(aErrorCode);
  Unused << SendResponseData(aRequestId, response);
  Unused << Send__delete__(this);
  return NS_OK;
}

} // namespace virtualfilesystem
} // namespace dom
} // namespace mozilla