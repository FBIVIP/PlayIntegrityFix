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

use android_hardware_virtualization_capabilities_capabilities_service::aidl::android::hardware::virtualization::capabilities::IVmCapabilitiesService::IVmCapabilitiesService;
use rdroidtest::rdroidtest;
use std::fs::File;

const VM_CAPABILITIES_SERVICE: &str =
    "android.hardware.virtualization.capabilities.IVmCapabilitiesService";

/// Returns all available instances of VmCapabilitiesService.
/// Note: it actually returns a pair of (<instance_name>, <instance_name)). This is a requirement
/// of the rdroidtest framework for parameterized tests. See
/// platform_testing/libraries/rdroidtest/README.md for more information.
fn get_instances() -> Vec<(String, String)> {
    binder::get_declared_instances(VM_CAPABILITIES_SERVICE)
        .unwrap_or_default()
        .into_iter()
        .map(|v| (v.clone(), v))
        .collect()
}

fn connect(instance: &str) -> binder::Strong<dyn IVmCapabilitiesService> {
    let name = format!("{VM_CAPABILITIES_SERVICE}/{instance}");
    binder::wait_for_interface(&name).unwrap()
}

/// A very basic test that simply connects to the service and send bogus data.
#[rdroidtest(get_instances())]
fn connect_to_service(instance: String) {
    let service = connect(&instance);
    let dev_null = File::open("/dev/null").expect("failed to open /dev/null");
    let fd = binder::ParcelFileDescriptor::new(dev_null);
    // In this test we don't care what service returns.
    let _ = service.grantAccessToVendorTeeServices(&fd, &[]);
}

rdroidtest::test_main!();
