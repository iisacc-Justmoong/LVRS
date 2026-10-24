#include "TextDocumentStorage.h"
#include <iiFileProviderNative.h>
#include <iiFileProviderStorage.h>
#include <mutex>
#include <limits>

namespace lvrs {
using namespace ii;
namespace {
namespace provider = iiFileProvider::native;
core::Error storage_error(const char* operation) {
    return {core::ErrorCode::failed, operation, std::current_exception()};
}
class FileSource final : public TextDocumentSource {
    mutable std::mutex mutex_;
    mutable provider::ReadFile file_;
public:
    explicit FileSource(const std::filesystem::path& path) : file_(path) {}
    std::uint64_t byte_size() const noexcept override { return file_.size(); }
    core::Result<std::string> read(std::uint64_t offset, std::size_t count) const override {
        if (offset > byte_size() || count > std::size_t(std::numeric_limits<std::int64_t>::max())) return core::fail(core::ErrorCode::out_of_range, "Text source read exceeds its range");
        try {
            std::lock_guard lock(mutex_);
            if (!file_.seek(offset)) return core::fail(core::ErrorCode::failed, "Text source seek failed");
            return file_.read(std::int64_t(count));
        } catch (...) { return std::unexpected(storage_error("iiFileProvider text read failed")); }
    }
};
class FileWrite final : public TextDocumentWrite {
    provider::StagedFile stage_;
    provider::OutputFile output_;
    bool finished_ = false;
public:
    explicit FileWrite(const std::filesystem::path& path) : stage_(path), output_(stage_.path(), std::ios::out | std::ios::binary | std::ios::trunc) {
        if (!output_) throw std::runtime_error("iiFileProvider text stage could not open");
    }
    core::Result<void> write(std::string_view value) override {
        if (finished_) return core::fail(core::ErrorCode::closed, "Text stage is closed");
        if(value.size()>std::size_t(std::numeric_limits<std::streamsize>::max()))return core::fail(core::ErrorCode::out_of_range,"Text stage chunk exceeds stream size");
        try {
            output_.write(value.data(), std::streamsize(value.size()));
            if (!output_) return core::fail(core::ErrorCode::failed, "iiFileProvider text stage write failed");
            return {};
        } catch (...) { return std::unexpected(storage_error("iiFileProvider text stage write failed")); }
    }
    core::Result<std::shared_ptr<const TextDocumentSource>> commit() override {
        if (finished_) return core::fail(core::ErrorCode::closed, "Text stage is closed");
        finished_ = true;
        try {
            output_.flush();
            if (!output_) return core::fail(core::ErrorCode::failed, "iiFileProvider text stage flush failed");
            output_.close();
            if (output_.fail()) return core::fail(core::ErrorCode::failed, "iiFileProvider text stage close failed");
            // ReadFile grants FILE_SHARE_DELETE on Windows. Retaining this
            // handle also keeps the exact published inode on POSIX systems.
            auto source = std::make_shared<FileSource>(stage_.path());
            const auto published = stage_.publish();
            if (!published.succeeded) return core::fail(core::ErrorCode::failed, published.message);
            return std::shared_ptr<const TextDocumentSource>(std::move(source));
        } catch (...) { return std::unexpected(storage_error("iiFileProvider text publication failed")); }
    }
};
class FileStorage final : public TextDocumentStorage {
public:
    core::Result<std::shared_ptr<const TextDocumentSource>> open_read(const std::filesystem::path& path) override {
        try { return std::shared_ptr<const TextDocumentSource>(std::make_shared<FileSource>(path)); }
        catch (...) {
            auto error = storage_error("iiFileProvider text open failed");
            std::error_code code;
            if (!std::filesystem::exists(path, code) && !code) error.code = core::ErrorCode::not_found;
            return std::unexpected(std::move(error));
        }
    }
    core::Result<std::unique_ptr<TextDocumentWrite>> begin_write(const std::filesystem::path& path) override {
        try { return std::unique_ptr<TextDocumentWrite>(std::make_unique<FileWrite>(path)); }
        catch (...) { return std::unexpected(storage_error("iiFileProvider text stage creation failed")); }
    }
};
}
std::shared_ptr<TextDocumentStorage> file_provider_text_storage() { return std::make_shared<FileStorage>(); }
} // namespace lvrs
