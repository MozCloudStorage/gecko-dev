/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/dom/virtualfilesystem/FakeVirtualFileSystemService.h"
#include "mozilla/ModuleUtils.h"
#include "nsIVirtualFileSystemService.h"
#include "nsVirtualFileSystemDataType.h"
#include "nsVirtualFileSystemRequestManager.h"
#include "nsVirtualFileSystemRequestValue.h"

using mozilla::dom::virtualfilesystem::nsVirtualFileSystemAbortRequestedOptions;
using mozilla::dom::virtualfilesystem::nsVirtualFileSystemCloseFileRequestedOptions;
using mozilla::dom::virtualfilesystem::nsVirtualFileSystemOpenFileRequestedOptions;
using mozilla::dom::virtualfilesystem::nsVirtualFileSystemGetMetadataRequestedOptions;
using mozilla::dom::virtualfilesystem::nsVirtualFileSystemReadDirectoryRequestedOptions;
using mozilla::dom::virtualfilesystem::nsVirtualFileSystemReadFileRequestedOptions;
using mozilla::dom::virtualfilesystem::nsVirtualFileSystemUnmountRequestedOptions;
using mozilla::dom::virtualfilesystem::nsVirtualFileSystemRequestManager;
using mozilla::dom::virtualfilesystem::nsVirtualFileSystemGetMetadataRequestValue;
using mozilla::dom::virtualfilesystem::nsVirtualFileSystemReadDirectoryRequestValue;
using mozilla::dom::virtualfilesystem::nsVirtualFileSystemReadFileRequestValue;
using mozilla::dom::virtualfilesystem::FakeVirtualFileSystemService;

NS_GENERIC_FACTORY_CONSTRUCTOR(nsVirtualFileSystemAbortRequestedOptions)
NS_DEFINE_NAMED_CID(VIRTUALFILESYSTEMABORTREQUESTEDOPTIONS_CID);

NS_GENERIC_FACTORY_CONSTRUCTOR(nsVirtualFileSystemCloseFileRequestedOptions)
NS_DEFINE_NAMED_CID(VIRTUALFILESYSTEMCLOSEFILEREQUESTEDOPTIONS_CID);

NS_GENERIC_FACTORY_CONSTRUCTOR(nsVirtualFileSystemOpenFileRequestedOptions)
NS_DEFINE_NAMED_CID(VIRTUALFILESYSTEMOPENFILEREQUESTEDOPTIONS_CID);

NS_GENERIC_FACTORY_CONSTRUCTOR(nsVirtualFileSystemGetMetadataRequestedOptions)
NS_DEFINE_NAMED_CID(VIRTUALFILESYSTEMGETMETADATAREQUESTEDOPTIONS_CID);

NS_GENERIC_FACTORY_CONSTRUCTOR(nsVirtualFileSystemReadDirectoryRequestedOptions)
NS_DEFINE_NAMED_CID(VIRTUALFILESYSTEMREADDIRECTORYREQUESTEDOPTIONS_CID);

NS_GENERIC_FACTORY_CONSTRUCTOR(nsVirtualFileSystemReadFileRequestedOptions)
NS_DEFINE_NAMED_CID(VIRTUALFILESYSTEMREADFILEREQUESTEDOPTIONS_CID);

NS_GENERIC_FACTORY_CONSTRUCTOR(nsVirtualFileSystemUnmountRequestedOptions)
NS_DEFINE_NAMED_CID(VIRTUALFILESYSTEMUNMOUNTREQUESTEDOPTIONS_CID);

NS_GENERIC_FACTORY_CONSTRUCTOR(nsVirtualFileSystemRequestManager)
NS_DEFINE_NAMED_CID(VIRTUALFILESYSTEMREQUESTMANAGER_CID);

NS_GENERIC_FACTORY_CONSTRUCTOR(nsVirtualFileSystemGetMetadataRequestValue)
NS_DEFINE_NAMED_CID(VIRTUALFILESYSTEMGETMETADATAREQUESTVALUE_CID);

NS_GENERIC_FACTORY_CONSTRUCTOR(nsVirtualFileSystemReadDirectoryRequestValue)
NS_DEFINE_NAMED_CID(VIRTUALFILESYSTEMREADDIRECTORYREQUESTVALUE_CID);

NS_GENERIC_FACTORY_CONSTRUCTOR(nsVirtualFileSystemReadFileRequestValue)
NS_DEFINE_NAMED_CID(VIRTUALFILESYSTEMREADFILEREQUESTVALUE_CID);

static const mozilla::Module::CIDEntry kVirtualFileSystemModuleCIDs[] = {
  { &kVIRTUALFILESYSTEMABORTREQUESTEDOPTIONS_CID, false, nullptr, nsVirtualFileSystemAbortRequestedOptionsConstructor },
  { &kVIRTUALFILESYSTEMCLOSEFILEREQUESTEDOPTIONS_CID, false, nullptr, nsVirtualFileSystemCloseFileRequestedOptionsConstructor },
  { &kVIRTUALFILESYSTEMOPENFILEREQUESTEDOPTIONS_CID, false, nullptr, nsVirtualFileSystemOpenFileRequestedOptionsConstructor },
  { &kVIRTUALFILESYSTEMGETMETADATAREQUESTEDOPTIONS_CID, false, nullptr, nsVirtualFileSystemGetMetadataRequestedOptionsConstructor },
  { &kVIRTUALFILESYSTEMREADDIRECTORYREQUESTEDOPTIONS_CID, false, nullptr, nsVirtualFileSystemReadDirectoryRequestedOptionsConstructor },
  { &kVIRTUALFILESYSTEMREADFILEREQUESTEDOPTIONS_CID, false, nullptr, nsVirtualFileSystemReadFileRequestedOptionsConstructor },
  { &kVIRTUALFILESYSTEMUNMOUNTREQUESTEDOPTIONS_CID, false, nullptr, nsVirtualFileSystemUnmountRequestedOptionsConstructor },
  { &kVIRTUALFILESYSTEMREQUESTMANAGER_CID, false, nullptr, nsVirtualFileSystemRequestManagerConstructor },
  { &kVIRTUALFILESYSTEMGETMETADATAREQUESTVALUE_CID, false, nullptr, nsVirtualFileSystemGetMetadataRequestValueConstructor },
  { &kVIRTUALFILESYSTEMREADDIRECTORYREQUESTVALUE_CID, false, nullptr, nsVirtualFileSystemReadDirectoryRequestValueConstructor },
  { &kVIRTUALFILESYSTEMREADFILEREQUESTVALUE_CID, false, nullptr, nsVirtualFileSystemReadFileRequestValueConstructor },
  //{ &kFAKEVIRTUALFILESYSTEMSERVICE_CID, false, nullptr, FakeVirtualFileSystemServiceConstructor },
  //{ &kVIRTUALFILESYSTEMSERVICE_CID, false, nullptr, nsIVirtualFileSystemServiceConstructor },
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
  { VIRTUAL_FILE_SYSTEM_GETMETADATA_REQUEST_VALUE_CONTRACT_ID, &kVIRTUALFILESYSTEMGETMETADATAREQUESTVALUE_CID },
  { VIRTUAL_FILE_SYSTEM_READDIRECTORY_REQUEST_VALUE_CONTRACT_ID, &kVIRTUALFILESYSTEMREADDIRECTORYREQUESTVALUE_CID },
  { VIRTUAL_FILE_SYSTEM_READFILE_REQUEST_VALUE_CONTRACT_ID, &kVIRTUALFILESYSTEMREADFILEREQUESTVALUE_CID },
  //{ FAKE_VIRTUALFILESYSTEM_SERVICE_CONTRACTID, &kFAKEVIRTUALFILESYSTEMSERVICE_CID },
  //{ VIRTUAL_FILE_SYSTEM_SERVICE_CONTRACT_ID, &kVIRTUALFILESYSTEMSERVICE_CID },
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
