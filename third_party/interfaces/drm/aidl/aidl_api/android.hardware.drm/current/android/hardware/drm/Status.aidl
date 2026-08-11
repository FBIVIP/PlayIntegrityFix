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
///////////////////////////////////////////////////////////////////////////////
// THIS FILE IS IMMUTABLE. DO NOT EDIT IN ANY CASE.                          //
///////////////////////////////////////////////////////////////////////////////

// This file is a snapshot of an AIDL file. Do not edit it manually. There are
// two cases:
// 1). this is a frozen version file - do not edit this in any case.
// 2). this is a 'current' file. If you make a backwards compatible change to
//     the interface (from the latest frozen version), the build system will
//     prompt you to update this file with `m <name>-update-api`.
//
// You must not make a backward incompatible change to any AIDL file built
// with the aidl_interface module type with versions property set. The module
// type is used to build AIDL files in a way that they can be used across
// independently updatable components of the system. If a device is shipped
// with such a backward incompatible change, it has a high risk of breaking
// later when a module using the interface is updated, e.g., Mainline modules.

package android.hardware.drm;
@Backing(type="int") @VintfStability
enum Status {
  OK,
  ERROR_DRM_NO_LICENSE,
  ERROR_DRM_LICENSE_EXPIRED,
  ERROR_DRM_SESSION_NOT_OPENED,
  ERROR_DRM_CANNOT_HANDLE,
  ERROR_DRM_INVALID_STATE,
  BAD_VALUE,
  ERROR_DRM_NOT_PROVISIONED,
  ERROR_DRM_RESOURCE_BUSY,
  ERROR_DRM_INSUFFICIENT_OUTPUT_PROTECTION,
  ERROR_DRM_DEVICE_REVOKED,
  ERROR_DRM_DECRYPT,
  ERROR_DRM_UNKNOWN,
  ERROR_DRM_INSUFFICIENT_SECURITY,
  ERROR_DRM_FRAME_TOO_LARGE,
  ERROR_DRM_SESSION_LOST_STATE,
  ERROR_DRM_RESOURCE_CONTENTION,
  CANNOT_DECRYPT_ZERO_SUBSAMPLES,
  CRYPTO_LIBRARY_ERROR,
  GENERAL_OEM_ERROR,
  GENERAL_PLUGIN_ERROR,
  INIT_DATA_INVALID,
  KEY_NOT_LOADED,
  LICENSE_PARSE_ERROR,
  LICENSE_POLICY_ERROR,
  LICENSE_RELEASE_ERROR,
  LICENSE_REQUEST_REJECTED,
  LICENSE_RESTORE_ERROR,
  LICENSE_STATE_ERROR,
  MALFORMED_CERTIFICATE,
  MEDIA_FRAMEWORK_ERROR,
  MISSING_CERTIFICATE,
  PROVISIONING_CERTIFICATE_ERROR,
  PROVISIONING_CONFIGURATION_ERROR,
  PROVISIONING_PARSE_ERROR,
  PROVISIONING_REQUEST_REJECTED,
  RETRYABLE_PROVISIONING_ERROR,
  SECURE_STOP_RELEASE_ERROR,
  STORAGE_READ_FAILURE,
  STORAGE_WRITE_FAILURE,
}
