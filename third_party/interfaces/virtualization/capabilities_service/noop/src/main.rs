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

//! No-op implementation of the IVmCapabilitiesService.

mod aidl;

use crate::aidl::NoOpVmCapabilitiesService;
use anyhow::{bail, Context, Result};
use log::{error, info, LevelFilter};
use binder::{register_lazy_service, BinderFeatures, ProcessState};
use android_hardware_virtualization_capabilities_capabilities_service::aidl::android::hardware::virtualization::capabilities::IVmCapabilitiesService::BnVmCapabilitiesService;

const SERVICE_NAME: &str = "android.hardware.virtualization.capabilities.IVmCapabilitiesService/noop";

fn try_main() -> Result<()> {
    // Initialize Android logging.
    android_logger::init_once(
        android_logger::Config::default()
            .with_tag("NoOpIVmCapabilitiesService")
            .with_max_level(LevelFilter::Info)
            .with_log_buffer(android_logger::LogId::System),
    );

    ProcessState::start_thread_pool();
    let service_impl = NoOpVmCapabilitiesService::init();
    let service = BnVmCapabilitiesService::new_binder(service_impl, BinderFeatures::default());
    register_lazy_service(SERVICE_NAME, service.as_binder())
        .with_context(|| format!("failed to register {SERVICE_NAME}"))?;
    info!("Registered Binder service, joining threadpool.");
    ProcessState::join_thread_pool();
    bail!("thread pool unexpectedly ended");
}

fn main() {
    if let Err(e) = try_main() {
        error!("failed with {e:?}");
        std::process::exit(1);
    }
}
