/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/dom/FileSystemProviderGetMetadataEventBinding.h"
#include "mozilla/dom/TypedArray.h"
#include "nsArrayUtils.h"
#include "nsVirtualFileSystemDataType.h"
#include "nsVirtualFileSystemRequestValue.h"
#include "nsComponentManagerUtils.h"
#include "nsIMutableArray.h"
#include "nsTArray.h"

namespace mozilla {
namespace dom {
namespace virtualfilesystem {

NS_IMPL_ISUPPORTS(nsVirtualFileSystemGetMetadataRequestValue,
                  nsIVirtualFileSystemRequestValue,
                  nsIVirtualFileSystemGetMetadataRequestValue)

nsVirtualFileSystemGetMetadataRequestValue::nsVirtualFileSystemGetMetadataRequestValue(
  const EntryMetadata& aData)
{
  nsCOMPtr<nsIEntryMetadata> metadata =
    nsEntryMetadata::FromEntryMetadata(aData);

  mMetadata = metadata;
}

NS_IMETHODIMP
nsVirtualFileSystemGetMetadataRequestValue::GetMetadata(nsIEntryMetadata** aMetadata)
{
  if (NS_WARN_IF(!aMetadata)) {
    return NS_ERROR_INVALID_POINTER;
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
  const nsTArray<nsCOMPtr<nsIEntryMetadata>>& aArray)
{
  nsresult rv;
  nsCOMPtr<nsIMutableArray> entries = do_CreateInstance(NS_ARRAY_CONTRACTID, &rv);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return;
  }

  for (uint32_t i = 0; i < aArray.Length(); ++i) {
    rv = entries->AppendElement(aArray[i], false);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return;
    }
  }

  mEntries = entries;
}

NS_IMETHODIMP
nsVirtualFileSystemReadDirectoryRequestValue::GetEntries(nsIArray** aEntries)
{
  if (NS_WARN_IF(!aEntries)) {
    return NS_ERROR_INVALID_POINTER;
  }

  nsresult rv;
  nsCOMPtr<nsIMutableArray> entries = do_CreateInstance(NS_ARRAY_CONTRACTID, &rv);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  uint32_t length = 0;
  mEntries->GetLength(&length);
  for (uint32_t i = 0; i < length; i++) {
    nsCOMPtr<nsIEntryMetadata> metadata = do_QueryElementAt(mEntries, i);
    if (metadata) {
      entries->AppendElement(metadata, false);
    }
  }

  entries.forget(aEntries);
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
  nsCOMPtr<nsIArray> orgEntries;
  rv = GetEntries(getter_AddRefs(orgEntries));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  nsCOMPtr<nsIArray> entries;
  rv = value->GetEntries(getter_AddRefs(entries));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  nsCOMPtr<nsIMutableArray> mergedEntries = do_CreateInstance(NS_ARRAY_CONTRACTID, &rv);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  AppendElementsInArray(mergedEntries, orgEntries);
  AppendElementsInArray(mergedEntries, entries);

  mEntries = mergedEntries;
  return NS_OK;
}

void
nsVirtualFileSystemReadDirectoryRequestValue::AppendElementsInArray(
  nsIMutableArray* aMergedArray, nsIArray* aToBeMergedArray)
{
  uint32_t length = 0;
  aToBeMergedArray->GetLength(&length);
  for (uint32_t i = 0; i < length; i++) {
    nsCOMPtr<nsIEntryMetadata> metadata = do_QueryElementAt(aToBeMergedArray, i);
    if (metadata) {
      aMergedArray->AppendElement(metadata, false);
    }
  }
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
nsVirtualFileSystemReadFileRequestValue::GetData(nsACString & aData)
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
