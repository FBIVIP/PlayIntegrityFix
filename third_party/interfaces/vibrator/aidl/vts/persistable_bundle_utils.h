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
#ifndef VIBRATOR_HAL_PERSISTABLE_BUNDLE_UTILS_H
#define VIBRATOR_HAL_PERSISTABLE_BUNDLE_UTILS_H

#include <android/persistable_bundle_aidl.h>

#include <cstdlib>

namespace aidl {
namespace android {
namespace hardware {
namespace vibrator {
namespace testing {

using aidl::android::os::PersistableBundle;

namespace {

template <typename T>
T nextValue() {
    return static_cast<T>(std::rand());
}

template <>
std::string nextValue() {
    std::string str;
    uint8_t entryCount = nextValue<uint8_t>();
    for (uint8_t i = 0; i < entryCount; i++) {
        str.push_back(nextValue<char>());
    }
    return str;
}

template <typename T>
T nextValue(T limit) {
    assert(limit > 0);
    return static_cast<T>(std::rand()) / (static_cast<T>(RAND_MAX / limit));
}

template <typename T>
void fillVector(std::vector<T>* values) {
    uint8_t entryCount = nextValue<uint8_t>();
    for (uint8_t i = 0; i < entryCount; i++) {
        values->push_back(nextValue<T>());
    }
}

const std::vector<std::function<void(PersistableBundle*, const std::string&)>>
        sPersistableBundleSetters = {[](PersistableBundle* bundle, const std::string& key) -> void {
                                         bundle->putBoolean(key, nextValue<bool>());
                                     },
                                     [](PersistableBundle* bundle, const std::string& key) -> void {
                                         bundle->putInt(key, nextValue<int32_t>());
                                     },
                                     [](PersistableBundle* bundle, const std::string& key) -> void {
                                         bundle->putLong(key, nextValue<int64_t>());
                                     },
                                     [](PersistableBundle* bundle, const std::string& key) -> void {
                                         bundle->putDouble(key, nextValue<double>());
                                     },
                                     [](PersistableBundle* bundle, const std::string& key) -> void {
                                         bundle->putString(key, nextValue<std::string>());
                                     },
                                     [](PersistableBundle* bundle, const std::string& key) -> void {
                                         std::vector<bool> value;
                                         fillVector<bool>(&value);
                                         bundle->putBooleanVector(key, value);
                                     },
                                     [](PersistableBundle* bundle, const std::string& key) -> void {
                                         std::vector<int32_t> value;
                                         fillVector<int32_t>(&value);
                                         bundle->putIntVector(key, value);
                                     },
                                     [](PersistableBundle* bundle, const std::string& key) -> void {
                                         std::vector<int64_t> value;
                                         fillVector<int64_t>(&value);
                                         bundle->putLongVector(key, value);
                                     },
                                     [](PersistableBundle* bundle, const std::string& key) -> void {
                                         std::vector<double> value;
                                         fillVector<double>(&value);
                                         bundle->putDoubleVector(key, value);
                                     },
                                     [](PersistableBundle* bundle, const std::string& key) -> void {
                                         std::vector<std::string> value;
                                         fillVector<std::string>(&value);
                                         bundle->putStringVector(key, value);
                                     }};

}  // namespace

void fillBasicData(PersistableBundle* bundle) {
    bundle->putBoolean("test_bool", true);
    bundle->putInt("test_int", 2147483647);
    bundle->putLong("test_long", 2147483647L);
    bundle->putDouble("test_double", 1.23);
    bundle->putString("test_string", "test data");
    bundle->putBooleanVector("test_bool_vector", {true, false, false});
    bundle->putIntVector("test_int_vector", {1, 2, 3, 4});
    bundle->putLongVector("test_long_vector", {100L, 200L, 300L});
    bundle->putDoubleVector("test_double_vector", {1.1, 2.2});
    bundle->putStringVector("test_string_vector", {"test", "val"});
}

void fillRandomData(PersistableBundle* bundle) {
    uint8_t entryCount = nextValue<uint8_t>();
    for (uint8_t i = 0; i < entryCount; i++) {
        std::string key(nextValue<std::string>());
        uint8_t setterIdx = nextValue<uint8_t>(sPersistableBundleSetters.size() - 1);
        sPersistableBundleSetters[setterIdx](bundle, key);
    }
}

}  // namespace testing
}  // namespace vibrator
}  // namespace hardware
}  // namespace android
}  // namespace aidl

#endif  // VIBRATOR_HAL_PERSISTABLE_BUNDLE_UTILS_H
