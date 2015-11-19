/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_virtualfilesystem_nsVirtualFileSystemData_h
#define mozilla_dom_virtualfilesystem_nsVirtualFileSystemData_h

#include "nsIVirtualFileSystemDataType.h"
#include "nsString.h"

namespace mozilla {
namespace dom {

struct EntryMetadata;

namespace virtualfilesystem {

class nsVirtualFileSystemRequestedOptions : public nsIVirtualFileSystemRequestedOptions
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIVIRTUALFILESYSTEMREQUESTEDOPTIONS

  nsVirtualFileSystemRequestedOptions() = default;

protected:
  virtual ~nsVirtualFileSystemRequestedOptions() = default;

  nsString mFileSystemId;
  uint32_t mRequestId;
};

class nsVirtualFileSystemAbortRequestedOptions final
  : public nsVirtualFileSystemRequestedOptions
  , public nsIVirtualFileSystemAbortRequestedOptions
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIVIRTUALFILESYSTEMABORTREQUESTEDOPTIONS
  NS_FORWARD_NSIVIRTUALFILESYSTEMREQUESTEDOPTIONS(nsVirtualFileSystemRequestedOptions::)

  nsVirtualFileSystemAbortRequestedOptions() = default;

private:
  ~nsVirtualFileSystemAbortRequestedOptions() = default;

  uint32_t mOperationRequestId;
};

class nsVirtualFileSystemCloseFileRequestedOptions final
  : public nsVirtualFileSystemRequestedOptions
  , public nsIVirtualFileSystemCloseFileRequestedOptions
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIVIRTUALFILESYSTEMCLOSEFILEREQUESTEDOPTIONS
  NS_FORWARD_NSIVIRTUALFILESYSTEMREQUESTEDOPTIONS(nsVirtualFileSystemRequestedOptions::)

  nsVirtualFileSystemCloseFileRequestedOptions() = default;

private:
  ~nsVirtualFileSystemCloseFileRequestedOptions() = default;

  uint32_t mOpenRequestId;
};

class nsVirtualFileSystemGetMetadataRequestedOptions final
  : public nsVirtualFileSystemRequestedOptions
  , public nsIVirtualFileSystemGetMetadataRequestedOptions
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIVIRTUALFILESYSTEMGETMETADATAREQUESTEDOPTIONS
  NS_FORWARD_NSIVIRTUALFILESYSTEMREQUESTEDOPTIONS(nsVirtualFileSystemRequestedOptions::)

  nsVirtualFileSystemGetMetadataRequestedOptions() = default;

private:
  ~nsVirtualFileSystemGetMetadataRequestedOptions() = default;

  nsString mEntryPath;
};

class nsVirtualFileSystemOpenFileRequestedOptions
  : public nsVirtualFileSystemRequestedOptions
  , public nsIVirtualFileSystemOpenFileRequestedOptions
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIVIRTUALFILESYSTEMOPENFILEREQUESTEDOPTIONS
  NS_FORWARD_NSIVIRTUALFILESYSTEMREQUESTEDOPTIONS(nsVirtualFileSystemRequestedOptions::)

  nsVirtualFileSystemOpenFileRequestedOptions() = default;

protected:
  virtual ~nsVirtualFileSystemOpenFileRequestedOptions() = default;

  nsString mFilePath;
  uint32_t mMode;
};

class nsVirtualFileSystemReadDirectoryRequestedOptions final
  : public nsVirtualFileSystemRequestedOptions
  , public nsIVirtualFileSystemReadDirectoryRequestedOptions
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIVIRTUALFILESYSTEMREADDIRECTORYREQUESTEDOPTIONS
  NS_FORWARD_NSIVIRTUALFILESYSTEMREQUESTEDOPTIONS(nsVirtualFileSystemRequestedOptions::)

  nsVirtualFileSystemReadDirectoryRequestedOptions() = default;

private:
  ~nsVirtualFileSystemReadDirectoryRequestedOptions() = default;

  nsString mDirectoryPath;
};

class nsVirtualFileSystemReadFileRequestedOptions final
  : public nsVirtualFileSystemRequestedOptions
  , public nsIVirtualFileSystemReadFileRequestedOptions
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIVIRTUALFILESYSTEMREADFILEREQUESTEDOPTIONS
  NS_FORWARD_NSIVIRTUALFILESYSTEMREQUESTEDOPTIONS(nsVirtualFileSystemRequestedOptions::)

  nsVirtualFileSystemReadFileRequestedOptions() = default;

private:
  ~nsVirtualFileSystemReadFileRequestedOptions() = default;

  uint32_t mOpenRequestId;
  uint64_t mOffset;
  uint64_t mLength;
};

class nsVirtualFileSystemUnmountRequestedOptions final
  : public nsVirtualFileSystemRequestedOptions
  , public nsIVirtualFileSystemUnmountRequestedOptions
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIVIRTUALFILESYSTEMUNMOUNTREQUESTEDOPTIONS
  NS_FORWARD_NSIVIRTUALFILESYSTEMREQUESTEDOPTIONS(nsVirtualFileSystemRequestedOptions::)

  nsVirtualFileSystemUnmountRequestedOptions() = default;

private:
  ~nsVirtualFileSystemUnmountRequestedOptions() = default;

};

class nsVirtualFileSystemMountRequestedOptions
  : public nsVirtualFileSystemRequestedOptions
  , public nsIVirtualFileSystemMountRequestedOptions
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIVIRTUALFILESYSTEMMOUNTREQUESTEDOPTIONS
  NS_FORWARD_NSIVIRTUALFILESYSTEMREQUESTEDOPTIONS(nsVirtualFileSystemRequestedOptions::)

  nsVirtualFileSystemMountRequestedOptions()
    : mWritable(false)
    , mOpenedFilesLimit(0)
  {}

protected:
  virtual ~nsVirtualFileSystemMountRequestedOptions() = default;

  nsString mDisplayName;
  bool mWritable;
  uint32_t mOpenedFilesLimit;
};

class nsVirtualFileSystemOpenedFileInfo final : public nsVirtualFileSystemOpenFileRequestedOptions
                                              , public nsIVirtualFileSystemOpenedFileInfo
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIVIRTUALFILESYSTEMOPENEDFILEINFO
  NS_FORWARD_NSIVIRTUALFILESYSTEMREQUESTEDOPTIONS(nsVirtualFileSystemRequestedOptions::)
  NS_FORWARD_NSIVIRTUALFILESYSTEMOPENFILEREQUESTEDOPTIONS(nsVirtualFileSystemOpenFileRequestedOptions::)

  nsVirtualFileSystemOpenedFileInfo()
    : mOpenRequestId(0)
  {}

private:
  virtual ~nsVirtualFileSystemOpenedFileInfo() = default;

  uint32_t mOpenRequestId;
};

class nsVirtualFileSystemInfo final : public nsVirtualFileSystemMountRequestedOptions
                                    , public nsIVirtualFileSystemInfo
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIVIRTUALFILESYSTEMINFO
  NS_FORWARD_NSIVIRTUALFILESYSTEMREQUESTEDOPTIONS(nsVirtualFileSystemRequestedOptions::)
  NS_FORWARD_NSIVIRTUALFILESYSTEMMOUNTREQUESTEDOPTIONS(nsVirtualFileSystemMountRequestedOptions::)

  nsVirtualFileSystemInfo() = default;

private:
  virtual ~nsVirtualFileSystemInfo() = default;

  nsTArray<nsCOMPtr<nsIVirtualFileSystemOpenedFileInfo>> mOpenedFiles;
};

class nsEntryMetadata final : public nsIEntryMetadata
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIENTRYMETADATA

  explicit nsEntryMetadata() = default;
  static already_AddRefed<nsIEntryMetadata> FromEntryMetadata(const EntryMetadata& data);

private:
  virtual ~nsEntryMetadata() = default;

  bool mIsDirectory;
  nsString mName;
  uint64_t mSize;
  DOMTimeStamp mModificationTime;
  nsString mMimeType;
};


} // namespace virtualfilesystem
} // namespace dom
} // namespace mozilla

#endif // mozilla_dom_virtualfilesystem_nsVirtualFileSystemData_h
