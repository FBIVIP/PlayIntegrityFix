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
 * Visualizer specific definitions. Visualizer enables application to retrieve part of the currently
 * playing audio for visualization purpose. The output is identical to the input, while
 * visualization data is generated separately based on scalingMode and captureSamples parameter
 * values.
 *
 * All parameter settings must be inside the range of Capability.Range.visualizer definition if the
 * definition for the corresponding parameter tag exist. See more details about Range in Range.aidl.
 *
 */
@VintfStability
union Visualizer {
    /**
     * Effect parameter tag to identify the parameters for getParameter().
     */
    @VintfStability
    union Id {
        VendorExtension vendorExtensionTag;
        Visualizer.Tag commonTag;
    }
    Id id;

    /**
     * Vendor Visualizer implementation definition for additional parameters.
     */
    VendorExtension vendor;

    /**
     * Type of scaling applied on the captured visualization data.
     */
    @VintfStability
    enum ScalingMode {
        /**
         * Defines a capture mode where amplification is applied based on the content of the
         * captured data in order to normalize it to the unsigned 8 bit sample range. This is the
         * default Visualizer mode, and is suitable for music visualization.
         *
         * For example,
         * Input Range:[-0.5, 0.5] -> Visualization Data Range:[0, 255]
         * Input Range:[-1,1]      -> Visualization Data Range:[0, 255]
         *
         */
        NORMALIZED = 0,
        /**
         * Defines a capture mode where no additional scaling is done on the input audio data thus
         * the visualization data remains as close as possible to the input. The visualization
         * directly reflects the actual loudness and waveform shape, rather than fitting everything
         * into a normalized visual range.
         *
         * For example,
         * Input Range:[-0.5, 0.5] -> Visualization Data Range:[64, 192]
         * Input Range:[-1,1]      -> Visualization Data Range:[0, 255]
         */
        AS_PLAYED,
    }

    /**
     * Measurement modes to be performed.
     */
    @VintfStability
    enum MeasurementMode {
        /**
         * No measurements are performed.
         */
        NONE = 0,
        /**
         * Defines a measurement mode which computes the peak and RMS value in mB below the "full
         * scale", where 0mB is normally the maximum sample value (but see the note below). Minimum
         * value depends on the resolution of audio samples used by the audio framework. The value
         * of -9600mB is the minimum value for 16-bit audio systems and -14400mB or below for "high
         * resolution" systems. Values for peak and RMS can be retrieved with {@link
         * #getMeasurementPeakRms(MeasurementPeakRms)}.
         */
        PEAK_RMS,
    }

    /**
     * Get only parameter to get the current measurements.
     */
    @VintfStability
    parcelable Measurement {
        int rms;
        int peak;
    }
    Measurement measurement;

    /**
     * Get only parameter to get the latest captured samples of PCM samples (8 bits per sample). It
     * represents the visualization data. The capture is intended for passing to applications, and
     * it contains the same audio data as the input, but with intentionally lower sample resolution,
     * and optionally normalized, depending on the scalingMode.
     */
    byte[] captureSampleBuffer;

    /**
     * Used by framework to inform the visualizer about the downstream latency (audio hardware
     * driver estimated latency in milliseconds).
     *
     * Visualizer implementation must use Range.VisualizerRange to define the range of supported
     * latency.
     */
    int latencyMs;

    /**
     * Current capture size in number of samples.
     *
     * Visualizer implementation must use Range.VisualizerRange to define the range of supported
     * capture size.
     */
    int captureSamples;

    /**
     * Visualizer capture mode
     */
    ScalingMode scalingMode;

    /**
     * Visualizer measurement mode.
     */
    MeasurementMode measurementMode;
}
