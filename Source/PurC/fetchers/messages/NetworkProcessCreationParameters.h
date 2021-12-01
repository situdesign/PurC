/*
 * Copyright (C) 2012-2017 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include "fetcher-messages-basic.h"
#include "SandboxExtension.h"
#include "WebsiteDataStoreParameters.h"
#include "NetworkProxySettings.h"

#include <wtf/ProcessID.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>


namespace IPC {
class Decoder;
class Encoder;
}

namespace PurCFetcher {

struct NetworkProcessCreationParameters {
    NetworkProcessCreationParameters();

    void encode(IPC::Encoder&) const;
    static WARN_UNUSED_RETURN bool decode(IPC::Decoder&, NetworkProcessCreationParameters&);

    CacheModel cacheModel { CacheModel::DocumentViewer };

#if PLATFORM(MAC) || PLATFORM(MACCATALYST)
    Vector<uint8_t> uiProcessCookieStorageIdentifier;
#endif
#if PLATFORM(IOS_FAMILY)
    SandboxExtension::Handle cookieStorageDirectoryExtensionHandle;
    SandboxExtension::Handle containerCachesDirectoryExtensionHandle;
    SandboxExtension::Handle parentBundleDirectoryExtensionHandle;
#endif
    bool shouldSuppressMemoryPressureHandler { false };

    Vector<String> urlSchemesRegisteredForCustomProtocols;

#if 0//PLATFORM(COCOA)
    String uiProcessBundleIdentifier;
    uint32_t uiProcessSDKVersion { 0 };
    RetainPtr<CFDataRef> networkATSContext;
    bool storageAccessAPIEnabled;
#endif

    WebsiteDataStoreParameters defaultDataStoreParameters;

    PurCFetcher::HTTPCookieAcceptPolicy cookieAcceptPolicy { PurCFetcher::HTTPCookieAcceptPolicy::AlwaysAccept };
    bool ignoreTLSErrors { false };
    Vector<String> languages;
    PurCFetcher::NetworkProxySettings proxySettings;

    Vector<String> urlSchemesRegisteredAsSecure;
    Vector<String> urlSchemesRegisteredAsBypassingContentSecurityPolicy;
    Vector<String> urlSchemesRegisteredAsLocal;
    Vector<String> urlSchemesRegisteredAsNoAccess;

#if ENABLE(SERVICE_WORKER)
    String serviceWorkerRegistrationDirectory;
    SandboxExtension::Handle serviceWorkerRegistrationDirectoryExtensionHandle;
    bool shouldDisableServiceWorkerProcessTerminationDelay { false };
#endif
    bool shouldEnableITPDatabase { false };
    bool enableAdClickAttributionDebugMode { false };
    String hstsStorageDirectory;
    SandboxExtension::Handle hstsStorageDirectoryExtensionHandle;
};

} // namespace PurCFetcher