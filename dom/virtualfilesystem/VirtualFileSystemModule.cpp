/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "nsVirtualFileSystemRequestManager.h"
#include "nsVirtualFileSystemDataType.h"
//#include "mozilla/dom/FakeVirtualFileSystemService.h"
#include "nsIVirtualFileSystemDataType.h"
#include "nsVirtualFileSystemService.h"
#include "mozilla/dom/FileSystemProviderAbortEvent.h"
#include "mozilla/dom/FileSystemProviderCloseFileEvent.h"
#include "mozilla/dom/FileSystemProviderGetMetadataEvent.h"
#include "mozilla/dom/FileSystemProviderOpenFileEvent.h"
#include "mozilla/dom/FileSystemProviderReadDirectoryEvent.h"
#include "mozilla/dom/FileSystemProviderReadFileEvent.h"
#include "mozilla/dom/FileSystemProviderUnmountEvent.h"
#include "mozilla/ModuleUtils.h"

using mozilla::dom::virtualfilesystem::nsEntryMetadata;
using mozilla::dom::AbortRequestedOptions;
using mozilla::dom::CloseFileRequestedOptions;
using mozilla::dom::OpenFileRequestedOptions;
using mozilla::dom::GetMetadataRequestedOptions;
using mozilla::dom::ReadDirectoryRequestedOptions;
using mozilla::dom::ReadFileRequestedOptions;
using mozilla::dom::UnmountRequestedOptions;
//using mozilla::dom::FakeVirtualFileSystemService;
using mozilla::dom::virtualfilesystem::nsVirtualFileSystemService;
using mozilla::dom::virtualfilesystem::nsVirtualFileSystemRequestManager;

NS_GENERIC_FACTORY_CONSTRUCTOR(AbortRequestedOptions)
NS_DEFINE_NAMED_CID(VIRTUALFILESYSTEMABORTREQUESTEDOPTIONS_CID);

NS_GENERIC_FACTORY_CONSTRUCTOR(CloseFileRequestedOptions)
NS_DEFINE_NAMED_CID(VIRTUALFILESYSTEMCLOSEFILEREQUESTEDOPTIONS_CID);

NS_GENERIC_FACTORY_CONSTRUCTOR(OpenFileRequestedOptions)
NS_DEFINE_NAMED_CID(VIRTUALFILESYSTEMOPENFILEREQUESTEDOPTIONS_CID);

NS_GENERIC_FACTORY_CONSTRUCTOR(GetMetadataRequestedOptions)
NS_DEFINE_NAMED_CID(VIRTUALFILESYSTEMGETMETADATAREQUESTEDOPTIONS_CID);

NS_GENERIC_FACTORY_CONSTRUCTOR(ReadDirectoryRequestedOptions)
NS_DEFINE_NAMED_CID(VIRTUALFILESYSTEMREADDIRECTORYREQUESTEDOPTIONS_CID);

NS_GENERIC_FACTORY_CONSTRUCTOR(ReadFileRequestedOptions)
NS_DEFINE_NAMED_CID(VIRTUALFILESYSTEMREADFILEREQUESTEDOPTIONS_CID);

NS_GENERIC_FACTORY_CONSTRUCTOR(UnmountRequestedOptions)
NS_DEFINE_NAMED_CID(VIRTUALFILESYSTEMUNMOUNTREQUESTEDOPTIONS_CID);

NS_GENERIC_FACTORY_CONSTRUCTOR(nsVirtualFileSystemRequestManager)
NS_DEFINE_NAMED_CID(VIRTUALFILESYSTEMREQUESTMANAGER_CID);

//NS_GENERIC_FACTORY_CONSTRUCTOR(FakeVirtualFileSystemService)
//NS_DEFINE_NAMED_CID(FAKEVIRTUALFILESYSTEMSERVICE_CID);

NS_GENERIC_FACTORY_CONSTRUCTOR(nsVirtualFileSystemService)
NS_DEFINE_NAMED_CID(VIRTUALFILESYSTEMSERVICE_CID);

static const mozilla::Module::CIDEntry kVirtualFileSystemModuleCIDs[] = {
  { &kVIRTUALFILESYSTEMABORTREQUESTEDOPTIONS_CID, false, nullptr, AbortRequestedOptionsConstructor },
  { &kVIRTUALFILESYSTEMCLOSEFILEREQUESTEDOPTIONS_CID, false, nullptr, CloseFileRequestedOptionsConstructor },
  { &kVIRTUALFILESYSTEMOPENFILEREQUESTEDOPTIONS_CID, false, nullptr, OpenFileRequestedOptionsConstructor },
  { &kVIRTUALFILESYSTEMGETMETADATAREQUESTEDOPTIONS_CID, false, nullptr, GetMetadataRequestedOptionsConstructor },
  { &kVIRTUALFILESYSTEMREADDIRECTORYREQUESTEDOPTIONS_CID, false, nullptr, ReadDirectoryRequestedOptionsConstructor },
  { &kVIRTUALFILESYSTEMREADFILEREQUESTEDOPTIONS_CID, false, nullptr, ReadFileRequestedOptionsConstructor },
  { &kVIRTUALFILESYSTEMUNMOUNTREQUESTEDOPTIONS_CID, false, nullptr, UnmountRequestedOptionsConstructor },
  { &kVIRTUALFILESYSTEMREQUESTMANAGER_CID, false, nullptr, nsVirtualFileSystemRequestManagerConstructor },
//  { &kFAKEVIRTUALFILESYSTEMSERVICE_CID, false, nullptr, FakeVirtualFileSystemServiceConstructor },
  { &kVIRTUALFILESYSTEMSERVICE_CID, false, nullptr, nsVirtualFileSystemServiceConstructor },

  { nullptr }
};

static const mozilla::Module::ContractIDEntry kVirtualFileSystemModuleContracts[] = {
  { VIRTUAL_FILE_SYSTEM_ABORT_REQUESTED_OPTIONS_CONTRACT_ID, &kVIRTUALFILESYSTEMABORTREQUESTEDOPTIONS_CID },
  { VIRTUAL_FILE_SYSTEM_CLOSEFILE_REQUESTED_OPTIONS_CONTRACT_ID, &kVIRTUALFILESYSTEMCLOSEFILEREQUESTEDOPTIONS_CID },
  { VIRTUAL_FILE_SYSTEM_OPENFILE_REQUESTED_OPTIONS_CONTRACT_ID, &kVIRTUALFILESYSTEMOPENFILEREQUESTEDOPTIONS_CID },
  { VIRTUAL_FILE_SYSTEM_GETMETADATA_REQUESTED_OPTIONS_CONTRACT_ID, &kVIRTUALFILESYSTEMGETMETADATAREQUESTEDOPTIONS_CID },
  { VIRTUAL_FILE_SYSTEM_READDIRECTORY_REQUESTED_OPTIONS_CONTRACT_ID, &kVIRTUALFILESYSTEMREADDIRECTORYREQUESTEDOPTIONS_CID },
  { VIRTUAL_FILE_SYSTEM_READFILE_REQUESTED_OPTIONS_CONTRACT_ID, &kVIRTUALFILESYSTEMREADFILEREQUESTEDOPTIONS_CID },
  { VIRTUAL_FILE_SYSTEM_UNMOUNT_REQUESTED_OPTIONS_CONTRACT_ID, &kVIRTUALFILESYSTEMUNMOUNTREQUESTEDOPTIONS_CID },
  { VIRTUAL_FILE_SYSTEM_REQUEST_MANAGER_CONTRACT_ID, &kVIRTUALFILESYSTEMREQUESTMANAGER_CID },
//  { FAKE_VIRTUALFILESYSTEM_SERVICE_CONTRACTID, &kFAKEVIRTUALFILESYSTEMSERVICE_CID },
  { VIRTUAL_FILE_SYSTEM_SERVICE_CONTRACT_ID, &kVIRTUALFILESYSTEMSERVICE_CID },
  { nullptr }
};

static const mozilla::Module::CategoryEntry kVirtualFileSystemModuleCategories[] = {
  { nullptr }
};

static const mozilla::Module kVirtualFileSystemModule = {
  mozilla::Module::kVersion,
  kVirtualFileSystemModuleCIDs,
  kVirtualFileSystemModuleContracts,
  kVirtualFileSystemModuleCategories
};

NSMODULE_DEFN(VirtualFileSystemModule) = &kVirtualFileSystemModule;
