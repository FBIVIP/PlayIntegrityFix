# Fuzzers for libkeymint_support

## Plugin Design Considerations
The fuzzer plugins for libkeymint_support are designed based on the understanding of the source code and try to achieve the following:

#### Maximize code coverage
The configuration parameters are not hardcoded, but instead selected based on incoming data. This ensures more code paths are reached by the fuzzers.

#### Maximize utilization of input data
The plugins feed the entire input data to the module. This ensures that the plugins tolerate any kind of input (empty, huge, malformed, etc) and dont `exit()` on any input and thereby increasing the chance of identifying vulnerabilities.

## Table of contents
+ [keymint_attestation_fuzzer](#KeyMintAttestation)
+ [keymint_authSet_fuzzer](#KeyMintAuthSet)
+ [keymint_remote_prov_fuzzer](#KeyMintRemoteProv)
+ [keymint_rkpsupport_fuzzer](#KeyMintRemoteKeyProvSupport)

# <a name="KeyMintAttestation"></a> Fuzzer for KeyMintAttestation
KeyMintAttestation supports the following parameters:
1. PaddingMode(parameter name: "padding")
2. Digest(parameter name: "digest")
3. Index(parameter name: "idx")
4. Timestamp(parameter name: "timestamp")
5. AuthSet(parameter name: "authSet")
6. IssuerSubjectName(parameter name: "issuerSubjectName")
7. AttestationChallenge(parameter name: "challenge")
8. AttestationApplicationId(parameter name: "id")
9. EcCurve(parameter name: "ecCurve")
10. BlockMode(parameter name: "blockmode")
11. minMacLength(parameter name: "minMacLength")
12. macLength(parameter name: "macLength")

| Parameter| Valid Values| Configured Value|
|------------- |--------------| -------------------- |
|`padding`| `PaddingMode` |Value obtained from FuzzedDataProvider|
|`digest`| `Digest` |Value obtained from FuzzedDataProvider|
|`idx`| `size_t` |Value obtained from FuzzedDataProvider|
|`timestamp`| `uint64_t` |Value obtained from FuzzedDataProvider|
|`authSet`| `uint32_t` |Value obtained from FuzzedDataProvider|
|`issuerSubjectName`| `uint8_t` |Value obtained from FuzzedDataProvider|
|`AttestationChallenge`| `string` |Value obtained from FuzzedDataProvider|
|`AttestationApplicationId`| `string` |Value obtained from FuzzedDataProvider|
|`blockmode`| `BlockMode` |Value obtained from FuzzedDataProvider|
|`minMacLength`| `uint32_t` |Value obtained from FuzzedDataProvider|
|`macLength`| `uint32_t` |Value obtained from FuzzedDataProvider|

#### Steps to run
1. Build the fuzzer
```
$ mm -j$(nproc) keymint_attestation_fuzzer
```
2. Run on device
```
$ adb sync data
$ adb shell /data/fuzz/arm64/keymint_attestation_fuzzer/keymint_attestation_fuzzer
```

# <a name="KeyMintAuthSet"></a> Fuzzer for KeyMintAuthSet
KeyMintAuthSet supports the following parameters:
1. AuthorizationSet(parameter name: "authSet")
2. AuthorizationSet(parameter name: "params")
3. KeyParameters(parameter name: "numKeyParam")
4. Tag(parameter name: "tag")

| Parameter| Valid Values| Configured Value|
|------------- |--------------| -------------------- |
|`authSet`| `AuthorizationSet` |Value obtained from FuzzedDataProvider|
|`params`| `AuthorizationSet` |Value obtained from FuzzedDataProvider|
|`numKeyParam`| `size_t` |Value obtained from FuzzedDataProvider|
|`tag`| `Tag` |Value obtained from FuzzedDataProvider|

#### Steps to run
1. Build the fuzzer
```
$ mm -j$(nproc) keymint_authSet_fuzzer
```
2. Run on device
```
$ adb sync data
$ adb shell /data/fuzz/arm64/keymint_authSet_fuzzer/keymint_authSet_fuzzer
```

# <a name="KeyMintRemoteProv"></a> Fuzzer for KeyMintRemoteProv
KeyMintRemoteProv supports the following parameters:
1. ChallengeSize(parameter name: "challengeSize")
2. Challenge(parameter name: "challenge")
3. NumKeys(parameter name: "numKeys")

| Parameter| Valid Values| Configured Value|
|------------- |--------------| -------------------- |
|`challengeSize`| `uint8_t` |Value obtained from FuzzedDataProvider|
|`challenge`| `std::vector<uint8_t>` |Value obtained from FuzzedDataProvider|
|`numKeys`| `uint8_t` |Value obtained from FuzzedDataProvider|

#### Steps to run
1. Build the fuzzer
```
$ mm -j$(nproc) keymint_remote_prov_fuzzer
```
2. Run on device
```
$ adb sync data
$ adb shell /data/fuzz/arm64/keymint_remote_prov_fuzzer/keymint_remote_prov_fuzzer
```

# <a name="KeyMintRemoteKeyProvSupport"></a> Fuzzer for KeyMintRemoteKeyProvSupport
KeyMintRemoteKeyProvSupport supports the following parameters:
1. SupportedEekCurve(parameter name: "supportedEekCurve")
2. Length(parameter name: "length")
3. SerialNumberProp(parameter name: "serialNoProp")
4. InstanceName(parameter name: "instanceName")
5. Value(parameter name: "value")

| Parameter| Valid Values| Configured Value|
|------------- |--------------| -------------------- |
|`supportedEekCurve`| `uint8_t` |Value obtained from FuzzedDataProvider|
|`length`| `uint8_t` |Value obtained from FuzzedDataProvider|
|`serialNoProp`| `string` |Value obtained from FuzzedDataProvider|
|`instanceName`| `string` |Value obtained from FuzzedDataProvider|
|`value`| `uint8_t` |Value obtained from FuzzedDataProvider|

#### Steps to run
1. Build the fuzzer
```
$ mm -j$(nproc) keymint_rkpsupport_fuzzer
```
2. Run on device
```
$ adb sync data
$ adb shell /data/fuzz/arm64/keymint_rkpsupport_fuzzer/keymint_rkpsupport_fuzzer
```
