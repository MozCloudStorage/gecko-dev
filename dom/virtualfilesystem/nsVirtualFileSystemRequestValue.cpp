/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/dom/FileSystemProviderGetMetadataEventBinding.h"
#include "mozilla/dom/TypedArray.h"
#include "mozilla/Move.h"
#include "mozilla/mozalloc.h"
#include "nsArrayUtils.h"
#include "nsVirtualFileSystemDataType.h"
#include "nsVirtualFileSystemRequestValue.h"
#include "nsComponentManagerUtils.h"
#include "nsMemory.h"
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
  nsTArray<nsCOMPtr<nsIEntryMetadata>>&& aArray)
  : mEntries(Move(aArray))
{
}

NS_IMETHODIMP
nsVirtualFileSystemReadDirectoryRequestValue::GetEntries(uint32_t* aCount, nsIEntryMetadata*** aEntries)
{
  if (NS_WARN_IF(!(aEntries && aEntries))) {
    return NS_ERROR_INVALID_POINTER;
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
