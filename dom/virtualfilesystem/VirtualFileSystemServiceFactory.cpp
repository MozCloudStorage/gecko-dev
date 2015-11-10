/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

//#include "mozilla/dom/FakeVirtualFileSystemService.h"
#include "nsVirtualFileSystemService.h"
#include "VirtualFileSystemServiceFactory.h"
#include "nsIVirtualFileSystemService.h"

namespace mozilla {
namespace dom {

/* static */ already_AddRefed<nsIVirtualFileSystemService>
VirtualFileSystemServiceFactory::AutoCreateVirtualFileSystemService()
{
  nsresult rv;
  nsCOMPtr<nsIVirtualFileSystemService> service = do_GetService(VIRTUAL_FILE_SYSTEM_SERVICE_CONTRACT_ID);
  if (!service) {
    service = do_CreateInstance(VIRTUAL_FILE_SYSTEM_SERVICE_CONTRACT_ID, &rv);

    if (NS_WARN_IF(NS_FAILED(rv))) {
      return nullptr;
    }
  }

  return service.forget();
}

} // namespace dom
} // namespace mozilla
