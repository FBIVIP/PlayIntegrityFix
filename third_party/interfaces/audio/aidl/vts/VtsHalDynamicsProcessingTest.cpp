/*
 * Copyright (C) 2023 The Android Open Source Project
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

#include <set>
#include <string>
#include <unordered_set>

#define LOG_TAG "VtsHalDynamicsProcessingTest"
#include <android-base/logging.h>
#include <audio_utils/power.h>
#include <audio_utils/primitives.h>

#include <Utils.h>

#include "EffectHelper.h"
#include "EffectRangeSpecific.h"

using namespace android;
using namespace aidl::android::hardware::audio::effect::DynamicsProcessingRanges;

using aidl::android::hardware::audio::effect::Descriptor;
using aidl::android::hardware::audio::effect::DynamicsProcessing;
using aidl::android::hardware::audio::effect::getEffectTypeUuidDynamicsProcessing;
using aidl::android::hardware::audio::effect::IEffect;
using aidl::android::hardware::audio::effect::IFactory;
using aidl::android::hardware::audio::effect::Parameter;
using android::hardware::audio::common::testing::detail::TestExecutionTracer;

constexpr int32_t kMinDataTestHalVersion = 3;

/**
 * Here we focus on specific parameter checking, general IEffect interfaces testing performed in
 * VtsAudioEffectTargetTest.
 */
class DynamicsProcessingTestHelper : public EffectHelper {
  public:
    DynamicsProcessingTestHelper(std::pair<std::shared_ptr<IFactory>, Descriptor> pair,
                                 int32_t channelLayout = kDefaultChannelLayout)
        : mChannelLayout(channelLayout),
          mChannelCount(::aidl::android::hardware::audio::common::getChannelCount(
                  AudioChannelLayout::make<AudioChannelLayout::layoutMask>(mChannelLayout))) {
        std::tie(mFactory, mDescriptor) = pair;
    }

    // setup
    void SetUpDynamicsProcessingEffect() {
        ASSERT_NE(nullptr, mFactory);
        ASSERT_NO_FATAL_FAILURE(create(mFactory, mEffect, mDescriptor));
        Parameter::Specific specific = getDefaultParamSpecific();
        Parameter::Common common = createParamCommon(
                0 /* session */, 1 /* ioHandle */, kSamplingFrequency /* iSampleRate */,
                kSamplingFrequency /* oSampleRate */, kFrameCount /* iFrameCount */,
                kFrameCount /* oFrameCount */,
                AudioChannelLayout::make<AudioChannelLayout::layoutMask>(mChannelLayout),
                AudioChannelLayout::make<AudioChannelLayout::layoutMask>(mChannelLayout));
        ASSERT_NO_FATAL_FAILURE(open(mEffect, common, specific, &mOpenEffectReturn, EX_NONE));
        ASSERT_NE(nullptr, mEffect);
        mEngineConfigApplied = mEngineConfigPreset;
    }

    Parameter::Specific getDefaultParamSpecific() {
        DynamicsProcessing dp = DynamicsProcessing::make<DynamicsProcessing::engineArchitecture>(
                mEngineConfigPreset);
        Parameter::Specific specific =
                Parameter::Specific::make<Parameter::Specific::dynamicsProcessing>(dp);
        return specific;
    }

    // teardown
    void TearDownDynamicsProcessingEffect() {
        ASSERT_NO_FATAL_FAILURE(close(mEffect));
        ASSERT_NO_FATAL_FAILURE(destroy(mFactory, mEffect));
    }

    // utils functions for parameter checking
    bool isParamEqual(const DynamicsProcessing::Tag& tag, const DynamicsProcessing& dpRef,
                      const DynamicsProcessing& dpTest);
    bool isEngineConfigEqual(const DynamicsProcessing::EngineArchitecture& refCfg,
                             const DynamicsProcessing::EngineArchitecture& testCfg);

    template <typename T>
    std::vector<T> filterEnabledVector(const std::vector<T>& vec);

    template <typename T>
    bool isAidlVectorEqualAfterFilter(const std::vector<T>& source, const std::vector<T>& target);

    template <typename T>
    bool isAidlVectorEqual(const std::vector<T>& source, const std::vector<T>& target);

    template <typename T>
    bool isChannelConfigValid(const std::vector<T>& cfgs) {
        auto& channelCount = mChannelCount;
        return std::all_of(cfgs.cbegin(), cfgs.cend(), [channelCount](const T& cfg) {
            return (cfg.channel >= 0 && cfg.channel < channelCount);
        });
    }

    template <typename T>
    bool isBandConfigValid(const std::vector<T>& cfgs, int bandCount);

    bool isParamValid(const DynamicsProcessing::Tag& tag, const DynamicsProcessing& dp);

    // get set params and validate
    void SetAndGetDynamicsProcessingParameters();

    bool isAllParamsValid();

    void setParamsAndProcess(std::vector<float>& input, std::vector<float>& output);

    float calculateDb(const std::vector<float>& input, size_t startSamplePos, size_t endSamplePos);

    void getMagnitudeValue(const std::vector<float>& output, std::vector<float>& bufferMag);

    void checkInputAndOutputEquality(const std::vector<float>& outputMag);

    void setUpDataTest(const std::vector<int>& testFrequencies, float fullScaleSineDb);

    void createChannelConfig();

    // enqueue test parameters
    void addEngineConfig(const DynamicsProcessing::EngineArchitecture& cfg);
    void addPreEqChannelConfig(const std::vector<DynamicsProcessing::ChannelConfig>& cfg);
    void addPostEqChannelConfig(const std::vector<DynamicsProcessing::ChannelConfig>& cfg);
    void addMbcChannelConfig(const std::vector<DynamicsProcessing::ChannelConfig>& cfg);
    void addPreEqBandConfigs(const std::vector<DynamicsProcessing::EqBandConfig>& cfgs);
    void addPostEqBandConfigs(const std::vector<DynamicsProcessing::EqBandConfig>& cfgs);
    void addMbcBandConfigs(const std::vector<DynamicsProcessing::MbcBandConfig>& cfgs);
    void addLimiterConfig(const std::vector<DynamicsProcessing::LimiterConfig>& cfg);
    void addInputGain(const std::vector<DynamicsProcessing::InputGain>& inputGain);

    void checkHalVersion();

    static constexpr float kPreferredProcessingDurationMs = 10.0f;
    static constexpr int kBandCount = 5;
    static constexpr int kSamplingFrequency = 44100;
    static constexpr int kFrameCount = 2048;
    static constexpr int kInputFrequency = 1000;
    static constexpr size_t kStartIndex = 15 * kSamplingFrequency / 1000;  // skip 15ms
    static constexpr float kToleranceDb = 0.5;
    static constexpr int kNPointFFT = 1024;
    static constexpr float kBinWidth = (float)kSamplingFrequency / kNPointFFT;
    // Full scale sine wave with 1000 Hz frequency is -3 dB
    static constexpr float kSineFullScaleDb = -3;
    // Full scale sine wave with 100 Hz and 1000 Hz frequency is -6 dB
    static constexpr float kSineMultitoneFullScaleDb = -6;
    const std::vector<int> kCutoffFreqHz = {200 /*0th band cutoff*/, 2000 /*1st band cutoff*/};
    std::vector<int> mMultitoneTestFrequencies = {100, 1000};
    // Calculating normalizing factor by dividing the number of FFT points by half and the number of
    // test frequencies. The normalization accounts for the FFT splitting the signal into positive
    // and negative frequencies. Additionally, during multi-tone input generation, sample values are
    // normalized to the range [-1, 1] by dividing them by the number of test frequencies.
    float mNormalizingFactor = (kNPointFFT / (2 * mMultitoneTestFrequencies.size()));
    std::vector<int> mBinOffsets;
    std::vector<DynamicsProcessing::ChannelConfig> mChannelConfig;
    std::vector<float> mInput;
    float mInputDb;
    std::shared_ptr<IFactory> mFactory;
    std::shared_ptr<IEffect> mEffect;
    Descriptor mDescriptor;
    IEffect::OpenEffectReturn mOpenEffectReturn;
    DynamicsProcessing::EngineArchitecture mEngineConfigApplied;
    DynamicsProcessing::EngineArchitecture mEngineConfigPreset{
            .resolutionPreference =
                    DynamicsProcessing::ResolutionPreference::FAVOR_FREQUENCY_RESOLUTION,
            .preferredProcessingDurationMs = kPreferredProcessingDurationMs,
            .preEqStage = {.inUse = true, .bandCount = kBandCount},
            .postEqStage = {.inUse = true, .bandCount = kBandCount},
            .mbcStage = {.inUse = true, .bandCount = kBandCount},
            .limiterInUse = true,
    };

    std::unordered_set<int /* channelId */> mPreEqChannelEnable;
    std::unordered_set<int /* channelId */> mPostEqChannelEnable;
    std::unordered_set<int /* channelId */> mMbcChannelEnable;
    std::unordered_set<int /* channelId */> mLimiterChannelEnable;
    static const std::set<std::vector<DynamicsProcessing::ChannelConfig>> kChannelConfigTestSet;
    static const std::set<DynamicsProcessing::StageEnablement> kStageEnablementTestSet;
    static const std::set<std::vector<DynamicsProcessing::InputGain>> kInputGainTestSet;

  private:
    std::vector<std::pair<DynamicsProcessing::Tag, DynamicsProcessing>> mTags;

  protected:
    const int32_t mChannelLayout;
    const int mChannelCount;

    void CleanUp() {
        mTags.clear();
        mPreEqChannelEnable.clear();
        mPostEqChannelEnable.clear();
        mMbcChannelEnable.clear();
        mLimiterChannelEnable.clear();
    }
};

// test value set for DynamicsProcessing::StageEnablement
const std::set<DynamicsProcessing::StageEnablement>
        DynamicsProcessingTestHelper::kStageEnablementTestSet = {
                {.inUse = true, .bandCount = DynamicsProcessingTestHelper::kBandCount},
                {.inUse = true, .bandCount = 0},
                {.inUse = true, .bandCount = -1},
                {.inUse = false, .bandCount = 0},
                {.inUse = false, .bandCount = -1},
                {.inUse = false, .bandCount = DynamicsProcessingTestHelper::kBandCount}};

// test value set for DynamicsProcessing::ChannelConfig
const std::set<std::vector<DynamicsProcessing::ChannelConfig>>
        DynamicsProcessingTestHelper::kChannelConfigTestSet = {
                {{.channel = -1, .enable = false},
                 {.channel = 0, .enable = true},
                 {.channel = 1, .enable = false},
                 {.channel = 2, .enable = true}},
                {{.channel = -1, .enable = false}, {.channel = 2, .enable = true}},
                {{.channel = 0, .enable = true}, {.channel = 1, .enable = true}}};

// test value set for DynamicsProcessing::InputGain
const std::set<std::vector<DynamicsProcessing::InputGain>>
        DynamicsProcessingTestHelper::kInputGainTestSet = {
                {{.channel = 0, .gainDb = 10.f},
                 {.channel = 1, .gainDb = 0.f},
                 {.channel = 2, .gainDb = -10.f}},
                {{.channel = -1, .gainDb = -10.f}, {.channel = -2, .gainDb = 10.f}},
                {{.channel = -1, .gainDb = 10.f}, {.channel = 0, .gainDb = -10.f}},
                {{.channel = 0, .gainDb = 10.f}, {.channel = 1, .gainDb = -10.f}}};

template <typename T>
bool DynamicsProcessingTestHelper::isBandConfigValid(const std::vector<T>& cfgs, int bandCount) {
    std::unordered_set<int> freqs;
    for (auto cfg : cfgs) {
        if (cfg.channel < 0 || cfg.channel >= mChannelCount) return false;
        if (cfg.band < 0 || cfg.band >= bandCount) return false;
        // duplicated band index
        if (freqs.find(cfg.band) != freqs.end()) return false;
        freqs.insert(cfg.band);
    }
    return true;
}

bool DynamicsProcessingTestHelper::isParamValid(const DynamicsProcessing::Tag& tag,
                                                const DynamicsProcessing& dp) {
    switch (tag) {
        case DynamicsProcessing::preEq: {
            return isChannelConfigValid(dp.get<DynamicsProcessing::preEq>());
        }
        case DynamicsProcessing::postEq: {
            return isChannelConfigValid(dp.get<DynamicsProcessing::postEq>());
        }
        case DynamicsProcessing::mbc: {
            return isChannelConfigValid(dp.get<DynamicsProcessing::mbc>());
        }
        case DynamicsProcessing::preEqBand: {
            return isBandConfigValid(dp.get<DynamicsProcessing::preEqBand>(),
                                     mEngineConfigApplied.preEqStage.bandCount);
        }
        case DynamicsProcessing::postEqBand: {
            return isBandConfigValid(dp.get<DynamicsProcessing::postEqBand>(),
                                     mEngineConfigApplied.postEqStage.bandCount);
        }
        case DynamicsProcessing::mbcBand: {
            return isBandConfigValid(dp.get<DynamicsProcessing::mbcBand>(),
                                     mEngineConfigApplied.mbcStage.bandCount);
        }
        case DynamicsProcessing::limiter: {
            return isChannelConfigValid(dp.get<DynamicsProcessing::limiter>());
        }
        case DynamicsProcessing::inputGain: {
            return isChannelConfigValid(dp.get<DynamicsProcessing::inputGain>());
        }
        default: {
            return true;
        }
    }
    return true;
}

bool DynamicsProcessingTestHelper::isParamEqual(const DynamicsProcessing::Tag& tag,
                                                const DynamicsProcessing& dpRef,
                                                const DynamicsProcessing& dpTest) {
    switch (tag) {
        case DynamicsProcessing::engineArchitecture: {
            return isEngineConfigEqual(dpRef.get<DynamicsProcessing::engineArchitecture>(),
                                       dpTest.get<DynamicsProcessing::engineArchitecture>());
        }
        case DynamicsProcessing::preEq: {
            const auto& source = dpRef.get<DynamicsProcessing::preEq>();
            const auto& target = dpTest.get<DynamicsProcessing::preEq>();
            return isAidlVectorEqualAfterFilter<DynamicsProcessing::ChannelConfig>(source, target);
        }
        case DynamicsProcessing::postEq: {
            return isAidlVectorEqualAfterFilter<DynamicsProcessing::ChannelConfig>(
                    dpRef.get<DynamicsProcessing::postEq>(),
                    dpTest.get<DynamicsProcessing::postEq>());
        }
        case DynamicsProcessing::mbc: {
            return isAidlVectorEqualAfterFilter<DynamicsProcessing::ChannelConfig>(
                    dpRef.get<DynamicsProcessing::mbc>(), dpTest.get<DynamicsProcessing::mbc>());
        }
        case DynamicsProcessing::preEqBand: {
            return isAidlVectorEqualAfterFilter<DynamicsProcessing::EqBandConfig>(
                    dpRef.get<DynamicsProcessing::preEqBand>(),
                    dpTest.get<DynamicsProcessing::preEqBand>());
        }
        case DynamicsProcessing::postEqBand: {
            return isAidlVectorEqualAfterFilter<DynamicsProcessing::EqBandConfig>(
                    dpRef.get<DynamicsProcessing::postEqBand>(),
                    dpTest.get<DynamicsProcessing::postEqBand>());
        }
        case DynamicsProcessing::mbcBand: {
            return isAidlVectorEqualAfterFilter<DynamicsProcessing::MbcBandConfig>(
                    dpRef.get<DynamicsProcessing::mbcBand>(),
                    dpTest.get<DynamicsProcessing::mbcBand>());
        }
        case DynamicsProcessing::limiter: {
            return isAidlVectorEqualAfterFilter<DynamicsProcessing::LimiterConfig>(
                    dpRef.get<DynamicsProcessing::limiter>(),
                    dpTest.get<DynamicsProcessing::limiter>());
        }
        case DynamicsProcessing::inputGain: {
            return isAidlVectorEqual<DynamicsProcessing::InputGain>(
                    dpRef.get<DynamicsProcessing::inputGain>(),
                    dpTest.get<DynamicsProcessing::inputGain>());
        }
        case DynamicsProcessing::vendor: {
            return false;
        }
    }
}

bool DynamicsProcessingTestHelper::isEngineConfigEqual(
        const DynamicsProcessing::EngineArchitecture& ref,
        const DynamicsProcessing::EngineArchitecture& test) {
    return ref == test;
}

template <typename T>
std::vector<T> DynamicsProcessingTestHelper::filterEnabledVector(const std::vector<T>& vec) {
    std::vector<T> ret;
    std::copy_if(vec.begin(), vec.end(), std::back_inserter(ret),
                 [](const auto& v) { return v.enable; });
    return ret;
}

template <typename T>
bool DynamicsProcessingTestHelper::isAidlVectorEqual(const std::vector<T>& source,
                                                     const std::vector<T>& target) {
    if (source.size() != target.size()) return false;

    auto tempS = source;
    auto tempT = target;
    std::sort(tempS.begin(), tempS.end());
    std::sort(tempT.begin(), tempT.end());
    return tempS == tempT;
}

template <typename T>
bool DynamicsProcessingTestHelper::isAidlVectorEqualAfterFilter(const std::vector<T>& source,
                                                                const std::vector<T>& target) {
    return isAidlVectorEqual<T>(filterEnabledVector<T>(source), filterEnabledVector<T>(target));
}

void DynamicsProcessingTestHelper::SetAndGetDynamicsProcessingParameters() {
    for (const auto& [tag, dp] : mTags) {
        // validate parameter
        Descriptor desc;
        ASSERT_STATUS(EX_NONE, mEffect->getDescriptor(&desc));
        bool valid = isParamInRange(dp, desc.capability.range.get<Range::dynamicsProcessing>());
        if (valid) valid = isParamValid(tag, dp);
        const binder_exception_t expected = valid ? EX_NONE : EX_ILLEGAL_ARGUMENT;

        // set parameter
        Parameter expectParam;
        Parameter::Specific specific;
        specific.set<Parameter::Specific::dynamicsProcessing>(dp);
        expectParam.set<Parameter::specific>(specific);
        ASSERT_STATUS(expected, mEffect->setParameter(expectParam))
                << "\n"
                << expectParam.toString() << "\n"
                << desc.toString();

        // only get if parameter in range and set success
        if (expected == EX_NONE) {
            Parameter getParam;
            Parameter::Id id;
            DynamicsProcessing::Id dpId;
            dpId.set<DynamicsProcessing::Id::commonTag>(tag);
            id.set<Parameter::Id::dynamicsProcessingTag>(dpId);
            // if set success, then get should match
            EXPECT_STATUS(expected, mEffect->getParameter(id, &getParam));
            Parameter::Specific specificTest = getParam.get<Parameter::specific>();
            const auto& target = specificTest.get<Parameter::Specific::dynamicsProcessing>();
            EXPECT_TRUE(isParamEqual(tag, dp, target)) << dp.toString() << "\n"
                                                       << target.toString();
            // update mEngineConfigApplied after setting successfully
            if (tag == DynamicsProcessing::engineArchitecture) {
                mEngineConfigApplied = target.get<DynamicsProcessing::engineArchitecture>();
            }
        }
    }
}

bool DynamicsProcessingTestHelper::isAllParamsValid() {
    if (mTags.empty()) {
        return false;
    }
    for (const auto& [tag, dp] : mTags) {
        // validate parameter
        if (!isParamInRange(dp, mDescriptor.capability.range.get<Range::dynamicsProcessing>())) {
            return false;
        }
        if (!isParamValid(tag, dp)) {
            return false;
        }
    }
    return true;
}

// This function calculates power for both and mono and stereo data as the total power for
// interleaved multichannel data can be calculated by treating it as a continuous mono input.
float DynamicsProcessingTestHelper::calculateDb(const std::vector<float>& input,
                                                size_t startSamplePos = 0,
                                                size_t endSamplePos = 0) {
    size_t sampleCount = (endSamplePos == 0 ? input.size() : endSamplePos) - startSamplePos;
    return audio_utils_compute_power_mono(input.data() + startSamplePos, AUDIO_FORMAT_PCM_FLOAT,
                                          sampleCount);
}

void DynamicsProcessingTestHelper::setParamsAndProcess(std::vector<float>& input,
                                                       std::vector<float>& output) {
    ASSERT_NO_FATAL_FAILURE(SetAndGetDynamicsProcessingParameters());
    if (isAllParamsValid()) {
        ASSERT_NO_FATAL_FAILURE(
                processAndWriteToOutput(input, output, mEffect, &mOpenEffectReturn));
        ASSERT_GT(output.size(), kStartIndex);
    }
}

void DynamicsProcessingTestHelper::getMagnitudeValue(const std::vector<float>& output,
                                                     std::vector<float>& bufferMag) {
    std::vector<float> subOutput(output.begin() + kStartIndex, output.end());
    EXPECT_NO_FATAL_FAILURE(calculateMagnitudeMono(bufferMag, subOutput, mBinOffsets, kNPointFFT));
}

void DynamicsProcessingTestHelper::checkInputAndOutputEquality(
        const std::vector<float>& outputMag) {
    std::vector<float> inputMag(mBinOffsets.size());
    EXPECT_NO_FATAL_FAILURE(getMagnitudeValue(mInput, inputMag));
    for (size_t i = 0; i < inputMag.size(); i++) {
        EXPECT_NEAR(calculateDb({inputMag[i] / mNormalizingFactor}),
                    calculateDb({outputMag[i] / mNormalizingFactor}), kToleranceDb);
    }
}

void DynamicsProcessingTestHelper::setUpDataTest(const std::vector<int>& testFrequencies,
                                                 float fullScaleSineDb) {
    ASSERT_NO_FATAL_FAILURE(SetUpDynamicsProcessingEffect());
    SKIP_TEST_IF_DATA_UNSUPPORTED(mDescriptor.common.flags);
    mInput.resize(kFrameCount * mChannelCount);
    ASSERT_NO_FATAL_FAILURE(
            generateSineWave(testFrequencies, mInput, 1.0, kSamplingFrequency, mChannelLayout));
    mInputDb = calculateDb(mInput);
    ASSERT_NEAR(mInputDb, fullScaleSineDb, kToleranceDb);
}

void DynamicsProcessingTestHelper::createChannelConfig() {
    for (int i = 0; i < mChannelCount; i++) {
        mChannelConfig.push_back(DynamicsProcessing::ChannelConfig(i, true));
    }
}

void DynamicsProcessingTestHelper::addEngineConfig(
        const DynamicsProcessing::EngineArchitecture& cfg) {
    DynamicsProcessing dp;
    dp.set<DynamicsProcessing::engineArchitecture>(cfg);
    mTags.push_back({DynamicsProcessing::engineArchitecture, dp});
}

void DynamicsProcessingTestHelper::addPreEqChannelConfig(
        const std::vector<DynamicsProcessing::ChannelConfig>& cfgs) {
    DynamicsProcessing dp;
    dp.set<DynamicsProcessing::preEq>(cfgs);
    mTags.push_back({DynamicsProcessing::preEq, dp});
    for (auto& cfg : cfgs) {
        if (cfg.enable) mPreEqChannelEnable.insert(cfg.channel);
    }
}

void DynamicsProcessingTestHelper::addPostEqChannelConfig(
        const std::vector<DynamicsProcessing::ChannelConfig>& cfgs) {
    DynamicsProcessing dp;
    dp.set<DynamicsProcessing::postEq>(cfgs);
    mTags.push_back({DynamicsProcessing::postEq, dp});
    for (auto& cfg : cfgs) {
        if (cfg.enable) mPostEqChannelEnable.insert(cfg.channel);
    }
}

void DynamicsProcessingTestHelper::addMbcChannelConfig(
        const std::vector<DynamicsProcessing::ChannelConfig>& cfgs) {
    DynamicsProcessing dp;
    dp.set<DynamicsProcessing::mbc>(cfgs);
    mTags.push_back({DynamicsProcessing::mbc, dp});
    for (auto& cfg : cfgs) {
        if (cfg.enable) mMbcChannelEnable.insert(cfg.channel);
    }
}

void DynamicsProcessingTestHelper::addPreEqBandConfigs(
        const std::vector<DynamicsProcessing::EqBandConfig>& cfgs) {
    DynamicsProcessing dp;
    dp.set<DynamicsProcessing::preEqBand>(cfgs);
    mTags.push_back({DynamicsProcessing::preEqBand, dp});
}

void DynamicsProcessingTestHelper::addPostEqBandConfigs(
        const std::vector<DynamicsProcessing::EqBandConfig>& cfgs) {
    DynamicsProcessing dp;
    dp.set<DynamicsProcessing::postEqBand>(cfgs);
    mTags.push_back({DynamicsProcessing::postEqBand, dp});
}

void DynamicsProcessingTestHelper::addMbcBandConfigs(
        const std::vector<DynamicsProcessing::MbcBandConfig>& cfgs) {
    DynamicsProcessing dp;
    dp.set<DynamicsProcessing::mbcBand>(cfgs);
    mTags.push_back({DynamicsProcessing::mbcBand, dp});
}

void DynamicsProcessingTestHelper::addLimiterConfig(
        const std::vector<DynamicsProcessing::LimiterConfig>& cfgs) {
    DynamicsProcessing dp;
    dp.set<DynamicsProcessing::limiter>(cfgs);
    mTags.push_back({DynamicsProcessing::limiter, dp});
    for (auto& cfg : cfgs) {
        if (cfg.enable) mLimiterChannelEnable.insert(cfg.channel);
    }
}

void DynamicsProcessingTestHelper::addInputGain(
        const std::vector<DynamicsProcessing::InputGain>& inputGains) {
    DynamicsProcessing dp;
    dp.set<DynamicsProcessing::inputGain>(inputGains);
    mTags.push_back({DynamicsProcessing::inputGain, dp});
}

void DynamicsProcessingTestHelper::checkHalVersion() {
    if (int32_t version;
        mEffect->getInterfaceVersion(&version).isOk() && version < kMinDataTestHalVersion) {
        GTEST_SKIP() << "Skipping the data test for version: " << version << "\n";
    }
}

void fillLimiterConfig(std::vector<DynamicsProcessing::LimiterConfig>& limiterConfigList,
                       int channelIndex, bool enable, int linkGroup, float attackTime,
                       float releaseTime, float ratio, float threshold, float postGain) {
    DynamicsProcessing::LimiterConfig cfg;
    cfg.channel = channelIndex;
    cfg.enable = enable;
    cfg.linkGroup = linkGroup;
    cfg.attackTimeMs = attackTime;
    cfg.releaseTimeMs = releaseTime;
    cfg.ratio = ratio;
    cfg.thresholdDb = threshold;
    cfg.postGainDb = postGain;
    limiterConfigList.push_back(cfg);
}

DynamicsProcessing::MbcBandConfig createMbcBandConfig(int channel, int band, float cutoffFreqHz,
                                                      float attackTimeMs, float releaseTimeMs,
                                                      float ratio, float thresholdDb,
                                                      float kneeWidthDb, float noiseGate,
                                                      float expanderRatio, float preGainDb,
                                                      float postGainDb) {
    return DynamicsProcessing::MbcBandConfig{.channel = channel,
                                             .band = band,
                                             .enable = true,
                                             .cutoffFrequencyHz = cutoffFreqHz,
                                             .attackTimeMs = attackTimeMs,
                                             .releaseTimeMs = releaseTimeMs,
                                             .ratio = ratio,
                                             .thresholdDb = thresholdDb,
                                             .kneeWidthDb = kneeWidthDb,
                                             .noiseGateThresholdDb = noiseGate,
                                             .expanderRatio = expanderRatio,
                                             .preGainDb = preGainDb,
                                             .postGainDb = postGainDb};
}

DynamicsProcessing::EqBandConfig creatEqBandConfig(int channel, int band, float cutOffFreqHz,
                                                   float gainDb, bool enable) {
    return DynamicsProcessing::EqBandConfig{.channel = channel,
                                            .band = band,
                                            .enable = enable,
                                            .cutoffFrequencyHz = cutOffFreqHz,
                                            .gainDb = gainDb};
}

/**
 * Test DynamicsProcessing Engine Configuration
 */
enum EngineArchitectureTestParamName {
    ENGINE_TEST_INSTANCE_NAME,
    ENGINE_TEST_RESOLUTION_PREFERENCE,
    ENGINE_TEST_PREFERRED_DURATION,
    ENGINE_TEST_STAGE_ENABLEMENT
};
using EngineArchitectureTestParams = std::tuple<std::pair<std::shared_ptr<IFactory>, Descriptor>,
                                                DynamicsProcessing::ResolutionPreference, float,
                                                DynamicsProcessing::StageEnablement>;

void fillEngineArchConfig(DynamicsProcessing::EngineArchitecture& cfg,
                          const EngineArchitectureTestParams& params) {
    cfg.resolutionPreference = std::get<ENGINE_TEST_RESOLUTION_PREFERENCE>(params);
    cfg.preferredProcessingDurationMs = std::get<ENGINE_TEST_PREFERRED_DURATION>(params);
    cfg.preEqStage = cfg.postEqStage = cfg.mbcStage =
            std::get<ENGINE_TEST_STAGE_ENABLEMENT>(params);
    cfg.limiterInUse = true;
}

class DynamicsProcessingTestEngineArchitecture
    : public ::testing::TestWithParam<EngineArchitectureTestParams>,
      public DynamicsProcessingTestHelper {
  public:
    DynamicsProcessingTestEngineArchitecture()
        : DynamicsProcessingTestHelper(std::get<ENGINE_TEST_INSTANCE_NAME>(GetParam())) {
        fillEngineArchConfig(mCfg, GetParam());
    };

    void SetUp() override { ASSERT_NO_FATAL_FAILURE(SetUpDynamicsProcessingEffect()); }

    void TearDown() override { TearDownDynamicsProcessingEffect(); }

    DynamicsProcessing::EngineArchitecture mCfg;
};

TEST_P(DynamicsProcessingTestEngineArchitecture, SetAndGetEngineArch) {
    addEngineConfig(mCfg);
    ASSERT_NO_FATAL_FAILURE(SetAndGetDynamicsProcessingParameters());
}

INSTANTIATE_TEST_SUITE_P(
        DynamicsProcessingTest, DynamicsProcessingTestEngineArchitecture,
        ::testing::Combine(
                testing::ValuesIn(EffectFactoryHelper::getAllEffectDescriptors(
                        IFactory::descriptor, getEffectTypeUuidDynamicsProcessing())),
                testing::Values(
                        DynamicsProcessing::ResolutionPreference::FAVOR_TIME_RESOLUTION,
                        DynamicsProcessing::ResolutionPreference::FAVOR_FREQUENCY_RESOLUTION,
                        static_cast<DynamicsProcessing::ResolutionPreference>(-1)),  // variant
                testing::Values(-10.f, 0.f, 10.f),  // processing duration
                testing::ValuesIn(
                        DynamicsProcessingTestHelper::kStageEnablementTestSet)  // preEQ/postEQ/mbc
                ),
        [](const auto& info) {
            auto descriptor = std::get<ENGINE_TEST_INSTANCE_NAME>(info.param).second;
            DynamicsProcessing::EngineArchitecture cfg;
            fillEngineArchConfig(cfg, info.param);
            std::string name = getPrefix(descriptor) + "_Cfg_" + cfg.toString();
            std::replace_if(
                    name.begin(), name.end(), [](const char c) { return !std::isalnum(c); }, '_');
            return name;
        });
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(DynamicsProcessingTestEngineArchitecture);

/**
 * Test DynamicsProcessing Input Gain
 */
enum InputGainTestParamName {
    INPUT_GAIN_INSTANCE_NAME,
    INPUT_GAIN_PARAM,
};
class DynamicsProcessingTestInputGain
    : public ::testing::TestWithParam<std::tuple<std::pair<std::shared_ptr<IFactory>, Descriptor>,
                                                 std::vector<DynamicsProcessing::InputGain>>>,
      public DynamicsProcessingTestHelper {
  public:
    DynamicsProcessingTestInputGain()
        : DynamicsProcessingTestHelper(std::get<INPUT_GAIN_INSTANCE_NAME>(GetParam())),
          mInputGain(std::get<INPUT_GAIN_PARAM>(GetParam())) {};

    void SetUp() override { ASSERT_NO_FATAL_FAILURE(SetUpDynamicsProcessingEffect()); }

    void TearDown() override { TearDownDynamicsProcessingEffect(); }

    const std::vector<DynamicsProcessing::InputGain> mInputGain;
};

TEST_P(DynamicsProcessingTestInputGain, SetAndGetInputGain) {
    addInputGain(mInputGain);
    ASSERT_NO_FATAL_FAILURE(SetAndGetDynamicsProcessingParameters());
}

INSTANTIATE_TEST_SUITE_P(
        DynamicsProcessingTest, DynamicsProcessingTestInputGain,
        ::testing::Combine(testing::ValuesIn(EffectFactoryHelper::getAllEffectDescriptors(
                                   IFactory::descriptor, getEffectTypeUuidDynamicsProcessing())),
                           testing::ValuesIn(DynamicsProcessingTestInputGain::kInputGainTestSet)),
        [](const auto& info) {
            auto descriptor = std::get<INPUT_GAIN_INSTANCE_NAME>(info.param).second;
            std::string gains =
                    ::android::internal::ToString(std::get<INPUT_GAIN_PARAM>(info.param));
            std::string name = "Implementor_" + descriptor.common.implementor + "_name_" +
                               descriptor.common.name + "_UUID_" +
                               toString(descriptor.common.id.uuid) + "_inputGains_" + gains;
            std::replace_if(
                    name.begin(), name.end(), [](const char c) { return !std::isalnum(c); }, '_');
            return name;
        });
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(DynamicsProcessingTestInputGain);

class DynamicsProcessingInputGainDataTest
    : public ::testing::TestWithParam<std::pair<std::shared_ptr<IFactory>, Descriptor>>,
      public DynamicsProcessingTestHelper {
  public:
    DynamicsProcessingInputGainDataTest()
        : DynamicsProcessingTestHelper((GetParam()), AudioChannelLayout::LAYOUT_MONO) {}

    void SetUp() override {
        ASSERT_NO_FATAL_FAILURE(setUpDataTest({kInputFrequency}, kSineFullScaleDb));
    }

    void TearDown() override { TearDownDynamicsProcessingEffect(); }

    void cleanUpInputGainConfig() {
        CleanUp();
        mInputGain.clear();
    }

    std::vector<DynamicsProcessing::InputGain> mInputGain;
};

TEST_P(DynamicsProcessingInputGainDataTest, SetAndGetInputGain) {
    std::vector<float> gainDbValues = {-85, -40, 0, 40, 85};
    for (float gainDb : gainDbValues) {
        cleanUpInputGainConfig();
        for (int i = 0; i < mChannelCount; i++) {
            mInputGain.push_back(DynamicsProcessing::InputGain(i, gainDb));
        }
        std::vector<float> output(mInput.size());
        addInputGain(mInputGain);
        EXPECT_NO_FATAL_FAILURE(setParamsAndProcess(mInput, output));
        if (!isAllParamsValid()) {
            continue;
        }
        float outputDb = calculateDb(output, kStartIndex);
        EXPECT_NEAR(outputDb, mInputDb + gainDb, kToleranceDb)
                << "InputGain: " << gainDb << ", OutputDb: " << outputDb;
    }
}

INSTANTIATE_TEST_SUITE_P(DynamicsProcessingTest, DynamicsProcessingInputGainDataTest,
                         testing::ValuesIn(EffectFactoryHelper::getAllEffectDescriptors(
                                 IFactory::descriptor, getEffectTypeUuidDynamicsProcessing())),
                         [](const auto& info) {
                             auto descriptor = info.param;
                             std::string name = getPrefix(descriptor.second);
                             std::replace_if(
                                     name.begin(), name.end(),
                                     [](const char c) { return !std::isalnum(c); }, '_');
                             return name;
                         });
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(DynamicsProcessingInputGainDataTest);

/**
 * Test DynamicsProcessing Limiter Config
 */
enum LimiterConfigTestParamName {
    LIMITER_INSTANCE_NAME,
    LIMITER_CHANNEL,
    LIMITER_LINK_GROUP,
    LIMITER_ATTACK_TIME,
    LIMITER_RELEASE_TIME,
    LIMITER_RATIO,
    LIMITER_THRESHOLD,
    LIMITER_POST_GAIN,
};

using LimiterConfigTestParams = std::tuple<std::pair<std::shared_ptr<IFactory>, Descriptor>,
                                           int32_t, int32_t, float, float, float, float, float>;

void fillLimiterConfig(std::vector<DynamicsProcessing::LimiterConfig>& cfg,
                       const LimiterConfigTestParams& params) {
    fillLimiterConfig(cfg, std::get<LIMITER_CHANNEL>(params), true,
                      std::get<LIMITER_LINK_GROUP>(params), std::get<LIMITER_ATTACK_TIME>(params),
                      std::get<LIMITER_RELEASE_TIME>(params), std::get<LIMITER_RATIO>(params),
                      std::get<LIMITER_THRESHOLD>(params), std::get<LIMITER_POST_GAIN>(params));
}

class DynamicsProcessingTestLimiterConfig
    : public ::testing::TestWithParam<LimiterConfigTestParams>,
      public DynamicsProcessingTestHelper {
  public:
    DynamicsProcessingTestLimiterConfig()
        : DynamicsProcessingTestHelper(std::get<LIMITER_INSTANCE_NAME>(GetParam())) {
        fillLimiterConfig(mLimiterConfigList, GetParam());
    }

    void SetUp() override { ASSERT_NO_FATAL_FAILURE(SetUpDynamicsProcessingEffect()); }

    void TearDown() override { TearDownDynamicsProcessingEffect(); }

    DynamicsProcessing::LimiterConfig mCfg;
    std::vector<DynamicsProcessing::LimiterConfig> mLimiterConfigList;
};

TEST_P(DynamicsProcessingTestLimiterConfig, SetAndGetLimiterConfig) {
    addEngineConfig(mEngineConfigPreset);
    addLimiterConfig(mLimiterConfigList);
    ASSERT_NO_FATAL_FAILURE(SetAndGetDynamicsProcessingParameters());
}

INSTANTIATE_TEST_SUITE_P(
        DynamicsProcessingTest, DynamicsProcessingTestLimiterConfig,
        ::testing::Combine(testing::ValuesIn(EffectFactoryHelper::getAllEffectDescriptors(
                                   IFactory::descriptor, getEffectTypeUuidDynamicsProcessing())),
                           testing::Values(-1, 0, 1, 2),  // channel index
                           testing::Values(3),            // link group
                           testing::Values(-1, 1),        // attackTime
                           testing::Values(-60, 60),      // releaseTime
                           testing::Values(-2.5, 2.5),    // ratio
                           testing::Values(-2, 2),        // thresh
                           testing::Values(-3.14, 3.14)   // postGain
                           ),
        [](const auto& info) {
            auto descriptor = std::get<LIMITER_INSTANCE_NAME>(info.param).second;
            std::vector<DynamicsProcessing::LimiterConfig> cfg;
            fillLimiterConfig(cfg, info.param);
            std::string name =
                    "Implementer_" + getPrefix(descriptor) + "_limiterConfig_" + cfg[0].toString();
            std::replace_if(
                    name.begin(), name.end(), [](const char c) { return !std::isalnum(c); }, '_');
            return name;
        });
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(DynamicsProcessingTestLimiterConfig);

using LimiterConfigDataTestParams = std::pair<std::shared_ptr<IFactory>, Descriptor>;

class DynamicsProcessingLimiterConfigDataTest
    : public ::testing::TestWithParam<LimiterConfigDataTestParams>,
      public DynamicsProcessingTestHelper {
  public:
    DynamicsProcessingLimiterConfigDataTest(LimiterConfigDataTestParams param = GetParam(),
                                            int32_t layout = AudioChannelLayout::LAYOUT_MONO)
        : DynamicsProcessingTestHelper(param, layout) {}

    void SetUp() override {
        ASSERT_NO_FATAL_FAILURE(setUpDataTest({kInputFrequency}, kSineFullScaleDb));
    }

    void TearDown() override { TearDownDynamicsProcessingEffect(); }

    void computeThreshold(float ratio, float outputDb, float& threshold) {
        EXPECT_NE(ratio, 0);
        threshold = (mInputDb - (ratio * outputDb)) / (1 - ratio);
    }

    void computeRatio(float threshold, float outputDb, float& ratio) {
        float inputOverThreshold = mInputDb - threshold;
        float outputOverThreshold = outputDb - threshold;
        EXPECT_NE(outputOverThreshold, 0);
        ratio = inputOverThreshold / outputOverThreshold;
    }

    void setLimiterParamsAndProcess(std::vector<float>& input, std::vector<float>& output,
                                    bool isEngineLimiterEnabled = true) {
        mEngineConfigPreset.limiterInUse = isEngineLimiterEnabled;
        addEngineConfig(mEngineConfigPreset);
        addLimiterConfig(mLimiterConfigList);
        EXPECT_NO_FATAL_FAILURE(setParamsAndProcess(input, output));
    }

    void testEnableDisableConfiguration(bool isLimiterEnabled, bool isEngineLimiterEnabled) {
        cleanUpLimiterConfig();
        std::vector<float> output(mInput.size());
        for (int i = 0; i < mChannelCount; i++) {
            // Set non-default values
            fillLimiterConfig(mLimiterConfigList, i, isLimiterEnabled, kDefaultLinkerGroup,
                              5 /*attack time*/, 5 /*release time*/, 10 /*ratio*/,
                              -20 /*threshold*/, 5 /*postgain*/);
        }
        ASSERT_NO_FATAL_FAILURE(setLimiterParamsAndProcess(mInput, output, isEngineLimiterEnabled));
        float outputdB = calculateDb(output, kStartIndex);
        if (isAllParamsValid()) {
            if (isLimiterEnabled && isEngineLimiterEnabled) {
                EXPECT_GT(std::abs(mInputDb - outputdB), kMinDifferenceDb)
                        << "Input level: " << mInputDb << " Output level: " << outputdB;
            } else {
                EXPECT_NEAR(mInputDb, outputdB, kLimiterTestToleranceDb);
            }
        }
    }

    void testReleaseTimeParam(float thresholdDb, bool isLimiterEngaged) {
        for (size_t i = mInput.size() / 2; i < mInput.size(); i++) {
            mInput[i] = mInput[i] / 2;
        }
        float firstHalfDb = calculateDb(mInput, 0, mInput.size() / 2);
        float secondHalfDb = calculateDb(mInput, mInput.size() / 2, mInput.size());
        mInputDb = calculateDb(mInput, 0, mInput.size());

        float referenceDb;
        if (isLimiterEngaged) {
            ASSERT_TRUE(thresholdDb < firstHalfDb && thresholdDb >= secondHalfDb)
                    << "Threshold level: " << thresholdDb << "First half level: " << firstHalfDb
                    << "Second half level: " << secondHalfDb;
            referenceDb = FLT_MAX;
        } else {
            ASSERT_TRUE(thresholdDb > firstHalfDb && thresholdDb > secondHalfDb)
                    << "Threshold level: " << thresholdDb << "First half level: " << firstHalfDb
                    << "Second half level: " << secondHalfDb;
            referenceDb = mInputDb;
        }
        std::vector<float> output(mInput.size());
        for (float releaseTimeMs : kReleaseTimeMsValues) {
            cleanUpLimiterConfig();
            for (int i = 0; i < mChannelCount; i++) {
                fillLimiterConfig(mLimiterConfigList, i, true, kDefaultLinkerGroup,
                                  kDefaultAttackTime, releaseTimeMs, kDefaultRatio, thresholdDb,
                                  kDefaultPostGain);
            }
            ASSERT_NO_FATAL_FAILURE(setLimiterParamsAndProcess(mInput, output));
            if (!isAllParamsValid()) {
                continue;
            }
            float outputDb = calculateDb(output, kStartIndex);
            if (isLimiterEngaged) {
                /*Release time determines how quickly the compressor returns to normal after the
                 * input falls below the threshold. As the release time increases, it takes longer
                 * for the compressor to stop compressing, resulting in a decrease in output
                 * decibels as the release time increases*/
                ASSERT_LT(outputDb, referenceDb) << "Release Time: " << releaseTimeMs;
                referenceDb = outputDb;
            } else {
                // No change in the outputdB when the limiter is not enganged
                EXPECT_NEAR(outputDb, referenceDb, kToleranceDb)
                        << "Release Time: " << releaseTimeMs;
            }
        }
    }

    void cleanUpLimiterConfig() {
        CleanUp();
        mLimiterConfigList.clear();
    }
    static constexpr float kDefaultLinkerGroup = 3;
    static constexpr float kDefaultAttackTime = 0;
    static constexpr float kDefaultReleaseTime = 0;
    static constexpr float kDefaultRatio = 4;
    static constexpr float kDefaultThreshold = -10;
    static constexpr float kDefaultPostGain = 0;
    static constexpr float kLimiterTestToleranceDb = 0.05;
    static constexpr float kMinDifferenceDb = 5;
    const std::vector<bool> kEnableValues = {true, false, true};
    const std::vector<float> kReleaseTimeMsValues = {0, 10, 20, 30, 40, 50};
    std::vector<DynamicsProcessing::LimiterConfig> mLimiterConfigList;
    int mBufferSize;
};

TEST_P(DynamicsProcessingLimiterConfigDataTest, IncreasingThresholdDb) {
    std::vector<float> thresholdValues = {-200, -150, -100, -50, -5, 0};
    std::vector<float> output(mInput.size());
    float previousThreshold = -FLT_MAX;
    for (float threshold : thresholdValues) {
        cleanUpLimiterConfig();
        for (int i = 0; i < mChannelCount; i++) {
            fillLimiterConfig(mLimiterConfigList, i, true, kDefaultLinkerGroup, kDefaultAttackTime,
                              kDefaultReleaseTime, kDefaultRatio, threshold, kDefaultPostGain);
        }
        ASSERT_NO_FATAL_FAILURE(setLimiterParamsAndProcess(mInput, output));
        if (!isAllParamsValid()) {
            continue;
        }
        float outputDb = calculateDb(output, kStartIndex);
        if (threshold >= mInputDb || kDefaultRatio == 1) {
            EXPECT_NEAR(mInputDb, outputDb, kLimiterTestToleranceDb);
        } else {
            float calculatedThreshold = 0;
            ASSERT_NO_FATAL_FAILURE(computeThreshold(kDefaultRatio, outputDb, calculatedThreshold));
            ASSERT_GT(calculatedThreshold, previousThreshold);
            previousThreshold = calculatedThreshold;
        }
    }
}

TEST_P(DynamicsProcessingLimiterConfigDataTest, IncreasingRatio) {
    std::vector<float> ratioValues = {1, 10, 20, 30, 40, 50};
    std::vector<float> output(mInput.size());
    float previousRatio = 0;
    for (float ratio : ratioValues) {
        cleanUpLimiterConfig();
        for (int i = 0; i < mChannelCount; i++) {
            fillLimiterConfig(mLimiterConfigList, i, true, kDefaultLinkerGroup, kDefaultAttackTime,
                              kDefaultReleaseTime, ratio, kDefaultThreshold, kDefaultPostGain);
        }
        ASSERT_NO_FATAL_FAILURE(setLimiterParamsAndProcess(mInput, output));
        if (!isAllParamsValid()) {
            continue;
        }
        float outputDb = calculateDb(output, kStartIndex);

        if (kDefaultThreshold >= mInputDb) {
            EXPECT_NEAR(mInputDb, outputDb, kLimiterTestToleranceDb);
        } else {
            float calculatedRatio = 0;
            ASSERT_NO_FATAL_FAILURE(computeRatio(kDefaultThreshold, outputDb, calculatedRatio));
            ASSERT_GT(calculatedRatio, previousRatio);
            previousRatio = calculatedRatio;
        }
    }
}

TEST_P(DynamicsProcessingLimiterConfigDataTest, IncreasingPostGain) {
    std::vector<float> postGainDbValues = {-85, -40, 0, 40, 85};
    std::vector<float> output(mInput.size());
    for (float postGainDb : postGainDbValues) {
        cleanUpLimiterConfig();
        ASSERT_NO_FATAL_FAILURE(generateSineWave(kInputFrequency, mInput, dBToAmplitude(postGainDb),
                                                 kSamplingFrequency, mChannelLayout));
        mInputDb = calculateDb(mInput);
        EXPECT_NEAR(mInputDb, kSineFullScaleDb - postGainDb, kLimiterTestToleranceDb);
        for (int i = 0; i < mChannelCount; i++) {
            fillLimiterConfig(mLimiterConfigList, i, true, kDefaultLinkerGroup, kDefaultAttackTime,
                              kDefaultReleaseTime, 1, kDefaultThreshold, postGainDb);
        }
        ASSERT_NO_FATAL_FAILURE(setLimiterParamsAndProcess(mInput, output));
        if (!isAllParamsValid()) {
            continue;
        }
        float outputDb = calculateDb(output, kStartIndex);
        EXPECT_NEAR(outputDb, mInputDb + postGainDb, kLimiterTestToleranceDb)
                << "PostGain: " << postGainDb << ", OutputDb: " << outputDb;
    }
}

TEST_P(DynamicsProcessingLimiterConfigDataTest, LimiterEnableDisable) {
    for (bool isLimiterEnabled : kEnableValues) {
        ASSERT_NO_FATAL_FAILURE(
                testEnableDisableConfiguration(isLimiterEnabled, true /*Engine Enabled*/));
    }
}

TEST_P(DynamicsProcessingLimiterConfigDataTest, LimiterEnableDisableViaEngine) {
    for (bool isEngineLimiterEnabled : kEnableValues) {
        ASSERT_NO_FATAL_FAILURE(
                testEnableDisableConfiguration(true /*Limiter Enabled*/, isEngineLimiterEnabled));
    }
}

TEST_P(DynamicsProcessingLimiterConfigDataTest, LimiterReleaseTime) {
    // Using a threshold dB value that compresses only the first half of the input.
    float thresholdDb = -7;
    ASSERT_NO_FATAL_FAILURE(testReleaseTimeParam(thresholdDb, true));
}

TEST_P(DynamicsProcessingLimiterConfigDataTest, LimiterNotEngagedReleaseTimeTest) {
    // Using threshold value such that limiter does not engage with the input
    float thresholdDb = -1;
    ASSERT_NO_FATAL_FAILURE(testReleaseTimeParam(thresholdDb, false));
}

INSTANTIATE_TEST_SUITE_P(DynamicsProcessingTest, DynamicsProcessingLimiterConfigDataTest,
                         testing::ValuesIn(EffectFactoryHelper::getAllEffectDescriptors(
                                 IFactory::descriptor, getEffectTypeUuidDynamicsProcessing())),
                         [](const auto& info) {
                             auto descriptor = info.param;
                             std::string name = getPrefix(descriptor.second);
                             std::replace_if(
                                     name.begin(), name.end(),
                                     [](const char c) { return !std::isalnum(c); }, '_');
                             return name;
                         });
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(DynamicsProcessingLimiterConfigDataTest);

class DynamicsProcessingLimiterLinkerDataTest : public DynamicsProcessingLimiterConfigDataTest {
  public:
    DynamicsProcessingLimiterLinkerDataTest()
        : DynamicsProcessingLimiterConfigDataTest(GetParam(), AudioChannelLayout::LAYOUT_STEREO) {}

    void calculateExpectedOutputDb(std::vector<float>& expectedOutputDb) {
        std::vector<float> inputDbValues = calculateStereoDb(mInput, kStartIndex);
        ASSERT_EQ(inputDbValues.size(), kRatioThresholdPairValues.size());
        EXPECT_NEAR(inputDbValues[0], inputDbValues[1], kToleranceDb);
        for (size_t i = 0; i < kRatioThresholdPairValues.size(); i++) {
            const auto& [ratio, threshold] = kRatioThresholdPairValues[i];
            expectedOutputDb.push_back((inputDbValues[i] - threshold) / ratio + threshold);
        }
    }

    std::vector<float> calculateStereoDb(const std::vector<float>& input,
                                         size_t startSamplePos = 0) {
        std::vector<float> leftChannel;
        std::vector<float> rightChannel;
        for (size_t i = 0; i < input.size(); i += 2) {
            leftChannel.push_back(input[i]);
            if (i + 1 < input.size()) {
                rightChannel.push_back(input[i + 1]);
            }
        }
        return {calculateDb(leftChannel, startSamplePos),
                calculateDb(rightChannel, startSamplePos)};
    }

    void setLinkGroupAndProcess(std::vector<float>& output, bool hasSameLinkGroup) {
        for (int i = 0; i < mChannelCount; i++) {
            const auto& [ratio, threshold] = kRatioThresholdPairValues[i];
            ASSERT_NE(ratio, 0);
            int linkGroup = hasSameLinkGroup ? kDefaultLinkerGroup : i;
            fillLimiterConfig(mLimiterConfigList, i, true, linkGroup, kDefaultAttackTime,
                              kDefaultReleaseTime, ratio, threshold, kDefaultPostGain);
        }

        ASSERT_NO_FATAL_FAILURE(setLimiterParamsAndProcess(mInput, output));

        if (!isAllParamsValid()) {
            GTEST_SKIP() << "Invalid parameters. Skipping the test\n";
        }
    }

    const std::vector<std::pair<float, float>> kRatioThresholdPairValues = {{2, -10}, {5, -20}};
};

TEST_P(DynamicsProcessingLimiterLinkerDataTest, SameLinkGroupDifferentConfigs) {
    std::vector<float> output(mInput.size());

    ASSERT_NO_FATAL_FAILURE(setLinkGroupAndProcess(output, true));

    std::vector<float> outputDbValues = calculateStereoDb(output, kStartIndex);

    std::vector<float> expectedOutputDbValues;
    ASSERT_NO_FATAL_FAILURE(calculateExpectedOutputDb(expectedOutputDbValues));

    // Verify that the actual output dB is same as the calculated maximum attenuation.
    float expectedOutputDb = std::min(expectedOutputDbValues[0], expectedOutputDbValues[1]);
    EXPECT_NEAR(outputDbValues[0], expectedOutputDb, kToleranceDb);
    EXPECT_NEAR(outputDbValues[1], expectedOutputDb, kToleranceDb);
}

TEST_P(DynamicsProcessingLimiterLinkerDataTest, DifferentLinkGroupDifferentConfigs) {
    std::vector<float> output(mInput.size());

    ASSERT_NO_FATAL_FAILURE(setLinkGroupAndProcess(output, false));

    std::vector<float> outputDbValues = calculateStereoDb(output, kStartIndex);

    std::vector<float> expectedOutputDbValues;
    ASSERT_NO_FATAL_FAILURE(calculateExpectedOutputDb(expectedOutputDbValues));

    // Verify that both channels have different compression levels
    EXPECT_GT(abs(expectedOutputDbValues[0] - expectedOutputDbValues[1]), kMinDifferenceDb)
            << "Left channel level: " << expectedOutputDbValues[0]
            << " Right channel level: " << expectedOutputDbValues[1];

    // Verify that the actual output and the calculated dB values are same
    EXPECT_NEAR(outputDbValues[0], expectedOutputDbValues[0], kToleranceDb);
    EXPECT_NEAR(outputDbValues[1], expectedOutputDbValues[1], kToleranceDb);
}

INSTANTIATE_TEST_SUITE_P(DynamicsProcessingTest, DynamicsProcessingLimiterLinkerDataTest,
                         testing::ValuesIn(EffectFactoryHelper::getAllEffectDescriptors(
                                 IFactory::descriptor, getEffectTypeUuidDynamicsProcessing())),
                         [](const auto& info) {
                             auto descriptor = info.param;
                             std::string name = getPrefix(descriptor.second);
                             std::replace_if(
                                     name.begin(), name.end(),
                                     [](const char c) { return !std::isalnum(c); }, '_');
                             return name;
                         });
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(DynamicsProcessingLimiterLinkerDataTest);

/**
 * Test DynamicsProcessing ChannelConfig
 */
enum ChannelConfigTestParamName {
    BAND_CHANNEL_TEST_INSTANCE_NAME,
    BAND_CHANNEL_TEST_CHANNEL_CONFIG
};
using ChannelConfigTestParams = std::tuple<std::pair<std::shared_ptr<IFactory>, Descriptor>,
                                           std::vector<DynamicsProcessing::ChannelConfig>>;

class DynamicsProcessingTestChannelConfig
    : public ::testing::TestWithParam<ChannelConfigTestParams>,
      public DynamicsProcessingTestHelper {
  public:
    DynamicsProcessingTestChannelConfig()
        : DynamicsProcessingTestHelper(std::get<BAND_CHANNEL_TEST_INSTANCE_NAME>(GetParam())),
          mCfg(std::get<BAND_CHANNEL_TEST_CHANNEL_CONFIG>(GetParam())) {}

    void SetUp() override { ASSERT_NO_FATAL_FAILURE(SetUpDynamicsProcessingEffect()); }

    void TearDown() override { TearDownDynamicsProcessingEffect(); }

    std::vector<DynamicsProcessing::ChannelConfig> mCfg;
};

TEST_P(DynamicsProcessingTestChannelConfig, SetAndGetPreEqChannelConfig) {
    addEngineConfig(mEngineConfigPreset);
    addPreEqChannelConfig(mCfg);
    ASSERT_NO_FATAL_FAILURE(SetAndGetDynamicsProcessingParameters());
}

TEST_P(DynamicsProcessingTestChannelConfig, SetAndGetPostEqChannelConfig) {
    addEngineConfig(mEngineConfigPreset);
    addPostEqChannelConfig(mCfg);
    ASSERT_NO_FATAL_FAILURE(SetAndGetDynamicsProcessingParameters());
}

TEST_P(DynamicsProcessingTestChannelConfig, SetAndGetMbcChannelConfig) {
    addEngineConfig(mEngineConfigPreset);
    addMbcChannelConfig(mCfg);
    ASSERT_NO_FATAL_FAILURE(SetAndGetDynamicsProcessingParameters());
}

INSTANTIATE_TEST_SUITE_P(
        DynamicsProcessingTest, DynamicsProcessingTestChannelConfig,
        ::testing::Combine(
                testing::ValuesIn(EffectFactoryHelper::getAllEffectDescriptors(
                        IFactory::descriptor, getEffectTypeUuidDynamicsProcessing())),
                testing::ValuesIn(
                        DynamicsProcessingTestHelper::kChannelConfigTestSet)),  // channel config
        [](const auto& info) {
            auto descriptor = std::get<BAND_CHANNEL_TEST_INSTANCE_NAME>(info.param).second;
            std::string channelConfig = ::android::internal::ToString(
                    std::get<BAND_CHANNEL_TEST_CHANNEL_CONFIG>(info.param));

            std::string name = "Implementor_" + descriptor.common.implementor + "_name_" +
                               descriptor.common.name + "_UUID_" +
                               toString(descriptor.common.id.uuid) + "_" + channelConfig;
            std::replace_if(
                    name.begin(), name.end(), [](const char c) { return !std::isalnum(c); }, '_');
            return name;
        });
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(DynamicsProcessingTestChannelConfig);

/**
 * Test DynamicsProcessing EqBandConfig
 */
enum EqBandConfigTestParamName {
    EQ_BAND_INSTANCE_NAME,
    EQ_BAND_CHANNEL,
    EQ_BAND_CUT_OFF_FREQ,
    EQ_BAND_GAIN
};
using EqBandConfigTestParams = std::tuple<std::pair<std::shared_ptr<IFactory>, Descriptor>, int32_t,
                                          std::vector<std::pair<int, float>>, float>;

void fillEqBandConfig(std::vector<DynamicsProcessing::EqBandConfig>& cfgs,
                      const EqBandConfigTestParams& params) {
    const std::vector<std::pair<int, float>> cutOffFreqs = std::get<EQ_BAND_CUT_OFF_FREQ>(params);
    int bandCount = cutOffFreqs.size();
    for (int i = 0; i < bandCount; i++) {
        cfgs.push_back(creatEqBandConfig(std::get<EQ_BAND_CHANNEL>(params), cutOffFreqs[i].first,
                                         cutOffFreqs[i].second, std::get<EQ_BAND_GAIN>(params),
                                         true));
    }
}

class DynamicsProcessingTestEqBandConfig : public ::testing::TestWithParam<EqBandConfigTestParams>,
                                           public DynamicsProcessingTestHelper {
  public:
    DynamicsProcessingTestEqBandConfig()
        : DynamicsProcessingTestHelper(std::get<EQ_BAND_INSTANCE_NAME>(GetParam())) {
        fillEqBandConfig(mCfgs, GetParam());
    }

    void SetUp() override { ASSERT_NO_FATAL_FAILURE(SetUpDynamicsProcessingEffect()); }

    void TearDown() override { TearDownDynamicsProcessingEffect(); }

    std::vector<DynamicsProcessing::EqBandConfig> mCfgs;
};

TEST_P(DynamicsProcessingTestEqBandConfig, SetAndGetPreEqBandConfig) {
    mEngineConfigPreset.preEqStage.bandCount = mCfgs.size();
    addEngineConfig(mEngineConfigPreset);
    std::vector<DynamicsProcessing::ChannelConfig> cfgs(mChannelCount);
    for (int i = 0; i < mChannelCount; i++) {
        cfgs[i].channel = i;
        cfgs[i].enable = true;
    }
    addPreEqChannelConfig(cfgs);
    addPreEqBandConfigs(mCfgs);
    ASSERT_NO_FATAL_FAILURE(SetAndGetDynamicsProcessingParameters());
}

TEST_P(DynamicsProcessingTestEqBandConfig, SetAndGetPostEqBandConfig) {
    mEngineConfigPreset.postEqStage.bandCount = mCfgs.size();
    addEngineConfig(mEngineConfigPreset);
    std::vector<DynamicsProcessing::ChannelConfig> cfgs(mChannelCount);
    for (int i = 0; i < mChannelCount; i++) {
        cfgs[i].channel = i;
        cfgs[i].enable = true;
    }
    addPostEqChannelConfig(cfgs);
    addPostEqBandConfigs(mCfgs);
    ASSERT_NO_FATAL_FAILURE(SetAndGetDynamicsProcessingParameters());
}

std::vector<std::vector<std::pair<int, float>>> kBands{
        {
                {0, 600},
                {1, 2000},
                {2, 6000},
                {3, 10000},
                {4, 16000},
                {5, 20000},
                {6, 26000},
                {7, 30000},
                {8, 36000},
                {9, 40000},
        },  // 10 bands
        {
                {0, 800},
                {3, 15000},
                {2, 6000},
                {1, 2000},
        },  // 4 bands, unsorted
        {
                {0, 650},
                {1, 2000},
                {2, 6000},
                {3, 10000},
                {3, 16000},
        },  // 5 bands, missing band
        {
                {0, 900},
                {1, 8000},
                {2, 4000},
                {3, 12000},
        },  // 4 bands, cutoff freq not increasing
        {
                {0, 450},
                {1, 2000},
                {7, 6000},
                {3, 10000},
                {4, 16000},
        },  // bad band index
        {
                {0, 1},
                {1, 8000},
        },  // too low cutoff freq
        {
                {0, 1200},
                {1, 80000},
        },  // too high cutoff freq
};

INSTANTIATE_TEST_SUITE_P(
        DynamicsProcessingTest, DynamicsProcessingTestEqBandConfig,
        ::testing::Combine(testing::ValuesIn(EffectFactoryHelper::getAllEffectDescriptors(
                                   IFactory::descriptor, getEffectTypeUuidDynamicsProcessing())),
                           testing::Values(-1, 0, 10),     // channel index
                           testing::ValuesIn(kBands),      // band index, cut off frequencies
                           testing::Values(-3.14f, 3.14f)  // gain
                           ),
        [](const auto& info) {
            auto descriptor = std::get<EQ_BAND_INSTANCE_NAME>(info.param).second;
            std::vector<DynamicsProcessing::EqBandConfig> cfgs;
            fillEqBandConfig(cfgs, info.param);
            std::string bands = ::android::internal::ToString(cfgs);
            std::string name = "Implementor_" + descriptor.common.implementor + "_name_" +
                               descriptor.common.name + "_UUID_" +
                               toString(descriptor.common.id.uuid) + "_bands_" + bands;
            std::replace_if(
                    name.begin(), name.end(), [](const char c) { return !std::isalnum(c); }, '_');
            return name;
        });
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(DynamicsProcessingTestEqBandConfig);

class DynamicsProcessingEqBandConfigDataTest
    : public ::testing::TestWithParam<std::pair<std::shared_ptr<IFactory>, Descriptor>>,
      public DynamicsProcessingTestHelper {
  public:
    DynamicsProcessingEqBandConfigDataTest()
        : DynamicsProcessingTestHelper(GetParam(), AudioChannelLayout::LAYOUT_MONO) {
        mBinOffsets.resize(mMultitoneTestFrequencies.size());
    }

    void SetUp() override {
        ASSERT_NO_FATAL_FAILURE(
                setUpDataTest(mMultitoneTestFrequencies, kSineMultitoneFullScaleDb));
    }

    void TearDown() override { TearDownDynamicsProcessingEffect(); }

    void addEqParam(bool isPreEq) {
        createChannelConfig();
        auto stage = isPreEq ? mEngineConfigPreset.preEqStage : mEngineConfigPreset.postEqStage;
        stage.bandCount = mCfgs.size();
        addEngineConfig(mEngineConfigPreset);
        isPreEq ? addPreEqChannelConfig(mChannelConfig) : addPostEqChannelConfig(mChannelConfig);
        isPreEq ? addPreEqBandConfigs(mCfgs) : addPostEqBandConfigs(mCfgs);
    }

    void setEqParamAndProcess(std::vector<float>& output, bool isPreEq) {
        addEqParam(isPreEq);
        ASSERT_NO_FATAL_FAILURE(setParamsAndProcess(mInput, output));
    }

    void fillEqBandConfig(std::vector<DynamicsProcessing::EqBandConfig>& cfgs, int channelIndex,
                          int bandIndex, int cutOffFreqHz, float gainDb, bool enable) {
        cfgs.push_back(creatEqBandConfig(channelIndex, bandIndex, static_cast<float>(cutOffFreqHz),
                                         gainDb, enable));
    }

    void validateOutput(const std::vector<float>& output, float gainDb, size_t bandIndex,
                        bool enable) {
        std::vector<float> outputMag(mBinOffsets.size());
        EXPECT_NO_FATAL_FAILURE(getMagnitudeValue(output, outputMag));
        if (gainDb == 0 || !enable) {
            EXPECT_NO_FATAL_FAILURE(checkInputAndOutputEquality(outputMag));
        } else if (gainDb > 0) {
            // For positive gain, current band's magnitude is greater than the other band's
            // magnitude
            EXPECT_GT(outputMag[bandIndex], outputMag[bandIndex ^ 1]);
        } else {
            // For negative gain, current band's magnitude is less than the other band's magnitude
            EXPECT_LT(outputMag[bandIndex], outputMag[bandIndex ^ 1]);
        }
    }

    void analyseMultiBandOutput(float gainDb, bool isPreEq, bool enable = true) {
        std::vector<float> output(mInput.size());
        roundToFreqCenteredToFftBin(mMultitoneTestFrequencies, mBinOffsets, kBinWidth);
        // Set Equalizer values for two bands
        for (size_t i = 0; i < kCutoffFreqHz.size(); i++) {
            for (int channelIndex = 0; channelIndex < mChannelCount; channelIndex++) {
                fillEqBandConfig(mCfgs, channelIndex, i, kCutoffFreqHz[i], gainDb, enable);
                fillEqBandConfig(mCfgs, channelIndex, i ^ 1, kCutoffFreqHz[i ^ 1], 0, enable);
            }
            ASSERT_NO_FATAL_FAILURE(setEqParamAndProcess(output, isPreEq));

            if (isAllParamsValid()) {
                ASSERT_NO_FATAL_FAILURE(validateOutput(output, gainDb, i, enable));
            }
            cleanUpEqConfig();
        }
    }

    void cleanUpEqConfig() {
        CleanUp();
        mCfgs.clear();
        mChannelConfig.clear();
    }

    const std::vector<float> kTestGainDbValues = {-200, -100, 0, 100, 200};
    std::vector<DynamicsProcessing::EqBandConfig> mCfgs;
};

TEST_P(DynamicsProcessingEqBandConfigDataTest, IncreasingPreEqGain) {
    for (float gainDb : kTestGainDbValues) {
        ASSERT_NO_FATAL_FAILURE(generateSineWave(mMultitoneTestFrequencies, mInput,
                                                 dBToAmplitude(gainDb), kSamplingFrequency,
                                                 mChannelLayout));
        cleanUpEqConfig();
        ASSERT_NO_FATAL_FAILURE(analyseMultiBandOutput(gainDb, true /*pre-equalizer*/));
    }
}

TEST_P(DynamicsProcessingEqBandConfigDataTest, IncreasingPostEqGain) {
    for (float gainDb : kTestGainDbValues) {
        ASSERT_NO_FATAL_FAILURE(generateSineWave(mMultitoneTestFrequencies, mInput,
                                                 dBToAmplitude(gainDb), kSamplingFrequency,
                                                 mChannelLayout));
        cleanUpEqConfig();
        ASSERT_NO_FATAL_FAILURE(analyseMultiBandOutput(gainDb, false /*post-equalizer*/));
    }
}

TEST_P(DynamicsProcessingEqBandConfigDataTest, PreEqEnableDisable) {
    ASSERT_NO_FATAL_FAILURE(analyseMultiBandOutput(10 /*gain dB*/, true /*pre-equalizer*/,
                                                   false /*disable equalizer*/));
}

TEST_P(DynamicsProcessingEqBandConfigDataTest, PostEqEnableDisable) {
    ASSERT_NO_FATAL_FAILURE(analyseMultiBandOutput(10 /*gain dB*/, false /*post-equalizer*/,
                                                   false /*disable equalizer*/));
}

INSTANTIATE_TEST_SUITE_P(DynamicsProcessingTest, DynamicsProcessingEqBandConfigDataTest,
                         testing::ValuesIn(EffectFactoryHelper::getAllEffectDescriptors(
                                 IFactory::descriptor, getEffectTypeUuidDynamicsProcessing())),
                         [](const auto& info) {
                             auto descriptor = info.param;
                             std::string name = getPrefix(descriptor.second);
                             std::replace_if(
                                     name.begin(), name.end(),
                                     [](const char c) { return !std::isalnum(c); }, '_');
                             return name;
                         });
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(DynamicsProcessingEqBandConfigDataTest);

/**
 * Test DynamicsProcessing MbcBandConfig
 */

enum MbcBandConfigParamName {
    MBC_BAND_INSTANCE_NAME,
    MBC_BAND_CHANNEL,
    MBC_BAND_CUTOFF_FREQ,
    MBC_BAND_ADDITIONAL
};
enum MbcBandConfigAdditional {
    MBC_ADD_ATTACK_TIME,
    MBC_ADD_RELEASE_TIME,
    MBC_ADD_RATIO,
    MBC_ADD_THRESHOLD,
    MBC_ADD_KNEE_WIDTH,
    MBC_ADD_NOISE_GATE_THRESHOLD,
    MBC_ADD_EXPENDER_RATIO,
    MBC_ADD_PRE_GAIN,
    MBC_ADD_POST_GAIN,
    MBC_ADD_MAX_NUM
};
using TestParamsMbcBandConfigAdditional = std::array<float, MBC_ADD_MAX_NUM>;

// attackTime, releaseTime, ratio, thresh, kneeWidth, noise, expander, preGain, postGain
static constexpr std::array<TestParamsMbcBandConfigAdditional, 4> kMbcBandConfigAdditionalParam = {
        {{-3, -10, -2, -2, -5, -90, -2.5, -2, -2},
         {0, 0, 0, 0, 0, 0, 0, 0, 0},
         {-3, 10, -2, 2, -5, 90, -2.5, 2, -2},
         {3, 10, 2, -2, -5, 90, 2.5, 2, 2}}};

using TestParamsMbcBandConfig =
        std::tuple<std::pair<std::shared_ptr<IFactory>, Descriptor>, int32_t,
                   std::vector<std::pair<int, float>>, TestParamsMbcBandConfigAdditional>;

void fillMbcBandConfig(std::vector<DynamicsProcessing::MbcBandConfig>& cfgs,
                       const TestParamsMbcBandConfig& params) {
    const auto& cutOffFreqs = std::get<MBC_BAND_CUTOFF_FREQ>(params);
    const auto& additional = std::get<MBC_BAND_ADDITIONAL>(params);

    cfgs.resize(cutOffFreqs.size());

    for (size_t i = 0; i < cutOffFreqs.size(); ++i) {
        cfgs[i] = createMbcBandConfig(std::get<MBC_BAND_CHANNEL>(params),
                                      cutOffFreqs[i].first,   // band channel
                                      cutOffFreqs[i].second,  // band cutoff frequency
                                      additional[MBC_ADD_ATTACK_TIME],
                                      additional[MBC_ADD_RELEASE_TIME], additional[MBC_ADD_RATIO],
                                      additional[MBC_ADD_THRESHOLD], additional[MBC_ADD_KNEE_WIDTH],
                                      additional[MBC_ADD_NOISE_GATE_THRESHOLD],
                                      additional[MBC_ADD_EXPENDER_RATIO],
                                      additional[MBC_ADD_PRE_GAIN], additional[MBC_ADD_POST_GAIN]);
    }
}

class DynamicsProcessingTestMbcBandConfig
    : public ::testing::TestWithParam<TestParamsMbcBandConfig>,
      public DynamicsProcessingTestHelper {
  public:
    DynamicsProcessingTestMbcBandConfig()
        : DynamicsProcessingTestHelper(std::get<MBC_BAND_INSTANCE_NAME>(GetParam())) {
        fillMbcBandConfig(mCfgs, GetParam());
    }

    void SetUp() override { ASSERT_NO_FATAL_FAILURE(SetUpDynamicsProcessingEffect()); }

    void TearDown() override { TearDownDynamicsProcessingEffect(); }

    std::vector<DynamicsProcessing::MbcBandConfig> mCfgs;
};

TEST_P(DynamicsProcessingTestMbcBandConfig, SetAndGetMbcBandConfig) {
    mEngineConfigPreset.mbcStage.bandCount = mCfgs.size();
    addEngineConfig(mEngineConfigPreset);
    std::vector<DynamicsProcessing::ChannelConfig> cfgs(mChannelCount);
    for (int i = 0; i < mChannelCount; i++) {
        cfgs[i].channel = i;
        cfgs[i].enable = true;
    }
    addMbcChannelConfig(cfgs);
    addMbcBandConfigs(mCfgs);
    ASSERT_NO_FATAL_FAILURE(SetAndGetDynamicsProcessingParameters());
}

INSTANTIATE_TEST_SUITE_P(
        DynamicsProcessingTest, DynamicsProcessingTestMbcBandConfig,
        ::testing::Combine(testing::ValuesIn(EffectFactoryHelper::getAllEffectDescriptors(
                                   IFactory::descriptor, getEffectTypeUuidDynamicsProcessing())),
                           testing::Values(-1, 0, 10),  // channel index
                           testing::ValuesIn(kBands),   // band index, cut off frequencies
                           testing::ValuesIn(kMbcBandConfigAdditionalParam)),  // Additional
        [](const auto& info) {
            auto descriptor = std::get<MBC_BAND_INSTANCE_NAME>(info.param).second;
            std::vector<DynamicsProcessing::MbcBandConfig> cfgs;
            fillMbcBandConfig(cfgs, info.param);
            std::string mbcBands = ::android::internal::ToString(cfgs);
            std::string name = "Implementor_" + descriptor.common.implementor + "_name_" +
                               descriptor.common.name + "_UUID_" +
                               toString(descriptor.common.id.uuid) + "_bands_" + mbcBands;
            std::replace_if(
                    name.begin(), name.end(), [](const char c) { return !std::isalnum(c); }, '_');
            return name;
        });
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(DynamicsProcessingTestMbcBandConfig);

class DynamicsProcessingMbcBandConfigDataTest
    : public ::testing::TestWithParam<std::pair<std::shared_ptr<IFactory>, Descriptor>>,
      public DynamicsProcessingTestHelper {
  public:
    DynamicsProcessingMbcBandConfigDataTest()
        : DynamicsProcessingTestHelper(GetParam(), AudioChannelLayout::LAYOUT_MONO) {
        mBinOffsets.resize(mMultitoneTestFrequencies.size());
    }

    void SetUp() override {
        ASSERT_NO_FATAL_FAILURE(
                setUpDataTest(mMultitoneTestFrequencies, kSineMultitoneFullScaleDb));
    }

    void TearDown() override { TearDownDynamicsProcessingEffect(); }

    void setMbcParamsAndProcess(std::vector<float>& output) {
        createChannelConfig();
        mEngineConfigPreset.mbcStage.bandCount = mCfgs.size();
        addEngineConfig(mEngineConfigPreset);
        addMbcChannelConfig(mChannelConfig);
        addMbcBandConfigs(mCfgs);
        ASSERT_NO_FATAL_FAILURE(setParamsAndProcess(mInput, output));
    }

    void fillMbcBandConfig(std::vector<DynamicsProcessing::MbcBandConfig>& cfgs, int channelIndex,
                           float threshold, float ratio, float noiseGate, float expanderRatio,
                           int bandIndex, int cutoffFreqHz, float preGain, float postGain) {
        cfgs.push_back(createMbcBandConfig(channelIndex, bandIndex,
                                           static_cast<float>(cutoffFreqHz), kDefaultAttackTime,
                                           kDefaultReleaseTime, ratio, threshold, kDefaultKneeWidth,
                                           noiseGate, expanderRatio, preGain, postGain));
    }

    void validateOutput(const std::vector<float>& output, float threshold, float ratio,
                        size_t bandIndex) {
        std::vector<float> outputMag(mBinOffsets.size());
        EXPECT_NO_FATAL_FAILURE(getMagnitudeValue(output, outputMag));
        if (threshold >= mInputDb || ratio == 1) {
            EXPECT_NO_FATAL_FAILURE(checkInputAndOutputEquality(outputMag));
        } else {
            // Current band's magnitude is less than the other band's magnitude
            EXPECT_LT(outputMag[bandIndex], outputMag[bandIndex ^ 1]);
        }
    }

    void analyseMultiBandOutput(float threshold, float ratio) {
        std::vector<float> output(mInput.size());
        roundToFreqCenteredToFftBin(mMultitoneTestFrequencies, mBinOffsets, kBinWidth);
        // Set MBC values for two bands
        for (size_t i = 0; i < kCutoffFreqHz.size(); i++) {
            for (int channelIndex = 0; channelIndex < mChannelCount; channelIndex++) {
                fillMbcBandConfig(mCfgs, channelIndex, threshold, ratio, kDefaultNoiseGateDb,
                                  kDefaultExpanderRatio, i, kCutoffFreqHz[i], kDefaultPreGainDb,
                                  kDefaultPostGainDb);
                fillMbcBandConfig(mCfgs, channelIndex, kDefaultThresholdDb, kDefaultRatio,
                                  kDefaultNoiseGateDb, kDefaultExpanderRatio, i ^ 1,
                                  kCutoffFreqHz[i ^ 1], kDefaultPreGainDb, kDefaultPostGainDb);
            }
            ASSERT_NO_FATAL_FAILURE(setMbcParamsAndProcess(output));

            if (isAllParamsValid()) {
                ASSERT_NO_FATAL_FAILURE(validateOutput(output, threshold, ratio, i));
            }
            cleanUpMbcConfig();
        }
    }

    void cleanUpMbcConfig() {
        CleanUp();
        mCfgs.clear();
        mChannelConfig.clear();
    }

    static constexpr float kDefaultPostGainDb = 0;
    static constexpr float kDefaultPreGainDb = 0;
    static constexpr float kDefaultAttackTime = 0;
    static constexpr float kDefaultReleaseTime = 0;
    static constexpr float kDefaultKneeWidth = 0;
    static constexpr float kDefaultThresholdDb = 0;
    static constexpr float kDefaultNoiseGateDb = -10;
    static constexpr float kDefaultExpanderRatio = 1;
    static constexpr float kDefaultRatio = 1;
    std::vector<DynamicsProcessing::MbcBandConfig> mCfgs;
};

TEST_P(DynamicsProcessingMbcBandConfigDataTest, IncreasingThreshold) {
    float ratio = 20;
    std::vector<float> thresholdValues = {-200, -100, 0, 100, 200};

    for (float threshold : thresholdValues) {
        cleanUpMbcConfig();
        ASSERT_NO_FATAL_FAILURE(analyseMultiBandOutput(threshold, ratio));
    }
}

TEST_P(DynamicsProcessingMbcBandConfigDataTest, IncreasingRatio) {
    float threshold = -20;
    std::vector<float> ratioValues = {1, 10, 20, 30, 40, 50};

    for (float ratio : ratioValues) {
        cleanUpMbcConfig();
        ASSERT_NO_FATAL_FAILURE(analyseMultiBandOutput(threshold, ratio));
    }
}

TEST_P(DynamicsProcessingMbcBandConfigDataTest, IncreasingPostGain) {
    std::vector<float> postGainDbValues = {-55, -30, 0, 30, 55};
    std::vector<float> output(mInput.size());
    for (float postGainDb : postGainDbValues) {
        ASSERT_NO_FATAL_FAILURE(generateSineWave(mMultitoneTestFrequencies, mInput,
                                                 dBToAmplitude(postGainDb), kSamplingFrequency,
                                                 mChannelLayout));
        mInputDb = calculateDb(mInput);
        EXPECT_NEAR(mInputDb, kSineMultitoneFullScaleDb - postGainDb, kToleranceDb);
        cleanUpMbcConfig();
        for (int i = 0; i < mChannelCount; i++) {
            fillMbcBandConfig(mCfgs, i, kDefaultThresholdDb, kDefaultRatio, kDefaultNoiseGateDb,
                              kDefaultExpanderRatio, 0 /*band index*/, 2000 /*cutoffFrequency*/,
                              kDefaultPreGainDb, postGainDb);
        }
        EXPECT_NO_FATAL_FAILURE(setMbcParamsAndProcess(output));
        if (!isAllParamsValid()) {
            continue;
        }
        float outputDb = calculateDb(output, kStartIndex);
        EXPECT_NEAR(outputDb, mInputDb + postGainDb, kToleranceDb)
                << "PostGain: " << postGainDb << ", OutputDb: " << outputDb;
    }
}

TEST_P(DynamicsProcessingMbcBandConfigDataTest, IncreasingPreGain) {
    /*
    Depending on the pregain values, samples undergo either compression or expansion process.
    At -6 dB input,
    - Expansion is expected at -60 dB,
    - Compression at 10, 34 and 60 dB
    - No compression or expansion at -34, -10, -1 dB.
     */
    std::vector<float> preGainDbValues = {-60, -34, -10, -1, 10, 34, 60};
    std::vector<float> output(mInput.size());
    float thresholdDb = -7;
    float noiseGateDb = -40;
    std::vector<float> ratioValues = {1, 1.5, 2, 2.5, 3};
    for (float ratio : ratioValues) {
        for (float preGainDb : preGainDbValues) {
            float expectedOutputDb;
            float inputWithPreGain = mInputDb + preGainDb;
            if (inputWithPreGain > thresholdDb) {
                SCOPED_TRACE("Compressor ratio: " + std::to_string(ratio));
                expectedOutputDb =
                        (inputWithPreGain - thresholdDb) / ratio + thresholdDb - preGainDb;
            } else if (inputWithPreGain < noiseGateDb) {
                SCOPED_TRACE("Expander ratio: " + std::to_string(ratio));
                expectedOutputDb =
                        (inputWithPreGain - noiseGateDb) * ratio + noiseGateDb - preGainDb;
            } else {
                expectedOutputDb = mInputDb;
            }
            cleanUpMbcConfig();
            for (int i = 0; i < mChannelCount; i++) {
                fillMbcBandConfig(mCfgs, i, thresholdDb, ratio /*compressor ratio*/, noiseGateDb,
                                  ratio /*expander ratio*/, 0 /*band index*/,
                                  2000 /*cutoffFrequency*/, preGainDb, kDefaultPostGainDb);
            }
            EXPECT_NO_FATAL_FAILURE(setMbcParamsAndProcess(output));
            if (!isAllParamsValid()) {
                continue;
            }
            float outputDb = calculateDb(output, kStartIndex);
            EXPECT_NEAR(outputDb, expectedOutputDb, kToleranceDb)
                    << "PreGain: " << preGainDb << ", OutputDb: " << outputDb;
        }
    }
}

INSTANTIATE_TEST_SUITE_P(DynamicsProcessingTest, DynamicsProcessingMbcBandConfigDataTest,
                         testing::ValuesIn(EffectFactoryHelper::getAllEffectDescriptors(
                                 IFactory::descriptor, getEffectTypeUuidDynamicsProcessing())),
                         [](const auto& info) {
                             auto descriptor = info.param;
                             std::string name = getPrefix(descriptor.second);
                             std::replace_if(
                                     name.begin(), name.end(),
                                     [](const char c) { return !std::isalnum(c); }, '_');
                             return name;
                         });

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(DynamicsProcessingMbcBandConfigDataTest);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::UnitTest::GetInstance()->listeners().Append(new TestExecutionTracer());
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}
