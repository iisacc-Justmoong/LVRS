#pragma once
#include <iiSDKCore/Core/Result/Result.h>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>

namespace lvrs {
// A retained source survives atomic replacement of its pathname. Implementations
// must return the requested bytes or a short EOF, never silently change offsets.
class TextDocumentSource {
public:
    virtual ~TextDocumentSource() = default;
    [[nodiscard]] virtual std::uint64_t byte_size() const noexcept = 0;
    [[nodiscard]] virtual ii::core::Result<std::string> read(std::uint64_t offset, std::size_t count) const = 0;
};
class TextDocumentWrite {
public:
    virtual ~TextDocumentWrite() = default;
    [[nodiscard]] virtual ii::core::Result<void> write(std::string_view) = 0;
    // Publish atomically and return a source for the exact published bytes.
    // A rejected/abandoned write must leave the previous destination intact.
    [[nodiscard]] virtual ii::core::Result<std::shared_ptr<const TextDocumentSource>> commit() = 0;
};
class TextDocumentStorage {
public:
    virtual ~TextDocumentStorage() = default;
    [[nodiscard]] virtual ii::core::Result<std::shared_ptr<const TextDocumentSource>> open_read(const std::filesystem::path&) = 0;
    [[nodiscard]] virtual ii::core::Result<std::unique_ptr<TextDocumentWrite>> begin_write(const std::filesystem::path&) = 0;
};
// The default adapter uses iiFileProvider Native for every creation/write/
// publication. Custom providers can implement virtual or account-backed files.
[[nodiscard]] std::shared_ptr<TextDocumentStorage> file_provider_text_storage();
} // namespace lvrs
