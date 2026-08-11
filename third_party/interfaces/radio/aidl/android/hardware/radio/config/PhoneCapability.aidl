/*
 * Copyright (C) 2021 The Android Open Source Project
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

package android.hardware.radio.config;

/**
 * Phone capability which describes the data connection capability of the modem.
 * It's used to evaluate a possible phone config change, for example, from single
 * SIM device to multi-SIM device.
 * @hide
 */
@VintfStability
@JavaDerive(toString=true)
parcelable PhoneCapability {
    const byte UNKNOWN = -1;
    /**
     * maxActiveData defines how many logical modems can have PS attached simultaneously. For
     * example, for a L+L modem, it should be 2.
     */
    byte maxActiveData;
    /**
     * maxActiveInternetData defines how many logical modems can have internet PDN connections
     * simultaneously. For example, for a L+L DSDS modem, it’s 1, and for a DSDA modem, it’s 2.
     */
    byte maxActiveInternetData;
    /**
     * Whether the modem supports both internet PDNs up, so that we can do a ping test on one PDN
     * before tearing down the other PDN.
     */
    boolean isInternetLingeringSupported;
    /**
     * List of logical modem IDs.
     */
    byte[] logicalModemIds;
    /**
     * maxActiveVoice defines how many logical modems can have cellular voice calls simultaneously.
     * For example, for cellular DSDA with simultaneous calling support, it should be 2.
     */
    byte maxActiveVoice = UNKNOWN;
}
