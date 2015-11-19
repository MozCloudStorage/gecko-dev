/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "nsVirtualFileSystem.h"
#include "nsVirtualFileSystemDataType.h"
#include "nsISupportsUtils.h"
#include "nsISupportsPrimitives.h"
#include "nsIMutableArray.h"
#include "nsArrayUtils.h"
#include "nsThreadUtils.h"
#include "nsServiceManagerUtils.h"
#include "mozilla/Services.h"
#include "nsVirtualFileSystemCallback.h"

//#include "FuseRequestHandler.h"
#ifdef VIRTUAL_FILE_SYSTEM_LOG_TAG
#undef VIRTUAL_FILE_SYSTEM_LOG_TAG
#endif
#define VIRTUAL_FILE_SYSTEM_LOG_TAG "VirtualFileSystem"
#include "VirtualFileSystemLog.h"


namespace mozilla {
namespace dom {
namespace virtualfilesystem {

NS_IMPL_ISUPPORTS(nsVirtualFileSystem, nsIVirtualFileSystem)

nsVirtualFileSystem::nsVirtualFileSystem()
  : mInfo(nullptr),
    mRequestManager(nullptr),
    mResponseHandler(nullptr)
{
}

const char*
nsVirtualFileSystem::FileSystemIdStr()
{
  nsString fileSystemId;
  mInfo->GetFileSystemId(fileSystemId);
  return NS_ConvertUTF16toUTF8(fileSystemId).get();
}

const char*
nsVirtualFileSystem::DisplayNameStr()
{
  nsString displayName;
  mInfo->GetDisplayName(displayName);
  return NS_ConvertUTF16toUTF8(displayName).get();
}

const bool
nsVirtualFileSystem::IsWritable()
{
  bool isWritable;
  mInfo->GetWritable(&isWritable);
  return isWritable;
}

const nsString
nsVirtualFileSystem::GetFileSystemId()
{
  nsString fileSystemId;
  mInfo->GetFileSystemId(fileSystemId);
  return fileSystemId;
}

void
nsVirtualFileSystem::SetInfo(nsIVirtualFileSystemInfo* aInfo)
{
  mInfo = aInfo;
}

void
nsVirtualFileSystem::SetResponseHandler(nsIVirtualFileSystemResponseHandler* aHandler)
{
  mResponseHandler = aHandler;
}

void
nsVirtualFileSystem::SetRequestManager(nsIVirtualFileSystemRequestManager* aManager)
{
  mRequestManager = aManager;
}

// nsIVirtualFileSystem interface implmentation
NS_IMETHODIMP
nsVirtualFileSystem::GetInfo(nsIVirtualFileSystemInfo** aInfo)
{
  RefPtr<nsIVirtualFileSystemInfo> info = mInfo;
  info.forget(aInfo);
  return NS_OK;
}

NS_IMETHODIMP
nsVirtualFileSystem::Abort(const uint32_t aOperationId, uint32_t* aRequestId)
{
  MOZ_ASSERT(NS_IsMainThread());
  nsCOMPtr<nsIVirtualFileSystemAbortRequestedOptions> option =
  do_CreateInstance(VIRTUAL_FILE_SYSTEM_ABORT_REQUESTED_OPTIONS_CONTRACT_ID);

  nsString fileSystemId;
  mInfo->GetFileSystemId(fileSystemId);
  option->SetFileSystemId(fileSystemId);
  option->SetOperationRequestId(aOperationId);

  RefPtr<nsIVirtualFileSystemCallback> callback = new nsVirtualFileSystemCallback(this);

  MOZ_ASSERT(mRequestManager);

  return mRequestManager->CreateRequest(
                                   nsIVirtualFileSystemRequestManager::REQUEST_ABORT,
                                   option,
                                   callback,
                                   aRequestId);
}

NS_IMETHODIMP
nsVirtualFileSystem::OpenFile(const nsAString& aPath,
                         const uint16_t aMode,
                         uint32_t* aRequestId)
{
  MOZ_ASSERT(NS_IsMainThread());
  nsCOMPtr<nsIVirtualFileSystemOpenFileRequestedOptions> option =
  do_CreateInstance(VIRTUAL_FILE_SYSTEM_OPENFILE_REQUESTED_OPTIONS_CONTRACT_ID);

  nsString fileSystemId;
  mInfo->GetFileSystemId(fileSystemId);
  option->SetFileSystemId(fileSystemId);
  option->SetFilePath(aPath);
  option->SetMode(aMode);


  RefPtr<nsIVirtualFileSystemOpenedFileInfo> info =
                                        new nsVirtualFileSystemOpenedFileInfo();

  info->SetFileSystemId(fileSystemId);
  info->SetFilePath(aPath);
  info->SetMode(aMode);

  RefPtr<nsIVirtualFileSystemCallback> callback =
         new nsVirtualFileSystemOpenFileCallback(this, info);

  MOZ_ASSERT(mRequestManager);

  return mRequestManager->CreateRequest(
                                nsIVirtualFileSystemRequestManager::REQUEST_OPENFILE,
                                option,
                                callback,
                                aRequestId);
}

NS_IMETHODIMP
nsVirtualFileSystem::CloseFile(const uint32_t aOpenFileId,
                          uint32_t* aRequestId)
{
  MOZ_ASSERT(NS_IsMainThread());
  nsCOMPtr<nsIVirtualFileSystemCloseFileRequestedOptions> option =
  do_CreateInstance(VIRTUAL_FILE_SYSTEM_CLOSEFILE_REQUESTED_OPTIONS_CONTRACT_ID);


  nsString fileSystemId;
  mInfo->GetFileSystemId(fileSystemId);
  option->SetFileSystemId(fileSystemId);
  option->SetOpenRequestId(aOpenFileId);

  RefPtr<nsIVirtualFileSystemCallback> callback =
         new nsVirtualFileSystemCloseFileCallback(this, aOpenFileId);

  MOZ_ASSERT(mRequestManager);

  return mRequestManager->CreateRequest(
                               nsIVirtualFileSystemRequestManager::REQUEST_CLOSEFILE,
                               option,
                               callback,
                               aRequestId);
}

NS_IMETHODIMP
nsVirtualFileSystem::GetMetadata(const nsAString& aEntryPath,
                            uint32_t* aRequestId)
{
  MOZ_ASSERT(NS_IsMainThread());
  nsCOMPtr<nsIVirtualFileSystemGetMetadataRequestedOptions> option =
  do_CreateInstance(VIRTUAL_FILE_SYSTEM_GETMETADATA_REQUESTED_OPTIONS_CONTRACT_ID);

  nsString fileSystemId;
  mInfo->GetFileSystemId(fileSystemId);
  option->SetFileSystemId(fileSystemId);

  option->SetEntryPath(aEntryPath);

  RefPtr<nsIVirtualFileSystemCallback> callback = new nsVirtualFileSystemCallback(this);

  //MOZ_ASSERT(mRequestManager);
  nsresult rv = mRequestManager->CreateRequest(
                             nsIVirtualFileSystemRequestManager::REQUEST_GETMETADATA,
                             option,
                             callback,
                             aRequestId);
  return rv;
}

NS_IMETHODIMP
nsVirtualFileSystem::ReadDirectory(const nsAString& aDirPath,
                              uint32_t* aRequestId)
{
  MOZ_ASSERT(NS_IsMainThread());
  nsCOMPtr<nsIVirtualFileSystemReadDirectoryRequestedOptions> option =
  do_CreateInstance(VIRTUAL_FILE_SYSTEM_READDIRECTORY_REQUESTED_OPTIONS_CONTRACT_ID);

  nsString fileSystemId;
  mInfo->GetFileSystemId(fileSystemId);
  option->SetFileSystemId(fileSystemId);
  option->SetDirPath(aDirPath);

  RefPtr<nsIVirtualFileSystemCallback> callback = new nsVirtualFileSystemCallback(this);

  MOZ_ASSERT(mRequestManager);

  return mRequestManager->CreateRequest(
                           nsIVirtualFileSystemRequestManager::REQUEST_READDIRECTORY,
                           option,
                           callback,
                           aRequestId);
}

NS_IMETHODIMP
nsVirtualFileSystem::ReadFile(const uint32_t aOpenFileId,
                         const uint64_t aOffset,
                         const uint64_t aLength,
                         uint32_t* aRequestId)
{
  MOZ_ASSERT(NS_IsMainThread());
  nsCOMPtr<nsIVirtualFileSystemReadFileRequestedOptions> option =
  do_CreateInstance(VIRTUAL_FILE_SYSTEM_READFILE_REQUESTED_OPTIONS_CONTRACT_ID);

  nsString fileSystemId;
  mInfo->GetFileSystemId(fileSystemId);
  option->SetFileSystemId(fileSystemId);
  option->SetOpenRequestId(aOpenFileId);
  option->SetOffset(aOffset);
  option->SetLength(aLength);

  RefPtr<nsIVirtualFileSystemCallback> callback = new nsVirtualFileSystemCallback(this);

  MOZ_ASSERT(mRequestManager);

  return mRequestManager->CreateRequest(
                                nsIVirtualFileSystemRequestManager::REQUEST_READFILE,
                                option,
                                callback,
                                aRequestId);
}

NS_IMETHODIMP
nsVirtualFileSystem::Unmount(uint32_t* aRequestId)
{
  MOZ_ASSERT(NS_IsMainThread());
  nsCOMPtr<nsIVirtualFileSystemUnmountRequestedOptions> option =
  do_CreateInstance(VIRTUAL_FILE_SYSTEM_UNMOUNT_REQUESTED_OPTIONS_CONTRACT_ID);

  nsString fileSystemId;
  mInfo->GetFileSystemId(fileSystemId);
  option->SetFileSystemId(fileSystemId);

  RefPtr<nsIVirtualFileSystemCallback> callback = new nsVirtualFileSystemCallback(this);

  MOZ_ASSERT(mRequestManager);

  return mRequestManager->CreateRequest(
                                nsIVirtualFileSystemRequestManager::REQUEST_UNMOUNT,
                                option,
                                callback,
                                aRequestId);
}

NS_IMETHODIMP
nsVirtualFileSystem::OnRequestSuccess(const uint32_t aRequestId,
                                 nsIVirtualFileSystemRequestValue* aValue)
{
  MOZ_ASSERT(mResponseHandler);
  mResponseHandler->OnSuccess(aRequestId, aValue);
  return NS_OK;
}

NS_IMETHODIMP
nsVirtualFileSystem::OnOpenFileSuccess(const uint32_t aRequestId,
                                  nsIVirtualFileSystemRequestValue* aValue,
                                  nsIVirtualFileSystemOpenedFileInfo* aFileInfo)
{
  MOZ_ASSERT(mInfo);
  aFileInfo->SetOpenRequestId(aRequestId);
  mInfo->AppendOpenedFile(aFileInfo);

  MOZ_ASSERT(mResponseHandler);
  mResponseHandler->OnSuccess(aRequestId, aValue);
  return NS_OK;
}

NS_IMETHODIMP
nsVirtualFileSystem::OnCloseFileSuccess(const uint32_t aRequestId,
                                   nsIVirtualFileSystemRequestValue* aValue,
                                   const uint32_t aOpenedFileId)
{
  MOZ_ASSERT(mInfo);
  mInfo->RemoveOpenedFile(aOpenedFileId);
  MOZ_ASSERT(mResponseHandler);
  mResponseHandler->OnSuccess(aRequestId, aValue);
  return NS_OK;
}

NS_IMETHODIMP
nsVirtualFileSystem::OnRequestError(const uint32_t aRequestId,
                               const uint32_t aError)
{
  MOZ_ASSERT(mResponseHandler);
  mResponseHandler->OnError(aRequestId, aError);
  return NS_OK;
}


} // end namespace virtualfilesystem
} // end namespace dom
} // end namespace mozilla
