/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_FileSystemProvider_h
#define mozilla_dom_FileSystemProvider_h

#include <map>
#include "ipc/IPCMessageUtils.h"
#include "mozilla/dom/FileSystemProviderBinding.h"
#include "mozilla/DOMEventTargetHelper.h"
#include "nsIVirtualFileSystemCallback.h"

namespace mozilla {
namespace dom {

namespace virtualfilesystem {

class BaseVirtualFileSystemService;
class nsVirtualFileSystem;
class nsVirtualFileSystemRequestManager;
class VirtualFileSystemIPCRequestedOptions;

} // namespace virtualfilesystem

struct MountOptions;
struct UnmountOptions;
class Promise;
class MountUnmountResultCallback;

class FileSystemProviderProxy : public nsISupports
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS

  explicit FileSystemProviderProxy(FileSystemProvider* aProvider)
    : mFileSystemProvider(aProvider) {}
  void Forget() { mFileSystemProvider = nullptr; }

protected:
  virtual ~FileSystemProviderProxy() {}

  FileSystemProvider* MOZ_NON_OWNING_REF mFileSystemProvider;
};

class FileSystemProviderEventDispatcher final
  : public FileSystemProviderProxy
{
public:
  NS_DECL_ISUPPORTS_INHERITED

  explicit FileSystemProviderEventDispatcher(FileSystemProvider* aProvider)
    : FileSystemProviderProxy(aProvider) {}

  nsresult DispatchFileSystemProviderEvent(
    uint32_t aRequestId,
    const nsAString& aFileSystemId,
    const virtualfilesystem::VirtualFileSystemIPCRequestedOptions& aOptions);

private:
  virtual ~FileSystemProviderEventDispatcher() {}

};

using virtualfilesystem::BaseVirtualFileSystemService;
using virtualfilesystem::nsVirtualFileSystemRequestManager;
using virtualfilesystem::nsVirtualFileSystem;

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
  friend class MountUnmountResultCallback;

  explicit FileSystemProvider(nsPIDOMWindow* aWindow);
  ~FileSystemProvider();
  bool Init();
  nsresult DispatchFileSystemProviderEventInternal(
    uint32_t aRequestId,
    const nsAString& aFileSystemId,
    const virtualfilesystem::VirtualFileSystemIPCRequestedOptions& aOptions);
  void NotifyMountUnmountResult(uint32_t aRequestId, bool aSucceeded);

  RefPtr<FileSystemProviderEventDispatcher> mEventDispatcher;
  RefPtr<BaseVirtualFileSystemService> mVirtualFileSystemService;
  RefPtr<nsVirtualFileSystemRequestManager> mRequestManager;
  std::map<uint32_t, RefPtr<Promise>> mPendingRequestPromises;
};

} // namespace dom
} // namespace mozilla

namespace IPC {
template<>
struct ParamTraits<mozilla::dom::MountOptions>
{
  typedef mozilla::dom::MountOptions paramType;

  static void Write(Message* aMsg, const paramType& aParam)
  {
    WriteParam(aMsg, aParam.mDisplayName);
    WriteParam(aMsg, aParam.mFileSystemId);
    if (aParam.mWritable.WasPassed() && !aParam.mWritable.Value().IsNull()) {
      WriteParam(aMsg, aParam.mWritable.Value().Value());
    }
    else {
      WriteParam(aMsg, false);
    }
    if (aParam.mOpenedFilesLimit.WasPassed() &&
        !aParam.mOpenedFilesLimit.Value().IsNull()) {
      WriteParam(aMsg, aParam.mOpenedFilesLimit.Value().Value());
    }
    else {
      WriteParam(aMsg, 0);
    }
  }

  static bool Read(const Message* aMsg, void** aIter, paramType* aResult)
  {
    aResult->mWritable.Construct();
    aResult->mOpenedFilesLimit.Construct();
    bool writable = false;
    uint32_t openedFilesLimit = 0;
    if (!ReadParam(aMsg, aIter, &(aResult->mDisplayName)) ||
        !ReadParam(aMsg, aIter, &(aResult->mFileSystemId)) ||
        !ReadParam(aMsg, aIter, &writable) ||
        !ReadParam(aMsg, aIter, &openedFilesLimit)) {
      return false;
    }
    return true;
  }
};

template<>
struct ParamTraits<mozilla::dom::UnmountOptions>
{
  typedef mozilla::dom::UnmountOptions paramType;

  static void Write(Message* aMsg, const paramType& aParam)
  {
    WriteParam(aMsg, aParam.mFileSystemId);
  }

  static bool Read(const Message* aMsg, void** aIter, paramType* aResult)
  {
    if (!ReadParam(aMsg, aIter, &(aResult->mFileSystemId))) {
      return false;
    }
    return true;
  }
};

} // namespace IPC

#endif // mozilla_dom_FileSystemProvider_h