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

#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <getopt.h>
#include <string>
#include "hwcryptokeyimpl.h"

static void showUsageAndExit(int code) {
    LOG(ERROR) << "usage: android.hardware.trusty.hwcryptohal-service -d <trusty_dev>";
    exit(code);
}

static void parseDeviceName(int argc, char* argv[], char*& device_name) {
    static const char* _sopts = "h:d:";
    static const struct option _lopts[] = {{"help", no_argument, nullptr, 'h'},
                                           {"trusty_dev", required_argument, nullptr, 'd'},
                                           {0, 0, 0, 0}};
    int opt;
    int oidx = 0;

    while ((opt = getopt_long(argc, argv, _sopts, _lopts, &oidx)) != -1) {
        switch (opt) {
            case 'd':
                device_name = strdup(optarg);
                break;
            case 'h':
                showUsageAndExit(EXIT_SUCCESS);
                break;
            default:
                LOG(ERROR) << "unrecognized option: " << opt;
                showUsageAndExit(EXIT_FAILURE);
        }
    }

    if (device_name == nullptr) {
        LOG(ERROR) << "missing required argument(s)";
        showUsageAndExit(EXIT_FAILURE);
    }

    LOG(INFO) << "starting android.hardware.trusty.hwcryptohal-service";
    LOG(INFO) << "trusty dev: " << device_name;
}

int main(int argc, char* argv[]) {
    char* device_name;
    parseDeviceName(argc, argv, device_name);

    auto hwCryptoServer = android::trusty::hwcryptohalservice::HwCryptoKey::Create(device_name);
    if (hwCryptoServer == nullptr) {
        LOG(ERROR) << "couldn't create hwcrypto service";
        exit(EXIT_FAILURE);
    }
    ABinderProcess_setThreadPoolMaxThreadCount(0);
    const std::string instance =
            std::string() + ndk_hwcrypto::IHwCryptoKey::descriptor + "/default";
    binder_status_t status =
            AServiceManager_addService(hwCryptoServer->asBinder().get(), instance.c_str());
    if (status != STATUS_OK) {
        LOG(ERROR) << "couldn't register hwcrypto service";
    }
    CHECK_EQ(status, STATUS_OK);
    ABinderProcess_joinThreadPool();

    return 0;
}
