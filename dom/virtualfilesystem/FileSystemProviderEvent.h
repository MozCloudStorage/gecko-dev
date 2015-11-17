/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_FileSystemProviderEvent_h
#define mozilla_dom_FileSystemProviderEvent_h

#include "mozilla/Attributes.h"
#include "mozilla/ErrorResult.h"
#include "mozilla/dom/BindingUtils.h"
#include "mozilla/dom/Event.h"
#include "mozilla/dom/FileSystemProviderEventBinding.h"
#include "nsIVirtualFileSystemDataType.h"

class nsIVirtualFileSystemRequestManager;
class nsIVirtualFileSystemRequestValue;

namespace mozilla {
namespace dom {

class FileSystemProviderRequestedOptions : public nsISupports
                                         , public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(FileSystemProviderRequestedOptions)

  explicit FileSystemProviderRequestedOptions(nsISupports* aParent,
                                              nsIVirtualFileSystemRequestedOptions* aOptions);

  nsISupports* GetParentObject() const
  {
    return mParent;
  }

  virtual JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  uint32_t RequestId() const;
  void GetFileSystemId(nsAString& aFileSystemId) const;

protected:
  virtual ~FileSystemProviderRequestedOptions() = default;

  nsCOMPtr<nsISupports> mParent;
  nsString mFileSystemId;
  uint32_t mRequestId;
};

class FileSystemProviderEvent : public Event
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_INHERITED(FileSystemProviderEvent,
                                                         Event)

  explicit FileSystemProviderEvent(EventTarget* aOwner,
                                   nsIVirtualFileSystemRequestManager* aManager,
                                   const nsAString& aEventName);

  virtual JSObject* WrapObjectInternal(JSContext* aCx,
                                       JS::Handle<JSObject*> aGivenProto) override;

  virtual nsresult InitFileSystemProviderEvent(
    nsIVirtualFileSystemRequestedOptions* aOptions) = 0;

  virtual void OnSuccess(nsIVirtualFileSystemRequestValue* aValue, bool aHasMore);

  void ErrorCallback(const FileSystemProviderError& aError);

protected:
  virtual ~FileSystemProviderEvent() = default;
  void InitFileSystemProviderEventInternal(const nsAString& aType,
                                           FileSystemProviderRequestedOptions* aOptions);

  nsCOMPtr<nsIVirtualFileSystemRequestManager> mRequestManager;
  RefPtr<FileSystemProviderRequestedOptions> mOptions;
  const nsString mEventName;
};

template <class WebIDLOptions, class XPIDLOptions>
class FileSystemProviderEventWrap : public FileSystemProviderEvent
{
public:
  explicit FileSystemProviderEventWrap(EventTarget* aOwner,
                                       nsIVirtualFileSystemRequestManager* aManager,
                                       const nsAString& aEventName)
    : FileSystemProviderEvent(aOwner, aManager, aEventName)
  {}

  WebIDLOptions* Options() const
  {
    MOZ_ASSERT(mOptions);

    return static_cast<WebIDLOptions*>(mOptions.get());
  }

  nsresult InitFileSystemProviderEvent(
    nsIVirtualFileSystemRequestedOptions* aOptions) override
  {
    nsCOMPtr<XPIDLOptions> opts = do_QueryInterface(aOptions);
    if (!opts) {
      MOZ_ASSERT(false, "Invalid nsIVirtualFileSystemRequestedOptions");
      return NS_ERROR_INVALID_ARG;
    }

    RefPtr<WebIDLOptions> options = new WebIDLOptions(mOwner, opts);
    mOptions = options;
    Event::InitEvent(mEventName, false, false);
    return NS_OK;
  }

protected:
  virtual ~FileSystemProviderEventWrap() = default;

};

} // namespace dom
} // namespace mozilla

#endif // mozilla_dom_FileSystemProviderEvent_h