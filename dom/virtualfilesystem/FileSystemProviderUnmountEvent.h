/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_FileSystemProviderUnmountEvent_h
#define mozilla_dom_FileSystemProviderUnmountEvent_h

#include "mozilla/Attributes.h"
#include "mozilla/ErrorResult.h"
#include "mozilla/dom/FileSystemProviderEvent.h"
#include "mozilla/dom/FileSystemProviderUnmountEventBinding.h"
#include "nsWrapperCache.h"

namespace mozilla {
namespace dom {

class UnmountRequestedOptions final : public FileSystemProviderRequestedOptions
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_INHERITED(UnmountRequestedOptions,
                                                         FileSystemProviderRequestedOptions)

  explicit UnmountRequestedOptions(nsISupports* aParent,
                                   nsIVirtualFileSystemUnmountRequestedOptions* aOptions);

  virtual JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

private:
  ~UnmountRequestedOptions() = default;
};

class FileSystemProviderUnmountEvent final
  : public FileSystemProviderEventWrap<UnmountRequestedOptions,
                                       nsIVirtualFileSystemUnmountRequestedOptions>
{
public:
  FileSystemProviderUnmountEvent(EventTarget* aOwner,
                                 nsIVirtualFileSystemRequestManager* aManager);

  virtual JSObject* WrapObjectInternal(JSContext* aCx,
                                       JS::Handle<JSObject*> aGivenProto) override;

  void SuccessCallback();

private:
  virtual ~FileSystemProviderUnmountEvent() = default;

};

} // namespace dom
} // namespace mozilla

#endif // mozilla_dom_FileSystemProviderUnmountEvent_h