/*
 * Copyright (C) 2024 The Android Open Source Project
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

//! VTS test library for HwCrypto functionality.
//! It provides the base clases necessaries to write HwCrypto VTS tests

use anyhow::Result;
use android_hardware_security_see_hwcrypto::aidl::android::hardware::security::see::hwcrypto::IHwCryptoKey::IHwCryptoKey;

pub const HWCRYPTO_SERVICE: &str = "android.hardware.security.see.hwcrypto.IHwCryptoKey";

/// Get a HwCryptoKey binder service object using the service manager
pub fn get_hwcryptokey() -> Result<binder::Strong<dyn IHwCryptoKey>, binder::Status> {
    let interface_name = HWCRYPTO_SERVICE.to_owned() + "/default";
    Ok(binder::get_interface(&interface_name)?)
}

pub fn get_supported_instances() -> Vec<(String, String)> {
    // Determine which instances are available.
    binder::get_declared_instances(HWCRYPTO_SERVICE)
        .unwrap_or_default()
        .into_iter()
        .map(|v| (v.clone(), v))
        .collect()
}

pub fn ignore_test() -> bool {
    let instances = get_supported_instances();
    instances.len() == 0
}
