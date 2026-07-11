// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/check.hpp>
#include <contract/contract.hpp>
#include <contract/adapters/binary/vector.hpp>
#include <contract/security.hpp>

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include <type_traits>

namespace {

struct BinaryPolicyRecord {
    std::string name = "contract";
    std::uint32_t count = 7;
    std::vector<std::uint32_t> ids{1, 2, 3};
    std::vector<std::uint32_t> payload{7, 8, 9};

    CONTRACT(BinaryPolicyRecord,
        (name, 1,
            contract::check::max_length(32),
            contract::security::sensitive(),
            contract::security::secret(),
            contract::security::no_log()),
        (count, 2,
            contract::check::not_empty()),
        (ids, 3,
            contract::check::max_items(4)),
        (payload, 4,
            contract::check::max_bytes(12)))
};

struct BinaryEncryptedInner {
    std::string name = "contract";
    std::vector<std::uint32_t> ids{1, 2, 3};

    CONTRACT(BinaryEncryptedInner,
        (name, 1,
            contract::check::max_length(32)),
        (ids, 2,
            contract::check::max_items(4)))
};

struct BinaryEncryptedRecord {
    BinaryEncryptedInner inner{};
    std::uint32_t count = 7;

    CONTRACT(BinaryEncryptedRecord,
        (inner, 1,
            contract::security::encrypt()),
        (count, 2))
};

struct BinaryEncryptedMixedRecord {
    std::string prefix = "plain";
    BinaryEncryptedInner inner{};
    std::string suffix = "tail";
    std::uint32_t count = 7;

    CONTRACT(BinaryEncryptedMixedRecord,
        (prefix, 1),
        (inner, 2,
            contract::security::encrypt()),
        (suffix, 3),
        (count, 4))
};

struct BinaryPlainRecord {
    BinaryEncryptedInner inner{};
    std::uint32_t count = 7;

    CONTRACT(BinaryPlainRecord,
        (inner, 1),
        (count, 2))
};

struct BinaryPlainMixedRecord {
    std::string prefix = "plain";
    BinaryEncryptedInner inner{};
    std::string suffix = "tail";
    std::uint32_t count = 7;

    CONTRACT(BinaryPlainMixedRecord,
        (prefix, 1),
        (inner, 2),
        (suffix, 3),
        (count, 4))
};

} // namespace

int main() {
    using binary_traits = contract::adapters::binary::adapter_traits;

    static_assert(contract::resolve_attribute_mode<binary_traits>(
        contract::check::max_length(32)).mode ==
        contract::attribute_mode::enforce);
    static_assert(contract::resolve_attribute_mode<binary_traits>(
        contract::check::not_empty()).mode ==
        contract::attribute_mode::ignore);
    static_assert(contract::resolve_attribute_mode<binary_traits>(
        contract::security::sensitive()).mode ==
        contract::attribute_mode::ignore);
    static_assert(contract::resolve_attribute_mode<binary_traits>(
        contract::security::secret()).mode ==
        contract::attribute_mode::ignore);
    static_assert(contract::resolve_attribute_mode<binary_traits>(
        contract::security::no_log()).mode ==
        contract::attribute_mode::out_of_scope);
    static_assert(contract::resolve_attribute_mode<binary_traits>(
        contract::security::encrypt()).mode ==
        contract::attribute_mode::enforce);

    static_assert(contract::adapter_mode_is_valid_v<BinaryPolicyRecord, binary_traits>);
    contract::require_adapter_mode<BinaryPolicyRecord, binary_traits>();

    static_assert(contract::adapter_mode_is_valid_v<BinaryEncryptedRecord, binary_traits>);
    contract::require_adapter_mode<BinaryEncryptedRecord, binary_traits>();
    static_assert(contract::adapter_mode_is_valid_v<BinaryEncryptedMixedRecord, binary_traits>);
    contract::require_adapter_mode<BinaryEncryptedMixedRecord, binary_traits>();

    BinaryPolicyRecord source{};
    source.name = "binary";
    source.count = 11;
    source.ids = {3, 2, 1, 0};
    source.payload = {9, 8, 7};

    std::array<unsigned char, 64> buffer{};
    contract::adapters::binary::writer<> out(buffer.data());
    out << source;

    BinaryPolicyRecord target{};
    target.name = "x";
    target.count = 0;
    target.ids.clear();

    contract::adapters::binary::reader<> in(buffer.data());
    in >> target;

    assert(target.name == "binary");
    assert(target.count == 11);
    assert(target.ids == (std::vector<std::uint32_t>{3, 2, 1, 0}));
    assert(target.payload == (std::vector<std::uint32_t>{9, 8, 7}));

    bool write_name_too_long_failed = false;
    try {
        BinaryPolicyRecord invalid = source;
        invalid.name = std::string(33, 'x');
        contract::adapters::binary::writer<> invalid_out(buffer.data());
        invalid_out << invalid;
    } catch (const std::runtime_error& error) {
        const std::string_view message = error.what();
        assert(message.find("max length exceeded") != std::string_view::npos);
        assert(message.find("field name") != std::string_view::npos);
        write_name_too_long_failed = true;
    }
    assert(write_name_too_long_failed);

    bool read_name_too_long_failed = false;
    {
        std::array<unsigned char, 128> invalid_buffer{};
        contract::adapters::binary::writer<> invalid_out(invalid_buffer.data());
        const std::size_t invalid_name_size = 33;
        const std::uint32_t count = 11;
        const std::size_t ids_size = 4;
        const std::uint32_t ids[] = {3, 2, 1, 0};

        invalid_out << invalid_name_size;
        invalid_out.write(std::string(33, 'x').data(), invalid_name_size);
        invalid_out << count;
        invalid_out << ids_size;
        invalid_out.write(ids, sizeof(ids));

        BinaryPolicyRecord invalid_target{};
        contract::adapters::binary::reader<> invalid_in(invalid_buffer.data());
        try {
            invalid_in >> invalid_target;
        } catch (const std::runtime_error& error) {
            const std::string_view message = error.what();
            assert(message.find("max length exceeded") != std::string_view::npos);
            assert(message.find("field name") != std::string_view::npos);
            read_name_too_long_failed = true;
        }
    }
    assert(read_name_too_long_failed);

    bool read_ids_too_many_failed = false;
    {
        std::array<unsigned char, 128> invalid_buffer{};
        contract::adapters::binary::writer<> invalid_out(invalid_buffer.data());
        const std::string valid_name = "binary";
        const std::uint32_t count = 11;
        const std::size_t ids_size = 5;
        const std::uint32_t ids[] = {3, 2, 1, 0, 9};
        const std::size_t payload_size = 3;
        const std::uint32_t payload[] = {9, 8, 7};

        invalid_out << valid_name.size();
        invalid_out.write(valid_name.data(), valid_name.size());
        invalid_out << count;
        invalid_out << ids_size;
        invalid_out.write(ids, sizeof(ids));
        invalid_out << payload_size;
        invalid_out.write(payload, sizeof(payload));

        BinaryPolicyRecord invalid_target{};
        contract::adapters::binary::reader<> invalid_in(invalid_buffer.data());
        try {
            invalid_in >> invalid_target;
        } catch (const std::runtime_error& error) {
            const std::string_view message = error.what();
            assert(message.find("max items exceeded") != std::string_view::npos);
            assert(message.find("field ids") != std::string_view::npos);
            read_ids_too_many_failed = true;
        }
    }
    assert(read_ids_too_many_failed);

    bool write_payload_too_many_bytes_failed = false;
    try {
        BinaryPolicyRecord invalid = source;
        invalid.payload = {9, 8, 7, 6};
        contract::adapters::binary::writer<> invalid_out(buffer.data());
        invalid_out << invalid;
    } catch (const std::runtime_error& error) {
        const std::string_view message = error.what();
        assert(message.find("max bytes exceeded") != std::string_view::npos);
        assert(message.find("field payload") != std::string_view::npos);
        write_payload_too_many_bytes_failed = true;
    }
    assert(write_payload_too_many_bytes_failed);

    bool read_payload_too_many_bytes_failed = false;
    {
        std::array<unsigned char, 128> invalid_buffer{};
        contract::adapters::binary::writer<> invalid_out(invalid_buffer.data());
        const std::string valid_name = "binary";
        const std::uint32_t count = 11;
        const std::size_t ids_size = 4;
        const std::uint32_t ids[] = {3, 2, 1, 0};
        const std::size_t payload_size = 4;
        const std::uint32_t payload[] = {9, 8, 7, 6};

        invalid_out << valid_name.size();
        invalid_out.write(valid_name.data(), valid_name.size());
        invalid_out << count;
        invalid_out << ids_size;
        invalid_out.write(ids, sizeof(ids));
        invalid_out << payload_size;
        invalid_out.write(payload, sizeof(payload));

        BinaryPolicyRecord invalid_target{};
        contract::adapters::binary::reader<> invalid_in(invalid_buffer.data());
        try {
            invalid_in >> invalid_target;
        } catch (const std::runtime_error& error) {
            const std::string_view message = error.what();
            assert(message.find("max bytes exceeded") != std::string_view::npos);
            assert(message.find("field payload") != std::string_view::npos);
            read_payload_too_many_bytes_failed = true;
        }
    }
    assert(read_payload_too_many_bytes_failed);

    BinaryPlainRecord plain_source{};
    plain_source.inner.name = "binary";
    plain_source.inner.ids = {3, 2, 1, 0};
    plain_source.count = 11;

    BinaryEncryptedRecord encrypted_source{};
    encrypted_source.inner.name = "binary";
    encrypted_source.inner.ids = {3, 2, 1, 0};
    encrypted_source.count = 11;

    std::array<unsigned char, 128> plain_buffer{};
    contract::adapters::binary::writer<> plain_out(plain_buffer.data());
    plain_out << plain_source;
    const auto plain_size = static_cast<std::size_t>(plain_out.current() - plain_buffer.data());

    std::array<unsigned char, 128> encrypted_buffer{};
    contract::adapters::binary::writer<> encrypted_out(encrypted_buffer.data());
    encrypted_out << encrypted_source;
    const auto encrypted_size = static_cast<std::size_t>(
        encrypted_out.current() - encrypted_buffer.data());

    assert(plain_size == encrypted_size);
    assert(!std::equal(
        plain_buffer.begin(),
        plain_buffer.begin() + plain_size,
        encrypted_buffer.begin()));

    BinaryEncryptedRecord encrypted_target{};
    encrypted_target.inner.name = "x";
    encrypted_target.inner.ids.clear();
    encrypted_target.count = 0;

    contract::adapters::binary::reader<> encrypted_in(encrypted_buffer.data());
    encrypted_in >> encrypted_target;

    assert(encrypted_target.inner.name == "binary");
    assert(encrypted_target.inner.ids == (std::vector<std::uint32_t>{3, 2, 1, 0}));
    assert(encrypted_target.count == 11);

    BinaryPlainMixedRecord plain_mixed_source{};
    plain_mixed_source.prefix = "prefix";
    plain_mixed_source.inner.name = "binary";
    plain_mixed_source.inner.ids = {3, 2, 1, 0};
    plain_mixed_source.suffix = "suffix";
    plain_mixed_source.count = 11;

    BinaryEncryptedMixedRecord encrypted_mixed_source{};
    encrypted_mixed_source.prefix = "prefix";
    encrypted_mixed_source.inner.name = "binary";
    encrypted_mixed_source.inner.ids = {3, 2, 1, 0};
    encrypted_mixed_source.suffix = "suffix";
    encrypted_mixed_source.count = 11;

    std::array<unsigned char, 256> plain_mixed_buffer{};
    contract::adapters::binary::writer<> plain_mixed_out(plain_mixed_buffer.data());
    plain_mixed_out << plain_mixed_source;
    const auto plain_mixed_size = static_cast<std::size_t>(
        plain_mixed_out.current() - plain_mixed_buffer.data());

    std::array<unsigned char, 256> encrypted_mixed_buffer{};
    contract::adapters::binary::writer<> encrypted_mixed_out(encrypted_mixed_buffer.data());
    encrypted_mixed_out << encrypted_mixed_source;
    const auto encrypted_mixed_size = static_cast<std::size_t>(
        encrypted_mixed_out.current() - encrypted_mixed_buffer.data());

    assert(plain_mixed_size == encrypted_mixed_size);
    assert(!std::equal(
        plain_mixed_buffer.begin(),
        plain_mixed_buffer.begin() + plain_mixed_size,
        encrypted_mixed_buffer.begin()));

    BinaryEncryptedMixedRecord encrypted_mixed_target{};
    encrypted_mixed_target.prefix = "x";
    encrypted_mixed_target.inner.name = "x";
    encrypted_mixed_target.inner.ids.clear();
    encrypted_mixed_target.suffix = "x";
    encrypted_mixed_target.count = 0;

    contract::adapters::binary::reader<> encrypted_mixed_in(encrypted_mixed_buffer.data());
    encrypted_mixed_in >> encrypted_mixed_target;

    assert(encrypted_mixed_target.prefix == "prefix");
    assert(encrypted_mixed_target.inner.name == "binary");
    assert(encrypted_mixed_target.inner.ids == (std::vector<std::uint32_t>{3, 2, 1, 0}));
    assert(encrypted_mixed_target.suffix == "suffix");
    assert(encrypted_mixed_target.count == 11);

    return 0;
}
