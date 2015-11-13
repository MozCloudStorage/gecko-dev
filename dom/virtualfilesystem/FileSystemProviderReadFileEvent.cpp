/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/dom/FileSystemProviderReadFileEvent.h"
#include "nsIVirtualFileSystemDataType.h"
#include "nsIVirtualFileSystemRequestManager.h"
#include "nsVirtualFileSystemRequestValue.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_CLASS(ReadFileRequestedOptions)

NS_IMPL_ADDREF_INHERITED(ReadFileRequestedOptions, FileSystemProviderRequestedOptions)
NS_IMPL_RELEASE_INHERITED(ReadFileRequestedOptions, FileSystemProviderRequestedOptions)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(ReadFileRequestedOptions,
                                                  FileSystemProviderRequestedOptions)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(ReadFileRequestedOptions,
                                                FileSystemProviderRequestedOptions)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(ReadFileRequestedOptions)
NS_INTERFACE_MAP_END_INHERITING(FileSystemProviderRequestedOptions)

ReadFileRequestedOptions::ReadFileRequestedOptions(
  nsISupports* aParent,
  nsIVirtualFileSystemRequestedOptions* aOptions)
  : FileSystemProviderRequestedOptions(aParent, aOptions)
{
  nsCOMPtr<nsIVirtualFileSystemReadFileRequestedOptions> options =
    do_QueryInterface(aOptions);
  if (!options) {
    MOZ_ASSERT(false, "Invalid nsIVirtualFileSystemRequestedOptions");
    return;
  }

  options->GetOpenRequestId(&mOpenRequestId);
  options->GetOffset(&mOffset);
  options->GetLength(&mLength);
}

JSObject*
ReadFileRequestedOptions::WrapObject(JSContext* aCx,
                                     JS::Handle<JSObject*> aGivenProto)
{
  return ReadFileRequestedOptionsBinding::Wrap(aCx, this, aGivenProto);
}

uint32_t
ReadFileRequestedOptions::OpenRequestId() const
{
  return mOpenRequestId;
}

uint64_t
ReadFileRequestedOptions::Offset() const
{
  return mOffset;
}

uint64_t
ReadFileRequestedOptions::Length() const
{
  return mLength;
}

FileSystemProviderReadFileEvent::FileSystemProviderReadFileEvent(
  EventTarget* aOwner,
  nsIVirtualFileSystemRequestManager* aManager)
  : FileSystemProviderEvent(aOwner, aManager)
{

}

JSObject*
FileSystemProviderReadFileEvent::WrapObjectInternal(JSContext* aCx,
                                                    JS::Handle<JSObject*> aGivenProto)
{
  return FileSystemProviderReadFileEventBinding::Wrap(aCx, this, aGivenProto);
}

ReadFileRequestedOptions*
FileSystemProviderReadFileEvent::Options() const
{
  MOZ_ASSERT(mOptions);

  return static_cast<ReadFileRequestedOptions*>(mOptions.get());
}

nsresult
FileSystemProviderReadFileEvent::InitFileSystemProviderEvent(
  nsIVirtualFileSystemRequestedOptions* aOptions)
{
  RefPtr<ReadFileRequestedOptions> options = new ReadFileRequestedOptions(mOwner, aOptions);
  InitFileSystemProviderEventInternal(NS_LITERAL_STRING("readfilerequested"),
                                      options);
  return NS_OK;
}

void
FileSystemProviderReadFileEvent::SuccessCallback(const ArrayBuffer& aData, bool aHasMore)
{
  nsCOMPtr<nsIVirtualFileSystemReadFileRequestValue> value =
    virtualfilesystem::nsVirtualFileSystemReadFileRequestValue::CreateFromArrayBuffer(aData);

  FileSystemProviderEvent::OnSuccess(value, aHasMore);
}

} // namespace dom
} // namespace mozilla