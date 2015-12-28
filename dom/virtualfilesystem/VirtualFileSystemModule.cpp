/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/ModuleUtils.h"
#include "nsVirtualFileSystemService.h"

using mozilla::dom::virtualfilesystem::nsVirtualFileSystemService;

NS_GENERIC_FACTORY_SINGLETON_CONSTRUCTOR(nsVirtualFileSystemService,
                                         nsVirtualFileSystemService::GetSingleton)
NS_DEFINE_NAMED_CID(VIRTUALFILESYSTEMSERVICE_CID);

static const mozilla::Module::CIDEntry kVirtualFileSystemModuleCIDs[] = {
  { &kVIRTUALFILESYSTEMSERVICE_CID, false, nullptr, nsVirtualFileSystemServiceConstructor },
  { nullptr }
};

static const mozilla::Module::ContractIDEntry kVirtualFileSystemModuleContracts[] = {
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
