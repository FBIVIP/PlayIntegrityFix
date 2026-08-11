/*
 * Copyright (C) 2022 The Android Open Source Project
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

package android.hardware.audio.effect;

import android.hardware.audio.effect.VendorExtension;

/**
 * HapticGenerator specific definitions. HapticGenerator effect provide HapticGenerator control and
 * mute/unmute functionality.
 *
 * All parameter settings must be inside the range of Capability.Range.hapticGenerator definition if
 * the definition for the corresponding parameter tag exist. See more detals about Range in
 * Range.aidl.
 */
@VintfStability
union HapticGenerator {
    /**
     * Effect parameter tag to identify the parameters for getParameter().
     */
    @VintfStability
    union Id {
        VendorExtension vendorExtensionTag;
        HapticGenerator.Tag commonTag;
    }

    /**
     * Vendor HapticGenerator implementation definition for additional parameters.
     */
    VendorExtension vendor;

    @VintfStability
    @Backing(type="int")
    enum VibratorScale {
        MUTE = -100,
        VERY_LOW = -2,
        LOW = -1,
        NONE = 0,
        HIGH = 1,
        VERY_HIGH = 2,
    }

    @VintfStability
    parcelable HapticScale {
        /**
         * Representation of undefined scale factor, applied by default for backwards compatibility.
         */
        const float UNDEFINED_SCALE_FACTOR = -1.0f;

        /**
         * Audio track ID.
         */
        int id;

        /**
         * Haptic intensity.
         *
         * This represents haptics scale as fixed levels defined by VibrationScale. If the field
         * scaleFactor is defined then this will be ignored in favor of scaleFactor, otherwise this
         * will be used to define the intensity for the haptics.
         */
        VibratorScale scale = VibratorScale.MUTE;

        /**
         * Haptic scale factor.
         *
         * This is a continuous scale representation of VibratorScale, allowing flexible number of
         * scale levels. If this field is defined then it will be used to define the intensity of
         * the haptics, instead of the old VibratorScale field. If this field is undefined then the
         * old VibratorScale field will be used.
         *
         * The value zero represents the same as VibratorScale.MUTE and the value one represents
         * VibratorScale.NONE. Values in (0,1) should scale down, and values > 1 should scale up
         * within hardware bounds. Negative values will be ignored.
         */
        float scaleFactor = -1.0f; // UNDEFINED_SCALE_FACTOR

        /**
         * Haptic adaptive scale factor.
         *
         * This is an additional scale value that should be applied on top of the vibrator scale to
         * adapt to the device current state. This should be applied to linearly scale the haptic
         * data after scale/scaleFactor is applied.
         *
         * The value zero mutes the haptics, even if the scale/scaleFactor are not set to MUTE/zero.
         * The value one will not scale the haptics, and can be used as a constant for no-op.
         * Values in (0,1) should scale down. Values > 1 should scale up within hardware bounds.
         * Negative values will be ignored.
         */
        float adaptiveScaleFactor = -1.0f; // UNDEFINED_SCALE_FACTOR
    }

    /**
     * Vibrator information including resonant frequency, Q factor.
     */
    @VintfStability
    parcelable VibratorInformation {
        /**
         * Resonant frequency in Hz.
         */
        float resonantFrequencyHz;
        float qFactor;
        float maxAmplitude;
    }

    HapticScale[] hapticScales;
    VibratorInformation vibratorInfo;
}
