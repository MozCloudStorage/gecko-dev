/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/dom/virtualfilesystem/VirtualFileSystemIPCService.h"
#include "nsVirtualFileSystemService.h"
#include "VirtualFileSystemServiceFactory.h"

namespace mozilla {
namespace dom {
namespace virtualfilesystem {

/* static */ already_AddRefed<BaseVirtualFileSystemService>
VirtualFileSystemServiceFactory::AutoCreateVirtualFileSystemService()
{
  MOZ_ASSERT(NS_IsMainThread());

  RefPtr<BaseVirtualFileSystemService> service;
  if (!XRE_IsParentProcess()) {
    service =
      VirtualFileSystemIPCService::GetSingleton();
  }
  else {
    service =
      nsVirtualFileSystemService::GetSingleton();
  }

  return service.forget();
}

} // namespace virtualfilesystem
} // namespace dom
} // namespace mozilla