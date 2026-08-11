/*
 * Copyright (C) 2025 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <aidl/android/hardware/security/see/hwcrypto/BnHwCryptoKey.h>
#include <aidl/android/hardware/security/see/hwcrypto/IHwCryptoKey.h>
#include <aidl/android/hardware/security/see/hwcrypto/types/HalErrorCode.h>
#include <android-base/logging.h>
#include <android-base/result.h>
#include <android/hardware/security/see/hwcrypto/IHwCryptoKey.h>
#include <binder/RpcSession.h>

// We use cpp interfaces to talk to Trusty, and ndk interfaces for the platform
namespace cpp_hwcrypto = android::hardware::security::see::hwcrypto;
namespace ndk_hwcrypto = aidl::android::hardware::security::see::hwcrypto;

namespace android {
namespace trusty {
namespace hwcryptohalservice {

class HwCryptoKey : public ndk_hwcrypto::BnHwCryptoKey {
  private:
    sp<cpp_hwcrypto::IHwCryptoKey> mHwCryptoServer;
    sp<IBinder> mRoot;
    sp<RpcSession> mSession;
    android::base::Result<void> connectToTrusty(const char* tipcDev);

  public:
    HwCryptoKey();

    static std::shared_ptr<HwCryptoKey> Create(const char* tipcDev);

    ndk::ScopedAStatus deriveCurrentDicePolicyBoundKey(
            const ndk_hwcrypto::IHwCryptoKey::DiceBoundDerivationKey& derivationKey,
            ndk_hwcrypto::IHwCryptoKey::DiceCurrentBoundKeyResult* aidl_return) override;

    ndk::ScopedAStatus deriveDicePolicyBoundKey(
            const ndk_hwcrypto::IHwCryptoKey::DiceBoundDerivationKey& derivationKey,
            const ::std::vector<uint8_t>& dicePolicyForKeyVersion,
            ndk_hwcrypto::IHwCryptoKey::DiceBoundKeyResult* aidl_return) override;
    ndk::ScopedAStatus deriveKey(const ndk_hwcrypto::IHwCryptoKey::DerivedKeyParameters& parameters,
                                 ndk_hwcrypto::IHwCryptoKey::DerivedKey* aidl_return) override;

    ndk::ScopedAStatus getHwCryptoOperations(
            std::shared_ptr<ndk_hwcrypto::IHwCryptoOperations>* aidl_return) override;

    ndk::ScopedAStatus importClearKey(
            const ndk_hwcrypto::types::ExplicitKeyMaterial& keyMaterial,
            const ndk_hwcrypto::KeyPolicy& newKeyPolicy,
            std::shared_ptr<ndk_hwcrypto::IOpaqueKey>* aidl_return) override;

    ndk::ScopedAStatus getCurrentDicePolicy(std::vector<uint8_t>* aidl_return) override;

    ndk::ScopedAStatus keyTokenImport(
            const ndk_hwcrypto::types::OpaqueKeyToken& requestedKey,
            const ::std::vector<uint8_t>& sealingDicePolicy,
            std::shared_ptr<ndk_hwcrypto::IOpaqueKey>* aidl_return) override;

    ndk::ScopedAStatus getKeyslotData(
            ndk_hwcrypto::IHwCryptoKey::KeySlot slotId,
            std::shared_ptr<ndk_hwcrypto::IOpaqueKey>* aidl_return) override;
};

template <typename LHP, typename RHP>
LHP convertKeyPolicy(const RHP& policyToConvert) {
    LHP policy = LHP();
    policy.usage = static_cast<decltype(policy.usage)>(policyToConvert.usage);
    policy.keyLifetime = static_cast<decltype(policy.keyLifetime)>(policyToConvert.keyLifetime);
    policy.keyType = static_cast<decltype(policy.keyType)>(policyToConvert.keyType);
    policy.keyManagementKey = policyToConvert.keyManagementKey;
    policy.keyPermissions.reserve(policyToConvert.keyPermissions.size());
    for (auto permission : policyToConvert.keyPermissions) {
        policy.keyPermissions.push_back(
                std::move(static_cast<decltype(policy.keyPermissions)::value_type>(permission)));
    }
    return policy;
}

template <typename CPP, typename NDK,
          std::map<std::weak_ptr<NDK>, wp<CPP>, std::owner_less<>>& mapping>
sp<CPP> retrieveCppBinder(const std::shared_ptr<NDK>& ndkBinder) {
    if (ndkBinder == nullptr) {
        return nullptr;
    }
    if (mapping.find(ndkBinder) == mapping.end()) {
        LOG(ERROR) << "couldn't find wrapped key";
        return nullptr;
    }
    auto cppBbinder = mapping[ndkBinder];
    return cppBbinder.promote();
}

template <typename CPP_BINDER, typename NDK_BINDER, typename NDK_BASE,
          std::map<std::weak_ptr<NDK_BINDER>, wp<CPP_BINDER>, std::owner_less<>>& mapping>
void insertBinderMapping(const sp<CPP_BINDER>& cppBinder, std::shared_ptr<NDK_BINDER>* ndkBinder) {
    std::shared_ptr<NDK_BINDER> spNdkBinder = NDK_BASE::Create(cppBinder);
    std::weak_ptr<NDK_BINDER> wptrNdkBinder = spNdkBinder;
    wp<CPP_BINDER> wpCppBinder = cppBinder;
    mapping.insert({wptrNdkBinder, wpCppBinder});
    *ndkBinder = spNdkBinder;
}

}  // namespace hwcryptohalservice
}  // namespace trusty
}  // namespace android
