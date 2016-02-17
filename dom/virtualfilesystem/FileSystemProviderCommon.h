/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_FileSystemProviderCommon_h
#define mozilla_dom_FileSystemProviderCommon_h

#include "ipc/IPCMessageUtils.h"
#include "mozilla/dom/FileSystemProviderBinding.h"
#include "mozilla/dom/FileSystemProviderGetMetadataEventBinding.h"
#include "mozilla/Function.h"
#include "nsIVirtualFileSystemCallback.h"


namespace mozilla {
namespace dom {

namespace virtualfilesystem {

class VirtualFileSystemIPCRequestedOptions;

class BaseVirtualFileSystemRequestManager
{
public:
  NS_INLINE_DECL_REFCOUNTING(BaseVirtualFileSystemRequestManager)

  BaseVirtualFileSystemRequestManager() = default;

  virtual nsresult CreateRequest(const nsAString& aFileSystemId,
                                 const VirtualFileSystemIPCRequestedOptions& aOptions,
                                 nsIVirtualFileSystemCallback* aCallback,
                                 uint32_t* aRequestId) = 0;
  virtual nsresult FufillRequest(uint32_t aRequestId,
                                 nsIVirtualFileSystemRequestValue* aValue,
                                 bool aHasMore) = 0;
  virtual nsresult RejectRequest(uint32_t aRequestId, uint32_t aErrorCode) = 0;

protected:
  virtual ~BaseVirtualFileSystemRequestManager() = default;

};

} // namespace virtualfilesystem

template <class T>
class FileSystemProviderProxy
{
public:
  explicit FileSystemProviderProxy(T* aProvider)
    : mFileSystemProvider(aProvider) {}
  void Forget() { mFileSystemProvider = nullptr; }

protected:
  virtual ~FileSystemProviderProxy() = default;

  T* MOZ_NON_OWNING_REF mFileSystemProvider;
};

class BaseFileSystemProviderEventDispatcher : public nsISupports
{
public:
  NS_DECL_ISUPPORTS

  BaseFileSystemProviderEventDispatcher() = default;

  virtual nsresult DispatchFileSystemProviderEvent(
    uint32_t aRequestId,
    const nsAString& aFileSystemId,
    const virtualfilesystem::VirtualFileSystemIPCRequestedOptions& aOptions,
    virtualfilesystem::BaseVirtualFileSystemRequestManager* aRequestManager) = 0;

protected:
  virtual ~BaseFileSystemProviderEventDispatcher() = default;

};

class BaseMountUnmountResultCallback : public nsIVirtualFileSystemCallback
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS

  BaseMountUnmountResultCallback() = default;

protected:
  virtual ~BaseMountUnmountResultCallback() = default;
};

template <class T>
class MountUnmountResultCallback final : public BaseMountUnmountResultCallback
                                       , public FileSystemProviderProxy<T>
{
public:
  using FileSystemProviderProxy<T>::mFileSystemProvider;
  using FileSystemProviderProxy<T>::Forget;
  NS_DECL_NSIVIRTUALFILESYSTEMCALLBACK

  explicit MountUnmountResultCallback(T* aProvider)
    : FileSystemProviderProxy<T>(aProvider) {}

private:
  virtual ~MountUnmountResultCallback() = default;
};

class VirtualFileSystemCallbackWrapper final : public nsIVirtualFileSystemCallback
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIVIRTUALFILESYSTEMCALLBACK

  explicit VirtualFileSystemCallbackWrapper(nsIVirtualFileSystemCallback* aCallback,
                                            mozilla::Function<void(uint32_t)> aOnSuccess,
                                            mozilla::Function<void(uint32_t)> aOnError);

private:
  ~VirtualFileSystemCallbackWrapper() = default;

  nsCOMPtr<nsIVirtualFileSystemCallback> mSavedCallback;
  mozilla::Function<void(uint32_t)> mOnSuccess;
  mozilla::Function<void(uint32_t)> mOnError;
};

bool operator==(const EntryMetadata &a, const EntryMetadata &b)
{
  nsString mimeTypeA;
  nsString mimeTypeB;
  if (a.mMimeType.WasPassed()) {
    mimeTypeA = a.mMimeType.Value();
  }
  if (b.mMimeType.WasPassed()) {
    mimeTypeB = b.mMimeType.Value();
  }
  return (a.mIsDirectory == b.mIsDirectory) &&
         (a.mModificationTime == b.mModificationTime) &&
         (a.mName == b.mName) &&
         (a.mSize == b.mSize) &&
         (mimeTypeA == mimeTypeB);
}

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
    bool writable = false;
    uint32_t openedFilesLimit = 0;
    if (!ReadParam(aMsg, aIter, &(aResult->mDisplayName)) ||
        !ReadParam(aMsg, aIter, &(aResult->mFileSystemId)) ||
        !ReadParam(aMsg, aIter, &writable) ||
        !ReadParam(aMsg, aIter, &openedFilesLimit)) {
      return false;
    }
    aResult->mWritable.Construct(mozilla::dom::Nullable<bool>(writable));
    aResult->mOpenedFilesLimit.Construct(
      mozilla::dom::Nullable<uint32_t>(openedFilesLimit));
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

template<>
struct ParamTraits<mozilla::dom::EntryMetadata>
{
  typedef mozilla::dom::EntryMetadata paramType;

  static void Write(Message* aMsg, const paramType& aParam)
  {
    WriteParam(aMsg, aParam.mIsDirectory);
    WriteParam(aMsg, aParam.mModificationTime);
    WriteParam(aMsg, aParam.mName);
    WriteParam(aMsg, aParam.mSize);
    if (aParam.mMimeType.WasPassed()) {
      WriteParam(aMsg, aParam.mMimeType.Value());
    }
    else {
      WriteParam(aMsg, EmptyString());
    }
  }

  static bool Read(const Message* aMsg, void** aIter, paramType* aResult)
  {
    aResult->mMimeType.Construct();
    if (!ReadParam(aMsg, aIter, &(aResult->mIsDirectory)) ||
        !ReadParam(aMsg, aIter, &(aResult->mModificationTime)) ||
        !ReadParam(aMsg, aIter, &(aResult->mName)) ||
        !ReadParam(aMsg, aIter, &(aResult->mSize)) ||
        !ReadParam(aMsg, aIter, &(aResult->mMimeType.Value()))) {
      return false;
    }
    return true;
  }
};

} // namespace IPC

#endif // mozilla_dom_FileSystemProviderCommon_h