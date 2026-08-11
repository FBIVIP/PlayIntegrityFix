/*
 * Copyright 2024 The Android Open Source Project
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
package android.hardware.security.see.hwcrypto;

import android.hardware.security.see.hwcrypto.CryptoOperationErrorAdditionalInfo;
import android.hardware.security.see.hwcrypto.CryptoOperationResult;
import android.hardware.security.see.hwcrypto.CryptoOperationSet;

/*
 * Interface used that provides cryptographic services, including the generation and use of
 * cryptographic keys. Interactions with this interface are done through a command-base API,
 * which allow callers to execute a large set of operations on a single call.
 */
@VintfStability
interface IHwCryptoOperations {
    /*
     * Executes a list of cryptographic commands in order
     *
     * @param operations:
     *      Parameter containing 1 or more set of commands to execute. Additionally, each set can
     *      also contain a context on which the commands will be executed. The parameter has type
     *      inout because it can contain buffers used to write the output of the operation.
     *
     * @return:
     *      CryptoOperationResult[] on success, which can contain a context to continue executing
     *      each of the provided operations sets.
     *
     * @throws:
     *      ServiceSpecificException based on <code>HalErrorCode</code> if any error occurs,
     *      in particular:
     *          - INVALID_KEY if the provided key is not compatible with the operation requested.
     *          - BAD_STATE if the provided <code>CryptoOperationSet</code> contains operations that
     *            cannot be carried out in the current server state.
     *          - UNSUPPORTED if the requested operation is not supported by the server.
     *          - ALLOCATION_ERROR if the system runs out of memory while carring out the operation.
     */
    CryptoOperationResult[] processCommandList(inout CryptoOperationSet[] operations);
}
