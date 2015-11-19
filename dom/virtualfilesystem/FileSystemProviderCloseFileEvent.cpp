/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/dom/FileSystemProviderCloseFileEvent.h"
#include "nsIVirtualFileSystemDataType.h"
#include "nsIVirtualFileSystemRequestManager.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_CLASS(CloseFileRequestedOptions)

NS_IMPL_ADDREF_INHERITED(CloseFileRequestedOptions, FileSystemProviderRequestedOptions)
NS_IMPL_RELEASE_INHERITED(CloseFileRequestedOptions, FileSystemProviderRequestedOptions)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(CloseFileRequestedOptions,
                                                  FileSystemProviderRequestedOptions)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(CloseFileRequestedOptions,
                                                FileSystemProviderRequestedOptions)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN_INHERITED(CloseFileRequestedOptions,
                                               FileSystemProviderRequestedOptions)
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(CloseFileRequestedOptions)
NS_INTERFACE_MAP_END_INHERITING(FileSystemProviderRequestedOptions)

CloseFileRequestedOptions::CloseFileRequestedOptions(
  nsISupports* aParent,
  nsIVirtualFileSystemCloseFileRequestedOptions* aOptions)
  : FileSystemProviderRequestedOptions(aParent, aOptions)
{
  aOptions->GetOpenRequestId(&mOpenRequestId);
}

JSObject*
CloseFileRequestedOptions::WrapObject(JSContext* aCx,
                                      JS::Handle<JSObject*> aGivenProto)
{
  return CloseFileRequestedOptionsBinding::Wrap(aCx, this, aGivenProto);
}

uint32_t
CloseFileRequestedOptions::OpenRequestId() const
{
  return mOpenRequestId;
}

FileSystemProviderCloseFileEvent::FileSystemProviderCloseFileEvent(
  EventTarget* aOwner,
  nsIVirtualFileSystemRequestManager* aManager)
  : FileSystemProviderEventWrap(
    aOwner, aManager, NS_LITERAL_STRING("closefilerequested"))
{
}

JSObject*
FileSystemProviderCloseFileEvent::WrapObjectInternal(JSContext* aCx,
                                                     JS::Handle<JSObject*> aGivenProto)
{
  return FileSystemProviderCloseFileEventBinding::Wrap(aCx, this, aGivenProto);
}

void
FileSystemProviderCloseFileEvent::SuccessCallback()
{
  FileSystemProviderEvent::OnSuccess(nullptr, false);
}

} // namespace dom
} // namespace mozilla