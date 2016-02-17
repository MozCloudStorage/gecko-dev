/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_FileSystemProvider_h
#define mozilla_dom_FileSystemProvider_h

#include <map>
#include "mozilla/dom/FileSystemProviderCommon.h"
#include "mozilla/DOMEventTargetHelper.h"

namespace mozilla {
namespace dom {

namespace virtualfilesystem {

class BaseVirtualFileSystemService;
class VirtualFileSystemIPCRequestedOptions;

} // namespace virtualfilesystem

class Promise;

using virtualfilesystem::BaseVirtualFileSystemService;

class FileSystemProviderEventDispatcher final
  : public FileSystemProviderProxy<FileSystemProvider>
  , public BaseFileSystemProviderEventDispatcher
{
public:
  explicit FileSystemProviderEventDispatcher(FileSystemProvider* aProvider)
    : FileSystemProviderProxy(aProvider) {}

  virtual nsresult DispatchFileSystemProviderEvent(
    uint32_t aRequestId,
    const nsAString& aFileSystemId,
    const virtualfilesystem::VirtualFileSystemIPCRequestedOptions& aOptions,
    virtualfilesystem::BaseVirtualFileSystemRequestManager* aRequestManager) override;

private:
  virtual ~FileSystemProviderEventDispatcher() = default;

};

typedef MountUnmountResultCallback<FileSystemProvider>
FileSystemProviderMountUnmountCallback;

class FileSystemProvider final : public DOMEventTargetHelper
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(FileSystemProvider, DOMEventTargetHelper)

  IMPL_EVENT_HANDLER(unmountrequested)
  IMPL_EVENT_HANDLER(getmetadatarequested)
  IMPL_EVENT_HANDLER(readdirectoryrequested)
  IMPL_EVENT_HANDLER(openfilerequested)
  IMPL_EVENT_HANDLER(closefilerequested)
  IMPL_EVENT_HANDLER(readfilerequested)
  IMPL_EVENT_HANDLER(abortrequested)

  static already_AddRefed<FileSystemProvider> Create(nsPIDOMWindow* aWindow);

  virtual JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  already_AddRefed<Promise> Mount(const MountOptions& aOptions, ErrorResult& aRv);

  already_AddRefed<Promise> Unmount(const UnmountOptions& aOptions, ErrorResult& aRv);

  void Get(const nsAString& aFileSystemId, Nullable<FileSystemInfo>& aInfo, ErrorResult& aRv);

  void GetAll(Nullable<nsTArray<FileSystemInfo>>& aRetVal, ErrorResult& aRv);

private:
  friend class FileSystemProviderEventDispatcher;
  friend FileSystemProviderMountUnmountCallback;

  explicit FileSystemProvider(nsPIDOMWindow* aWindow);
  ~FileSystemProvider();
  bool Init();
  nsresult DispatchFileSystemProviderEventInternal(
    uint32_t aRequestId,
    const nsAString& aFileSystemId,
    const virtualfilesystem::VirtualFileSystemIPCRequestedOptions& aOptions,
    virtualfilesystem::BaseVirtualFileSystemRequestManager* aRequestManager);
  void NotifyMountUnmountResult(uint32_t aRequestId, bool aSucceeded);

  RefPtr<FileSystemProviderEventDispatcher> mEventDispatcher;
  RefPtr<BaseVirtualFileSystemService> mVirtualFileSystemService;
  std::map<uint32_t, RefPtr<Promise>> mPendingRequestPromises;
};

} // namespace dom
} // namespace mozilla

#endif // mozilla_dom_FileSystemProvider_h