// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_GPU_WINDOWS_D3D11_CDM_PROXY_H_
#define MEDIA_GPU_WINDOWS_D3D11_CDM_PROXY_H_

#include "media/cdm/cdm_proxy.h"

#include <d3d11_1.h>
#include <wrl/client.h>

#include <map>
#include <vector>

#include "base/callback.h"
#include "media/gpu/media_gpu_export.h"

namespace media {

// This is a CdmProxy implementation that uses D3D11.
class MEDIA_GPU_EXPORT D3D11CdmProxy : public CdmProxy {
 public:
  using FunctionIdMap = std::map<Function, uint32_t>;
  // The signature matches D3D11CreateDevice(). decltype(D3D11CreateDevice) does
  // not work because __attribute__((stdcall)) gets appended, and the template
  // instantiation fails.
  using CreateDeviceCB =
      base::RepeatingCallback<HRESULT(IDXGIAdapter*,
                                      D3D_DRIVER_TYPE,
                                      HMODULE,
                                      UINT,
                                      const D3D_FEATURE_LEVEL*,
                                      UINT,
                                      UINT,
                                      ID3D11Device**,
                                      D3D_FEATURE_LEVEL*,
                                      ID3D11DeviceContext**)>;

  // |crypto_type| is the ID that should be using to do crypto session
  // operations. This includes creating a crypto session with
  // ID3D11VideoDevice::CreateCryptoSession(). In other words this is the
  // value passed to D3D11 functions that take 'pCryptoType'.
  // |protocol| determines what protocol this is operating in. This value is
  // passed to callbacks that require a protocol enum value.
  // |function_id_map| maps Function enum to an integer.
  D3D11CdmProxy(const GUID& crypto_type,
                CdmProxy::Protocol protocol,
                const FunctionIdMap& function_id_map);
  ~D3D11CdmProxy() override;

  // CdmProxy implementation.
  base::WeakPtr<CdmContext> GetCdmContext() override;
  void Initialize(Client* client, InitializeCB init_cb) override;
  void Process(Function function,
               uint32_t crypto_session_id,
               const std::vector<uint8_t>& input_data,
               uint32_t expected_output_data_size,
               ProcessCB process_cb) override;
  void CreateMediaCryptoSession(
      const std::vector<uint8_t>& input_data,
      CreateMediaCryptoSessionCB create_media_crypto_session_cb) override;
  void SetKey(uint32_t crypto_session_id,
              const std::vector<uint8_t>& key_id,
              const std::vector<uint8_t>& key_blob) override;
  void RemoveKey(uint32_t crypto_session_id,
                 const std::vector<uint8_t>& key_id) override;

  void SetCreateDeviceCallbackForTesting(CreateDeviceCB callback);

 private:
  template <typename T>
  using ComPtr = Microsoft::WRL::ComPtr<T>;

  // A structure to keep the data passed to SetKey(). See documentation for
  // SetKey() for what the fields mean.
  // TODO(rkuroiwa): Move this to D3D11CdmContext (or whatever class that would
  // be added) that this class will inherit to provide the key information to
  // the decoder.
  struct KeyInfo {
    KeyInfo();
    KeyInfo(uint32_t crypto_session_id,
            std::vector<uint8_t> key_id,
            std::vector<uint8_t> key_blob);
    KeyInfo(const KeyInfo&);
    ~KeyInfo();
    uint32_t crypto_session_id;
    std::vector<uint8_t> key_id;
    std::vector<uint8_t> key_blob;
  };

  const GUID stream_id_;
  const CdmProxy::Protocol protocol_;
  const FunctionIdMap function_id_map_;

  // Implmenenting this class does not require this to be a callback. But in
  // order to inject D3D11CreateDevice() function for testing, this member is
  // required. The test will replace this with a function that returns a mock
  // devices.
  CreateDeviceCB create_device_func_;

  // Counter for assigning IDs to crypto sessions.
  uint32_t next_crypto_session_id_ = 1;

  Client* client_ = nullptr;
  bool initialized_ = false;

  // These ComPtrs are refcounted pointers.
  // https://msdn.microsoft.com/en-us/library/br244983.aspx
  ComPtr<ID3D11Device> device_;
  ComPtr<ID3D11DeviceContext> device_context_;
  // TODO(crbug.com/788880): Remove ID3D11VideoDevice and ID3D11VideoContext if
  // they are not required.
  ComPtr<ID3D11VideoDevice> video_device_;
  ComPtr<ID3D11VideoDevice1> video_device1_;
  ComPtr<ID3D11VideoContext> video_context_;
  ComPtr<ID3D11VideoContext1> video_context1_;

  // Crypto session ID -> actual crypto session.
  std::map<uint32_t, ComPtr<ID3D11CryptoSession>> crypto_session_map_;

  // The values output from ID3D11VideoDevice1::GetCryptoSessionPrivateDataSize.
  // Used when calling NegotiateCryptoSessionKeyExchange.
  UINT private_input_size_ = 0;
  UINT private_output_size_ = 0;

  // Maps key ID to KeyInfo.
  std::map<std::vector<uint8_t>, KeyInfo> key_info_map_;

  DISALLOW_COPY_AND_ASSIGN(D3D11CdmProxy);
};

}  // namespace media

#endif  // MEDIA_GPU_WINDOWS_D3D11_CDM_PROXY_H_
