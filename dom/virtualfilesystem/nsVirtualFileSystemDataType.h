/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_virtualfilesystem_nsVirtualFileSystemData_h
#define mozilla_dom_virtualfilesystem_nsVirtualFileSystemData_h

#include "mozilla/dom/FileSystemProviderEvent.h"
#include "mozilla/dom/FileSystemProviderOpenFileEvent.h"
#include "nsIVirtualFileSystemDataType.h"
#include "nsString.h"

namespace mozilla {
namespace dom {

struct EntryMetadata;

namespace virtualfilesystem {

class nsVirtualFileSystemMountRequestedOptions
  : public FileSystemProviderRequestedOptions
  , public nsIVirtualFileSystemMountRequestedOptions
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIVIRTUALFILESYSTEMMOUNTREQUESTEDOPTIONS
  NS_FORWARD_NSIVIRTUALFILESYSTEMREQUESTEDOPTIONS(FileSystemProviderRequestedOptions::)

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

class nsVirtualFileSystemOpenedFileInfo final : public OpenFileRequestedOptions
                                              , public nsIVirtualFileSystemOpenedFileInfo
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIVIRTUALFILESYSTEMOPENEDFILEINFO
  NS_FORWARD_NSIVIRTUALFILESYSTEMREQUESTEDOPTIONS(FileSystemProviderRequestedOptions::)
  NS_FORWARD_NSIVIRTUALFILESYSTEMOPENFILEREQUESTEDOPTIONS(OpenFileRequestedOptions::)

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
  NS_FORWARD_NSIVIRTUALFILESYSTEMREQUESTEDOPTIONS(FileSystemProviderRequestedOptions::)
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
