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

#include <android-base/unique_fd.h>
#include <bpf/libbpf.h>
#include <gtest/gtest.h>
#include <linux/bpf.h>  // SO_ATTACH_BPF
#include <linux/netlink.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/socket.h>
#include <string>
#include <string_view>

#define ASSERT_UNIX_OK(e) ASSERT_GE(e, 0) << strerror(errno)

// TODO(bvanassche): remove the code below. See also b/357099095.
#ifndef SO_ATTACH_BPF
#define SO_ATTACH_BPF 50  // From <asm-generic/socket.h>.
#endif

using ::android::base::unique_fd;
using ::testing::ScopedTrace;

struct test_data {
    bool discarded;
    std::string_view str;
};

static const uint8_t binary_bpf_prog[] = {
#include "filterPowerSupplyEvents.h"
};

static std::vector<std::unique_ptr<ScopedTrace>>* msg_vec;

std::ostream& operator<<(std::ostream& os, const test_data& td) {
    os << "{.discarded=" << td.discarded << ", .str=";
    for (auto c : td.str) {
        if (isprint(c)) {
            os << c;
        } else {
            os << ".";
        }
    }
    return os << '}';
}

#define RECORD_ERR_MSG(fmt, ...)                                          \
    do {                                                                  \
        char* str;                                                        \
        if (asprintf(&str, fmt, ##__VA_ARGS__) < 0) break;                \
        auto st = std::make_unique<ScopedTrace>(__FILE__, __LINE__, str); \
        msg_vec->emplace_back(std::move(st));                             \
        free(str);                                                        \
    } while (0)

int libbpf_print_fn(enum libbpf_print_level, const char* fmt, va_list args) {
    char* str;
    if (vasprintf(&str, fmt, args) < 0) {
        return 0;
    }
    msg_vec->emplace_back(std::make_unique<ScopedTrace>(__FILE__, -1, str));
    free(str);
    return 0;
}

static void record_libbpf_output() {
    libbpf_set_print(libbpf_print_fn);
}

class filterPseTest : public testing::TestWithParam<test_data> {};

struct ConnectedSockets {
    unique_fd write_fd;
    unique_fd read_fd;
};

// socketpair() only supports AF_UNIX sockets. AF_UNIX sockets do not
// support BPF filters. Hence connect two TCP sockets with each other.
static ConnectedSockets ConnectSockets(int domain, int type, int protocol) {
    int _server_fd = socket(domain, type, protocol);
    if (_server_fd < 0) {
        return {};
    }
    unique_fd server_fd(_server_fd);

    int _write_fd = socket(domain, type, protocol);
    if (_write_fd < 0) {
        RECORD_ERR_MSG("socket: %s", strerror(errno));
        return {};
    }
    unique_fd write_fd(_write_fd);

    struct sockaddr_in sa = {.sin_family = AF_INET, .sin_addr.s_addr = INADDR_ANY};
    if (bind(_server_fd, (const struct sockaddr*)&sa, sizeof(sa)) < 0) {
        RECORD_ERR_MSG("bind: %s", strerror(errno));
        return {};
    }
    if (listen(_server_fd, 1) < 0) {
        RECORD_ERR_MSG("listen: %s", strerror(errno));
        return {};
    }
    socklen_t addr_len = sizeof(sa);
    if (getsockname(_server_fd, (struct sockaddr*)&sa, &addr_len) < 0) {
        RECORD_ERR_MSG("getsockname: %s", strerror(errno));
        return {};
    }
    errno = 0;
    if (connect(_write_fd, (const struct sockaddr*)&sa, sizeof(sa)) < 0 && errno != EINPROGRESS) {
        RECORD_ERR_MSG("connect: %s", strerror(errno));
        return {};
    }
    int _read_fd = accept(_server_fd, NULL, NULL);
    if (_read_fd < 0) {
        RECORD_ERR_MSG("accept: %s", strerror(errno));
        return {};
    }
    unique_fd read_fd(_read_fd);

    return {.write_fd = std::move(write_fd), .read_fd = std::move(read_fd)};
}

TEST_P(filterPseTest, filterPse) {
    if (getuid() != 0) {
        GTEST_SKIP() << "Must be run as root.";
        return;
    }
    if (!msg_vec) {
        msg_vec = new typeof(*msg_vec);
    }
    std::unique_ptr<int, void (*)(int*)> clear_msg_vec_at_end_of_scope(new int, [](int* p) {
        msg_vec->clear();
        delete p;
    });
    record_libbpf_output();

    auto connected_sockets = ConnectSockets(AF_INET, SOCK_STREAM, 0);
    unique_fd write_fd = std::move(connected_sockets.write_fd);
    unique_fd read_fd = std::move(connected_sockets.read_fd);

    ASSERT_UNIX_OK(fcntl(read_fd, F_SETFL, O_NONBLOCK));

    bpf_object* obj = bpf_object__open_mem(binary_bpf_prog, sizeof(binary_bpf_prog), NULL);
    ASSERT_TRUE(obj) << "bpf_object__open() failed" << strerror(errno);

    // Find the BPF program within the object.
    bpf_program* prog = bpf_object__find_program_by_name(obj, "filterPowerSupplyEvents");
    ASSERT_TRUE(prog);

    ASSERT_UNIX_OK(bpf_program__set_type(prog, BPF_PROG_TYPE_SOCKET_FILTER));

    ASSERT_UNIX_OK(bpf_object__load(obj));

    int filter_fd = bpf_program__fd(prog);
    ASSERT_UNIX_OK(filter_fd);

    int setsockopt_result =
            setsockopt(read_fd, SOL_SOCKET, SO_ATTACH_BPF, &filter_fd, sizeof(filter_fd));
    ASSERT_UNIX_OK(setsockopt_result);

    const test_data param = GetParam();
    const std::string header(sizeof(struct nlmsghdr), '\0');
    ASSERT_EQ(header.length(), sizeof(struct nlmsghdr));
    const std::string data = header + std::string(param.str);
    const size_t len = data.length();
    std::cerr.write(data.data(), data.length());
    std::cerr << ")\n";
    ASSERT_EQ(write(write_fd, data.data(), len), len);
    std::array<uint8_t, 512> read_buf;
    int bytes_read = read(read_fd, read_buf.data(), read_buf.size());
    if (bytes_read < 0) {
        ASSERT_EQ(errno, EAGAIN);
        bytes_read = 0;
    } else {
        ASSERT_LT(bytes_read, read_buf.size());
    }
    EXPECT_EQ(bytes_read, param.discarded ? 0 : len);

    bpf_object__close(obj);
}

static constexpr char input0[] = "a";
static constexpr char input1[] = "abc\0SUBSYSTEM=block\0";
static constexpr char input2[] = "\0SUBSYSTEM=block";
static constexpr char input3[] = "\0SUBSYSTEM=power_supply";
static constexpr char input4[] = "\0SUBSYSTEM=power_supply\0";
static constexpr char input5[] =
        "012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789"
        "012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789"
        "012345678901234567890123456789012345678901234567890123456789\0SUBSYSTEM=block\0";

INSTANTIATE_TEST_SUITE_P(
        filterPse, filterPseTest,
        testing::Values(test_data{false, std::string_view(input0, sizeof(input0) - 1)},
                        test_data{true, std::string_view(input1, sizeof(input1) - 1)},
                        test_data{true, std::string_view(input2, sizeof(input2) - 1)},
                        test_data{true, std::string_view(input3, sizeof(input3) - 1)},
                        test_data{false, std::string_view(input4, sizeof(input4) - 1)},
                        test_data{false, std::string_view(input5, sizeof(input5) - 1)}));
