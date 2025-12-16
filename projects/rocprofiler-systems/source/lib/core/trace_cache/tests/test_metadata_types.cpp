// MIT License
//
// Copyright (c) 2025 Advanced Micro Devices, Inc. All Rights Reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "core/trace_cache/metadata_types.hpp"

#include <array>
#include <cstdint>
#include <gtest/gtest.h>
#include <memory>

using namespace rocprofsys::trace_cache;

class metadata_types_test : public ::testing::Test
{
protected:
    void SetUp() override { buffer.fill(0); }

    std::array<uint8_t, 4096> buffer;
};

TEST_F(metadata_types_test, metadata_identifier_enum_values)
{
    EXPECT_EQ(static_cast<uint8_t>(info::metadata_identifier_t::process), 0x00);
    EXPECT_EQ(static_cast<uint8_t>(info::metadata_identifier_t::pmc), 0x01);
    EXPECT_EQ(static_cast<uint8_t>(info::metadata_identifier_t::thread), 0x02);
    EXPECT_EQ(static_cast<uint8_t>(info::metadata_identifier_t::track), 0x03);
    EXPECT_EQ(static_cast<uint8_t>(info::metadata_identifier_t::queue), 0x04);
    EXPECT_EQ(static_cast<uint8_t>(info::metadata_identifier_t::stream), 0x05);
    EXPECT_EQ(static_cast<uint8_t>(info::metadata_identifier_t::string), 0x06);
    EXPECT_EQ(static_cast<uint8_t>(info::metadata_identifier_t::code_object), 0x07);
    EXPECT_EQ(static_cast<uint8_t>(info::metadata_identifier_t::kernel_symbol), 0x08);
    EXPECT_EQ(static_cast<uint8_t>(info::metadata_identifier_t::agent), 0x09);
    EXPECT_EQ(static_cast<uint8_t>(info::metadata_identifier_t::process_start_time),
              0x0A);
    EXPECT_EQ(static_cast<uint8_t>(info::metadata_identifier_t::process_end_time), 0x0B);
    EXPECT_EQ(static_cast<uint8_t>(info::metadata_identifier_t::fragmented_space), 0xFF);
}

TEST_F(metadata_types_test, process_serialize_deserialize)
{
    info::process original(1234, 5678, "/usr/bin/test_application");

    serialize(buffer.data(), original);

    uint8_t* buffer_ptr   = buffer.data();
    auto     deserialized = deserialize<info::process>(buffer_ptr);

    EXPECT_EQ(deserialized.pid, original.pid);
    EXPECT_EQ(deserialized.ppid, original.ppid);
    EXPECT_EQ(deserialized.command, original.command);
}

TEST_F(metadata_types_test, process_get_size)
{
    info::process sample(1234, 5678, "/usr/bin/test");

    size_t expected_size =
        sizeof(int32_t) * 2 + sizeof(size_t) + std::string("/usr/bin/test").size();

    EXPECT_EQ(get_size(sample), expected_size);
}

TEST_F(metadata_types_test, process_type_identifier)
{
    EXPECT_EQ(info::process::type_identifier, info::metadata_identifier_t::process);
}

TEST_F(metadata_types_test, process_hash)
{
    info::process p1(1234, 5678, "/usr/bin/app");
    info::process p2(1234, 9999, "/different/app");
    info::process p3(9999, 5678, "/usr/bin/app");

    EXPECT_EQ(p1.hash(), p2.hash());
    EXPECT_NE(p1.hash(), p3.hash());
}

TEST_F(metadata_types_test, process_default_constructor)
{
    info::process sample;
    EXPECT_EQ(sample.type_identifier, info::metadata_identifier_t::process);
}

TEST_F(metadata_types_test, process_empty_command)
{
    info::process original(100, 200, "");

    serialize(buffer.data(), original);

    uint8_t* buffer_ptr   = buffer.data();
    auto     deserialized = deserialize<info::process>(buffer_ptr);

    EXPECT_EQ(deserialized.pid, 100);
    EXPECT_EQ(deserialized.ppid, 200);
    EXPECT_EQ(deserialized.command, "");
}

TEST_F(metadata_types_test, process_start_time_serialize_deserialize)
{
    info::process_start_time original(1000000000);

    serialize(buffer.data(), original);

    uint8_t* buffer_ptr   = buffer.data();
    auto     deserialized = deserialize<info::process_start_time>(buffer_ptr);

    EXPECT_EQ(deserialized.start, original.start);
}

TEST_F(metadata_types_test, process_start_time_get_size)
{
    info::process_start_time sample(1000000000);

    EXPECT_EQ(get_size(sample), sizeof(int64_t));
}

TEST_F(metadata_types_test, process_start_time_type_identifier)
{
    EXPECT_EQ(info::process_start_time::type_identifier,
              info::metadata_identifier_t::process_start_time);
}

TEST_F(metadata_types_test, process_start_time_hash)
{
    info::process_start_time t1(1000);
    info::process_start_time t2(1000);
    info::process_start_time t3(2000);

    EXPECT_EQ(t1.hash(), t2.hash());
    EXPECT_NE(t1.hash(), t3.hash());
}

TEST_F(metadata_types_test, process_start_time_default_constructor)
{
    info::process_start_time sample;
    EXPECT_EQ(sample.type_identifier, info::metadata_identifier_t::process_start_time);
}

TEST_F(metadata_types_test, process_end_time_serialize_deserialize)
{
    info::process_end_time original(2000000000);

    serialize(buffer.data(), original);

    uint8_t* buffer_ptr   = buffer.data();
    auto     deserialized = deserialize<info::process_end_time>(buffer_ptr);

    EXPECT_EQ(deserialized.end, original.end);
}

TEST_F(metadata_types_test, process_end_time_get_size)
{
    info::process_end_time sample(2000000000);

    EXPECT_EQ(get_size(sample), sizeof(int64_t));
}

TEST_F(metadata_types_test, process_end_time_type_identifier)
{
    EXPECT_EQ(info::process_end_time::type_identifier,
              info::metadata_identifier_t::process_end_time);
}

TEST_F(metadata_types_test, process_end_time_hash)
{
    info::process_end_time t1(1000);
    info::process_end_time t2(1000);
    info::process_end_time t3(2000);

    EXPECT_EQ(t1.hash(), t2.hash());
    EXPECT_NE(t1.hash(), t3.hash());
}

TEST_F(metadata_types_test, process_end_time_default_constructor)
{
    info::process_end_time sample;
    EXPECT_EQ(sample.type_identifier, info::metadata_identifier_t::process_end_time);
}

TEST_F(metadata_types_test, pmc_serialize_deserialize)
{
    info::pmc original(rocprofsys::agent_type::GPU, 0, "gfx90a", 100, 0, "counter_name",
                       "CNT", "Description", "Long description", "component", "cycles",
                       "uint64", "block1", "expr", 0, 1, "{\"key\":\"value\"}");

    serialize(buffer.data(), original);

    uint8_t* buffer_ptr   = buffer.data();
    auto     deserialized = deserialize<info::pmc>(buffer_ptr);

    EXPECT_EQ(deserialized.type, original.type);
    EXPECT_EQ(deserialized.agent_type_index, original.agent_type_index);
    EXPECT_EQ(deserialized.target_arch, original.target_arch);
    EXPECT_EQ(deserialized.event_code, original.event_code);
    EXPECT_EQ(deserialized.instance_id, original.instance_id);
    EXPECT_EQ(deserialized.name, original.name);
    EXPECT_EQ(deserialized.symbol, original.symbol);
    EXPECT_EQ(deserialized.description, original.description);
    EXPECT_EQ(deserialized.long_description, original.long_description);
    EXPECT_EQ(deserialized.component, original.component);
    EXPECT_EQ(deserialized.units, original.units);
    EXPECT_EQ(deserialized.value_type, original.value_type);
    EXPECT_EQ(deserialized.block, original.block);
    EXPECT_EQ(deserialized.expression, original.expression);
    EXPECT_EQ(deserialized.is_constant, original.is_constant);
    EXPECT_EQ(deserialized.is_derived, original.is_derived);
    EXPECT_EQ(deserialized.extdata, original.extdata);
}

TEST_F(metadata_types_test, pmc_type_identifier)
{
    EXPECT_EQ(info::pmc::type_identifier, info::metadata_identifier_t::pmc);
}

TEST_F(metadata_types_test, pmc_hash)
{
    info::pmc p1(rocprofsys::agent_type::GPU, 0, "gfx90a", 100, 0, "counter1", "C1", "",
                 "", "", "", "", "", "", 0, 0);
    info::pmc p2(rocprofsys::agent_type::GPU, 0, "gfx90a", 200, 1, "counter1", "C2", "",
                 "", "", "", "", "", "", 1, 1);
    info::pmc p3(rocprofsys::agent_type::CPU, 1, "zen3", 100, 0, "counter2", "C1", "", "",
                 "", "", "", "", "", 0, 0);

    EXPECT_EQ(p1.hash(), p2.hash());
    EXPECT_NE(p1.hash(), p3.hash());
}

TEST_F(metadata_types_test, pmc_default_constructor)
{
    info::pmc sample;
    EXPECT_EQ(sample.type_identifier, info::metadata_identifier_t::pmc);
}

TEST_F(metadata_types_test, pmc_empty_strings)
{
    info::pmc original(rocprofsys::agent_type::CPU, 0, "", 0, 0, "", "", "", "", "", "",
                       "", "", "", 0, 0, "");

    serialize(buffer.data(), original);

    uint8_t* buffer_ptr   = buffer.data();
    auto     deserialized = deserialize<info::pmc>(buffer_ptr);

    EXPECT_EQ(deserialized.target_arch, "");
    EXPECT_EQ(deserialized.name, "");
    EXPECT_EQ(deserialized.symbol, "");
    EXPECT_EQ(deserialized.extdata, "");
}

TEST_F(metadata_types_test, thread_serialize_deserialize)
{
    info::thread original(100, 1234, 9999, 0, 1000, "{\"name\":\"worker\"}");

    serialize(buffer.data(), original);

    uint8_t* buffer_ptr   = buffer.data();
    auto     deserialized = deserialize<info::thread>(buffer_ptr);

    EXPECT_EQ(deserialized.parent_process_id, original.parent_process_id);
    EXPECT_EQ(deserialized.process_id, original.process_id);
    EXPECT_EQ(deserialized.thread_id, original.thread_id);
    EXPECT_EQ(deserialized.start, original.start);
    EXPECT_EQ(deserialized.end, original.end);
    EXPECT_EQ(deserialized.extdata, original.extdata);
}

TEST_F(metadata_types_test, thread_get_size)
{
    info::thread sample(100, 1234, 9999, 0, 1000, "{}");

    size_t expected_size = sizeof(int32_t) * 2 + sizeof(uint64_t) + sizeof(uint32_t) * 2 +
                           sizeof(size_t) + 2;

    EXPECT_EQ(get_size(sample), expected_size);
}

TEST_F(metadata_types_test, thread_type_identifier)
{
    EXPECT_EQ(info::thread::type_identifier, info::metadata_identifier_t::thread);
}

TEST_F(metadata_types_test, thread_hash)
{
    info::thread t1(100, 1234, 9999, 0, 1000, "{}");
    info::thread t2(200, 5678, 9999, 100, 2000, "{\"other\":true}");
    info::thread t3(100, 1234, 1111, 0, 1000, "{}");

    EXPECT_EQ(t1.hash(), t2.hash());
    EXPECT_NE(t1.hash(), t3.hash());
}

TEST_F(metadata_types_test, thread_default_constructor)
{
    info::thread sample;
    EXPECT_EQ(sample.type_identifier, info::metadata_identifier_t::thread);
}

TEST_F(metadata_types_test, track_serialize_deserialize)
{
    info::track original("GPU Track 0", std::make_optional<size_t>(1001),
                         "{\"gpu_id\":0}");

    serialize(buffer.data(), original);

    uint8_t* buffer_ptr   = buffer.data();
    auto     deserialized = deserialize<info::track>(buffer_ptr);

    EXPECT_EQ(deserialized.track_name, original.track_name);
    EXPECT_TRUE(deserialized.thread_id.has_value());
    EXPECT_EQ(deserialized.thread_id.value(), original.thread_id.value());
    EXPECT_EQ(deserialized.extdata, original.extdata);
}

TEST_F(metadata_types_test, track_serialize_deserialize_nullopt)
{
    info::track original("CPU Track", std::nullopt, "{}");

    serialize(buffer.data(), original);

    uint8_t* buffer_ptr   = buffer.data();
    auto     deserialized = deserialize<info::track>(buffer_ptr);

    EXPECT_EQ(deserialized.track_name, original.track_name);
    EXPECT_FALSE(deserialized.thread_id.has_value());
    EXPECT_EQ(deserialized.extdata, original.extdata);
}

TEST_F(metadata_types_test, track_get_size)
{
    info::track sample("Track", std::make_optional<size_t>(100), "{}");

    size_t expected_size = sizeof(size_t) + 5 + sizeof(size_t) + sizeof(size_t) + 2;

    EXPECT_EQ(get_size(sample), expected_size);
}

TEST_F(metadata_types_test, track_type_identifier)
{
    EXPECT_EQ(info::track::type_identifier, info::metadata_identifier_t::track);
}

TEST_F(metadata_types_test, track_hash)
{
    info::track t1("GPU Track", std::make_optional<size_t>(100), "{}");
    info::track t2("GPU Track", std::nullopt, "{\"other\":true}");
    info::track t3("CPU Track", std::make_optional<size_t>(100), "{}");

    EXPECT_EQ(t1.hash(), t2.hash());
    EXPECT_NE(t1.hash(), t3.hash());
}

TEST_F(metadata_types_test, track_default_constructor)
{
    info::track sample;
    EXPECT_EQ(sample.type_identifier, info::metadata_identifier_t::track);
}

TEST_F(metadata_types_test, queue_serialize_deserialize)
{
    info::queue original(0xDEADBEEF12345678);

    serialize(buffer.data(), original);

    uint8_t* buffer_ptr   = buffer.data();
    auto     deserialized = deserialize<info::queue>(buffer_ptr);

    EXPECT_EQ(deserialized.handle, original.handle);
}

TEST_F(metadata_types_test, queue_get_size)
{
    info::queue sample(0x1234);

    EXPECT_EQ(get_size(sample), sizeof(uint64_t));
}

TEST_F(metadata_types_test, queue_type_identifier)
{
    EXPECT_EQ(info::queue::type_identifier, info::metadata_identifier_t::queue);
}

TEST_F(metadata_types_test, queue_hash)
{
    info::queue q1(0x1234);
    info::queue q2(0x1234);
    info::queue q3(0x5678);

    EXPECT_EQ(q1.hash(), q2.hash());
    EXPECT_NE(q1.hash(), q3.hash());
}

TEST_F(metadata_types_test, queue_default_constructor)
{
    info::queue sample;
    EXPECT_EQ(sample.type_identifier, info::metadata_identifier_t::queue);
}

TEST_F(metadata_types_test, stream_serialize_deserialize)
{
    info::stream original(0xCAFEBABE);

    serialize(buffer.data(), original);

    uint8_t* buffer_ptr   = buffer.data();
    auto     deserialized = deserialize<info::stream>(buffer_ptr);

    EXPECT_EQ(deserialized.handle, original.handle);
}

TEST_F(metadata_types_test, stream_get_size)
{
    info::stream sample(0x1234);

    EXPECT_EQ(get_size(sample), sizeof(uint64_t));
}

TEST_F(metadata_types_test, stream_type_identifier)
{
    EXPECT_EQ(info::stream::type_identifier, info::metadata_identifier_t::stream);
}

TEST_F(metadata_types_test, stream_hash)
{
    info::stream s1(0x1234);
    info::stream s2(0x1234);
    info::stream s3(0x5678);

    EXPECT_EQ(s1.hash(), s2.hash());
    EXPECT_NE(s1.hash(), s3.hash());
}

TEST_F(metadata_types_test, stream_default_constructor)
{
    info::stream sample;
    EXPECT_EQ(sample.type_identifier, info::metadata_identifier_t::stream);
}

TEST_F(metadata_types_test, string_entry_serialize_deserialize)
{
    std::string        str = "test_string_value";
    info::string_entry original(str);

    serialize(buffer.data(), original);

    uint8_t* buffer_ptr   = buffer.data();
    auto     deserialized = deserialize<info::string_entry>(buffer_ptr);

    EXPECT_EQ(deserialized.value, original.value);
}

TEST_F(metadata_types_test, string_entry_get_size)
{
    std::string        str = "hello";
    info::string_entry sample(str);

    size_t expected_size = sizeof(size_t) + 5;

    EXPECT_EQ(get_size(sample), expected_size);
}

TEST_F(metadata_types_test, string_entry_type_identifier)
{
    EXPECT_EQ(info::string_entry::type_identifier, info::metadata_identifier_t::string);
}

TEST_F(metadata_types_test, string_entry_hash)
{
    std::string        str1 = "hello";
    std::string        str2 = "hello";
    std::string        str3 = "world";
    info::string_entry s1(str1);
    info::string_entry s2(str2);
    info::string_entry s3(str3);

    EXPECT_EQ(s1.hash(), s2.hash());
    EXPECT_NE(s1.hash(), s3.hash());
}

TEST_F(metadata_types_test, string_entry_default_constructor)
{
    info::string_entry sample;
    EXPECT_EQ(sample.type_identifier, info::metadata_identifier_t::string);
}

TEST_F(metadata_types_test, string_entry_empty)
{
    std::string        str;
    info::string_entry original(str);

    serialize(buffer.data(), original);

    uint8_t* buffer_ptr   = buffer.data();
    auto     deserialized = deserialize<info::string_entry>(buffer_ptr);

    EXPECT_EQ(deserialized.value, "");
}

TEST_F(metadata_types_test, code_object_serialize_deserialize)
{
    info::code_object original(42, "file:///path/to/code.so", 0x1000, 0x5000, -100, 1,
                               0xABCD);

    serialize(buffer.data(), original);

    uint8_t* buffer_ptr   = buffer.data();
    auto     deserialized = deserialize<info::code_object>(buffer_ptr);

    EXPECT_EQ(deserialized.code_object_id, original.code_object_id);
    EXPECT_EQ(deserialized.uri, original.uri);
    EXPECT_EQ(deserialized.load_base, original.load_base);
    EXPECT_EQ(deserialized.load_size, original.load_size);
    EXPECT_EQ(deserialized.load_delta, original.load_delta);
    EXPECT_EQ(deserialized.storage_type, original.storage_type);
    EXPECT_EQ(deserialized.agent_id_handle, original.agent_id_handle);
}

TEST_F(metadata_types_test, code_object_get_size)
{
    info::code_object sample(42, "file:///test.so", 0x1000, 0x5000, 0, 0, 0);

    size_t expected_size = sizeof(uint64_t) + sizeof(size_t) + 15 + sizeof(uint64_t) * 2 +
                           sizeof(int64_t) + sizeof(int32_t) + sizeof(uint64_t);

    EXPECT_EQ(get_size(sample), expected_size);
}

TEST_F(metadata_types_test, code_object_type_identifier)
{
    EXPECT_EQ(info::code_object::type_identifier,
              info::metadata_identifier_t::code_object);
}

TEST_F(metadata_types_test, code_object_hash)
{
    info::code_object co1(42, "file:///a.so", 0x1000, 0x5000, 0, 0, 0);
    info::code_object co2(42, "file:///b.so", 0x2000, 0x6000, 100, 1, 1);
    info::code_object co3(99, "file:///a.so", 0x1000, 0x5000, 0, 0, 0);

    EXPECT_EQ(co1.hash(), co2.hash());
    EXPECT_NE(co1.hash(), co3.hash());
}

TEST_F(metadata_types_test, code_object_default_constructor)
{
    info::code_object sample;
    EXPECT_EQ(sample.type_identifier, info::metadata_identifier_t::code_object);
}

TEST_F(metadata_types_test, code_object_empty_uri)
{
    info::code_object original(1, "", 0, 0, 0, 0, 0);

    serialize(buffer.data(), original);

    uint8_t* buffer_ptr   = buffer.data();
    auto     deserialized = deserialize<info::code_object>(buffer_ptr);

    EXPECT_EQ(deserialized.uri, "");
}

TEST_F(metadata_types_test, kernel_symbol_serialize_deserialize)
{
    info::kernel_symbol original(100, 42, "my_kernel", 0x3000, 256, 8, 1024, 512, 32, 64,
                                 16);

    serialize(buffer.data(), original);

    uint8_t* buffer_ptr   = buffer.data();
    auto     deserialized = deserialize<info::kernel_symbol>(buffer_ptr);

    EXPECT_EQ(deserialized.kernel_id, original.kernel_id);
    EXPECT_EQ(deserialized.code_object_id, original.code_object_id);
    EXPECT_EQ(deserialized.kernel_name, original.kernel_name);
    EXPECT_EQ(deserialized.kernel_object, original.kernel_object);
    EXPECT_EQ(deserialized.kernarg_segment_size, original.kernarg_segment_size);
    EXPECT_EQ(deserialized.kernarg_segment_alignment, original.kernarg_segment_alignment);
    EXPECT_EQ(deserialized.group_segment_size, original.group_segment_size);
    EXPECT_EQ(deserialized.private_segment_size, original.private_segment_size);
    EXPECT_EQ(deserialized.sgpr_count, original.sgpr_count);
    EXPECT_EQ(deserialized.arch_vgpr_count, original.arch_vgpr_count);
    EXPECT_EQ(deserialized.accum_vgpr_count, original.accum_vgpr_count);
}

TEST_F(metadata_types_test, kernel_symbol_get_size)
{
    info::kernel_symbol sample(100, 42, "kernel", 0x3000, 256, 8, 1024, 512, 32, 64, 16);

    size_t expected_size =
        sizeof(uint64_t) * 3 + sizeof(size_t) + 6 + sizeof(uint32_t) * 7;

    EXPECT_EQ(get_size(sample), expected_size);
}

TEST_F(metadata_types_test, kernel_symbol_type_identifier)
{
    EXPECT_EQ(info::kernel_symbol::type_identifier,
              info::metadata_identifier_t::kernel_symbol);
}

TEST_F(metadata_types_test, kernel_symbol_hash)
{
    info::kernel_symbol ks1(100, 42, "kernel_a", 0x3000, 256, 8, 1024, 512, 32, 64, 16);
    info::kernel_symbol ks2(100, 99, "kernel_b", 0x4000, 512, 16, 2048, 1024, 64, 128,
                            32);
    info::kernel_symbol ks3(200, 42, "kernel_a", 0x3000, 256, 8, 1024, 512, 32, 64, 16);

    EXPECT_EQ(ks1.hash(), ks2.hash());
    EXPECT_NE(ks1.hash(), ks3.hash());
}

TEST_F(metadata_types_test, kernel_symbol_default_constructor)
{
    info::kernel_symbol sample;
    EXPECT_EQ(sample.type_identifier, info::metadata_identifier_t::kernel_symbol);
}

TEST_F(metadata_types_test, kernel_symbol_empty_name)
{
    info::kernel_symbol original(1, 1, "", 0, 0, 0, 0, 0, 0, 0, 0);

    serialize(buffer.data(), original);

    uint8_t* buffer_ptr   = buffer.data();
    auto     deserialized = deserialize<info::kernel_symbol>(buffer_ptr);

    EXPECT_EQ(deserialized.kernel_name, "");
}

TEST_F(metadata_types_test, agent_serialize_deserialize)
{
    auto agent_ptr                  = std::make_shared<rocprofsys::agent>();
    agent_ptr->type                 = rocprofsys::agent_type::GPU;
    agent_ptr->handle               = 0x1234;
    agent_ptr->device_id            = 0;
    agent_ptr->node_id              = 1;
    agent_ptr->logical_node_id      = 2;
    agent_ptr->logical_node_type_id = 3;
    agent_ptr->name                 = "gfx90a";
    agent_ptr->model_name           = "AMD Instinct MI200";
    agent_ptr->vendor_name          = "AMD";
    agent_ptr->product_name         = "MI250X";
    agent_ptr->device_type_index    = 0;
    agent_ptr->base_id              = 100;
    agent_ptr->agent_info           = "{\"compute_units\":110}";

    info::agent_t original(agent_ptr);

    serialize(buffer.data(), original);

    uint8_t* buffer_ptr   = buffer.data();
    auto     deserialized = deserialize<info::agent_t>(buffer_ptr);

    EXPECT_EQ(deserialized.agent_ptr->type, original.agent_ptr->type);
    EXPECT_EQ(deserialized.agent_ptr->handle, original.agent_ptr->handle);
    EXPECT_EQ(deserialized.agent_ptr->device_id, original.agent_ptr->device_id);
    EXPECT_EQ(deserialized.agent_ptr->node_id, original.agent_ptr->node_id);
    EXPECT_EQ(deserialized.agent_ptr->logical_node_id,
              original.agent_ptr->logical_node_id);
    EXPECT_EQ(deserialized.agent_ptr->logical_node_type_id,
              original.agent_ptr->logical_node_type_id);
    EXPECT_EQ(deserialized.agent_ptr->name, original.agent_ptr->name);
    EXPECT_EQ(deserialized.agent_ptr->model_name, original.agent_ptr->model_name);
    EXPECT_EQ(deserialized.agent_ptr->vendor_name, original.agent_ptr->vendor_name);
    EXPECT_EQ(deserialized.agent_ptr->product_name, original.agent_ptr->product_name);
    EXPECT_EQ(deserialized.agent_ptr->device_type_index,
              original.agent_ptr->device_type_index);
    EXPECT_EQ(deserialized.agent_ptr->base_id, original.agent_ptr->base_id);
    EXPECT_EQ(deserialized.agent_ptr->agent_info, original.agent_ptr->agent_info);
}

TEST_F(metadata_types_test, agent_type_identifier)
{
    EXPECT_EQ(info::agent_t::type_identifier, info::metadata_identifier_t::agent);
}

TEST_F(metadata_types_test, agent_hash)
{
    auto a1_ptr               = std::make_shared<rocprofsys::agent>();
    a1_ptr->type              = rocprofsys::agent_type::GPU;
    a1_ptr->device_id         = 0;
    a1_ptr->device_type_index = 0;

    auto a2_ptr               = std::make_shared<rocprofsys::agent>();
    a2_ptr->type              = rocprofsys::agent_type::GPU;
    a2_ptr->device_id         = 0;
    a2_ptr->device_type_index = 0;

    auto a3_ptr               = std::make_shared<rocprofsys::agent>();
    a3_ptr->type              = rocprofsys::agent_type::CPU;
    a3_ptr->device_id         = 0;
    a3_ptr->device_type_index = 0;

    info::agent_t a1(a1_ptr);
    info::agent_t a2(a2_ptr);
    info::agent_t a3(a3_ptr);

    EXPECT_EQ(a1.hash(), a2.hash());
    EXPECT_NE(a1.hash(), a3.hash());
}

TEST_F(metadata_types_test, agent_default_constructor)
{
    info::agent_t sample;
    EXPECT_EQ(sample.type_identifier, info::metadata_identifier_t::agent);
}

TEST_F(metadata_types_test, agent_empty_strings)
{
    auto agent_ptr                  = std::make_shared<rocprofsys::agent>();
    agent_ptr->type                 = rocprofsys::agent_type::CPU;
    agent_ptr->handle               = 0;
    agent_ptr->device_id            = 0;
    agent_ptr->node_id              = 0;
    agent_ptr->logical_node_id      = 0;
    agent_ptr->logical_node_type_id = 0;
    agent_ptr->name                 = "";
    agent_ptr->model_name           = "";
    agent_ptr->vendor_name          = "";
    agent_ptr->product_name         = "";
    agent_ptr->device_type_index    = 0;
    agent_ptr->base_id              = 0;
    agent_ptr->agent_info           = "";

    info::agent_t original(agent_ptr);

    serialize(buffer.data(), original);

    uint8_t* buffer_ptr   = buffer.data();
    auto     deserialized = deserialize<info::agent_t>(buffer_ptr);

    EXPECT_EQ(deserialized.agent_ptr->name, "");
    EXPECT_EQ(deserialized.agent_ptr->model_name, "");
    EXPECT_EQ(deserialized.agent_ptr->vendor_name, "");
    EXPECT_EQ(deserialized.agent_ptr->product_name, "");
    EXPECT_EQ(deserialized.agent_ptr->agent_info, "");
}

TEST_F(metadata_types_test, process_large_values)
{
    info::process original(INT32_MAX, INT32_MIN, std::string(1000, 'x'));

    serialize(buffer.data(), original);

    uint8_t* buffer_ptr   = buffer.data();
    auto     deserialized = deserialize<info::process>(buffer_ptr);

    EXPECT_EQ(deserialized.pid, INT32_MAX);
    EXPECT_EQ(deserialized.ppid, INT32_MIN);
    EXPECT_EQ(deserialized.command.size(), 1000);
}

TEST_F(metadata_types_test, process_times_large_values)
{
    info::process_start_time start(INT64_MAX);
    info::process_end_time   end(INT64_MIN);

    serialize(buffer.data(), start);
    uint8_t* buffer_ptr      = buffer.data();
    auto     deserialized_st = deserialize<info::process_start_time>(buffer_ptr);
    EXPECT_EQ(deserialized_st.start, INT64_MAX);

    buffer.fill(0);
    serialize(buffer.data(), end);
    buffer_ptr           = buffer.data();
    auto deserialized_et = deserialize<info::process_end_time>(buffer_ptr);
    EXPECT_EQ(deserialized_et.end, INT64_MIN);
}

TEST_F(metadata_types_test, queue_stream_large_values)
{
    info::queue  q(UINT64_MAX);
    info::stream s(UINT64_MAX);

    serialize(buffer.data(), q);
    uint8_t* buffer_ptr = buffer.data();
    auto     deser_q    = deserialize<info::queue>(buffer_ptr);
    EXPECT_EQ(deser_q.handle, UINT64_MAX);

    buffer.fill(0);
    serialize(buffer.data(), s);
    buffer_ptr   = buffer.data();
    auto deser_s = deserialize<info::stream>(buffer_ptr);
    EXPECT_EQ(deser_s.handle, UINT64_MAX);
}

TEST_F(metadata_types_test, thread_large_values)
{
    info::thread original(INT32_MAX, INT32_MIN, UINT64_MAX, UINT32_MAX, UINT32_MAX, "{}");

    serialize(buffer.data(), original);

    uint8_t* buffer_ptr   = buffer.data();
    auto     deserialized = deserialize<info::thread>(buffer_ptr);

    EXPECT_EQ(deserialized.parent_process_id, INT32_MAX);
    EXPECT_EQ(deserialized.process_id, INT32_MIN);
    EXPECT_EQ(deserialized.thread_id, UINT64_MAX);
    EXPECT_EQ(deserialized.start, UINT32_MAX);
    EXPECT_EQ(deserialized.end, UINT32_MAX);
}
