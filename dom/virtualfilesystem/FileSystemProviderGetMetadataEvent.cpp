/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/dom/FileSystemProviderGetMetadataEvent.h"
#include "nsVirtualFileSystemDataType.h"
#include "nsVirtualFileSystemRequestValue.h"
#include "nsIVirtualFileSystemRequestManager.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_CLASS(GetMetadataRequestedOptions)

NS_IMPL_ADDREF_INHERITED(GetMetadataRequestedOptions, FileSystemProviderRequestedOptions)
NS_IMPL_RELEASE_INHERITED(GetMetadataRequestedOptions, FileSystemProviderRequestedOptions)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(GetMetadataRequestedOptions,
                                                  FileSystemProviderRequestedOptions)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(GetMetadataRequestedOptions,
                                                FileSystemProviderRequestedOptions)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN_INHERITED(GetMetadataRequestedOptions,
                                               FileSystemProviderRequestedOptions)
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(GetMetadataRequestedOptions)
NS_INTERFACE_MAP_END_INHERITING(FileSystemProviderRequestedOptions)

GetMetadataRequestedOptions::GetMetadataRequestedOptions(
  nsISupports* aParent,
  nsIVirtualFileSystemGetMetadataRequestedOptions* aOptions)
  : FileSystemProviderRequestedOptions(aParent, aOptions)
{
  aOptions->GetEntryPath(mEntryPath);
}

JSObject*
GetMetadataRequestedOptions::WrapObject(JSContext* aCx,
                                        JS::Handle<JSObject*> aGivenProto)
{
  return GetMetadataRequestedOptionsBinding::Wrap(aCx, this, aGivenProto);
}

void
GetMetadataRequestedOptions::GetEntryPath(nsAString& aPath) const
{
  aPath = mEntryPath;
}

FileSystemProviderGetMetadataEvent::FileSystemProviderGetMetadataEvent(
  EventTarget* aOwner,
  nsIVirtualFileSystemRequestManager* aManager)
  : FileSystemProviderEventWrap(
    aOwner, aManager, NS_LITERAL_STRING("getmetadatarequested"))
{

}

JSObject*
FileSystemProviderGetMetadataEvent::WrapObjectInternal(JSContext* aCx,
                                                       JS::Handle<JSObject*> aGivenProto)
{
  return FileSystemProviderGetMetadataEventBinding::Wrap(aCx, this, aGivenProto);
}

void
FileSystemProviderGetMetadataEvent::SuccessCallback(const EntryMetadata& aData)
{
  nsCOMPtr<nsIVirtualFileSystemGetMetadataRequestValue> value =
    new virtualfilesystem::nsVirtualFileSystemGetMetadataRequestValue(aData);

  FileSystemProviderEvent::OnSuccess(value, false);
}

} // namespace dom
} // namespace mozilla