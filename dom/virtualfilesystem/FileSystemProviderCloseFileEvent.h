/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_FileSystemProviderCloseFileEvent_h
#define mozilla_dom_FileSystemProviderCloseFileEvent_h

#include "mozilla/Attributes.h"
#include "mozilla/ErrorResult.h"
#include "mozilla/dom/FileSystemProviderEvent.h"
#include "mozilla/dom/FileSystemProviderCloseFileEventBinding.h"
#include "nsWrapperCache.h"

namespace mozilla {
namespace dom {

class CloseFileRequestedOptions final : public FileSystemProviderRequestedOptions
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_INHERITED(CloseFileRequestedOptions,
                                                         FileSystemProviderRequestedOptions)

  explicit CloseFileRequestedOptions(nsISupports* aParent,
                                     uint32_t aRequestId,
                                     const nsAString& aFileSystemId,
                                     const VirtualFileSystemIPCRequestedOptions& aOptions);

  virtual JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  uint32_t OpenRequestId() const;

private:
  virtual ~CloseFileRequestedOptions() = default;
};

class FileSystemProviderCloseFileEvent final
  : public FileSystemProviderEventWrap<
    CloseFileRequestedOptions,
    VirtualFileSystemIPCRequestedOptions::TVirtualFileSystemCloseFileRequestedOptions>
{
public:
  FileSystemProviderCloseFileEvent(EventTarget* aOwner,
                                   nsVirtualFileSystemRequestManager* aManager);

  virtual JSObject* WrapObjectInternal(JSContext* aCx,
                                       JS::Handle<JSObject*> aGivenProto) override;

  void SuccessCallback();

private:
  ~FileSystemProviderCloseFileEvent() = default;

};

} // namespace dom
} // namespace mozilla

#endif // mozilla_dom_FileSystemProviderCloseFileEvent_h