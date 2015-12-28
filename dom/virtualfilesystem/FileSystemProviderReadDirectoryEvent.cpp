/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/dom/FileSystemProviderReadDirectoryEvent.h"
#include "mozilla/Move.h"
#include "nsVirtualFileSystemRequestValue.h"

using mozilla::dom::virtualfilesystem::VirtualFileSystemReadDirectoryRequestedOptions;

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_CLASS(ReadDirectoryRequestedOptions)

NS_IMPL_ADDREF_INHERITED(ReadDirectoryRequestedOptions, FileSystemProviderRequestedOptions)
NS_IMPL_RELEASE_INHERITED(ReadDirectoryRequestedOptions, FileSystemProviderRequestedOptions)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(ReadDirectoryRequestedOptions,
                                                  FileSystemProviderRequestedOptions)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(ReadDirectoryRequestedOptions,
                                                FileSystemProviderRequestedOptions)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN_INHERITED(ReadDirectoryRequestedOptions,
                                               FileSystemProviderRequestedOptions)
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(ReadDirectoryRequestedOptions)
NS_INTERFACE_MAP_END_INHERITING(FileSystemProviderRequestedOptions)

ReadDirectoryRequestedOptions::ReadDirectoryRequestedOptions(
  nsISupports* aParent,
  uint32_t aRequestId,
  const nsAString& aFileSystemId,
  const VirtualFileSystemIPCRequestedOptions& aOptions)
  : FileSystemProviderRequestedOptions(aParent, aRequestId, aFileSystemId, aOptions)
{
}

JSObject*
ReadDirectoryRequestedOptions::WrapObject(JSContext* aCx,
                                          JS::Handle<JSObject*> aGivenProto)
{
  return ReadDirectoryRequestedOptionsBinding::Wrap(aCx, this, aGivenProto);
}

void
ReadDirectoryRequestedOptions::GetDirectoryPath(nsAString& aPath) const
{
  const VirtualFileSystemReadDirectoryRequestedOptions options = mOptions;
  aPath = options.directoryPath();
}

FileSystemProviderReadDirectoryEvent::FileSystemProviderReadDirectoryEvent(
  EventTarget* aOwner,
  nsVirtualFileSystemRequestManager* aManager)
  : FileSystemProviderEventWrap(
    aOwner, aManager, NS_LITERAL_STRING("readdirectoryrequested"))
{

}

JSObject*
FileSystemProviderReadDirectoryEvent::WrapObjectInternal(JSContext* aCx,
                                                         JS::Handle<JSObject*> aGivenProto)
{
  return FileSystemProviderReadDirectoryEventBinding::Wrap(aCx, this, aGivenProto);
}

void
FileSystemProviderReadDirectoryEvent::SuccessCallback(const Sequence<EntryMetadata>& aEntries,
                                                      bool aHasMore)
{
  nsTArray<nsCOMPtr<nsIEntryMetadata>> entries;
  for (uint32_t i = 0; i < aEntries.Length(); i++) {
    entries.AppendElement(virtualfilesystem::nsEntryMetadata::FromEntryMetadata(aEntries[i]));
  }

  nsCOMPtr<nsIVirtualFileSystemReadDirectoryRequestValue> value =
    new virtualfilesystem::nsVirtualFileSystemReadDirectoryRequestValue(Move(entries));

  FileSystemProviderEvent::OnSuccess(value, aHasMore);
}

} // namespace dom
} // namespace mozilla