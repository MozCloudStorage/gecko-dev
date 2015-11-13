/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/dom/FileSystemProviderAbortEvent.h"
#include "nsIVirtualFileSystemDataType.h"
#include "nsIVirtualFileSystemRequestManager.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_CLASS(AbortRequestedOptions)

NS_IMPL_ADDREF_INHERITED(AbortRequestedOptions, FileSystemProviderRequestedOptions)
NS_IMPL_RELEASE_INHERITED(AbortRequestedOptions, FileSystemProviderRequestedOptions)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(AbortRequestedOptions,
                                                  FileSystemProviderRequestedOptions)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(AbortRequestedOptions,
                                                FileSystemProviderRequestedOptions)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(AbortRequestedOptions)
NS_INTERFACE_MAP_END_INHERITING(FileSystemProviderRequestedOptions)

AbortRequestedOptions::AbortRequestedOptions(nsISupports* aParent,
                                             nsIVirtualFileSystemRequestedOptions* aOptions)
  : FileSystemProviderRequestedOptions(aParent, aOptions)
{
  nsCOMPtr<nsIVirtualFileSystemAbortRequestedOptions> options =
    do_QueryInterface(aOptions);
  if (!options) {
    MOZ_ASSERT(false, "Invalid nsIVirtualFileSystemRequestedOptions");
    return;
  }

  options->GetOperationRequestId(&mOperationRequestId);
}

JSObject*
AbortRequestedOptions::WrapObject(JSContext* aCx,
                                  JS::Handle<JSObject*> aGivenProto)
{
  return AbortRequestedOptionsBinding::Wrap(aCx, this, aGivenProto);
}

uint32_t
AbortRequestedOptions::OperationRequestId() const
{
  return mOperationRequestId;
}

FileSystemProviderAbortEvent::FileSystemProviderAbortEvent(EventTarget* aOwner,
                                                           nsIVirtualFileSystemRequestManager* aManager)
  : FileSystemProviderEvent(aOwner, aManager)
{

}

JSObject*
FileSystemProviderAbortEvent::WrapObjectInternal(JSContext* aCx,
                                                 JS::Handle<JSObject*> aGivenProto)
{
  return FileSystemProviderAbortEventBinding::Wrap(aCx, this, aGivenProto);
}

AbortRequestedOptions*
FileSystemProviderAbortEvent::Options() const
{
  MOZ_ASSERT(mOptions);

  return static_cast<AbortRequestedOptions*>(mOptions.get());
}

nsresult
FileSystemProviderAbortEvent::InitFileSystemProviderEvent(
  nsIVirtualFileSystemRequestedOptions* aOptions)
{
  RefPtr<AbortRequestedOptions> options = new AbortRequestedOptions(mOwner, aOptions);
  InitFileSystemProviderEventInternal(NS_LITERAL_STRING("abortrequested"), options);
  return NS_OK;
}

void
FileSystemProviderAbortEvent::SuccessCallback()
{
  FileSystemProviderEvent::OnSuccess(nullptr, false);
}

} // namespace dom
} // namespace mozilla