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

#define LOG_TAG "FaceConfig"

#include "FaceConfig.h"

#include <android-base/logging.h>

#include <face.sysprop.h>

using namespace ::android::face::virt;

namespace aidl::android::hardware::biometrics::face {

// Wrapper to system property access functions
#define CREATE_GETTER_SETTER_WRAPPER(_NAME_, _T_)           \
    ConfigValue _NAME_##Getter() {                          \
        return FaceHalProperties::_NAME_();                 \
    }                                                       \
    bool _NAME_##Setter(const ConfigValue& v) {             \
        return FaceHalProperties::_NAME_(std::get<_T_>(v)); \
    }

CREATE_GETTER_SETTER_WRAPPER(type, OptString)
CREATE_GETTER_SETTER_WRAPPER(enrollments, OptIntVec)
CREATE_GETTER_SETTER_WRAPPER(enrollment_hit, OptInt32)
CREATE_GETTER_SETTER_WRAPPER(next_enrollment, OptString)
CREATE_GETTER_SETTER_WRAPPER(authenticator_id, OptInt64)
CREATE_GETTER_SETTER_WRAPPER(challenge, OptInt64)
CREATE_GETTER_SETTER_WRAPPER(strength, OptString)
CREATE_GETTER_SETTER_WRAPPER(operation_authenticate_fails, OptBool)
CREATE_GETTER_SETTER_WRAPPER(operation_authenticate_latency, OptIntVec)
CREATE_GETTER_SETTER_WRAPPER(operation_authenticate_duration, OptInt32)
CREATE_GETTER_SETTER_WRAPPER(operation_authenticate_error, OptInt32)
CREATE_GETTER_SETTER_WRAPPER(operation_authenticate_acquired, OptString)
CREATE_GETTER_SETTER_WRAPPER(operation_enroll_latency, OptIntVec)
CREATE_GETTER_SETTER_WRAPPER(operation_detect_interaction_fails, OptBool)
CREATE_GETTER_SETTER_WRAPPER(operation_detect_interaction_latency, OptIntVec)
CREATE_GETTER_SETTER_WRAPPER(lockout, OptBool)
CREATE_GETTER_SETTER_WRAPPER(lockout_enable, OptBool)
CREATE_GETTER_SETTER_WRAPPER(lockout_timed_enable, OptBool)
CREATE_GETTER_SETTER_WRAPPER(lockout_timed_threshold, OptInt32)
CREATE_GETTER_SETTER_WRAPPER(lockout_timed_duration, OptInt32)
CREATE_GETTER_SETTER_WRAPPER(lockout_permanent_threshold, OptInt32)
CREATE_GETTER_SETTER_WRAPPER(features, OptIntVec)

// Name, Getter, Setter, Parser and default value
#define NGS(_NAME_) #_NAME_, _NAME_##Getter, _NAME_##Setter
static Config::Data configData[] = {
        {NGS(type), &Config::parseString, "rgb"},
        {NGS(enrollments), &Config::parseIntVec, ""},
        {NGS(enrollment_hit), &Config::parseInt32, "0"},
        {NGS(next_enrollment), &Config::parseString,
         "1:1000-[21,7,1,1103],1500-[1108,1],2000-[1113,1],2500-[1118,1]:true"},
        {NGS(authenticator_id), &Config::parseInt64, "0"},
        {NGS(challenge), &Config::parseInt64, ""},
        {NGS(strength), &Config::parseString, "strong"},
        {NGS(operation_authenticate_fails), &Config::parseBool, "false"},
        {NGS(operation_authenticate_latency), &Config::parseIntVec, ""},
        {NGS(operation_authenticate_duration), &Config::parseInt32, "500"},
        {NGS(operation_authenticate_error), &Config::parseInt32, "0"},
        {NGS(operation_authenticate_acquired), &Config::parseString, ""},
        {NGS(operation_enroll_latency), &Config::parseIntVec, ""},
        {NGS(operation_detect_interaction_latency), &Config::parseIntVec, ""},
        {NGS(operation_detect_interaction_fails), &Config::parseBool, "false"},
        {NGS(lockout), &Config::parseBool, "false"},
        {NGS(lockout_enable), &Config::parseBool, "false"},
        {NGS(lockout_timed_enable), &Config::parseBool, "false"},
        {NGS(lockout_timed_threshold), &Config::parseInt32, "3"},
        {NGS(lockout_timed_duration), &Config::parseInt32, "10000"},
        {NGS(lockout_permanent_threshold), &Config::parseInt32, "5"},
        {NGS(features), &Config::parseIntVec, ""}};

Config::Data* FaceConfig::getConfigData(int* size) {
    *size = sizeof(configData) / sizeof(configData[0]);
    return configData;
}

}  // namespace aidl::android::hardware::biometrics::face
