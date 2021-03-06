// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_MULTIDEVICE_CRYPTAUTH_CLIENT_FACTORY_IMPL
#define COMPONENTS_MULTIDEVICE_CRYPTAUTH_CLIENT_FACTORY_IMPL

#include "base/memory/ref_counted.h"
#include "components/cryptauth/cryptauth_client.h"
#include "components/cryptauth/proto/cryptauth_api.pb.h"

namespace identity {
class IdentityManager;
}  // namespace identity

namespace net {
class URLRequestContextGetter;
}  // namespace net

namespace multidevice {

// CryptAuthClientFactory implementation which utilizes IdentityManager.
class CryptAuthClientFactoryImpl : public cryptauth::CryptAuthClientFactory {
 public:
  CryptAuthClientFactoryImpl(
      identity::IdentityManager* identity_manager,
      scoped_refptr<net::URLRequestContextGetter> url_request_context,
      const cryptauth::DeviceClassifier& device_classifier);
  ~CryptAuthClientFactoryImpl() override;

  // cryptauth::CryptAuthClientFactory:
  std::unique_ptr<cryptauth::CryptAuthClient> CreateInstance() override;

 private:
  identity::IdentityManager* identity_manager_;
  const scoped_refptr<net::URLRequestContextGetter> url_request_context_;
  const cryptauth::DeviceClassifier device_classifier_;
};

}  // namespace multidevice

#endif  // COMPONENTS_MULTIDEVICE_CRYPTAUTH_CLIENT_FACTORY_IMPL
