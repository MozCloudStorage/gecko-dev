/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/dom/FileSystemProviderOpenFileEvent.h"
#include "nsIVirtualFileSystemDataType.h"
#include "nsIVirtualFileSystemRequestManager.h"

using mozilla::dom::virtualfilesystem::VirtualFileSystemOpenFileRequestedOptions;

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_CLASS(OpenFileRequestedOptions)

NS_IMPL_ADDREF_INHERITED(OpenFileRequestedOptions, FileSystemProviderRequestedOptions)
NS_IMPL_RELEASE_INHERITED(OpenFileRequestedOptions, FileSystemProviderRequestedOptions)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(OpenFileRequestedOptions,
                                                  FileSystemProviderRequestedOptions)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(OpenFileRequestedOptions,
                                                FileSystemProviderRequestedOptions)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN_INHERITED(OpenFileRequestedOptions,
                                               FileSystemProviderRequestedOptions)
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(OpenFileRequestedOptions)
NS_INTERFACE_MAP_END_INHERITING(FileSystemProviderRequestedOptions)

OpenFileRequestedOptions::OpenFileRequestedOptions(
  nsISupports* aParent,
  uint32_t aRequestId,
  const nsAString& aFileSystemId,
  const VirtualFileSystemIPCRequestedOptions& aOptions)
  : FileSystemProviderRequestedOptions(aParent, aRequestId, aFileSystemId, aOptions)
{
}

JSObject*
OpenFileRequestedOptions::WrapObject(JSContext* aCx,
                                     JS::Handle<JSObject*> aGivenProto)
{
  return OpenFileRequestedOptionsBinding::Wrap(aCx, this, aGivenProto);
}

void
OpenFileRequestedOptions::GetFilePath(nsAString& aPath) const
{
  const VirtualFileSystemOpenFileRequestedOptions options = mOptions;
  aPath = options.filePath();
}

OpenFileMode
OpenFileRequestedOptions::Mode() const
{
  const VirtualFileSystemOpenFileRequestedOptions options = mOptions;
  return static_cast<OpenFileMode>(options.mode());
}

FileSystemProviderOpenFileEvent::FileSystemProviderOpenFileEvent(
  EventTarget* aOwner,
  nsVirtualFileSystemRequestManager* aManager)
  : FileSystemProviderEventWrap(
    aOwner, aManager, NS_LITERAL_STRING("openfilerequested"))
{

}

JSObject*
FileSystemProviderOpenFileEvent::WrapObjectInternal(JSContext* aCx,
                                                 JS::Handle<JSObject*> aGivenProto)
{
  return FileSystemProviderOpenFileEventBinding::Wrap(aCx, this, aGivenProto);
}

void
FileSystemProviderOpenFileEvent::SuccessCallback()
{
  FileSystemProviderEvent::OnSuccess(nullptr, false);
}

} // namespace dom
} // namespace mozilla