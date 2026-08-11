/*
 * Copyright (C) 2020 The Android Open Source Project
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

#include "Fingerprint.h"
#include "VirtualHal.h"

#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

using aidl::android::hardware::biometrics::fingerprint::Fingerprint;
using aidl::android::hardware::biometrics::fingerprint::VirtualHal;

int main(int argc, char** argv) {
    if (argc < 2) {
        LOG(ERROR) << "Missing argument -> exiting";
        return EXIT_FAILURE;
    }

    LOG(INFO) << "Fingerprint HAL started: " << argv[1];
    ABinderProcess_setThreadPoolMaxThreadCount(0);
    std::shared_ptr<Fingerprint> hal = ndk::SharedRefBase::make<Fingerprint>();
    std::shared_ptr<VirtualHal> hal_vhal = ndk::SharedRefBase::make<VirtualHal>(hal);

    if (hal->connected()) {
        if (strcmp(argv[1], "default") == 0) {
            const std::string instance = std::string(Fingerprint::descriptor) + "/default";
            auto binder = hal->asBinder();
            binder_status_t status =
                    AServiceManager_registerLazyService(binder.get(), instance.c_str());
            CHECK_EQ(status, STATUS_OK);
            LOG(INFO) << "started IFingerprint/default";
        } else if (strcmp(argv[1], "virtual") == 0) {
            const std::string instance = std::string(VirtualHal::descriptor) + "/virtual";
            auto binder = hal_vhal->asBinder();
            binder_status_t status =
                    AServiceManager_registerLazyService(binder.get(), instance.c_str());
            CHECK_EQ(status, STATUS_OK);
            LOG(INFO) << "started IVirtualHal/virtual";
        } else {
            LOG(ERROR) << "Unexpected argument: " << argv[1];
            return EXIT_FAILURE;
        }
        AServiceManager_forceLazyServicesPersist(true);
    } else {
        LOG(ERROR) << "Fingerprint HAL is not connected";
    }

    ABinderProcess_joinThreadPool();
    return EXIT_FAILURE;  // should not reach
}
