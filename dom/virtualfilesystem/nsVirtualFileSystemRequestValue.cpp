/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/dom/FileSystemProviderGetMetadataEventBinding.h"
#include "mozilla/dom/TypedArray.h"
#include "mozilla/Move.h"
#include "mozilla/mozalloc.h"
#include "nsVirtualFileSystemRequestValue.h"
#include "nsComponentManagerUtils.h"
#include "nsMemory.h"
#include "nsTArray.h"

namespace mozilla {
namespace dom {
namespace virtualfilesystem {

NS_IMPL_ISUPPORTS(nsEntryMetadata, nsIEntryMetadata)

nsEntryMetadata::nsEntryMetadata(const EntryMetadata& aData)
{
  mIsDirectory = aData.mIsDirectory;
  mName = aData.mName;
  mSize = aData.mSize;
  mModificationTime = aData.mModificationTime;
  if (aData.mMimeType.WasPassed() && !aData.mMimeType.Value().IsEmpty()) {
    mMimeType = aData.mMimeType.Value();
  }
}

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

NS_IMPL_ISUPPORTS(nsVirtualFileSystemGetMetadataRequestValue,
                  nsIVirtualFileSystemRequestValue,
                  nsIVirtualFileSystemGetMetadataRequestValue)

nsVirtualFileSystemGetMetadataRequestValue::nsVirtualFileSystemGetMetadataRequestValue(
  const EntryMetadata& aData)
  : mMetadata(new nsEntryMetadata(aData))
{
}

NS_IMETHODIMP
nsVirtualFileSystemGetMetadataRequestValue::GetMetadata(nsIEntryMetadata** aMetadata)
{
  if (NS_WARN_IF(!aMetadata)) {
    return NS_ERROR_INVALID_ARG;
  }

  nsCOMPtr<nsIEntryMetadata> data = mMetadata;
  data.forget(aMetadata);
  return NS_OK;
}

NS_IMETHODIMP
nsVirtualFileSystemGetMetadataRequestValue::Concat(nsIVirtualFileSystemRequestValue* aValue)
{
  return NS_OK;
}

NS_IMPL_ISUPPORTS(nsVirtualFileSystemReadDirectoryRequestValue,
                  nsIVirtualFileSystemRequestValue,
                  nsIVirtualFileSystemReadDirectoryRequestValue)

nsVirtualFileSystemReadDirectoryRequestValue::nsVirtualFileSystemReadDirectoryRequestValue(
  const nsTArray<EntryMetadata>& aArray)
{
  for (const auto& iter : aArray) {
    mEntries.AppendElement(new nsEntryMetadata(iter));
  }
}

NS_IMETHODIMP
nsVirtualFileSystemReadDirectoryRequestValue::GetEntries(uint32_t* aCount, nsIEntryMetadata*** aEntries)
{
  if (NS_WARN_IF(!(aCount && aEntries))) {
    return NS_ERROR_INVALID_ARG;
  }

  *aCount = 0;
  *aEntries = nullptr;
  nsTArray<nsCOMPtr<nsIEntryMetadata>>::index_type count = mEntries.Length();
  if (!count) {
    return NS_OK;
  }

  *aEntries =
    static_cast<nsIEntryMetadata**>(moz_xmalloc(sizeof(nsIEntryMetadata*) * count));
  if(NS_WARN_IF(!*aEntries)) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  nsTArray<nsCOMPtr<nsIEntryMetadata>>::index_type i;
  for (i = 0; i < count; i++) {
    NS_ADDREF((*aEntries)[i] = mEntries[i]);
  }

  *aCount = count;
  return NS_OK;
}

NS_IMETHODIMP
nsVirtualFileSystemReadDirectoryRequestValue::Concat(nsIVirtualFileSystemRequestValue* aValue)
{
  nsCOMPtr<nsIVirtualFileSystemReadDirectoryRequestValue> value = do_QueryInterface(aValue);
  if (NS_WARN_IF(!value)) {
    return NS_ERROR_INVALID_ARG;
  }

  nsresult rv;
  uint32_t count;
  nsIEntryMetadata** entries;
  rv = value->GetEntries(&count, &entries);
  if (NS_FAILED(rv)) {
    return rv;
  }

  for (uint32_t i = 0; i < count; i++) {
    nsCOMPtr<nsIEntryMetadata> data = entries[i];
    mEntries.AppendElement(data);
  }

  NS_FREE_XPCOM_ISUPPORTS_POINTER_ARRAY(count, entries);
  return NS_OK;
}

NS_IMETHODIMP
nsVirtualFileSystemReadDirectoryRequestValue::AddEntryMetadata(nsIEntryMetadata* aMetaData)
{
  if (NS_WARN_IF(!aMetaData)) {
    return NS_ERROR_INVALID_ARG;
  }

  mEntries.AppendElement(aMetaData);
  return NS_OK;
}

NS_IMPL_ISUPPORTS(nsVirtualFileSystemReadFileRequestValue,
                  nsIVirtualFileSystemRequestValue,
                  nsIVirtualFileSystemReadFileRequestValue)

nsVirtualFileSystemReadFileRequestValue::nsVirtualFileSystemReadFileRequestValue(
  const ArrayBuffer& aBuffer)
{
  mData.Assign(reinterpret_cast<const char*>(aBuffer.Data()), aBuffer.Length());
}

NS_IMETHODIMP
nsVirtualFileSystemReadFileRequestValue::GetData(nsACString& aData)
{
  aData.Assign(mData);
  return NS_OK;
}

NS_IMETHODIMP
nsVirtualFileSystemReadFileRequestValue::Concat(nsIVirtualFileSystemRequestValue* aValue)
{
  nsCOMPtr<nsIVirtualFileSystemReadFileRequestValue> value = do_QueryInterface(aValue);
  if (NS_WARN_IF(!value)) {
    return NS_ERROR_INVALID_ARG;
  }

  nsAutoCString data;
  value->GetData(data);

  mData.Append(data.get(), data.Length());
  return NS_OK;
}

} // end of namespace mozilla
} // end of namespace dom
} // end of namespace virtualfilesystem
