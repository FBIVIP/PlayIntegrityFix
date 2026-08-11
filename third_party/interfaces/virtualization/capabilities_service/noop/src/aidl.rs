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

//! No-op implementation of the IVmCapabilitiesService AIDL interface.

use android_hardware_virtualization_capabilities_capabilities_service::aidl::android::hardware::virtualization::capabilities::IVmCapabilitiesService::IVmCapabilitiesService;
use binder::{Interface, ParcelFileDescriptor};
use log::info;

/// No-op implementation of IVmCapabilitiesService
pub struct NoOpVmCapabilitiesService {}

impl NoOpVmCapabilitiesService {
    pub fn init() -> NoOpVmCapabilitiesService {
        let service = NoOpVmCapabilitiesService {};
        service
    }
}

impl Interface for NoOpVmCapabilitiesService {}

impl IVmCapabilitiesService for NoOpVmCapabilitiesService {

    fn grantAccessToVendorTeeServices(&self,
        vm_fd: &ParcelFileDescriptor, tee_services: &[String]) -> binder::Result<()> {
        info!("received {vm_fd:?} {tee_services:?}");
        Ok(())
    }
}
