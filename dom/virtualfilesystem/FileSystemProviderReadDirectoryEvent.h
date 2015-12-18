/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_FileSystemProviderReadDirectoryEvent_h
#define mozilla_dom_FileSystemProviderReadDirectoryEvent_h

#include "mozilla/Attributes.h"
#include "mozilla/ErrorResult.h"
#include "mozilla/dom/FileSystemProviderEvent.h"
#include "mozilla/dom/FileSystemProviderReadDirectoryEventBinding.h"
#include "nsWrapperCache.h"

namespace mozilla {
namespace dom {

struct EntryMetadata;

class ReadDirectoryRequestedOptions final : public FileSystemProviderRequestedOptions
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_INHERITED(ReadDirectoryRequestedOptions,
                                                         FileSystemProviderRequestedOptions)

  explicit ReadDirectoryRequestedOptions(nsISupports* aParent,
                                         uint32_t aRequestId,
                                         const nsAString& aFileSystemId,
                                         const VirtualFileSystemIPCRequestedOptions& aOptions);

  virtual JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  void GetDirectoryPath(nsAString& aPath) const;

private:
  virtual ~ReadDirectoryRequestedOptions() = default;
};

class FileSystemProviderReadDirectoryEvent final
  : public FileSystemProviderEventWrap<
    ReadDirectoryRequestedOptions,
    VirtualFileSystemIPCRequestedOptions::TVirtualFileSystemReadDirectoryRequestedOptions>
{
public:
  FileSystemProviderReadDirectoryEvent(EventTarget* aOwner,
                                       nsVirtualFileSystemRequestManager* aManager);

  virtual JSObject* WrapObjectInternal(JSContext* aCx,
                                       JS::Handle<JSObject*> aGivenProto) override;

  void SuccessCallback(const Sequence<EntryMetadata>& aEntries, bool aHasMore);

private:
  virtual ~FileSystemProviderReadDirectoryEvent() = default;

};

} // namespace dom
} // namespace mozilla

#endif // mozilla_dom_FileSystemProviderReadDirectoryEvent_h