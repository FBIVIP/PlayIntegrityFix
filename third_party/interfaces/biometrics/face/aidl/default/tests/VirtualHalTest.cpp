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

#include <android/binder_process.h>
#include <face.sysprop.h>
#include <gtest/gtest.h>

#include <android-base/logging.h>

#include "Face.h"
#include "VirtualHal.h"

using namespace ::android::face::virt;
using namespace ::aidl::android::hardware::biometrics::face;

namespace aidl::android::hardware::biometrics::face {

class VirtualHalTest : public ::testing::Test {
  public:
    static const int32_t STATUS_FAILED_TO_SET_PARAMETER = 2;

  protected:
    void SetUp() override {
        mHal = ndk::SharedRefBase::make<Face>();
        mVhal = ndk::SharedRefBase::make<VirtualHal>(mHal);
        ASSERT_TRUE(mVhal != nullptr);
        mHal->resetConfigToDefault();
    }

    void TearDown() override { mHal->resetConfigToDefault(); }

    std::shared_ptr<VirtualHal> mVhal;

    ndk::ScopedAStatus validateNonNegativeInputOfInt32(const char* name,
                                                       ndk::ScopedAStatus (VirtualHal::*f)(int32_t),
                                                       const std::vector<int32_t>& in_good);

  private:
    std::shared_ptr<Face> mHal;
};

ndk::ScopedAStatus VirtualHalTest::validateNonNegativeInputOfInt32(
        const char* name, ndk::ScopedAStatus (VirtualHal::*f)(int32_t),
        const std::vector<int32_t>& in_params_good) {
    ndk::ScopedAStatus status;
    for (auto& param : in_params_good) {
        status = (*mVhal.*f)(param);
        if (!status.isOk()) return status;
        if (Face::cfg().get<int32_t>(name) != param) {
            return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                    VirtualHalTest::STATUS_FAILED_TO_SET_PARAMETER,
                    "Error: fail to set non-negative parameter"));
        }
    }

    int32_t old_param = Face::cfg().get<int32_t>(name);
    status = (*mVhal.*f)(-1);
    if (status.isOk()) {
        return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                VirtualHalTest::STATUS_FAILED_TO_SET_PARAMETER, "Error: should return NOK"));
    }
    if (status.getServiceSpecificError() != IVirtualHal::STATUS_INVALID_PARAMETER) {
        return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                VirtualHalTest::STATUS_FAILED_TO_SET_PARAMETER,
                "Error: unexpected return error code"));
    }
    if (Face::cfg().get<int32_t>(name) != old_param) {
        return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                VirtualHalTest::STATUS_FAILED_TO_SET_PARAMETER,
                "Error: unexpected parameter change on failed attempt"));
    }
    return ndk::ScopedAStatus::ok();
}

TEST_F(VirtualHalTest, init) {
    mVhal->setLockout(false);
    ASSERT_TRUE(Face::cfg().get<bool>("lockout") == false);
    ASSERT_TRUE(Face::cfg().get<std::string>("type") == "rgb");
    ASSERT_TRUE(Face::cfg().get<std::string>("strength") == "strong");
    std::int64_t id = Face::cfg().get<std::int64_t>("authenticator_id");
    ASSERT_TRUE(Face::cfg().get<std::int64_t>("authenticator_id") == 0);
    ASSERT_TRUE(Face::cfg().getopt<OptIntVec>("enrollments") == OptIntVec());
}

TEST_F(VirtualHalTest, enrollment_hit_int32) {
    mVhal->setEnrollmentHit(11);
    ASSERT_TRUE(Face::cfg().get<int32_t>("enrollment_hit") == 11);
}

TEST_F(VirtualHalTest, next_enrollment) {
    struct {
        std::string nextEnrollmentStr;
        face::NextEnrollment nextEnrollment;
    } testData[] = {
            {"1:20:true", {1, {{20}}, true}},
            {"1:50,60,70:true", {1, {{50}, {60}, {70}}, true}},
            {"2:50-[21],60,70-[4,1002,1]:false",
             {2,
              {{50, {{AcquiredInfo::START}}},
               {60},
               {70, {{AcquiredInfo::TOO_DARK}, {1002}, {AcquiredInfo::GOOD}}}},
              false}},
    };

    for (auto& d : testData) {
        mVhal->setNextEnrollment(d.nextEnrollment);
        ASSERT_TRUE(Face::cfg().get<std::string>("next_enrollment") == d.nextEnrollmentStr);
    }
}

TEST_F(VirtualHalTest, authenticator_id_int64) {
    mVhal->setAuthenticatorId(12345678900);
    ASSERT_TRUE(Face::cfg().get<int64_t>("authenticator_id") == 12345678900);
}

TEST_F(VirtualHalTest, opeationAuthenticateFails_bool) {
    mVhal->setOperationAuthenticateFails(true);
    ASSERT_TRUE(Face::cfg().get<bool>("operation_authenticate_fails"));
}

TEST_F(VirtualHalTest, operationAuthenticateAcquired_int32_vector) {
    using Tag = AcquiredInfoAndVendorCode::Tag;
    std::vector<AcquiredInfoAndVendorCode> ac{
            {AcquiredInfo::START}, {AcquiredInfo::TOO_FAR}, {1023}};
    mVhal->setOperationAuthenticateAcquired(ac);
    OptIntVec ac_get = Face::cfg().getopt<OptIntVec>("operation_authenticate_acquired");
    ASSERT_TRUE(ac_get.size() == ac.size());
    for (int i = 0; i < ac.size(); i++) {
        int acCode = (ac[i].getTag() == Tag::acquiredInfo) ? (int)ac[i].get<Tag::acquiredInfo>()
                                                           : ac[i].get<Tag::vendorCode>();
        ASSERT_TRUE(acCode == ac_get[i]);
    }
}

TEST_F(VirtualHalTest, type) {
    struct {
        FaceSensorType type;
        const char* typeStr;
    } typeMap[] = {{FaceSensorType::RGB, "rgb"},
                   {FaceSensorType::IR, "ir"},
                   {FaceSensorType::UNKNOWN, "unknown"}};
    for (auto const& x : typeMap) {
        mVhal->setType(x.type);
        ASSERT_TRUE(Face::cfg().get<std::string>("type") == x.typeStr);
    }
}

TEST_F(VirtualHalTest, sensorStrength) {
    struct {
        common::SensorStrength strength;
        const char* strengthStr;
    } strengths[] = {{common::SensorStrength::CONVENIENCE, "CONVENIENCE"},
                     {common::SensorStrength::WEAK, "WEAK"},
                     {common::SensorStrength::STRONG, "STRONG"}};

    for (auto const& x : strengths) {
        mVhal->setSensorStrength(x.strength);
        ASSERT_TRUE(Face::cfg().get<std::string>("strength") == x.strengthStr);
    }
}

TEST_F(VirtualHalTest, setLatency) {
    ndk::ScopedAStatus status;
    std::vector<int32_t> in_lats[] = {{1}, {2, 3}, {5, 4}};
    for (auto const& in_lat : in_lats) {
        status = mVhal->setOperationAuthenticateLatency(in_lat);
        ASSERT_TRUE(status.isOk());
        OptIntVec out_lat = Face::cfg().getopt<OptIntVec>("operation_authenticate_latency");
        ASSERT_TRUE(in_lat.size() == out_lat.size());
        for (int i = 0; i < in_lat.size(); i++) {
            ASSERT_TRUE(in_lat[i] == out_lat[i]);
        }
    }

    std::vector<int32_t> bad_in_lats[] = {{}, {1, 2, 3}, {1, -3}};
    for (auto const& in_lat : bad_in_lats) {
        status = mVhal->setOperationAuthenticateLatency(in_lat);
        ASSERT_TRUE(!status.isOk());
        ASSERT_TRUE(status.getServiceSpecificError() == IVirtualHal::STATUS_INVALID_PARAMETER);
    }
}

TEST_F(VirtualHalTest, setOperationAuthenticateDuration) {
    ndk::ScopedAStatus status = validateNonNegativeInputOfInt32(
            "operation_authenticate_duration", &IVirtualHal::setOperationAuthenticateDuration,
            {0, 33});
    ASSERT_TRUE(status.isOk());
}

TEST_F(VirtualHalTest, setLockoutTimedDuration) {
    ndk::ScopedAStatus status = validateNonNegativeInputOfInt32(
            "lockout_timed_duration", &IVirtualHal::setLockoutTimedDuration, {0, 35});
    ASSERT_TRUE(status.isOk());
}

TEST_F(VirtualHalTest, setLockoutTimedThreshold) {
    ndk::ScopedAStatus status = validateNonNegativeInputOfInt32(
            "lockout_timed_threshold", &IVirtualHal::setLockoutTimedThreshold, {0, 36});
    ASSERT_TRUE(status.isOk());
}

TEST_F(VirtualHalTest, setLockoutPermanentThreshold) {
    ndk::ScopedAStatus status = validateNonNegativeInputOfInt32(
            "lockout_permanent_threshold", &IVirtualHal::setLockoutPermanentThreshold, {0, 37});
    ASSERT_TRUE(status.isOk());
}

TEST_F(VirtualHalTest, setOthers) {
    // Verify that there is no CHECK() failures
    mVhal->setEnrollments({7, 6, 5});
    mVhal->setChallenge(111222333444555666);
    mVhal->setOperationAuthenticateError(4);
    mVhal->setOperationEnrollLatency({4, 5});
    mVhal->setLockout(false);
    mVhal->setLockoutEnable(false);
}

}  // namespace aidl::android::hardware::biometrics::face

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}
