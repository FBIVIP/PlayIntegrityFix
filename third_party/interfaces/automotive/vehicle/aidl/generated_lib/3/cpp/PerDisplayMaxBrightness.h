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

#pragma once

#include <aidl/android/hardware/automotive/vehicle/VehicleProperty.h>

namespace aidl::android::hardware::automotive::vehicle {

// Same as VehicleProperty::PER_DISPLAY_MAX_BRIGHTNESS as defined in v4.
static constexpr VehicleProperty PER_DISPLAY_MAX_BRIGHTNESS = (VehicleProperty)0x11410F4E;

}  // namespace aidl::android::hardware::automotive::vehicle
