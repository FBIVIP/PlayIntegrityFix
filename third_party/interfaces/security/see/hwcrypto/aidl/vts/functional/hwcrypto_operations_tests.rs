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

//! HwCryptoOperations tests.

use android_hardware_security_see_hwcrypto::aidl::android::hardware::security::see::hwcrypto::types::{
    AesKey::AesKey, ExplicitKeyMaterial::ExplicitKeyMaterial, KeyType::KeyType, KeyLifetime::KeyLifetime,
    KeyUse::KeyUse, OperationData::OperationData, HmacOperationParameters::HmacOperationParameters,
    SymmetricOperationParameters::SymmetricOperationParameters, SymmetricOperation::SymmetricOperation,
    HmacKey::HmacKey, CipherModeParameters::CipherModeParameters, AesCipherMode::AesCipherMode,
    SymmetricCryptoParameters::SymmetricCryptoParameters,
};
use android_hardware_security_see_hwcrypto::aidl::android::hardware::security::see::hwcrypto::{
    KeyPolicy::KeyPolicy,CryptoOperation::CryptoOperation,CryptoOperationSet::CryptoOperationSet,
    OperationParameters::OperationParameters, PatternParameters::PatternParameters,
};
use rdroidtest::{ignore_if, rdroidtest};

#[rdroidtest]
#[ignore_if(hwcryptohal_vts_test::ignore_test())]
fn test_hwcrypto_key_operations_connection() {
    let hw_crypto_key = hwcryptohal_vts_test::get_hwcryptokey()
        .expect("Couldn't get back a hwcryptokey binder object");
    let hw_crypto_operations = hw_crypto_key.getHwCryptoOperations();
    assert!(hw_crypto_operations.is_ok(), "Couldn't get back a hwcrypto operations binder object");
}

#[rdroidtest]
#[ignore_if(hwcryptohal_vts_test::ignore_test())]
fn test_hwcrypto_key_operations_simple_aes_test() {
    let hw_crypto_key = hwcryptohal_vts_test::get_hwcryptokey()
        .expect("Couldn't get back a hwcryptokey binder object");
    let hw_crypto_operations = hw_crypto_key
        .getHwCryptoOperations()
        .expect("Couldn't get back a hwcryptokey operations binder object");
    let clear_key = ExplicitKeyMaterial::Aes(AesKey::Aes128([0; 16]));
    let policy = KeyPolicy {
        usage: KeyUse::ENCRYPT_DECRYPT,
        keyLifetime: KeyLifetime::PORTABLE,
        keyPermissions: Vec::new(),
        keyManagementKey: false,
        keyType: KeyType::AES_128_CBC_PKCS7_PADDING,
    };
    let key = hw_crypto_key.importClearKey(&clear_key, &policy).expect("couldn't import clear key");

    let nonce = [0u8; 16];
    let parameters = SymmetricCryptoParameters::Aes(AesCipherMode::Cbc(CipherModeParameters {
        nonce: nonce.into(),
    }));
    let direction = SymmetricOperation::ENCRYPT;
    let sym_op_params =
        SymmetricOperationParameters { key: Some(key.clone()), direction, parameters };
    let op_params = OperationParameters::SymmetricCrypto(sym_op_params);
    let mut cmd_list = Vec::<CryptoOperation>::new();
    let data_output = OperationData::DataBuffer(Vec::new());
    cmd_list.push(CryptoOperation::DataOutput(data_output));
    cmd_list.push(CryptoOperation::SetOperationParameters(op_params));
    let input_data = OperationData::DataBuffer("string to be encrypted".as_bytes().to_vec());
    cmd_list.push(CryptoOperation::DataInput(input_data));
    let crypto_op_set = CryptoOperationSet { context: None, operations: cmd_list };
    let mut crypto_sets = Vec::new();
    crypto_sets.push(crypto_op_set);
    let mut op_result = hw_crypto_operations
        .processCommandList(&mut crypto_sets)
        .expect("couldn't process commands");
    // Extracting the vector from the command list because of ownership
    let CryptoOperation::DataOutput(OperationData::DataBuffer(encrypted_data)) =
        crypto_sets.remove(0).operations.remove(0)
    else {
        panic!("not reachable, we created this object above on the test");
    };
    let context = op_result.remove(0).context;
    // Separating the finish call on a different command set to test the returned context
    let mut cmd_list = Vec::<CryptoOperation>::new();
    let data_output = OperationData::DataBuffer(encrypted_data);
    cmd_list.push(CryptoOperation::DataOutput(data_output));
    cmd_list.push(CryptoOperation::Finish(None));
    let crypto_op_set = CryptoOperationSet { context, operations: cmd_list };
    let mut crypto_sets = Vec::new();
    crypto_sets.push(crypto_op_set);
    hw_crypto_operations.processCommandList(&mut crypto_sets).expect("couldn't process commands");
    let CryptoOperation::DataOutput(OperationData::DataBuffer(encrypted_data)) =
        crypto_sets.remove(0).operations.remove(0)
    else {
        panic!("not reachable, we created this object above on the test");
    };

    // Decrypting
    let parameters = SymmetricCryptoParameters::Aes(AesCipherMode::Cbc(CipherModeParameters {
        nonce: nonce.into(),
    }));
    let direction = SymmetricOperation::DECRYPT;
    let sym_op_params = SymmetricOperationParameters { key: Some(key), direction, parameters };
    let op_params = OperationParameters::SymmetricCrypto(sym_op_params);
    let mut cmd_list = Vec::<CryptoOperation>::new();
    let data_output = OperationData::DataBuffer(Vec::new());
    cmd_list.push(CryptoOperation::DataOutput(data_output));
    cmd_list.push(CryptoOperation::SetOperationParameters(op_params));
    cmd_list.push(CryptoOperation::DataInput(OperationData::DataBuffer(encrypted_data)));
    cmd_list.push(CryptoOperation::Finish(None));
    let crypto_op_set = CryptoOperationSet { context: None, operations: cmd_list };
    let mut crypto_sets = Vec::new();
    crypto_sets.push(crypto_op_set);
    hw_crypto_operations.processCommandList(&mut crypto_sets).expect("couldn't process commands");
    // Extracting the vector from the command list because of ownership
    let CryptoOperation::DataOutput(OperationData::DataBuffer(decrypted_data)) =
        crypto_sets.remove(0).operations.remove(0)
    else {
        panic!("not reachable, we created this object above on the test");
    };
    let decrypted_msg =
        String::from_utf8(decrypted_data).expect("couldn't decode received message");
    assert_eq!(decrypted_msg, "string to be encrypted", "couldn't retrieve original message");
}

#[rdroidtest]
#[ignore_if(hwcryptohal_vts_test::ignore_test())]
fn test_hwcrypto_key_operations_simple_hmac_test() {
    let hw_crypto_key = hwcryptohal_vts_test::get_hwcryptokey()
        .expect("Couldn't get back a hwcryptokey binder object");
    let hw_crypto_operations = hw_crypto_key
        .getHwCryptoOperations()
        .expect("Couldn't get back a hwcryptokey operations binder object");
    let clear_key = ExplicitKeyMaterial::Hmac(HmacKey::Sha256([0; 32]));
    let policy = KeyPolicy {
        usage: KeyUse::SIGN,
        keyLifetime: KeyLifetime::PORTABLE,
        keyPermissions: Vec::new(),
        keyManagementKey: false,
        keyType: KeyType::HMAC_SHA256,
    };
    let key = hw_crypto_key.importClearKey(&clear_key, &policy).expect("couldn't import clear key");

    let hmac_parameters = HmacOperationParameters { key: Some(key.clone()) };
    let op_parameters = OperationParameters::Hmac(hmac_parameters);
    let mut cmd_list = Vec::<CryptoOperation>::new();
    let data_output = OperationData::DataBuffer(Vec::new());
    cmd_list.push(CryptoOperation::DataOutput(data_output));
    cmd_list.push(CryptoOperation::SetOperationParameters(op_parameters));
    let input_data = OperationData::DataBuffer("text to be mac'ed".as_bytes().to_vec());
    cmd_list.push(CryptoOperation::DataInput(input_data));
    cmd_list.push(CryptoOperation::Finish(None));
    let crypto_op_set = CryptoOperationSet { context: None, operations: cmd_list };
    let mut crypto_sets = Vec::new();
    crypto_sets.push(crypto_op_set);
    hw_crypto_operations.processCommandList(&mut crypto_sets).expect("couldn't process commands");
    // Extracting the vector from the command list because of ownership
    let CryptoOperation::DataOutput(OperationData::DataBuffer(mac)) =
        crypto_sets.remove(0).operations.remove(0)
    else {
        panic!("not reachable, we created this object above on the test");
    };

    //Getting a second mac to compare
    let hmac_parameters = HmacOperationParameters { key: Some(key) };
    let op_parameters = OperationParameters::Hmac(hmac_parameters);
    let mut cmd_list = Vec::<CryptoOperation>::new();
    let data_output = OperationData::DataBuffer(Vec::new());
    cmd_list.push(CryptoOperation::DataOutput(data_output));
    cmd_list.push(CryptoOperation::SetOperationParameters(op_parameters));
    let input_data = OperationData::DataBuffer("text to be mac'ed".as_bytes().to_vec());
    cmd_list.push(CryptoOperation::DataInput(input_data));
    cmd_list.push(CryptoOperation::Finish(None));
    let crypto_op_set = CryptoOperationSet { context: None, operations: cmd_list };
    let mut crypto_sets = Vec::new();
    crypto_sets.push(crypto_op_set);
    hw_crypto_operations.processCommandList(&mut crypto_sets).expect("couldn't process commands");
    // Extracting the vector from the command list because of ownership
    let CryptoOperation::DataOutput(OperationData::DataBuffer(mac2)) =
        crypto_sets.remove(0).operations.remove(0)
    else {
        panic!("not reachable, we created this object above on the test");
    };
    assert_eq!(mac, mac2, "got a different mac");
}

#[rdroidtest]
#[ignore_if(hwcryptohal_vts_test::ignore_test())]
fn test_hwcrypto_key_operations_aes_simple_cbcs_test_non_block_multiple() {
    let hw_crypto_key = hwcryptohal_vts_test::get_hwcryptokey()
        .expect("Couldn't get back a hwcryptokey binder object");
    let hw_crypto_operations = hw_crypto_key
        .getHwCryptoOperations()
        .expect("Couldn't get back a hwcryptokey operations binder object");

    let usage = KeyUse::ENCRYPT_DECRYPT;
    let key_type = KeyType::AES_128_CBC_NO_PADDING;
    let policy = KeyPolicy {
        usage,
        keyLifetime: KeyLifetime::PORTABLE,
        keyPermissions: Vec::new(),
        keyType: key_type,
        keyManagementKey: false,
    };
    let clear_key = ExplicitKeyMaterial::Aes(AesKey::Aes128([0; 16]));
    let key = hw_crypto_key.importClearKey(&clear_key, &policy).expect("couldn't import clear key");

    let nonce = [0u8; 16];
    let parameters = SymmetricCryptoParameters::Aes(AesCipherMode::Cbc(CipherModeParameters {
        nonce: nonce.into(),
    }));
    let direction = SymmetricOperation::ENCRYPT;
    let sym_op_params =
        SymmetricOperationParameters { key: Some(key.clone()), direction, parameters };
    let op_params = OperationParameters::SymmetricCrypto(sym_op_params);
    let mut cmd_list = Vec::<CryptoOperation>::new();
    let data_output = OperationData::DataBuffer(Vec::new());
    cmd_list.push(CryptoOperation::DataOutput(data_output));
    cmd_list.push(CryptoOperation::SetOperationParameters(op_params));
    cmd_list.push(CryptoOperation::SetPattern(PatternParameters {
        numberBlocksProcess: 1,
        numberBlocksCopy: 0,
    }));
    let input_data =
        OperationData::DataBuffer("encryption data.0123456789abcdef".as_bytes().to_vec());
    cmd_list.push(CryptoOperation::DataInput(input_data));
    let input_data =
        OperationData::DataBuffer("fedcba98765432100123456789abcdef".as_bytes().to_vec());
    cmd_list.push(CryptoOperation::DataInput(input_data));
    let input_data = OperationData::DataBuffer("unencrypted".as_bytes().to_vec());
    cmd_list.push(CryptoOperation::DataInput(input_data));
    cmd_list.push(CryptoOperation::Finish(None));
    let crypto_op_set = CryptoOperationSet { context: None, operations: cmd_list };
    let mut crypto_sets = Vec::new();
    crypto_sets.push(crypto_op_set);
    hw_crypto_operations.processCommandList(&mut crypto_sets).expect("couldn't process commands");
    let CryptoOperation::DataOutput(OperationData::DataBuffer(encrypted_data)) =
        crypto_sets.remove(0).operations.remove(0)
    else {
        panic!("not reachable, we created this object above on the test");
    };

    let clear_encrypted_msg =
        String::from_utf8(encrypted_data[encrypted_data.len() - "unencrypted".len()..].to_vec())
            .expect("couldn't decode message");
    assert_eq!(clear_encrypted_msg, "unencrypted");

    // Decrypting
    let parameters = SymmetricCryptoParameters::Aes(AesCipherMode::Cbc(CipherModeParameters {
        nonce: nonce.into(),
    }));
    let direction = SymmetricOperation::DECRYPT;
    let sym_op_params =
        SymmetricOperationParameters { key: Some(key.clone()), direction, parameters };
    let op_params = OperationParameters::SymmetricCrypto(sym_op_params);
    let mut cmd_list = Vec::<CryptoOperation>::new();
    let data_output = OperationData::DataBuffer(Vec::new());
    cmd_list.push(CryptoOperation::DataOutput(data_output));
    cmd_list.push(CryptoOperation::SetOperationParameters(op_params));
    cmd_list.push(CryptoOperation::SetPattern(PatternParameters {
        numberBlocksProcess: 1,
        numberBlocksCopy: 0,
    }));
    cmd_list.push(CryptoOperation::DataInput(OperationData::DataBuffer(encrypted_data)));
    cmd_list.push(CryptoOperation::Finish(None));
    let crypto_op_set = CryptoOperationSet { context: None, operations: cmd_list };
    let mut crypto_sets = Vec::new();
    crypto_sets.push(crypto_op_set);
    hw_crypto_operations.processCommandList(&mut crypto_sets).expect("couldn't process commands");
    let CryptoOperation::DataOutput(OperationData::DataBuffer(decrypted_data)) =
        crypto_sets.remove(0).operations.remove(0)
    else {
        panic!("not reachable, we created this object above on the test");
    };
    let decrypted_msg =
        String::from_utf8(decrypted_data).expect("couldn't decode received message");
    assert_eq!(
        decrypted_msg,
        "encryption data.0123456789abcdeffedcba9876543210\
        0123456789abcdefunencrypted",
        "couldn't retrieve original message"
    );
}

#[rdroidtest]
#[ignore_if(hwcryptohal_vts_test::ignore_test())]
fn test_hwcrypto_key_operations_aes_simple_all_encrypted_cbcs_test() {
    let hw_crypto_key = hwcryptohal_vts_test::get_hwcryptokey()
        .expect("Couldn't get back a hwcryptokey binder object");
    let hw_crypto_operations = hw_crypto_key
        .getHwCryptoOperations()
        .expect("Couldn't get back a hwcryptokey operations binder object");

    let usage = KeyUse::ENCRYPT_DECRYPT;
    let key_type = KeyType::AES_128_CBC_NO_PADDING;
    let policy = KeyPolicy {
        usage,
        keyLifetime: KeyLifetime::PORTABLE,
        keyPermissions: Vec::new(),
        keyType: key_type,
        keyManagementKey: false,
    };
    let clear_key = ExplicitKeyMaterial::Aes(AesKey::Aes128([0; 16]));
    let key = hw_crypto_key.importClearKey(&clear_key, &policy).expect("couldn't import clear key");

    let nonce = [0u8; 16];
    let parameters = SymmetricCryptoParameters::Aes(AesCipherMode::Cbc(CipherModeParameters {
        nonce: nonce.into(),
    }));
    let direction = SymmetricOperation::ENCRYPT;
    let sym_op_params =
        SymmetricOperationParameters { key: Some(key.clone()), direction, parameters };
    let op_params = OperationParameters::SymmetricCrypto(sym_op_params);
    let mut cmd_list = Vec::<CryptoOperation>::new();
    let data_output = OperationData::DataBuffer(Vec::new());
    cmd_list.push(CryptoOperation::DataOutput(data_output));
    cmd_list.push(CryptoOperation::SetOperationParameters(op_params));
    cmd_list.push(CryptoOperation::SetPattern(PatternParameters {
        numberBlocksProcess: 1,
        numberBlocksCopy: 0,
    }));
    let input_data =
        OperationData::DataBuffer("encryption data.0123456789abcdef".as_bytes().to_vec());
    cmd_list.push(CryptoOperation::DataInput(input_data));
    let input_data =
        OperationData::DataBuffer("fedcba98765432100123456789abcdef".as_bytes().to_vec());
    cmd_list.push(CryptoOperation::DataInput(input_data));
    cmd_list.push(CryptoOperation::Finish(None));
    let crypto_op_set = CryptoOperationSet { context: None, operations: cmd_list };
    let mut crypto_sets = Vec::new();
    crypto_sets.push(crypto_op_set);
    hw_crypto_operations.processCommandList(&mut crypto_sets).expect("couldn't process commands");

    let CryptoOperation::DataOutput(OperationData::DataBuffer(encrypted_data)) =
        crypto_sets.remove(0).operations.remove(0)
    else {
        panic!("not reachable, we created this object above on the test");
    };

    // Checking that encrypting with patter 0,0 is equivalent to pattern 1,0
    let parameters = SymmetricCryptoParameters::Aes(AesCipherMode::Cbc(CipherModeParameters {
        nonce: nonce.into(),
    }));
    let direction = SymmetricOperation::ENCRYPT;
    let sym_op_params =
        SymmetricOperationParameters { key: Some(key.clone()), direction, parameters };
    let op_params = OperationParameters::SymmetricCrypto(sym_op_params);
    let mut cmd_list = Vec::<CryptoOperation>::new();
    let data_output = OperationData::DataBuffer(Vec::new());
    cmd_list.push(CryptoOperation::DataOutput(data_output));
    cmd_list.push(CryptoOperation::SetOperationParameters(op_params));
    cmd_list.push(CryptoOperation::SetPattern(PatternParameters {
        numberBlocksProcess: 0,
        numberBlocksCopy: 0,
    }));
    let input_data =
        OperationData::DataBuffer("encryption data.0123456789abcdef".as_bytes().to_vec());
    cmd_list.push(CryptoOperation::DataInput(input_data));
    let input_data =
        OperationData::DataBuffer("fedcba98765432100123456789abcdef".as_bytes().to_vec());
    cmd_list.push(CryptoOperation::DataInput(input_data));
    cmd_list.push(CryptoOperation::Finish(None));
    let crypto_op_set = CryptoOperationSet { context: None, operations: cmd_list };
    let mut crypto_sets = Vec::new();
    crypto_sets.push(crypto_op_set);
    hw_crypto_operations.processCommandList(&mut crypto_sets).expect("couldn't process commands");
    let CryptoOperation::DataOutput(OperationData::DataBuffer(encrypted_data1)) =
        crypto_sets.remove(0).operations.remove(0)
    else {
        panic!("not reachable, we created this object above on the test");
    };
    assert_eq!(encrypted_data, encrypted_data1, "encrypted data should match");

    // Decrypting
    let parameters = SymmetricCryptoParameters::Aes(AesCipherMode::Cbc(CipherModeParameters {
        nonce: nonce.into(),
    }));
    let direction = SymmetricOperation::DECRYPT;
    let sym_op_params =
        SymmetricOperationParameters { key: Some(key.clone()), direction, parameters };
    let op_params = OperationParameters::SymmetricCrypto(sym_op_params);
    let mut cmd_list = Vec::<CryptoOperation>::new();
    let data_output = OperationData::DataBuffer(Vec::new());
    cmd_list.push(CryptoOperation::DataOutput(data_output));
    cmd_list.push(CryptoOperation::SetOperationParameters(op_params));
    cmd_list.push(CryptoOperation::DataInput(OperationData::DataBuffer(encrypted_data)));
    cmd_list.push(CryptoOperation::Finish(None));
    let crypto_op_set = CryptoOperationSet { context: None, operations: cmd_list };
    let mut crypto_sets = Vec::new();
    crypto_sets.push(crypto_op_set);
    hw_crypto_operations.processCommandList(&mut crypto_sets).expect("couldn't process commands");

    let CryptoOperation::DataOutput(OperationData::DataBuffer(decrypted_data)) =
        crypto_sets.remove(0).operations.remove(0)
    else {
        panic!("not reachable, we created this object above on the test");
    };
    let decrypted_msg =
        String::from_utf8(decrypted_data).expect("couldn't decode received message");
    assert_eq!(
        decrypted_msg,
        "encryption data.0123456789abcdeffedcba9876543210\
        0123456789abcdef",
        "couldn't retrieve original message"
    );
}

#[rdroidtest]
#[ignore_if(hwcryptohal_vts_test::ignore_test())]
fn check_cbcs_wrong_key_types() {
    let hw_crypto_key = hwcryptohal_vts_test::get_hwcryptokey()
        .expect("Couldn't get back a hwcryptokey binder object");
    let hw_crypto_operations = hw_crypto_key
        .getHwCryptoOperations()
        .expect("Couldn't get back a hwcryptokey operations binder object");

    let usage = KeyUse::ENCRYPT_DECRYPT;
    let key_type = KeyType::AES_128_CBC_PKCS7_PADDING;
    let policy = KeyPolicy {
        usage,
        keyLifetime: KeyLifetime::PORTABLE,
        keyPermissions: Vec::new(),
        keyType: key_type,
        keyManagementKey: false,
    };
    let clear_key = ExplicitKeyMaterial::Aes(AesKey::Aes128([0; 16]));
    let key = hw_crypto_key.importClearKey(&clear_key, &policy).expect("couldn't import clear key");

    let nonce = [0u8; 16];
    let parameters = SymmetricCryptoParameters::Aes(AesCipherMode::Cbc(CipherModeParameters {
        nonce: nonce.into(),
    }));
    let direction = SymmetricOperation::ENCRYPT;
    let sym_op_params = SymmetricOperationParameters { key: Some(key), direction, parameters };
    let op_params = OperationParameters::SymmetricCrypto(sym_op_params);
    let mut cmd_list = Vec::<CryptoOperation>::new();
    let data_output = OperationData::DataBuffer(Vec::new());
    cmd_list.push(CryptoOperation::DataOutput(data_output));
    cmd_list.push(CryptoOperation::SetOperationParameters(op_params));
    cmd_list.push(CryptoOperation::SetPattern(PatternParameters {
        numberBlocksProcess: 1,
        numberBlocksCopy: 9,
    }));
    let crypto_op_set = CryptoOperationSet { context: None, operations: cmd_list };
    let mut crypto_sets = Vec::new();
    crypto_sets.push(crypto_op_set);
    let process_result = hw_crypto_operations.processCommandList(&mut crypto_sets);
    assert!(process_result.is_err(), "Should not be able to use cbcs mode with this key type");

    let policy = KeyPolicy {
        usage,
        keyLifetime: KeyLifetime::PORTABLE,
        keyPermissions: Vec::new(),
        keyType: KeyType::AES_256_CBC_NO_PADDING,
        keyManagementKey: false,
    };
    let clear_key = ExplicitKeyMaterial::Aes(AesKey::Aes256([0; 32]));
    let key = hw_crypto_key.importClearKey(&clear_key, &policy).expect("couldn't import clear key");
    let parameters = SymmetricCryptoParameters::Aes(AesCipherMode::Cbc(CipherModeParameters {
        nonce: nonce.into(),
    }));
    let sym_op_params = SymmetricOperationParameters { key: Some(key), direction, parameters };
    let op_params = OperationParameters::SymmetricCrypto(sym_op_params);
    let mut cmd_list = Vec::<CryptoOperation>::new();
    let data_output = OperationData::DataBuffer(Vec::new());
    cmd_list.push(CryptoOperation::DataOutput(data_output));
    cmd_list.push(CryptoOperation::SetOperationParameters(op_params));
    cmd_list.push(CryptoOperation::SetPattern(PatternParameters {
        numberBlocksProcess: 1,
        numberBlocksCopy: 9,
    }));
    let crypto_op_set = CryptoOperationSet { context: None, operations: cmd_list };
    let mut crypto_sets = Vec::new();
    crypto_sets.push(crypto_op_set);
    let process_result = hw_crypto_operations.processCommandList(&mut crypto_sets);

    assert!(process_result.is_err(), "Should not be able to use cbcs mode with this key type");
}

#[rdroidtest]
#[ignore_if(hwcryptohal_vts_test::ignore_test())]
fn aes_simple_cbcs_test() {
    let hw_crypto_key = hwcryptohal_vts_test::get_hwcryptokey()
        .expect("Couldn't get back a hwcryptokey binder object");
    let hw_crypto_operations = hw_crypto_key
        .getHwCryptoOperations()
        .expect("Couldn't get back a hwcryptokey operations binder object");

    let usage = KeyUse::ENCRYPT_DECRYPT;
    let key_type = KeyType::AES_128_CBC_NO_PADDING;
    let policy = KeyPolicy {
        usage,
        keyLifetime: KeyLifetime::PORTABLE,
        keyPermissions: Vec::new(),
        keyType: key_type,
        keyManagementKey: false,
    };
    let clear_key = ExplicitKeyMaterial::Aes(AesKey::Aes128([0; 16]));
    let key = hw_crypto_key.importClearKey(&clear_key, &policy).expect("couldn't import clear key");

    let nonce = [0u8; 16];
    let parameters = SymmetricCryptoParameters::Aes(AesCipherMode::Cbc(CipherModeParameters {
        nonce: nonce.into(),
    }));
    let direction = SymmetricOperation::ENCRYPT;
    let sym_op_params =
        SymmetricOperationParameters { key: Some(key.clone()), direction, parameters };
    let op_params = OperationParameters::SymmetricCrypto(sym_op_params);
    let mut cmd_list = Vec::<CryptoOperation>::new();
    let data_output = OperationData::DataBuffer(Vec::new());
    cmd_list.push(CryptoOperation::DataOutput(data_output));
    cmd_list.push(CryptoOperation::SetOperationParameters(op_params));
    cmd_list.push(CryptoOperation::SetPattern(PatternParameters {
        numberBlocksProcess: 1,
        numberBlocksCopy: 9,
    }));
    let input_data =
        OperationData::DataBuffer("encryption data.0123456789abcdef".as_bytes().to_vec());
    cmd_list.push(CryptoOperation::DataInput(input_data));
    let input_data =
        OperationData::DataBuffer("fedcba98765432100123456789abcdef".as_bytes().to_vec());
    cmd_list.push(CryptoOperation::DataInput(input_data));
    let input_data =
        OperationData::DataBuffer("fedcba98765432100123456789abcdef".as_bytes().to_vec());
    cmd_list.push(CryptoOperation::DataInput(input_data));
    let input_data =
        OperationData::DataBuffer("fedcba98765432100123456789abcdef".as_bytes().to_vec());
    cmd_list.push(CryptoOperation::DataInput(input_data));
    let input_data = OperationData::DataBuffer(
        "fedcba98765432100123456789abcdefProtectedSection".as_bytes().to_vec(),
    );
    cmd_list.push(CryptoOperation::DataInput(input_data));
    cmd_list.push(CryptoOperation::Finish(None));
    let crypto_op_set = CryptoOperationSet { context: None, operations: cmd_list };
    let mut crypto_sets = Vec::new();
    crypto_sets.push(crypto_op_set);
    hw_crypto_operations.processCommandList(&mut crypto_sets).expect("couldn't process commands");

    let CryptoOperation::DataOutput(OperationData::DataBuffer(encrypted_data)) =
        crypto_sets.remove(0).operations.remove(0)
    else {
        panic!("not reachable, we created this object above on the test");
    };
    let clear_encrypted_msg =
        String::from_utf8(encrypted_data[16..encrypted_data.len() - 16].to_vec())
            .expect("couldn't decode received message");
    assert_eq!(
        clear_encrypted_msg,
        "0123456789abcdeffedcba98765432100123456789abcdeffedcba9876543210\
        0123456789abcdeffedcba98765432100123456789abcdeffedcba98765432100123456789abcdef",
        "couldn't retrieve clear portion"
    );

    // Decrypting
    let parameters = SymmetricCryptoParameters::Aes(AesCipherMode::Cbc(CipherModeParameters {
        nonce: nonce.into(),
    }));
    let direction = SymmetricOperation::DECRYPT;
    let sym_op_params =
        SymmetricOperationParameters { key: Some(key.clone()), direction, parameters };
    let op_params = OperationParameters::SymmetricCrypto(sym_op_params);
    let mut cmd_list = Vec::<CryptoOperation>::new();
    let data_output = OperationData::DataBuffer(Vec::new());
    cmd_list.push(CryptoOperation::DataOutput(data_output));
    cmd_list.push(CryptoOperation::SetOperationParameters(op_params));
    cmd_list.push(CryptoOperation::SetPattern(PatternParameters {
        numberBlocksProcess: 1,
        numberBlocksCopy: 9,
    }));
    cmd_list.push(CryptoOperation::DataInput(OperationData::DataBuffer(encrypted_data)));
    cmd_list.push(CryptoOperation::Finish(None));
    let crypto_op_set = CryptoOperationSet { context: None, operations: cmd_list };
    let mut crypto_sets = Vec::new();
    crypto_sets.push(crypto_op_set);
    hw_crypto_operations.processCommandList(&mut crypto_sets).expect("couldn't process commands");

    let CryptoOperation::DataOutput(OperationData::DataBuffer(decrypted_data)) =
        crypto_sets.remove(0).operations.remove(0)
    else {
        panic!("not reachable, we created this object above on the test");
    };
    let decrypted_msg =
        String::from_utf8(decrypted_data).expect("couldn't decode received message");
    assert_eq!(
        decrypted_msg,
        "encryption data.0123456789abcdeffedcba9876543210\
        0123456789abcdeffedcba98765432100123456789abcdeffedcba9876543210\
        0123456789abcdeffedcba98765432100123456789abcdefProtectedSection",
        "couldn't retrieve original message"
    );
}

rdroidtest::test_main!();
