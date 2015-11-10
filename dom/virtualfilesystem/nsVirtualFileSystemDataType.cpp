/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/dom/FileSystemProviderGetMetadataEventBinding.h"
#include "nsIMutableArray.h"
#include "nsVirtualFileSystemDataType.h"
#include "nsTArray.h"

namespace mozilla {
namespace dom {
namespace virtualfilesystem {

NS_IMPL_ISUPPORTS_INHERITED(nsVirtualFileSystemMountRequestedOptions,
                            FileSystemProviderRequestedOptions,
                            nsIVirtualFileSystemMountRequestedOptions)

NS_IMETHODIMP
nsVirtualFileSystemMountRequestedOptions::GetDisplayName(nsAString& aDisplayName)
{
  aDisplayName = mDisplayName;
  return NS_OK;
}

NS_IMETHODIMP
nsVirtualFileSystemMountRequestedOptions::SetDisplayName(const nsAString& aDisplayName)
{
  mDisplayName = aDisplayName;
  return NS_OK;
}

NS_IMETHODIMP
nsVirtualFileSystemMountRequestedOptions::GetWritable(bool* aWritable)
{
  if (NS_WARN_IF(!aWritable)) {
    return NS_ERROR_INVALID_ARG;
  }

  *aWritable = mWritable;
  return NS_OK;
}

NS_IMETHODIMP
nsVirtualFileSystemMountRequestedOptions::SetWritable(bool aWritable)
{
  mWritable = aWritable;
  return NS_OK;
}

NS_IMETHODIMP
nsVirtualFileSystemMountRequestedOptions::GetOpenedFilesLimit(uint32_t* aOpenedFilesLimit)
{
  if (NS_WARN_IF(!aOpenedFilesLimit)) {
    return NS_ERROR_INVALID_ARG;
  }

  *aOpenedFilesLimit = mOpenedFilesLimit;
  return NS_OK;
}
NS_IMETHODIMP
nsVirtualFileSystemMountRequestedOptions::SetOpenedFilesLimit(uint32_t aOpenedFilesLimit)
{
  mOpenedFilesLimit = aOpenedFilesLimit;
  return NS_OK;
}

NS_IMPL_ISUPPORTS_INHERITED(nsVirtualFileSystemOpenedFileInfo,
                            OpenFileRequestedOptions,
                            nsIVirtualFileSystemOpenedFileInfo)

NS_IMETHODIMP
nsVirtualFileSystemOpenedFileInfo::GetOpenRequestId(uint32_t* aOpenRequestId)
{
  if (NS_WARN_IF(!aOpenRequestId)) {
    return NS_ERROR_INVALID_ARG;
  }

  *aOpenRequestId = mOpenRequestId;
  return NS_OK;
}
NS_IMETHODIMP
nsVirtualFileSystemOpenedFileInfo::SetOpenRequestId(uint32_t aOpenRequestId)
{
  mOpenRequestId = aOpenRequestId;
  return NS_OK;
}

NS_IMPL_ISUPPORTS_INHERITED(nsVirtualFileSystemInfo,
                            nsVirtualFileSystemMountRequestedOptions,
                            nsIVirtualFileSystemInfo)

NS_IMETHODIMP
nsVirtualFileSystemInfo::GetOpenedFiles(nsIArray** aOpenedFiles)
{
  if (NS_WARN_IF(!aOpenedFiles)) {
    return NS_ERROR_INVALID_POINTER;
  }

  nsresult rv;
  nsCOMPtr<nsIMutableArray> openedFiles = do_CreateInstance(NS_ARRAY_CONTRACTID, &rv);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  uint32_t length = mOpenedFiles.Length();
  for (uint32_t i = 0; i < length; i++) {
    nsCOMPtr<nsIVirtualFileSystemOpenedFileInfo> info = mOpenedFiles[i];
    if (info) {
      openedFiles->AppendElement(info, false);
    }
  }

  openedFiles.forget(aOpenedFiles);
  return NS_OK;
}

NS_IMETHODIMP
nsVirtualFileSystemInfo::AppendOpenedFile(nsIVirtualFileSystemOpenedFileInfo* aFileInfo)
{
  mOpenedFiles.AppendElement(aFileInfo);
  return NS_OK;
}

NS_IMETHODIMP
nsVirtualFileSystemInfo::RemoveOpenedFile(const uint32_t aOpenRequestId)
{
  uint32_t length = mOpenedFiles.Length();
  uint32_t idx;
  for (idx = 0; idx < length; idx++) {
    nsCOMPtr<nsIVirtualFileSystemOpenedFileInfo> info = mOpenedFiles[idx];
    MOZ_ASSERT(info);
    uint32_t requestId;
    info->GetRequestId(&requestId);
    if (requestId == aOpenRequestId) {
      break;
    }
  }
  mOpenedFiles.RemoveElementAt(idx);
  return NS_OK;
}

NS_IMPL_ISUPPORTS(nsEntryMetadata, nsIEntryMetadata)

NS_IMETHODIMP
nsEntryMetadata::GetIsDirectory(bool* aIsDirectory)
{
  if (NS_WARN_IF(!aIsDirectory)) {
    return NS_ERROR_INVALID_ARG;
  }

  *aIsDirectory = mIsDirectory;
  return NS_OK;
}

NS_IMETHODIMP
nsEntryMetadata::SetIsDirectory(bool aIsDirectory)
{
  mIsDirectory = aIsDirectory;
  return NS_OK;
}

NS_IMETHODIMP
nsEntryMetadata::GetName(nsAString& aName)
{
  aName = mName;
  return NS_OK;
}

NS_IMETHODIMP
nsEntryMetadata::SetName(const nsAString& aName)
{
  mName = aName;
  return NS_OK;
}

NS_IMETHODIMP
nsEntryMetadata::GetSize(uint64_t* aSize)
{
  if (NS_WARN_IF(!aSize)) {
    return NS_ERROR_INVALID_ARG;
  }

  *aSize = mSize;
  return NS_OK;
}

NS_IMETHODIMP
nsEntryMetadata::SetSize(uint64_t aSize)
{
  mSize = aSize;
  return NS_OK;
}

NS_IMETHODIMP
nsEntryMetadata::GetModificationTime(DOMTimeStamp* aModificationTime)
{
  if (NS_WARN_IF(!aModificationTime)) {
    return NS_ERROR_INVALID_ARG;
  }

  *aModificationTime = mModificationTime;
  return NS_OK;
}

NS_IMETHODIMP
nsEntryMetadata::SetModificationTime(DOMTimeStamp aModificationTime)
{
  mModificationTime = aModificationTime;
  return NS_OK;
}

NS_IMETHODIMP
nsEntryMetadata::GetMimeType(nsAString& aMimeType)
{
  aMimeType = mMimeType;
  return NS_OK;
}

NS_IMETHODIMP
nsEntryMetadata::SetMimeType(const nsAString& aMimeType)
{
  mMimeType = aMimeType;
  return NS_OK;
}

/* static */ already_AddRefed<nsIEntryMetadata>
nsEntryMetadata::FromEntryMetadata(const EntryMetadata& aData)
{
  nsCOMPtr<nsIEntryMetadata> data = new nsEntryMetadata();
  data->SetIsDirectory(aData.mIsDirectory);
  data->SetName(aData.mName);
  data->SetSize(aData.mSize);
  data->SetModificationTime(aData.mModificationTime);
  if (aData.mMimeType.WasPassed() && !aData.mMimeType.Value().IsEmpty()) {
    data->SetMimeType(aData.mMimeType.Value());
  }
  return data.forget();
}

} // namespace virtualfilesystem
} // namespace dom
} // namespace mozilla
