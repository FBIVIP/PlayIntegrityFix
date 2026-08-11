/*
 * Copyright (C) 2024 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
#include <fuzzer/FuzzedDataProvider.h>
#include <remote_prov/remote_prov_utils.h>

namespace android::hardware::security::keymint_support::fuzzer {

using namespace aidl::android::hardware::security::keymint::remote_prov;

constexpr size_t kMaxBytes = 128;

class KeyMintRemoteKeyProvSupport {
  public:
    KeyMintRemoteKeyProvSupport(const uint8_t* data, size_t size) : mFdp(data, size) {}
    void process();

  private:
    FuzzedDataProvider mFdp;
};

void KeyMintRemoteKeyProvSupport::process() {
    while (mFdp.remaining_bytes()) {
        auto invokeProvAPI = mFdp.PickValueInArray<const std::function<void()>>({
                [&]() {
                    std::vector<uint8_t> eekId;
                    if (mFdp.ConsumeBool()) {
                        eekId = mFdp.ConsumeBytes<uint8_t>(kMaxBytes);
                    }
                    generateEekChain(mFdp.ConsumeIntegral<uint8_t>() /* supportedEekCurve */,
                                     mFdp.ConsumeIntegral<uint8_t>() /* length */, eekId);
                },
                [&]() { getProdEekChain(mFdp.ConsumeIntegral<uint8_t>() /* supportedEekCurve */); },
                [&]() {
                    std::string serialNoProp = mFdp.ConsumeRandomLengthString(kMaxBytes);
                    std::string instanceName = mFdp.ConsumeRandomLengthString(kMaxBytes);
                    cppbor::Array array;
                    array.add(mFdp.ConsumeIntegral<uint8_t>() /* value */);
                    jsonEncodeCsrWithBuild(instanceName, array, serialNoProp);
                },
        });
        invokeProvAPI();
    }
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    KeyMintRemoteKeyProvSupport keymintRKPSupport(data, size);
    keymintRKPSupport.process();
    return 0;
}

}  // namespace android::hardware::security::keymint_support::fuzzer
