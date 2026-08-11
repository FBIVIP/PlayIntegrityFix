#include <android/binder_manager.h>
#include <gtest/gtest.h>
#include <linux/dma-heap.h>
#include <sys/auxv.h>
#include <sys/mman.h>
#include "hwcryptokeyimpl.h"

static inline bool align_overflow(size_t size, size_t alignment, size_t* aligned) {
    if (size % alignment == 0) {
        *aligned = size;
        return false;
    }
    size_t temp = 0;
    bool overflow = __builtin_add_overflow(size / alignment, 1, &temp);
    overflow |= __builtin_mul_overflow(temp, alignment, aligned);
    return overflow;
}

static int allocate_buffers(size_t size) {
    const char* device_name = "/dev/dma_heap/system";
    int dma_heap_fd = open(device_name, O_RDONLY | O_CLOEXEC);
    if (dma_heap_fd < 0) {
        LOG(ERROR) << "Cannot open " << device_name;
        return -1;
    }
    size_t aligned = 0;
    if (align_overflow(size, getauxval(AT_PAGESZ), &aligned)) {
        LOG(ERROR) << "Rounding up buffer size oveflowed";
        return -1;
    }
    struct dma_heap_allocation_data allocation_request = {
            .len = aligned,
            .fd_flags = O_RDWR | O_CLOEXEC,
    };
    int rc = ioctl(dma_heap_fd, DMA_HEAP_IOCTL_ALLOC, &allocation_request);
    if (rc < 0) {
        LOG(ERROR) << "Buffer allocation request failed  " << rc;
        return -1;
    }
    int fd = allocation_request.fd;
    if (fd < 0) {
        LOG(ERROR) << "Allocation request returned bad fd" << fd;
        return -1;
    }
    return fd;
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

TEST(HwCryptoHalDelegator, FdTest) {
    const std::string instance =
            std::string() + ndk_hwcrypto::IHwCryptoKey::descriptor + "/default";
    ndk::SpAIBinder binder(AServiceManager_waitForService(instance.c_str()));
    ASSERT_NE(binder, nullptr);
    auto hwCryptoKey = ndk_hwcrypto::IHwCryptoKey::fromBinder(binder);
    ASSERT_NE(hwCryptoKey, nullptr);
    auto fd = allocate_buffers(4096);
    EXPECT_GE(fd, 0);
    ndk::ScopedFileDescriptor ndkFd(fd);
    ndk_hwcrypto::MemoryBufferParameter memBuffParam = ndk_hwcrypto::MemoryBufferParameter();
    memBuffParam.bufferHandle.set<ndk_hwcrypto::MemoryBufferParameter::MemoryBuffer::input>(
            std::move(ndkFd));
    memBuffParam.sizeBytes = 4096;
    auto operation = ndk_hwcrypto::CryptoOperation();
    operation.set<ndk_hwcrypto::CryptoOperation::setMemoryBuffer>(std::move(memBuffParam));
    ndk_hwcrypto::CryptoOperationSet operationSet = ndk_hwcrypto::CryptoOperationSet();
    operationSet.context = nullptr;
    operationSet.operations.push_back(std::move(operation));
    std::vector<ndk_hwcrypto::CryptoOperationSet> operationSets;
    operationSets.push_back(std::move(operationSet));
    std::vector<ndk_hwcrypto::CryptoOperationResult> aidl_return;
    std::shared_ptr<ndk_hwcrypto::IHwCryptoOperations> hwCryptoOperations;
    auto res = hwCryptoKey->getHwCryptoOperations(&hwCryptoOperations);
    EXPECT_TRUE(res.isOk());
    res = hwCryptoOperations->processCommandList(&operationSets, &aidl_return);
    EXPECT_TRUE(res.isOk());
}

TEST(HwCryptoHalDelegator, keyPolicyCppToNdk) {
    cpp_hwcrypto::KeyPolicy cppPolicy = cpp_hwcrypto::KeyPolicy();
    cppPolicy.keyType = cpp_hwcrypto::types::KeyType::AES_128_CBC_PKCS7_PADDING;
    cppPolicy.usage = cpp_hwcrypto::types::KeyUse::DECRYPT;
    cppPolicy.keyLifetime = cpp_hwcrypto::types::KeyLifetime::PORTABLE;
    cppPolicy.keyManagementKey = false;
    cppPolicy.keyPermissions.push_back(
            cpp_hwcrypto::types::KeyPermissions::ALLOW_PORTABLE_KEY_WRAPPING);
    ndk_hwcrypto::KeyPolicy ndkPolicy = android::trusty::hwcryptohalservice::convertKeyPolicy<
            ndk_hwcrypto::KeyPolicy, cpp_hwcrypto::KeyPolicy>(cppPolicy);
    EXPECT_EQ(ndkPolicy.keyType, ndk_hwcrypto::types::KeyType::AES_128_CBC_PKCS7_PADDING);
    EXPECT_EQ(ndkPolicy.usage, ndk_hwcrypto::types::KeyUse::DECRYPT);
    EXPECT_EQ(ndkPolicy.keyLifetime, ndk_hwcrypto::types::KeyLifetime::PORTABLE);
    EXPECT_EQ(ndkPolicy.keyManagementKey, false);
    EXPECT_EQ(ndkPolicy.keyPermissions.size(), 1ul);
    EXPECT_EQ(ndkPolicy.keyPermissions[0],
              ndk_hwcrypto::types::KeyPermissions::ALLOW_PORTABLE_KEY_WRAPPING);
}

TEST(HwCryptoHalDelegator, keyPolicyNdkToCpp) {
    ndk_hwcrypto::KeyPolicy ndkPolicy = ndk_hwcrypto::KeyPolicy();
    ndkPolicy.keyType = ndk_hwcrypto::types::KeyType::AES_128_CTR;
    ndkPolicy.usage = ndk_hwcrypto::types::KeyUse::ENCRYPT_DECRYPT;
    ndkPolicy.keyLifetime = ndk_hwcrypto::types::KeyLifetime::HARDWARE;
    ndkPolicy.keyManagementKey = true;
    ndkPolicy.keyPermissions.push_back(
            ndk_hwcrypto::types::KeyPermissions::ALLOW_EPHEMERAL_KEY_WRAPPING);
    ndkPolicy.keyPermissions.push_back(
            ndk_hwcrypto::types::KeyPermissions::ALLOW_HARDWARE_KEY_WRAPPING);
    cpp_hwcrypto::KeyPolicy cppPolicy = android::trusty::hwcryptohalservice::convertKeyPolicy<
            cpp_hwcrypto::KeyPolicy, ndk_hwcrypto::KeyPolicy>(ndkPolicy);
    EXPECT_EQ(cppPolicy.keyType, cpp_hwcrypto::types::KeyType::AES_128_CTR);
    EXPECT_EQ(cppPolicy.usage, cpp_hwcrypto::types::KeyUse::ENCRYPT_DECRYPT);
    EXPECT_EQ(cppPolicy.keyLifetime, cpp_hwcrypto::types::KeyLifetime::HARDWARE);
    EXPECT_EQ(cppPolicy.keyManagementKey, true);
    EXPECT_EQ(cppPolicy.keyPermissions.size(), 2ul);
    EXPECT_EQ(cppPolicy.keyPermissions[0],
              cpp_hwcrypto::types::KeyPermissions::ALLOW_EPHEMERAL_KEY_WRAPPING);
    EXPECT_EQ(cppPolicy.keyPermissions[1],
              cpp_hwcrypto::types::KeyPermissions::ALLOW_HARDWARE_KEY_WRAPPING);
}
