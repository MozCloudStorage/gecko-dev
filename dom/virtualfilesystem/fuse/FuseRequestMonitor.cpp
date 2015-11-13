/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "FuseRequestMonitor.h"
#include "nsIVirtualFileSystem.h"
#include <fcntl.h>
#include <errno.h>
#include <sys/mount.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/uio.h>
#include <math.h>
#include <ctype.h>

#ifdef VIRTUAL_FILE_SYSTEM_LOG_TAG
#undef VIRTUAL_FILE_SYSTEM_LOG_TAG
#endif
#define VIRTUAL_FILE_SYSTEM_LOG_TAG "FuseRequestMonitor"
#include "VirtualFileSystemLog.h"

namespace mozilla {
namespace dom {
namespace virtualfilesystem {


FileSystemProviderRequestRunnable::FileSystemProviderRequestRunnable(FuseHandler* aHandler,
                                                                     nsIVirtualFileSystem* aFileSystem)
  : mHandler(aHandler),
    mFileSystem(aFileSystem)
{}

nsresult
FileSystemProviderRequestRunnable::Run()
{
  MOZ_ASSERT(NS_IsMainThread() && mHandler && mFileSystem);
  uint32_t requestId;
  switch (mType) {
  case FUSE_LOOKUP:
  case FUSE_GETATTR: {
    mFileSystem->GetMetadata(mPath, &requestId); break;
  }
  case FUSE_OPEN: {
    mFileSystem->OpenFile(mPath, mOpenMode, &requestId); break;
  }
  case FUSE_RELEASE: {
    mFileSystem->CloseFile(mOpenFileId, &requestId); break;
  }
  case FUSE_READDIR: {
    mFileSystem->ReadDirectory(mPath, &requestId); break;
  }
  case FUSE_READ: {
    mFileSystem->ReadFile(mOpenFileId, mOffset, mLength, &requestId); break;
  }
  default: {
    return NS_ERROR_NOT_IMPLEMENTED;
  }
  }
  mHandler->SetOperationByRequestId(requestId, FUSE_LOOKUP);
  return NS_OK;
}


// FuseRequestMonitor

FuseRequestMonitor::FuseRequestMonitor(FuseHandler* aFuseHandler)
  : mHandler(aFuseHandler)
{
}

void
FuseRequestMonitor::Monitor(nsIVirtualFileSystem* aVirtualFileSystem)
{
  MOZ_ASSERT(mHandler);
  RefPtr<FuseMonitorRunnable> runnable =
                              new FuseMonitorRunnable(mHandler, aVirtualFileSystem);

  nsresult rv = mHandler->DispatchRunnable(runnable);
  if (NS_FAILED(rv)) {
    ERR("Dispatching request monitor job on FUSE device failed.");
  }
}

void
FuseRequestMonitor::Stop()
{
  MOZ_ASSERT(mHandler);
  RefPtr<FuseStopRunnable> runnable = new FuseStopRunnable(mHandler);
  nsresult rv = mHandler->DispatchRunnable(runnable);
  if (NS_FAILED(rv)) {
    ERR("Dispatching stop monitor job on FUSE device failed.");
  }
}

// FuseMonitorRunnable

FuseRequestMonitor::FuseMonitorRunnable::FuseMonitorRunnable(
                                         FuseHandler* aFuseHandler,
                                         nsIVirtualFileSystem* aVirtualFileSystem)
  : mHandler(aFuseHandler),
    mVirtualFileSystem(aVirtualFileSystem)
{
}

nsresult
FuseRequestMonitor::FuseMonitorRunnable::Run()
{

  MOZ_ASSERT(!NS_IsMainThread());

  MozFuse& fuse = mHandler->GetFuse();

  if (fuse.fuseFd == -1) {
    ERR("FUSE device file descriptor should not be -1");
    return NS_ERROR_FAILURE;
  }

  LOG("monitor fuse device. fusefd=%d, stopfd=%d", fuse.fuseFd, fuse.stopFds[0]);

  while (true) {
    if (fuse.waitForResponse) {
      NS_ProcessNextEvent();
      continue;
    }
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(fuse.fuseFd, &fds);
    FD_SET(fuse.stopFds[0], &fds);
    struct timespec timeout;
    timeout.tv_sec = 10;
    timeout.tv_nsec = 0;

    int numFd = fuse.fuseFd;
    if (numFd < fuse.stopFds[0]) {
      numFd = fuse.stopFds[0];
    }

    int res = pselect(numFd+1, &fds, NULL, NULL, &timeout, NULL);
    if (res == -1 && errno != EINTR) {
      ERR("pselect error: %s", strerror(errno));
      continue;
    } else if (res == 0) { //timeout
      continue;
    } else if (FD_ISSET(fuse.fuseFd, &fds)) {
      // Handle request from FUSE device
      if (!HandleRequest()) {
        continue;
      }
    } else if (FD_ISSET(fuse.stopFds[0], &fds)) {
      LOG("the monitor job for fuse device is going to finish.");
      break;
    } else {
      ERR("should not be here.");
    }
  }
  return NS_OK;
}


bool
FuseRequestMonitor::FuseMonitorRunnable::HandleRequest()
{
  MOZ_ASSERT(!NS_IsMainThread());
  // Read one request from FUSE device
  MozFuse& fuse = mHandler->GetFuse();

  ssize_t len = read(fuse.fuseFd, fuse.requestBuffer,
                                    sizeof(fuse.requestBuffer));
  if (len < 0) {
    if (errno != EINTR) {
      ERR("handle_fuse_requests: [%d] %s", errno, strerror(errno));
    }
    return false;
  }
  if ((size_t)len < sizeof(struct fuse_in_header)) {
    ERR("request too short: len=%zu", (size_t)len);
    return false;
  }

  const struct fuse_in_header *hdr =
        (const struct fuse_in_header*)((void*)fuse.requestBuffer);
  if (hdr->len != (size_t)len) {
    ERR("malformed header: len=%zu, hdr->len=%u", (size_t)len, hdr->len);
    return false;
  }

  switch (hdr->opcode) {
    case FUSE_LOOKUP:     { HandleLookup(); break; }
    case FUSE_GETATTR:    { HandleGetAttr(); break; }
    case FUSE_OPEN:       { HandleOpen(); break; }
    case FUSE_READ:       { HandleRead(); break; }
    case FUSE_OPENDIR:    { HandleOpenDir(); break; }
    case FUSE_READDIR:    { HandleReadDir(); break; }
    case FUSE_RELEASEDIR: { HandleReleaseDir(); break; }
    case FUSE_RELEASE:    { HandleRelease(); break; }
    case FUSE_INIT:       { HandleInit(); break; }
    case FUSE_FORGET:     { LOG("FORGET operation"); break; }
    case FUSE_SETATTR:    { LOG("SETATTR opertion"); break; }
    case FUSE_MKNOD:      { LOG("MKNOD operation"); break; }
    case FUSE_MKDIR:      { LOG("MKDIR operation"); break; }
    case FUSE_UNLINK:     { LOG("UNLINK operation"); break; }
    case FUSE_RMDIR:      { LOG("RMDIR operation"); break; }
    case FUSE_RENAME:     { LOG("RENAME operation"); break; }
    case FUSE_WRITE:      { LOG("WRITE operation"); break; }
    case FUSE_STATFS:     { LOG("STATFS operation"); break; }
    case FUSE_FSYNC:      { LOG("FSYNC operation"); break; }
    case FUSE_FLUSH:      { LOG("FLUSH operation"); break; }
    case FUSE_FSYNCDIR:   { LOG("FSYNCDIR operation"); break; }
    default: {
      LOG("[%d] NOTIMPL op=%d uniq=%llx nid=%llx",
           fuse.token, hdr->opcode, hdr->unique, hdr->nodeid);
      ResponseError(-ENOSYS);
      break;
    }
  }
  return true;
}

void
FuseRequestMonitor::FuseMonitorRunnable::Response(void* aData, size_t aSize)
{
  MOZ_ASSERT(!NS_IsMainThread());
  MozFuse& fuse = mHandler->GetFuse();
  const struct fuse_in_header* hdr =
        (const struct fuse_in_header*)((void*)fuse.requestBuffer);

  struct fuse_out_header outhdr;
  struct iovec vec[2];
  outhdr.len = aSize + sizeof(outhdr);
  outhdr.error = 0;
  outhdr.unique = hdr->unique;
  vec[0].iov_base = &outhdr;
  vec[0].iov_len = sizeof(outhdr);
  vec[1].iov_base = aData;
  vec[1].iov_len = aSize;
  int res = writev(fuse.fuseFd, vec, 2);
  if (res < 0) {
    ERR("Response to FUSE device failed. [%d]\n", errno);
  }
}

void
FuseRequestMonitor::FuseMonitorRunnable::ResponseError(int32_t aError)
{
  MOZ_ASSERT(!NS_IsMainThread());
  MozFuse& fuse = mHandler->GetFuse();
  const struct fuse_in_header *hdr =
        (const struct fuse_in_header*)((void*)fuse.requestBuffer);
  struct fuse_out_header outhdr;
  outhdr.len = sizeof(outhdr);
  outhdr.error = aError;
  outhdr.unique = hdr->unique;
  int res = write(fuse.fuseFd, &outhdr, outhdr.len);
  if (res < 0) {
    ERR("reply error to FUSE device failed. [%d]\n", errno);
  }
}

void
FuseRequestMonitor::FuseMonitorRunnable::HandleInit()
{
  MOZ_ASSERT(!NS_IsMainThread());
  MozFuse& fuse = mHandler->GetFuse();

  const struct fuse_init_in *req = (const struct fuse_init_in*)
               (fuse.requestBuffer + sizeof(struct fuse_in_header));

  LOG("INIT ver=%d.%d maxread=%d flags=%x",
       req->major, req->minor, req->max_readahead, req->flags);

  // FUSE_KERNEL_VERSION and FUSE_KERNEL_MINOR_VERSION is from
  // system/core/sdcard/fuse.h, which is used in AOSP's emulated sdcard
  struct fuse_init_out out;
  out.major = FUSE_KERNEL_VERSION;
  out.minor = FUSE_KERNEL_MINOR_VERSION;
  out.max_readahead = req->max_readahead;
  out.flags = FUSE_ATOMIC_O_TRUNC | FUSE_BIG_WRITES;
  out.max_background = 32;
  out.congestion_threshold = 32;
  out.max_write = VIRTUAL_FILE_SYSTEM_MAX_WRITE;

  Response(((void*)&out), sizeof(out));
}

void
FuseRequestMonitor::FuseMonitorRunnable::HandleLookup()
{
  MOZ_ASSERT(!NS_IsMainThread());
  MozFuse& fuse = mHandler->GetFuse();
  const struct fuse_in_header* hdr =
        (const struct fuse_in_header*)((void*)fuse.requestBuffer);

  const char* name = (const char*)(fuse.requestBuffer+sizeof(fuse_in_header));
  nsString path = mHandler->GetPathByNodeId(hdr->nodeid);

  if (path.IsEmpty() || path.Equals(NS_LITERAL_STRING(""))) {
    LOG("Getting path by node id [%llu] failed.", hdr->nodeid);
    ResponseError(-ENOENT);
    return;
  }

  nsString childpath = path;
  if (!childpath.Equals(NS_LITERAL_STRING("/"))) {
    childpath.AppendLiteral("/");
  }
  childpath.AppendASCII(name);
  mHandler->GetNodeIdByPath(childpath);

  LOG("LOOKUP path=%s", NS_ConvertUTF16toUTF8(childpath).get());

  RefPtr<FileSystemProviderRequestRunnable> runnable =
      new FileSystemProviderRequestRunnable(mHandler, mVirtualFileSystem);
  runnable->SetType(FUSE_LOOKUP);
  runnable->SetPath(childpath);
  if(NS_FAILED(NS_DispatchToMainThread(runnable))) {
    LOG("Dispatching GetMetadata request to main thread failed");
    ResponseError(-ENOSYS);
    return;
  }
  fuse.waitForResponse = true;
}

void
FuseRequestMonitor::FuseMonitorRunnable::HandleGetAttr()
{
  MOZ_ASSERT(!NS_IsMainThread());
  MozFuse& fuse = mHandler->GetFuse();
  const struct fuse_in_header* hdr =
        (const struct fuse_in_header*)((void*)fuse.requestBuffer);

  nsString path = mHandler->GetPathByNodeId(hdr->nodeid);

  if (path.IsEmpty() || path.Equals(NS_LITERAL_STRING(""))) {
    LOG("Getting path by node id [%llu] failed.", hdr->nodeid);
    ResponseError(-ENOENT);
    return;
  }

  LOG("GETATTR path=%s", NS_ConvertUTF16toUTF8(path).get());

  RefPtr<FileSystemProviderRequestRunnable> runnable =
      new FileSystemProviderRequestRunnable(mHandler, mVirtualFileSystem);
  runnable->SetType(FUSE_GETATTR);
  runnable->SetPath(path);
  if(NS_FAILED(NS_DispatchToMainThread(runnable))) {
    LOG("Dispatching GetMetadata request to main thread failed");
    ResponseError(-ENOSYS);
    return;
  }
  fuse.waitForResponse = true;
}

void
FuseRequestMonitor::FuseMonitorRunnable::HandleOpen()
{
  MOZ_ASSERT(!NS_IsMainThread());
  MozFuse& fuse = mHandler->GetFuse();
  const struct fuse_in_header* hdr =
        (const struct fuse_in_header*)((void*)fuse.requestBuffer);

  const struct fuse_open_in* req =
        (const struct fuse_open_in*)(fuse.requestBuffer+sizeof(fuse_in_header));

  nsString path = mHandler->GetPathByNodeId(hdr->nodeid);

  if (path.IsEmpty() || path.Equals(NS_LITERAL_STRING(""))) {
    LOG("Getting path by node id [%llu] failed.", hdr->nodeid);
    ResponseError(-ENOENT);
    return;
  }

  LOG("OPEN path=%s, flags=%x", NS_ConvertUTF16toUTF8(path).get(), req->flags);

  RefPtr<FileSystemProviderRequestRunnable> runnable =
      new FileSystemProviderRequestRunnable(mHandler, mVirtualFileSystem);
  runnable->SetType(FUSE_OPEN);
  runnable->SetPath(path);
  runnable->SetOpenMode(req->flags);
  if(NS_FAILED(NS_DispatchToMainThread(runnable))) {
    LOG("Dispatching OpenFile request to main thread failed");
    ResponseError(-ENOSYS);
    return;
  }

  fuse.waitForResponse = true;
}

void
FuseRequestMonitor::FuseMonitorRunnable::HandleRead()
{
  MOZ_ASSERT(!NS_IsMainThread());
  MozFuse& fuse = mHandler->GetFuse();

  const struct fuse_read_in* req =
        (const struct fuse_read_in*)(fuse.requestBuffer+sizeof(fuse_in_header));

  LOG("READ fh=%llu, offset=%llu, size=%u", req->fh, req->offset, req->size);

  RefPtr<FileSystemProviderRequestRunnable> runnable =
      new FileSystemProviderRequestRunnable(mHandler, mVirtualFileSystem);
  runnable->SetType(FUSE_READ);
  runnable->SetOpenFileId(req->fh);
  runnable->SetOffset(req->offset);
  runnable->SetLength(req->size);
  if(NS_FAILED(NS_DispatchToMainThread(runnable))) {
    LOG("Dispatching ReadFile request to main thread failed");
    ResponseError(-ENOSYS);
    return;
  }

  fuse.waitForResponse = true;
}

void
FuseRequestMonitor::FuseMonitorRunnable::HandleRelease()
{
  MOZ_ASSERT(!NS_IsMainThread());
  MozFuse& fuse = mHandler->GetFuse();

  const struct fuse_release_in* req =
     (const struct fuse_release_in*)(fuse.requestBuffer+sizeof(fuse_in_header));

  LOG("RELEASE fh=%llu", req->fh);

  RefPtr<FileSystemProviderRequestRunnable> runnable =
      new FileSystemProviderRequestRunnable(mHandler, mVirtualFileSystem);
  runnable->SetType(FUSE_RELEASE);
  runnable->SetOpenFileId(req->fh);
  if(NS_FAILED(NS_DispatchToMainThread(runnable))) {
    LOG("Dispatching CloseFile request to main thread failed");
    ResponseError(-ENOSYS);
    return;
  }

  fuse.waitForResponse = true;
}

void
FuseRequestMonitor::FuseMonitorRunnable::HandleReleaseDir()
{
}

void
FuseRequestMonitor::FuseMonitorRunnable::HandleOpenDir()
{
  MOZ_ASSERT(!NS_IsMainThread());
  MozFuse& fuse = mHandler->GetFuse();
  const struct fuse_in_header* hdr =
        (const struct fuse_in_header*)((void*)fuse.requestBuffer);

  nsString path = mHandler->GetPathByNodeId(hdr->nodeid);

  if (path.IsEmpty() || path.Equals(NS_LITERAL_STRING(""))) {
    LOG("Getting path by node id [%llu] failed.", hdr->nodeid);
    ResponseError(-ENOENT);
    return;
  }

  LOG("OPENDIR path=%s", NS_ConvertUTF16toUTF8(path).get());

  struct fuse_open_out out;
  out.fh = (uint64_t)time(NULL);
  out.open_flags = 0;
  out.padding = 0;
  Response(((void*)&out), sizeof(out));
}

void
FuseRequestMonitor::FuseMonitorRunnable::HandleReadDir()
{
  MOZ_ASSERT(!NS_IsMainThread());
  MOZ_ASSERT(mHandler);
  MozFuse& fuse = mHandler->GetFuse();
  const struct fuse_in_header* hdr =
        (const struct fuse_in_header*)((void*)fuse.requestBuffer);

  nsString path = mHandler->GetPathByNodeId(hdr->nodeid);

  if (path.IsEmpty() || path.Equals(NS_LITERAL_STRING(""))) {
    LOG("Getting path by node id [%llu] failed.", hdr->nodeid);
    ResponseError(-ENOENT);
    return;
  }

  LOG("READDIR path=%s", NS_ConvertUTF16toUTF8(path).get());

  RefPtr<FileSystemProviderRequestRunnable> runnable =
      new FileSystemProviderRequestRunnable(mHandler, mVirtualFileSystem);
  runnable->SetType(FUSE_READDIR);
  runnable->SetPath(path);
  if(NS_FAILED(NS_DispatchToMainThread(runnable))) {
    LOG("Dispatching ReadDirectory request to main thread failed");
    ResponseError(-ENOSYS);
    return;
  }

  fuse.waitForResponse = true;
}

// FuseStopRunnable

FuseRequestMonitor::FuseStopRunnable::FuseStopRunnable(FuseHandler* aFuseHandler)
  : mHandler(aFuseHandler)
{
}

nsresult
FuseRequestMonitor::FuseStopRunnable::Run()
{
  MOZ_ASSERT(!NS_IsMainThread());
  MozFuse& fuse = mHandler->GetFuse();
  char message[16] = "monitor byebye!";
  int res = write(fuse.stopFds[1], message, sizeof(message));
  if (res < 0) {
    ERR("Send stop monitor message failed.");
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}

} //end namespace virtualfilesystem
} //end namespace dom
} //end namespace mozilla
