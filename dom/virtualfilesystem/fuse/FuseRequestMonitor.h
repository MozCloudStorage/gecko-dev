/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_virtualfilesystem_fuserequestmonitor_h__
#define mozilla_dom_virtualfilesystem_fuserequestmonitor_h__

#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsThreadUtils.h"
#include "FuseHandler.h"
#include "nsIVirtualFileSystem.h"

namespace mozilla {
namespace dom {
namespace virtualfilesystem {

class FileSystemProviderRequestRunnable : public nsRunnable
{
public:
  NS_INLINE_DECL_REFCOUNTING(FuseMonitorRunnable)
  FileSystemProviderRequestRunnable(FuseHandler* aHandler,
                                    nsIVirtualFileSystem* aFileSystem);

  void SetType(const uint32_t aType) { mType = aType; }
  void SetPath(const nsAString& aPath) { mPath = aPath; }
  void SetOperationId(const uint32_t aOperationId) { mOperationId = aOperationId; }
  void SetOpenFileId(const uint32_t aOpenFileId) { mOpenFileId = aOpenFileId; }
  void SetOpenMode(const uint32_t aOpenMode) { mOpenMode = aOpenMode; }
  void SetOffset(const uint64_t aOffset) { mOffset = aOffset; }
  void SetLength(const uint64_t aLength) { mLength = aLength; }

  nsresult Run();

private:
  RefPtr<FuseHandler> mHandler;
  RefPtr<nsIVirtualFileSystem> mFileSystem;
  nsString mPath;
  uint32_t mType;
  uint32_t mOperationId;
  uint32_t mOpenFileId;
  uint32_t mOpenMode;
  uint64_t mOffset;
  uint64_t mLength;

};

class FuseRequestMonitor final
{
public:
  NS_INLINE_DECL_REFCOUNTING(FuseRequestMonitor)

  FuseRequestMonitor(FuseHandler* aFuseHanlder);

  void Monitor(nsIVirtualFileSystem* aVirtualFileSystem);
  void Stop();

private:
  ~FuseRequestMonitor() = default;

  class FuseMonitorRunnable final : public nsRunnable
  {
  public:
    NS_INLINE_DECL_REFCOUNTING(FuseMonitorRunnable)

    FuseMonitorRunnable(FuseHandler* aFuseHandler,
                        nsIVirtualFileSystem* aVirtualFileSystem);

    nsresult Run();

  private:
    ~FuseMonitorRunnable() = default;

    bool HandleRequest();

    void HandleInit();
    void HandleLookup();
    void HandleGetAttr();
    void HandleOpen();
    void HandleRead();
    void HandleRelease();
    void HandleOpenDir();
    void HandleReadDir();
    void HandleReleaseDir();
    void HandleFlush();
    void HandleFsync();

    void ResponseError(int32_t aError);
    void Response(void* aData, size_t aSize);

    RefPtr<FuseHandler> mHandler;
    RefPtr<nsIVirtualFileSystem> mVirtualFileSystem;
  };

  class FuseStopRunnable final : public nsRunnable
  {
  public:
    NS_INLINE_DECL_REFCOUNTING(FuseStopRunnable)

    FuseStopRunnable(FuseHandler* aFuseHandler);

    nsresult Run();
  private:
    ~FuseStopRunnable() = default;

    RefPtr<FuseHandler> mHandler;
  };

  RefPtr<FuseHandler> mHandler;
};

} // end namespace virtualfilesystem
} // end namespace dom
} // end namespace mozilla
#endif
