/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/dom/virtualfilesystem/FakeVirtualFileSystemService.h"
#include "mozilla/dom/VirtualFileSystemIPCService.h"
#include "VirtualFileSystemServiceFactory.h"

#ifdef MOZ_WIDGET_GONK
#include "nsVirtualFileSystemService.h"
#endif

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
      virtualfilesystem::VirtualFileSystemIPCService::GetSingleton();
  }
  else {
  #ifdef MOZ_WIDGET_GONK
    service =
      virtualfilesystem::nsVirtualFileSystemService::GetSingleton();
  #else
    service =
      virtualfilesystem::FakeVirtualFileSystemService::GetSingleton();
  #endif
  }

  return service.forget();
}

} // namespace virtualfilesystem
} // namespace dom
} // namespace mozilla