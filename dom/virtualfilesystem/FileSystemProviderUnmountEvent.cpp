/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/dom/FileSystemProviderUnmountEvent.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_CLASS(UnmountRequestedOptions)

NS_IMPL_ADDREF_INHERITED(UnmountRequestedOptions, FileSystemProviderRequestedOptions)
NS_IMPL_RELEASE_INHERITED(UnmountRequestedOptions, FileSystemProviderRequestedOptions)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(UnmountRequestedOptions,
                                                  FileSystemProviderRequestedOptions)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(UnmountRequestedOptions,
                                                FileSystemProviderRequestedOptions)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN_INHERITED(UnmountRequestedOptions,
                                               FileSystemProviderRequestedOptions)
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(UnmountRequestedOptions)
NS_INTERFACE_MAP_END_INHERITING(FileSystemProviderRequestedOptions)

UnmountRequestedOptions::UnmountRequestedOptions(
  nsISupports* aParent,
  uint32_t aRequestId,
  const nsAString& aFileSystemId,
  const VirtualFileSystemIPCRequestedOptions& aOptions)
  : FileSystemProviderRequestedOptions(aParent, aRequestId, aFileSystemId, aOptions)
{
}

JSObject*
UnmountRequestedOptions::WrapObject(JSContext* aCx,
                                    JS::Handle<JSObject*> aGivenProto)
{
  return UnmountRequestedOptionsBinding::Wrap(aCx, this, aGivenProto);
}

FileSystemProviderUnmountEvent::FileSystemProviderUnmountEvent(
  EventTarget* aOwner,
  BaseVirtualFileSystemRequestManager* aManager)
  : FileSystemProviderEventWrap(
    aOwner, aManager, NS_LITERAL_STRING("unmountrequested"))
{

}

JSObject*
FileSystemProviderUnmountEvent::WrapObjectInternal(JSContext* aCx,
                                                   JS::Handle<JSObject*> aGivenProto)
{
  return FileSystemProviderUnmountEventBinding::Wrap(aCx, this, aGivenProto);
}

void
FileSystemProviderUnmountEvent::SuccessCallback()
{
  FileSystemProviderEvent::OnSuccess(nullptr, false);
}

} // namespace dom
} // namespace mozilla