/*
 * Copyright (C) 2019 The Android Open Source Project
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

#include "benchmark/benchmark.h"

#include <aidl/android/hardware/vibrator/BnVibratorCallback.h>
#include <aidl/android/hardware/vibrator/IVibrator.h>

#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <android/hardware/vibrator/1.3/IVibrator.h>
#include <future>

using ::android::hardware::hidl_enum_range;
using ::android::hardware::Return;
using ::android::hardware::details::hidl_enum_values;
using ::benchmark::Counter;
using ::benchmark::Fixture;
using ::benchmark::kMicrosecond;
using ::benchmark::State;
using ::benchmark::internal::Benchmark;
using ::ndk::enum_range;

using namespace ::std::chrono_literals;

namespace Aidl = ::aidl::android::hardware::vibrator;
namespace V1_0 = ::android::hardware::vibrator::V1_0;
namespace V1_1 = ::android::hardware::vibrator::V1_1;
namespace V1_2 = ::android::hardware::vibrator::V1_2;
namespace V1_3 = ::android::hardware::vibrator::V1_3;

// Fixed number of iterations for benchmarks that trigger a vibration on the loop.
// They require slow cleanup to ensure a stable state on each run and less noisy metrics.
static constexpr auto VIBRATION_ITERATIONS = 500;

// Timeout to wait for vibration callback completion.
static constexpr auto VIBRATION_CALLBACK_TIMEOUT = 100ms;

// Max duration the vibrator can be turned on, in milliseconds.
static constexpr uint32_t MAX_ON_DURATION_MS = UINT16_MAX;

template <typename I>
class BaseBench : public Fixture {
  public:
    void SetUp(State& /*state*/) override {
        ABinderProcess_setThreadPoolMaxThreadCount(1);
        ABinderProcess_startThreadPool();
    }

    void TearDown(State& /*state*/) override {
        if (mVibrator) {
            mVibrator->off();
        }
    }

    static void DefaultConfig(Benchmark* b) { b->Unit(kMicrosecond); }

    static void DefaultArgs(Benchmark* /*b*/) { /* none */
    }

  protected:
    auto getOtherArg(const State& state, std::size_t index) const { return state.range(index + 0); }

  protected:
    std::shared_ptr<I> mVibrator;
};

template <typename I>
class VibratorBench : public BaseBench<I> {
  public:
    void SetUp(State& state) override {
        BaseBench<I>::SetUp(state);
        auto service = I::getService();
        if (service) {
            this->mVibrator = std::shared_ptr<I>(service.release());
        } else {
            this->mVibrator = nullptr;
        }
    }

  protected:
    bool shouldSkipWithError(State& state, const android::hardware::Return<V1_0::Status>&& ret) {
        if (!ret.isOk()) {
            state.SkipWithError(ret.description());
            return true;
        }
        return false;
    }
};

enum class EmptyEnum : uint32_t;
template <>
inline constexpr std::array<EmptyEnum, 0> hidl_enum_values<EmptyEnum> = {};

template <typename T, typename U>
std::set<T> difference(const hidl_enum_range<T>& t, const hidl_enum_range<U>& u) {
    class Compare {
      public:
        bool operator()(const T& a, const U& b) { return a < static_cast<T>(b); }
        bool operator()(const U& a, const T& b) { return static_cast<T>(a) < b; }
    };
    std::set<T> ret;

    std::set_difference(t.begin(), t.end(), u.begin(), u.end(),
                        std::insert_iterator<decltype(ret)>(ret, ret.begin()), Compare());

    return ret;
}

template <typename I, typename E1, typename E2 = EmptyEnum>
class VibratorEffectsBench : public VibratorBench<I> {
  public:
    using Effect = E1;
    using EffectStrength = V1_0::EffectStrength;
    using Status = V1_0::Status;

  public:
    static void DefaultArgs(Benchmark* b) {
        b->ArgNames({"Effect", "Strength"});
        for (const auto& effect : difference(hidl_enum_range<E1>(), hidl_enum_range<E2>())) {
            for (const auto& strength : hidl_enum_range<EffectStrength>()) {
                b->Args({static_cast<long>(effect), static_cast<long>(strength)});
            }
        }
    }

    void performBench(State* state, Return<void> (I::*performApi)(Effect, EffectStrength,
                                                                  typename I::perform_cb)) {
        auto effect = getEffect(*state);
        auto strength = getStrength(*state);
        bool supported = true;

        (*this->mVibrator.*performApi)(effect, strength, [&](Status status, uint32_t /*lengthMs*/) {
            if (status == Status::UNSUPPORTED_OPERATION) {
                supported = false;
            }
        });

        if (!supported) {
            state->SkipWithMessage("effect unsupported");
            return;
        }

        for (auto _ : *state) {
            // Test
            auto ret = (*this->mVibrator.*performApi)(
                    effect, strength, [](Status /*status*/, uint32_t /*lengthMs*/) {});

            // Cleanup
            state->PauseTiming();
            if (!ret.isOk()) {
                state->SkipWithError(ret.description());
                return;
            }
            if (this->shouldSkipWithError(*state, this->mVibrator->off())) {
                return;
            }
            state->ResumeTiming();
        }
    }

  protected:
    auto getEffect(const State& state) const {
        return static_cast<Effect>(this->getOtherArg(state, 0));
    }

    auto getStrength(const State& state) const {
        return static_cast<EffectStrength>(this->getOtherArg(state, 1));
    }
};

#define BENCHMARK_WRAPPER(fixt, test, code)           \
    BENCHMARK_DEFINE_F(fixt, test)                    \
    /* NOLINTNEXTLINE */                              \
    (State & state) {                                 \
        if (!mVibrator) {                             \
            state.SkipWithMessage("HAL unavailable"); \
            return;                                   \
        }                                             \
                                                      \
        code                                          \
    }                                                 \
    BENCHMARK_REGISTER_F(fixt, test)->Apply(fixt::DefaultConfig)->Apply(fixt::DefaultArgs)

using VibratorBench_V1_0 = VibratorBench<V1_0::IVibrator>;

BENCHMARK_WRAPPER(VibratorBench_V1_0, on, {
    auto ms = MAX_ON_DURATION_MS;

    for (auto _ : state) {
        // Test
        if (shouldSkipWithError(state, mVibrator->on(ms))) {
            return;
        }

        // Cleanup
        state.PauseTiming();
        if (shouldSkipWithError(state, mVibrator->off())) {
            return;
        }
        state.ResumeTiming();
    }
});

BENCHMARK_WRAPPER(VibratorBench_V1_0, off, {
    auto ms = MAX_ON_DURATION_MS;

    for (auto _ : state) {
        // Setup
        state.PauseTiming();
        if (shouldSkipWithError(state, mVibrator->on(ms))) {
            return;
        }
        state.ResumeTiming();

        // Test
        if (shouldSkipWithError(state, mVibrator->off())) {
            return;
        }
    }
});

BENCHMARK_WRAPPER(VibratorBench_V1_0, supportsAmplitudeControl, {
    for (auto _ : state) {
        mVibrator->supportsAmplitudeControl();
    }
});

BENCHMARK_WRAPPER(VibratorBench_V1_0, setAmplitude, {
    auto ms = MAX_ON_DURATION_MS;
    uint8_t amplitude = UINT8_MAX;

    if (!mVibrator->supportsAmplitudeControl()) {
        state.SkipWithMessage("amplitude control unavailable");
        return;
    }

    if (shouldSkipWithError(state, mVibrator->on(ms))) {
        return;
    }

    for (auto _ : state) {
        if (shouldSkipWithError(state, mVibrator->setAmplitude(amplitude))) {
            return;
        }
    }
});

using VibratorEffectsBench_V1_0 = VibratorEffectsBench<V1_0::IVibrator, V1_0::Effect>;

BENCHMARK_WRAPPER(VibratorEffectsBench_V1_0, perform,
                  { performBench(&state, &V1_0::IVibrator::perform); });

using VibratorEffectsBench_V1_1 =
        VibratorEffectsBench<V1_1::IVibrator, V1_1::Effect_1_1, V1_0::Effect>;

BENCHMARK_WRAPPER(VibratorEffectsBench_V1_1, perform_1_1,
                  { performBench(&state, &V1_1::IVibrator::perform_1_1); });

using VibratorEffectsBench_V1_2 =
        VibratorEffectsBench<V1_2::IVibrator, V1_2::Effect, V1_1::Effect_1_1>;

BENCHMARK_WRAPPER(VibratorEffectsBench_V1_2, perform_1_2,
                  { performBench(&state, &V1_2::IVibrator::perform_1_2); });

class VibratorBench_V1_3 : public VibratorBench<V1_3::IVibrator> {
  public:
    void TearDown(State& state) override {
        VibratorBench::TearDown(state);
        if (mVibrator) {
            mVibrator->setExternalControl(false);
        }
    }
};

BENCHMARK_WRAPPER(VibratorBench_V1_3, supportsExternalControl, {
    for (auto _ : state) {
        mVibrator->supportsExternalControl();
    }
});

BENCHMARK_WRAPPER(VibratorBench_V1_3, setExternalControl, {
    if (!mVibrator->supportsExternalControl()) {
        state.SkipWithMessage("external control unavailable");
        return;
    }

    for (auto _ : state) {
        // Test
        if (shouldSkipWithError(state, mVibrator->setExternalControl(true))) {
            return;
        }

        // Cleanup
        state.PauseTiming();
        if (shouldSkipWithError(state, mVibrator->setExternalControl(false))) {
            return;
        }
        state.ResumeTiming();
    }
});

BENCHMARK_WRAPPER(VibratorBench_V1_3, supportsExternalAmplitudeControl, {
    if (!mVibrator->supportsExternalControl()) {
        state.SkipWithMessage("external control unavailable");
        return;
    }

    if (shouldSkipWithError(state, mVibrator->setExternalControl(true))) {
        return;
    }

    for (auto _ : state) {
        mVibrator->supportsAmplitudeControl();
    }
});

BENCHMARK_WRAPPER(VibratorBench_V1_3, setExternalAmplitude, {
    uint8_t amplitude = UINT8_MAX;

    if (!mVibrator->supportsExternalControl()) {
        state.SkipWithMessage("external control unavailable");
        return;
    }

    if (shouldSkipWithError(state, mVibrator->setExternalControl(true))) {
        return;
    }

    if (!mVibrator->supportsAmplitudeControl()) {
        state.SkipWithMessage("amplitude control unavailable");
        return;
    }

    for (auto _ : state) {
        if (shouldSkipWithError(state, mVibrator->setAmplitude(amplitude))) {
            return;
        }
    }
});

using VibratorEffectsBench_V1_3 = VibratorEffectsBench<V1_3::IVibrator, V1_3::Effect, V1_2::Effect>;

BENCHMARK_WRAPPER(VibratorEffectsBench_V1_3, perform_1_3,
                  { performBench(&state, &V1_3::IVibrator::perform_1_3); });

class VibratorBench_Aidl : public BaseBench<Aidl::IVibrator> {
  public:
    void SetUp(State& state) override {
        BaseBench::SetUp(state);
        auto serviceName = std::string(Aidl::IVibrator::descriptor) + "/default";
        this->mVibrator = Aidl::IVibrator::fromBinder(
                ndk::SpAIBinder(AServiceManager_waitForService(serviceName.c_str())));
    }

    void TearDown(State& state) override {
        BaseBench::TearDown(state);
        if (mVibrator) {
            mVibrator->setExternalControl(false);
        }
    }

  protected:
    int32_t hasCapabilities(int32_t capabilities) {
        int32_t deviceCapabilities = 0;
        this->mVibrator->getCapabilities(&deviceCapabilities);
        return (deviceCapabilities & capabilities) == capabilities;
    }

    bool shouldSkipWithError(State& state, const ndk::ScopedAStatus&& status) {
        if (!status.isOk()) {
            state.SkipWithError(status.getMessage());
            return true;
        }
        return false;
    }

    void waitForComplete(std::future<void>& callbackFuture) {
        // Wait until the HAL has finished processing previous vibration before starting a new one,
        // so the HAL state is consistent on each run and metrics are less noisy. Some of the newest
        // HAL implementations are waiting on previous vibration cleanup and might be significantly
        // slower, so make sure we measure vibrations on a clean slate.
        if (callbackFuture.valid()) {
            callbackFuture.wait_for(VIBRATION_CALLBACK_TIMEOUT);
        }
    }

    static void SlowBenchConfig(Benchmark* b) { b->Iterations(VIBRATION_ITERATIONS); }
};

class SlowVibratorBench_Aidl : public VibratorBench_Aidl {
  public:
    static void DefaultConfig(Benchmark* b) {
        VibratorBench_Aidl::DefaultConfig(b);
        SlowBenchConfig(b);
    }
};

class HalCallback : public Aidl::BnVibratorCallback {
  public:
    HalCallback() = default;
    ~HalCallback() = default;

    ndk::ScopedAStatus onComplete() override {
        mPromise.set_value();
        return ndk::ScopedAStatus::ok();
    }

    std::future<void> getFuture() { return mPromise.get_future(); }

  private:
    std::promise<void> mPromise;
};

BENCHMARK_WRAPPER(SlowVibratorBench_Aidl, on, {
    auto ms = MAX_ON_DURATION_MS;

    for (auto _ : state) {
        auto cb = hasCapabilities(Aidl::IVibrator::CAP_ON_CALLBACK)
                          ? ndk::SharedRefBase::make<HalCallback>()
                          : nullptr;
        // Grab the future before callback promise is destroyed by the HAL.
        auto cbFuture = cb ? cb->getFuture() : std::future<void>();

        // Test
        if (shouldSkipWithError(state, mVibrator->on(ms, cb))) {
            return;
        }

        // Cleanup
        state.PauseTiming();
        if (shouldSkipWithError(state, mVibrator->off())) {
            return;
        }
        waitForComplete(cbFuture);
        state.ResumeTiming();
    }
});

BENCHMARK_WRAPPER(SlowVibratorBench_Aidl, off, {
    auto ms = MAX_ON_DURATION_MS;

    for (auto _ : state) {
        auto cb = hasCapabilities(Aidl::IVibrator::CAP_ON_CALLBACK)
                          ? ndk::SharedRefBase::make<HalCallback>()
                          : nullptr;
        // Grab the future before callback promise is destroyed by the HAL.
        auto cbFuture = cb ? cb->getFuture() : std::future<void>();

        // Setup
        state.PauseTiming();
        if (shouldSkipWithError(state, mVibrator->on(ms, cb))) {
            return;
        }
        state.ResumeTiming();

        // Test
        if (shouldSkipWithError(state, mVibrator->off())) {
            return;
        }

        // Cleanup
        state.PauseTiming();
        waitForComplete(cbFuture);
        state.ResumeTiming();
    }
});

BENCHMARK_WRAPPER(VibratorBench_Aidl, getCapabilities, {
    int32_t capabilities = 0;

    for (auto _ : state) {
        if (shouldSkipWithError(state, mVibrator->getCapabilities(&capabilities))) {
            return;
        }
    }
});

BENCHMARK_WRAPPER(VibratorBench_Aidl, setAmplitude, {
    auto ms = MAX_ON_DURATION_MS;
    float amplitude = 1.0f;

    if (!hasCapabilities(Aidl::IVibrator::CAP_AMPLITUDE_CONTROL)) {
        state.SkipWithMessage("amplitude control unavailable");
        return;
    }

    auto cb = hasCapabilities(Aidl::IVibrator::CAP_ON_CALLBACK)
                      ? ndk::SharedRefBase::make<HalCallback>()
                      : nullptr;
    if (shouldSkipWithError(state, mVibrator->on(ms, cb))) {
        return;
    }

    for (auto _ : state) {
        if (shouldSkipWithError(state, mVibrator->setAmplitude(amplitude))) {
            return;
        }
    }
});

BENCHMARK_WRAPPER(VibratorBench_Aidl, setExternalControl, {
    if (!hasCapabilities(Aidl::IVibrator::CAP_EXTERNAL_CONTROL)) {
        state.SkipWithMessage("external control unavailable");
        return;
    }

    for (auto _ : state) {
        // Test
        if (shouldSkipWithError(state, mVibrator->setExternalControl(true))) {
            return;
        }

        // Cleanup
        state.PauseTiming();
        if (shouldSkipWithError(state, mVibrator->setExternalControl(false))) {
            return;
        }
        state.ResumeTiming();
    }
});

BENCHMARK_WRAPPER(VibratorBench_Aidl, setExternalAmplitude, {
    auto externalControl = static_cast<int32_t>(Aidl::IVibrator::CAP_EXTERNAL_CONTROL);
    auto externalAmplitudeControl =
            static_cast<int32_t>(Aidl::IVibrator::CAP_EXTERNAL_AMPLITUDE_CONTROL);
    if (!hasCapabilities(externalControl | externalAmplitudeControl)) {
        state.SkipWithMessage("external amplitude control unavailable");
        return;
    }

    if (shouldSkipWithError(state, mVibrator->setExternalControl(true))) {
        return;
    }

    float amplitude = 1.0f;
    for (auto _ : state) {
        if (shouldSkipWithError(state, mVibrator->setAmplitude(amplitude))) {
            return;
        }
    }
});

BENCHMARK_WRAPPER(VibratorBench_Aidl, getSupportedEffects, {
    std::vector<Aidl::Effect> supportedEffects;

    for (auto _ : state) {
        if (shouldSkipWithError(state, mVibrator->getSupportedEffects(&supportedEffects))) {
            return;
        }
    }
});

BENCHMARK_WRAPPER(VibratorBench_Aidl, getSupportedAlwaysOnEffects, {
    if (!hasCapabilities(Aidl::IVibrator::CAP_ALWAYS_ON_CONTROL)) {
        state.SkipWithMessage("always on control unavailable");
        return;
    }

    std::vector<Aidl::Effect> supportedEffects;

    for (auto _ : state) {
        if (shouldSkipWithError(state, mVibrator->getSupportedAlwaysOnEffects(&supportedEffects))) {
            return;
        }
    }
});

BENCHMARK_WRAPPER(VibratorBench_Aidl, getSupportedPrimitives, {
    std::vector<Aidl::CompositePrimitive> supportedPrimitives;

    for (auto _ : state) {
        if (shouldSkipWithError(state, mVibrator->getSupportedPrimitives(&supportedPrimitives))) {
            return;
        }
    }
});

class VibratorEffectsBench_Aidl : public VibratorBench_Aidl {
  public:
    static void DefaultArgs(Benchmark* b) {
        b->ArgNames({"Effect", "Strength"});
        for (const auto& effect : enum_range<Aidl::Effect>()) {
            for (const auto& strength : enum_range<Aidl::EffectStrength>()) {
                b->Args({static_cast<long>(effect), static_cast<long>(strength)});
            }
        }
    }

  protected:
    auto getEffect(const State& state) const {
        return static_cast<Aidl::Effect>(this->getOtherArg(state, 0));
    }

    auto getStrength(const State& state) const {
        return static_cast<Aidl::EffectStrength>(this->getOtherArg(state, 1));
    }

    bool isEffectSupported(const Aidl::Effect& effect) {
        std::vector<Aidl::Effect> supported;
        mVibrator->getSupportedEffects(&supported);
        return std::find(supported.begin(), supported.end(), effect) != supported.end();
    }

    bool isAlwaysOnEffectSupported(const Aidl::Effect& effect) {
        std::vector<Aidl::Effect> supported;
        mVibrator->getSupportedAlwaysOnEffects(&supported);
        return std::find(supported.begin(), supported.end(), effect) != supported.end();
    }
};

class SlowVibratorEffectsBench_Aidl : public VibratorEffectsBench_Aidl {
  public:
    static void DefaultConfig(Benchmark* b) {
        VibratorEffectsBench_Aidl::DefaultConfig(b);
        SlowBenchConfig(b);
    }
};

BENCHMARK_WRAPPER(VibratorEffectsBench_Aidl, alwaysOnEnable, {
    if (!hasCapabilities(Aidl::IVibrator::CAP_ALWAYS_ON_CONTROL)) {
        state.SkipWithMessage("always on control unavailable");
        return;
    }

    int32_t id = 1;
    auto effect = getEffect(state);
    auto strength = getStrength(state);

    if (!isAlwaysOnEffectSupported(effect)) {
        state.SkipWithMessage("always on effect unsupported");
        return;
    }

    for (auto _ : state) {
        // Test
        if (shouldSkipWithError(state, mVibrator->alwaysOnEnable(id, effect, strength))) {
            return;
        }

        // Cleanup
        state.PauseTiming();
        if (shouldSkipWithError(state, mVibrator->alwaysOnDisable(id))) {
            return;
        }
        state.ResumeTiming();
    }
});

BENCHMARK_WRAPPER(VibratorEffectsBench_Aidl, alwaysOnDisable, {
    if (!hasCapabilities(Aidl::IVibrator::CAP_ALWAYS_ON_CONTROL)) {
        state.SkipWithMessage("always on control unavailable");
        return;
    }

    int32_t id = 1;
    auto effect = getEffect(state);
    auto strength = getStrength(state);

    if (!isAlwaysOnEffectSupported(effect)) {
        state.SkipWithMessage("always on effect unsupported");
        return;
    }

    for (auto _ : state) {
        // Setup
        state.PauseTiming();
        if (shouldSkipWithError(state, mVibrator->alwaysOnEnable(id, effect, strength))) {
            return;
        }
        state.ResumeTiming();

        // Test
        if (shouldSkipWithError(state, mVibrator->alwaysOnDisable(id))) {
            return;
        }
    }
});

BENCHMARK_WRAPPER(SlowVibratorEffectsBench_Aidl, perform, {
    auto effect = getEffect(state);
    auto strength = getStrength(state);

    if (!isEffectSupported(effect)) {
        state.SkipWithMessage("effect unsupported");
        return;
    }

    int32_t lengthMs = 0;

    for (auto _ : state) {
        auto cb = hasCapabilities(Aidl::IVibrator::CAP_PERFORM_CALLBACK)
                          ? ndk::SharedRefBase::make<HalCallback>()
                          : nullptr;
        // Grab the future before callback promise is destroyed by the HAL.
        auto cbFuture = cb ? cb->getFuture() : std::future<void>();

        // Test
        if (shouldSkipWithError(state, mVibrator->perform(effect, strength, cb, &lengthMs))) {
            return;
        }

        // Cleanup
        state.PauseTiming();
        if (shouldSkipWithError(state, mVibrator->off())) {
            return;
        }
        waitForComplete(cbFuture);
        state.ResumeTiming();
    }
});

class VibratorPrimitivesBench_Aidl : public VibratorBench_Aidl {
  public:
    static void DefaultArgs(Benchmark* b) {
        b->ArgNames({"Primitive"});
        for (const auto& primitive : enum_range<Aidl::CompositePrimitive>()) {
            b->Args({static_cast<long>(primitive)});
        }
    }

  protected:
    auto getPrimitive(const State& state) const {
        return static_cast<Aidl::CompositePrimitive>(this->getOtherArg(state, 0));
    }

    bool isPrimitiveSupported(const Aidl::CompositePrimitive& primitive) {
        std::vector<Aidl::CompositePrimitive> supported;
        mVibrator->getSupportedPrimitives(&supported);
        return std::find(supported.begin(), supported.end(), primitive) != supported.end();
    }
};

class SlowVibratorPrimitivesBench_Aidl : public VibratorPrimitivesBench_Aidl {
  public:
    static void DefaultConfig(Benchmark* b) {
        VibratorPrimitivesBench_Aidl::DefaultConfig(b);
        SlowBenchConfig(b);
    }
};

BENCHMARK_WRAPPER(VibratorBench_Aidl, getCompositionDelayMax, {
    int32_t ms = 0;

    for (auto _ : state) {
        if (shouldSkipWithError(state, mVibrator->getCompositionDelayMax(&ms))) {
            return;
        }
    }
});

BENCHMARK_WRAPPER(VibratorBench_Aidl, getCompositionSizeMax, {
    int32_t size = 0;

    for (auto _ : state) {
        if (shouldSkipWithError(state, mVibrator->getCompositionSizeMax(&size))) {
            return;
        }
    }
});

BENCHMARK_WRAPPER(VibratorPrimitivesBench_Aidl, getPrimitiveDuration, {
    if (!hasCapabilities(Aidl::IVibrator::CAP_COMPOSE_EFFECTS)) {
        state.SkipWithMessage("compose effects unavailable");
        return;
    }

    auto primitive = getPrimitive(state);
    int32_t ms = 0;

    if (!isPrimitiveSupported(primitive)) {
        state.SkipWithMessage("primitive unsupported");
        return;
    }

    for (auto _ : state) {
        if (shouldSkipWithError(state, mVibrator->getPrimitiveDuration(primitive, &ms))) {
            return;
        }
    }
});

BENCHMARK_WRAPPER(SlowVibratorPrimitivesBench_Aidl, compose, {
    if (!hasCapabilities(Aidl::IVibrator::CAP_COMPOSE_EFFECTS)) {
        state.SkipWithMessage("compose effects unavailable");
        return;
    }

    Aidl::CompositeEffect effect;
    effect.primitive = getPrimitive(state);
    effect.scale = 1.0f;
    effect.delayMs = 0;

    if (effect.primitive == Aidl::CompositePrimitive::NOOP) {
        state.SkipWithMessage("skipping primitive NOOP");
        return;
    }
    if (!isPrimitiveSupported(effect.primitive)) {
        state.SkipWithMessage("primitive unsupported");
        return;
    }

    std::vector<Aidl::CompositeEffect> effects;
    effects.push_back(effect);

    for (auto _ : state) {
        auto cb = ndk::SharedRefBase::make<HalCallback>();
        // Grab the future before callback promise is moved and destroyed by the HAL.
        auto cbFuture = cb->getFuture();

        // Test
        if (shouldSkipWithError(state, mVibrator->compose(effects, cb))) {
            return;
        }

        // Cleanup
        state.PauseTiming();
        if (shouldSkipWithError(state, mVibrator->off())) {
            return;
        }
        waitForComplete(cbFuture);
        state.ResumeTiming();
    }
});

BENCHMARK_MAIN();
