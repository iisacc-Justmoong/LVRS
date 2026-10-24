#include "TextDocumentModel.h"
#include "TextDocumentDetail.h"
#include <iiGUI/TextBoundary.h>
#include <iiSDKCore/Core/Text/Utf8.h>
#include <algorithm>
#include <atomic>
#include <deque>
#include <list>
#include <thread>
#include <unordered_map>

namespace lvrs {
using namespace ii;
namespace detail = text_detail;
namespace {
template<class T, class F> core::Result<T> protect(F&& operation) {
    try { return operation(); }
    catch (...) { return core::fail(core::ErrorCode::failed, "Text document operation failed", std::current_exception()); }
}
struct Flag final {
    bool& value;
    explicit Flag(bool& flag) : value(flag) { value = true; }
    ~Flag() { value = false; }
};
class SavedSource final : public TextDocumentSource {
public:
    std::shared_ptr<const TextDocumentSource> source;
    std::uint64_t byte_size()const noexcept override{return source?source->byte_size():0;}
    core::Result<std::string> read(std::uint64_t offset,std::size_t amount)const override{
        if(!source)return core::fail(core::ErrorCode::closed,"Text publication source is not bound");
        return source->read(offset,amount);
    }
};
core::Result<void> valid_options(const TextDocumentOptions& o) {
    if (!o.load_chunk_size || o.load_chunk_size > 16 * 1024 * 1024 || !o.maximum_line_bytes || o.maximum_line_bytes > gui::TextIndex::maximum_text_bytes ||
        !o.maximum_file_bytes || o.maximum_file_bytes > (std::uint64_t(1) << 60) || !o.maximum_lines || o.maximum_lines > 16000000 ||
        !o.maximum_materialized_bytes || o.decoded_cache_entries > 65536 || o.undo_entries > 65536)
        return core::fail(core::ErrorCode::invalid_argument, "Invalid text document resource limits");
    return {};
}
}
struct TextDocumentModel::Impl final : std::enable_shared_from_this<Impl> {
    struct Cache final {gui::TextIndex value; std::list<std::uint64_t>::iterator position;};
    struct History final {
        detail::Tree before, after;
        TextDocumentCursor cursor_before, cursor_after;
        std::uint64_t stamp_before = 0, stamp_after = 0;
        std::size_t cost = 0;
        TextPosition first = 0, removed = 0, inserted = 0;
    };
    runtime::EventLoop::Sender loop;
    runtime::TaskScope lifetime;
    std::thread::id owner = std::this_thread::get_id();
    std::atomic_bool closed = false, wrong_dispatch = false;
    TextDocumentOptions options;
    std::shared_ptr<TextDocumentStorage> storage;
    detail::Tree root;
    TextDocumentCursor cursor;
    std::filesystem::path file_path, loading_path;
    std::uint64_t revision = 1, document_revision = 1, record_sequence = 0, stamp = 1, next_stamp = 1, clean_stamp = 1;
    bool committing = false, notifying = false, reading = false, loading = false, pending_cr = false, finalized = false;
    std::uint64_t load_generation = 0, loaded_bytes = 0, total_bytes = 0, pending_offset = 0;
    std::string pending_line, last_error;
    std::shared_ptr<const TextDocumentSource> loading_source;
    runtime::TaskToken scheduled;
    std::vector<std::weak_ptr<const TextDocumentPort>> receivers;
    std::deque<History> undo, redo;
    std::list<std::uint64_t> recent;
    std::unordered_map<std::uint64_t,Cache> cache;
    TextDocumentStats statistics;

    Impl(runtime::EventLoop::Sender endpoint, TextDocumentOptions value, std::shared_ptr<TextDocumentStorage> provider)
        : loop(std::move(endpoint)), options(value), storage(std::move(provider)) { root = std::make_shared<detail::LineNode>(memory_record({})); }
    core::Result<void> ready(bool mutate = false) const {
        if (wrong_dispatch.load()) return core::fail(core::ErrorCode::wrong_executor, "Text document Core loop dispatched on another thread");
        if (closed.load()) return core::fail(core::ErrorCode::closed, "Text document is closed");
        if (owner != std::this_thread::get_id()) return core::fail(core::ErrorCode::wrong_executor, "Text document requires its owner thread");
        if (mutate && (committing || notifying || reading)) return core::fail(core::ErrorCode::busy, "Text document is delivering or committing an operation");
        if (mutate && (revision == UINT64_MAX || document_revision == UINT64_MAX || next_stamp == UINT64_MAX)) return core::fail(core::ErrorCode::full, "Text document revision exhausted");
        return {};
    }
    TextPosition characters() const noexcept { return detail::units(root) - 1; }
    TextDocumentState snapshot() const {
        return {.revision=revision, .document_revision=document_revision, .file_path=file_path,
            .has_file_path=!file_path.empty(), .dirty=stamp!=clean_stamp, .loading=loading,
            .character_count=characters(), .line_count=detail::count(root), .file_backed_line_count=detail::files(root),
            .memory_line_count=detail::count(root)-detail::files(root), .cursor=cursor, .last_error=last_error,
            .loaded_byte_count=loaded_bytes, .total_byte_count=total_bytes,
            .load_progress=total_bytes ? double(loaded_bytes)/double(total_bytes) : loading ? 0. : 1.,
            .load_chunk_size=options.load_chunk_size, .undo_depth=undo.size(), .redo_depth=redo.size()};
    }
    detail::Record memory_record(std::string text) {
        detail::Record result;
        result.id = ++record_sequence; result.units = detail::utf16_length(text); result.decoded_bytes = text.size();
        result.memory = std::make_shared<const std::string>(std::move(text)); return result;
    }
    void clear_cache() { cache.clear(); recent.clear(); statistics.cache_bytes = statistics.cached_lines = 0; }
    void clear_history() { undo.clear(); redo.clear(); statistics.history_bytes = 0; }
    void stop_load() {
        (void)scheduled.cancel(); scheduled = {}; ++load_generation;
        loading = false; loading_source.reset(); pending_line.clear(); pending_cr = false; loading_path.clear();
    }
    core::Result<void> publish(TextDocumentChange change) {
        ++revision; change.state = snapshot();if(change.path.empty())change.path=file_path;
        std::erase_if(receivers, [](const auto& receiver) { return receiver.expired(); });
        const auto targets = receivers; Flag delivery(notifying);
        bool failed=false;std::exception_ptr failure;
        for (const auto& weak : targets) {
            if (closed.load()) break;
            if (auto target = weak.lock()) {
                const auto delivered = target->send(change);
                if (!delivered && delivered.error().code != runtime::QueryErrorCode::receiver_expired) {
                    if(!failed)failure=delivered.error().cause;failed=true;
                }
            }
        }
        if(failed)return core::fail(core::ErrorCode::failed,"Committed text document change receiver failed",failure);
        return {};
    }
    core::Result<gui::TextIndex> index(const detail::Record& record) {
        if(reading)return core::fail(core::ErrorCode::busy,"Recursive text source read is not allowed");
        if (const auto found = cache.find(record.id); found != cache.end()) {
            ++statistics.cache_hits; recent.splice(recent.end(), recent, found->second.position); return found->second.value;
        }
        std::string value;
        if (record.memory) value = *record.memory;
        else {
            if (!record.source) return core::fail(core::ErrorCode::failed, "Text line has no source");
            core::Result<std::string> bytes;
            { Flag external(reading); bytes = record.source->read(record.offset, std::size_t(record.bytes)); }
            if (closed.load()) return core::fail(core::ErrorCode::closed, "Text source retired its model");
            if (!bytes) return std::unexpected(bytes.error());
            ++statistics.source_reads; statistics.source_bytes += bytes->size();
            if (bytes->size() != record.bytes || detail::fingerprint(*bytes) != record.fingerprint)
                return core::fail(core::ErrorCode::failed, "Text source changed in place or was truncated");
            auto decoded = detail::decode_line(*bytes, record.strip_bom);
            if (!decoded) return std::unexpected(decoded.error());
            if (decoded->units != record.units || decoded->text.size() != record.decoded_bytes)
                return core::fail(core::ErrorCode::failed, "Text line no longer matches its index");
            value = std::move(decoded->text);
        }
        auto result = gui::TextIndex::create(std::move(value)); if (!result) return result;
        ++statistics.decoded_lines;
        const auto required = result->storage_bytes();
        if (required <= options.decoded_cache_bytes && options.decoded_cache_entries) {
            while (!recent.empty() && (statistics.cache_bytes > options.decoded_cache_bytes - required || cache.size() >= options.decoded_cache_entries)) {
                const auto id = recent.front(); recent.pop_front();
                const auto found = cache.find(id); statistics.cache_bytes -= found->second.value.storage_bytes(); cache.erase(found);
            }
            recent.push_back(record.id);
            try { cache.emplace(record.id, Cache{*result,std::prev(recent.end())}); }
            catch (...) { recent.pop_back(); throw; }
            statistics.cache_bytes += required; statistics.cached_lines = cache.size();
        }
        return result;
    }
    core::Result<gui::TextIndex> index(std::size_t line) {
        if (line >= detail::count(root)) return core::fail(core::ErrorCode::out_of_range, "Text line index exceeds the document");
        return index(detail::at(root,line,statistics));
    }
    core::Result<std::string> range(TextPosition first, TextPosition last) {
        if (last < first) std::swap(first,last);
        first = std::min(first,characters()); last = std::min(last,characters());
        if (last - first > options.maximum_materialized_bytes) return core::fail(core::ErrorCode::out_of_range, "Text materialization exceeds its byte budget");
        const auto a = detail::locate(root,first,statistics), b = detail::locate(root,last,statistics);
        std::string result;
        for (std::size_t line = a.line; line <= b.line; ++line) {
            auto value = index(line); if (!value) return std::unexpected(value.error());
            auto begin = value->byte_for_utf16(line == a.line ? std::size_t(a.column) : 0);
            auto end = value->byte_for_utf16(line == b.line ? std::size_t(b.column) : value->utf16_size());
            if (!begin) return std::unexpected(begin.error()); if (!end) return std::unexpected(end.error());
            const auto added = *end - *begin + std::size_t(line != b.line);
            if (added > options.maximum_materialized_bytes - result.size()) return core::fail(core::ErrorCode::out_of_range, "Text materialization exceeds its byte budget");
            result.append(value->text().substr(*begin,*end-*begin)); if (line != b.line) result += '\n';
        }
        ++statistics.materializations; return result;
    }
    core::Result<TextPosition> word(TextPosition position, bool forward) {
        auto point = detail::locate(root,position,statistics);
        for (;;) {
            auto value = index(point.line); if (!value) return std::unexpected(value.error());
            auto column=std::size_t(point.column);
            if(forward){while(column<value->utf16_size()&&!value->word_at(column))column=value->next_grapheme(column);
                if(column<value->utf16_size())return detail::start(root,point.line,statistics)+value->next_word(column);}
            else{while(column&&!value->word_at(value->previous_grapheme(column)))column=value->previous_grapheme(column);
                if(column)return detail::start(root,point.line,statistics)+value->previous_word(column);}
            if (forward) { if (point.line + 1 == detail::count(root)) return characters(); ++point.line; point.column = 0; }
            else { if (!point.line) return TextPosition(0); --point.line; point.column = detail::at(root,point.line,statistics).units; }
        }
    }
    core::Result<TextDocumentCursor> position(std::size_t line, TextPosition column) {
        line = std::min(line,detail::count(root)-1);
        const auto record = detail::at(root,line,statistics); column = std::min(column,record.units);
        // A line edge and every ASCII column are scalar boundaries already
        // proven by the retained record. Metadata-only navigation need not
        // decode a large source paragraph to validate those positions again.
        if(column&&column!=record.units&&record.decoded_bytes!=record.units){
            auto value = index(record); if (!value) return std::unexpected(value.error());
            if (auto byte = value->byte_for_utf16(std::size_t(column)); !byte) return std::unexpected(byte.error());
        }
        return TextDocumentCursor{line,column,detail::start(root,line,statistics)+column};
    }
    core::Result<void> schedule(std::uint64_t generation) {
        auto submitted = lifetime.post(loop,[weak=weak_from_this(),generation] {
            auto p = weak.lock(); if (!p || p->closed.load()) return;
            if (p->owner != std::this_thread::get_id()) { p->wrong_dispatch.store(true); p->closed.store(true); p->lifetime.request_close(); return; }
            if (!p->loading || p->load_generation != generation) return;
            auto result = protect<void>([&] { return p->load_step(generation); });
            if (!result && !p->closed.load() && p->load_generation == generation) {
                auto path=p->loading_path;p->stop_load(); p->last_error = result.error().message;
                (void)p->publish({.kind=TextDocumentEventKind::load_failed,.path=std::move(path)});
            }
        });
        if (!submitted) return core::fail(submitted.error == runtime::SubmitError::full || submitted.error == runtime::SubmitError::overloaded ? core::ErrorCode::full : core::ErrorCode::closed, "Core rejected the text loading callback");
        scheduled = submitted.token; return {};
    }
    core::Result<void> load_step(std::uint64_t generation) {
        if (auto r = ready(true); !r) return r;
        Flag guard(committing);
        const auto remaining = total_bytes - loaded_bytes;
        const auto amount = std::size_t(std::min<std::uint64_t>(remaining,options.load_chunk_size));
        core::Result<std::string> chunk;
        const auto source=loading_source;
        { Flag external(reading); chunk = source->read(loaded_bytes,amount); }
        if (closed.load()) return core::fail(core::ErrorCode::closed, "Text source retired its model");
        if (!chunk) return std::unexpected(chunk.error());
        ++statistics.source_reads; statistics.source_bytes += chunk->size();
        if (chunk->size() != amount) return core::fail(core::ErrorCode::failed, "Text source ended before its declared byte size");
        auto next = root;
        auto pending = std::move(pending_line);
        auto line_start = pending_offset;
        auto carriage = pending_cr;
        const auto had_lines = finalized;
        bool has_lines = had_lines;
        std::size_t inserted = 0;
        const auto previous_count = detail::count(root);
        auto append = [&]() -> core::Result<void> {
            auto decoded = detail::decode_line(pending,line_start==0); if (!decoded) return std::unexpected(decoded.error());
            detail::Record record;
            record.id=++record_sequence; record.offset=line_start; record.bytes=pending.size(); record.decoded_bytes=decoded->text.size();
            record.units=decoded->units; record.strip_bom=line_start==0; record.fingerprint=detail::fingerprint(pending); record.source=loading_source;
            auto node=std::make_shared<detail::LineNode>(std::move(record));
            if (!has_lines) { next=std::move(node); has_lines=true; } else next=detail::merge(std::move(next),std::move(node),statistics);
            ++inserted;
            if (detail::count(next)>options.maximum_lines || detail::bytes(next)-1>options.maximum_file_bytes) return core::fail(core::ErrorCode::out_of_range,"Loaded text exceeds its line or byte budget");
            pending.clear(); return {};
        };
        std::size_t at=0;
        while(at<chunk->size()) {
            if(carriage){carriage=false;if((*chunk)[at]=='\n'){line_start=loaded_bytes+at+1;++at;continue;}}
            const auto end=chunk->find_first_of("\r\n",at);
            const auto stop=end==std::string::npos?chunk->size():end;
            if(stop-at>options.maximum_line_bytes-pending.size())return core::fail(core::ErrorCode::out_of_range,"Loaded line exceeds its byte budget");
            pending.append(*chunk,at,stop-at);at=stop;
            if(at==chunk->size())break;
            if(auto r=append();!r)return r;
            carriage=(*chunk)[at]=='\r';line_start=loaded_bytes+at+1;++at;
        }
        const auto read=loaded_bytes+chunk->size();
        const bool finished=read==total_bytes;
        if(finished){if(auto r=append();!r)return r;carriage=false;}
        root=std::move(next);pending_line=std::move(pending);pending_offset=line_start;pending_cr=carriage;finalized=has_lines;loaded_bytes=read;
        if(inserted){++document_revision;cursor=detail::locate(root,cursor.position,statistics);}
        if(finished){loading=false;loading_source.reset();pending_line.clear();scheduled={};clean_stamp=stamp;last_error.clear();}
        const auto path=loading_path;
        if(auto r=publish({.kind=TextDocumentEventKind::load_progress,.path=path,.first_line=had_lines?previous_count:0,
            .removed_lines=inserted&&!had_lines?1u:0u,.inserted_lines=inserted,
            .content=inserted!=0,.storage=inserted!=0});!r)return r;
        if(closed.load())return {};
        if(finished)return publish({.kind=TextDocumentEventKind::loaded,.path=path});
        if(load_generation!=generation||!loading)return {};
        return schedule(generation);
    }
    core::Result<detail::Tree> memory_tree(std::string_view text) {
        detail::Tree result;
        std::size_t begin=0,lines=0;
        do {
            const auto newline=text.find('\n',begin);const auto end=newline==text.npos?text.size():newline;
            if(end-begin>options.maximum_line_bytes)return core::fail(core::ErrorCode::out_of_range,"Edited line exceeds its byte budget");
            if(++lines>options.maximum_lines)return core::fail(core::ErrorCode::out_of_range,"Edited text exceeds its line budget");
            result=detail::merge(std::move(result),std::make_shared<detail::LineNode>(memory_record(std::string(text.substr(begin,end-begin)))),statistics);
            if(newline==text.npos)break;begin=newline+1;
        }while(true);
        return result;
    }
    void trim_history() {
        while (!undo.empty() && (undo.size()+redo.size()>options.undo_entries || statistics.history_bytes>options.undo_bytes)) { statistics.history_bytes-=undo.front().cost;undo.pop_front(); }
        while (!redo.empty() && (undo.size()+redo.size()>options.undo_entries || statistics.history_bytes>options.undo_bytes)) { statistics.history_bytes-=redo.front().cost;redo.pop_front(); }
    }
    core::Result<bool> replace(TextPosition first,TextPosition last,std::string_view text) {
        if(text.size()>options.maximum_materialized_bytes||!core::valid_utf8(text))return core::fail(core::ErrorCode::invalid_argument,"Edited text must be bounded UTF-8");
        auto normalized=detail::newlines(text);if(last<first)std::swap(first,last);first=std::min(first,characters());last=std::min(last,characters());
        if(first==last&&normalized.empty())return false;
        const auto a=detail::locate(root,first,statistics),b=detail::locate(root,last,statistics);
        auto left=index(a.line);if(!left)return std::unexpected(left.error());auto right=a.line==b.line?left:index(b.line);if(!right)return std::unexpected(right.error());
        auto begin=left->byte_for_utf16(std::size_t(a.column)),end=right->byte_for_utf16(std::size_t(b.column));
        if(!begin)return std::unexpected(begin.error());if(!end)return std::unexpected(end.error());
        if(a.line==b.line&&left->text().substr(*begin,*end-*begin)==normalized)return false;
        const auto prefix=left->text().substr(0,*begin),suffix=right->text().substr(*end);
        const auto previous_clones=statistics.cloned_index_nodes;
        const auto inserted_lines=std::size_t(std::count(normalized.begin(),normalized.end(),'\n'))+1;
        const auto removed_lines=b.line-a.line+1;
        if(inserted_lines>options.maximum_lines-(detail::count(root)-removed_lines))return core::fail(core::ErrorCode::out_of_range,"Edit exceeds the line budget");
        detail::Tree inserted;std::size_t offset=0,row=0;TextPosition final_column=0;
        for(;;){const auto delimiter=normalized.find('\n',offset);const auto limit=delimiter==normalized.npos?normalized.size():delimiter;
            const auto head=row==0?prefix:std::string_view{},tail=delimiter==normalized.npos?suffix:std::string_view{};
            if(head.size()+limit-offset+tail.size()>options.maximum_line_bytes)return core::fail(core::ErrorCode::out_of_range,"Edit exceeds the line byte budget");
            std::string value;value.reserve(head.size()+limit-offset+tail.size());value+=head;value.append(normalized,offset,limit-offset);
            final_column=detail::utf16_length(value);value+=tail;
            inserted=detail::merge(std::move(inserted),std::make_shared<detail::LineNode>(memory_record(std::move(value))),statistics);
            ++row;if(delimiter==normalized.npos)break;offset=delimiter+1;
        }
        auto prefix_tree=detail::split(root,a.line,statistics);auto suffix_tree=detail::split(std::move(prefix_tree.second),removed_lines,statistics);
        auto next=detail::merge(detail::merge(std::move(prefix_tree.first),std::move(inserted),statistics),std::move(suffix_tree.second),statistics);
        const auto cost=std::min<std::uint64_t>(SIZE_MAX,(suffix_tree.first?suffix_tree.first->memory_bytes:0)+
            (std::uint64_t(removed_lines)+statistics.cloned_index_nodes-previous_clones)*sizeof(detail::LineNode)+normalized.size()+sizeof(History));
        if(detail::bytes(next)-1>options.maximum_file_bytes)return core::fail(core::ErrorCode::out_of_range,"Edit exceeds the document byte budget");
        const auto next_cursor=TextDocumentCursor{a.line+inserted_lines-1,final_column,detail::start(next,a.line+inserted_lines-1,statistics)+final_column};
        const auto next_version=next_stamp+1;
        History entry{root,next,cursor,next_cursor,stamp,next_version,std::size_t(cost),first,last-first,detail::utf16_length(normalized)};
        // Allocate the history entry before changing document data.
        const bool retain=options.undo_entries&&entry.cost<=options.undo_bytes;
        if(retain)undo.push_back(entry);
        for(const auto& old:redo)statistics.history_bytes-=old.cost;redo.clear();
        if(retain)statistics.history_bytes+=entry.cost;
        else{undo.clear();statistics.history_bytes=0;}
        trim_history();stop_load();root=std::move(next);cursor=next_cursor;stamp=next_stamp=next_version;++document_revision;last_error.clear();
        if(auto r=publish({.first_line=a.line,.removed_lines=removed_lines,.inserted_lines=inserted_lines,.content=true,.cursor=true,.storage=true,.edited=true,
            .edit=TextDocumentEdit{first,entry.removed,entry.inserted,next_version,TextEditOperation::replace,retain}});!r)return std::unexpected(r.error());
        return true;
    }
    core::Result<bool> set_cursor(TextDocumentCursor point) {
        if(point==cursor)return false;
        cursor=point;if(auto r=publish({.cursor=true});!r)return std::unexpected(r.error());return true;
    }
    core::Result<bool> file_failure(TextDocumentEventKind kind,core::Error error,std::filesystem::path path={}) {
        if(closed.load())return core::fail(core::ErrorCode::closed,"Text storage retired its model");
        last_error=error.message;
        if(auto r=publish({.kind=kind,.path=std::move(path)});!r)return std::unexpected(r.error());
        return std::unexpected(std::move(error));
    }
};

TextDocumentModel::TextDocumentModel(std::shared_ptr<Impl> state):impl_(std::move(state)){}
core::Result<std::shared_ptr<TextDocumentModel>> TextDocumentModel::create(runtime::EventLoop& loop,TextDocumentOptions options,std::shared_ptr<TextDocumentStorage> storage){
    return protect<std::shared_ptr<TextDocumentModel>>([&]()->core::Result<std::shared_ptr<TextDocumentModel>>{
        if(auto r=valid_options(options);!r)return std::unexpected(r.error());
        if(!storage)storage=file_provider_text_storage();
        return std::shared_ptr<TextDocumentModel>(new TextDocumentModel(std::make_shared<Impl>(loop.sender(),options,std::move(storage))));
    });
}
TextDocumentModel::~TextDocumentModel(){auto p=std::move(impl_);p->closed.store(true);p->lifetime.request_close();(void)p->lifetime.close();}
core::Result<TextDocumentState> TextDocumentModel::state()const{
    return protect<TextDocumentState>([p=impl_]()->core::Result<TextDocumentState>{if(auto r=p->ready();!r)return std::unexpected(r.error());return p->snapshot();});
}
core::Result<TextDocumentStats> TextDocumentModel::stats()const{
    auto p=impl_;if(auto r=p->ready();!r)return std::unexpected(r.error());return p->statistics;
}
core::Result<void> TextDocumentModel::observe(std::shared_ptr<const TextDocumentPort> port){
    return protect<void>([p=impl_,port=std::move(port)]()->core::Result<void>{
        if(auto r=p->ready(true);!r)return r;if(!port)return core::fail(core::ErrorCode::invalid_argument,"Text observer is empty");
        std::erase_if(p->receivers,[](const auto& weak){return weak.expired();});
        for(const auto& weak:p->receivers)if(weak.lock()==port)return {};
        if(p->receivers.size()>=4096)return core::fail(core::ErrorCode::full,"Text observer budget exhausted");
        p->receivers.emplace_back(std::move(port));return {};
    });
}
core::Result<bool> TextDocumentModel::set_file_path(std::filesystem::path value){
    return protect<bool>([p=impl_,value=std::move(value)]()->core::Result<bool>{
        if(auto r=p->ready(true);!r)return std::unexpected(r.error());auto path=detail::path(value,true);if(!path)return std::unexpected(path.error());
        if(*path==p->file_path)return false;Flag guard(p->committing);p->file_path=std::move(*path);
        if(auto r=p->publish({});!r)return std::unexpected(r.error());return true;
    });
}
core::Result<bool> TextDocumentModel::set_load_chunk_size(std::size_t size){
    return protect<bool>([p=impl_,size]()->core::Result<bool>{
        if(auto r=p->ready(true);!r)return std::unexpected(r.error());
        if(!size||size>16*1024*1024)return core::fail(core::ErrorCode::out_of_range,"Text loading chunk is outside its byte budget");
        if(size==p->options.load_chunk_size)return false;Flag guard(p->committing);p->options.load_chunk_size=size;
        if(auto r=p->publish({});!r)return std::unexpected(r.error());return true;
    });
}
core::Result<std::string> TextDocumentModel::text()const{
    return protect<std::string>([p=impl_]()->core::Result<std::string>{if(auto r=p->ready();!r)return std::unexpected(r.error());return p->range(0,p->characters());});
}
core::Result<std::string> TextDocumentModel::text_range(TextPosition first,TextPosition last)const{
    return protect<std::string>([p=impl_,first,last]()->core::Result<std::string>{if(auto r=p->ready();!r)return std::unexpected(r.error());return p->range(first,last);});
}
core::Result<TextDocumentLine> TextDocumentModel::line_at(std::size_t line)const{
    return protect<TextDocumentLine>([p=impl_,line]()->core::Result<TextDocumentLine>{
        if(auto r=p->ready();!r)return std::unexpected(r.error());if(line>=detail::count(p->root))return core::fail(core::ErrorCode::out_of_range,"Text line exceeds the document");
        const auto record=detail::at(p->root,line,p->statistics);auto index=p->index(record);if(!index)return std::unexpected(index.error());
        return TextDocumentLine{line,line+1,record.units,std::string(index->text()),bool(record.source)};
    });
}
core::Result<std::string> TextDocumentModel::line_text(std::size_t line)const{
    return protect<std::string>([p=impl_,line]()->core::Result<std::string>{if(auto r=p->ready();!r)return std::unexpected(r.error());auto index=p->index(line);if(!index)return std::unexpected(index.error());return std::string(index->text());});
}
core::Result<TextDocumentLineInfo> TextDocumentModel::line_info(std::size_t line)const{
    return protect<TextDocumentLineInfo>([p=impl_,line]()->core::Result<TextDocumentLineInfo>{
        if(auto r=p->ready();!r)return std::unexpected(r.error());if(line>=detail::count(p->root))return core::fail(core::ErrorCode::out_of_range,"Text line exceeds the document");
        auto r=detail::at(p->root,line,p->statistics);return TextDocumentLineInfo{line,r.id,r.decoded_bytes,detail::start(p->root,line,p->statistics),r.units,bool(r.source)};
    });
}
core::Result<gui::TextIndex> TextDocumentModel::line_index(std::size_t line)const{
    return protect<gui::TextIndex>([p=impl_,line]()->core::Result<gui::TextIndex>{if(auto r=p->ready();!r)return std::unexpected(r.error());return p->index(line);});
}
core::Result<TextPosition> TextDocumentModel::line_length(std::size_t line)const{
    auto p=impl_;if(auto r=p->ready();!r)return std::unexpected(r.error());if(line>=detail::count(p->root))return core::fail(core::ErrorCode::out_of_range,"Text line exceeds the document");return detail::at(p->root,line,p->statistics).units;
}
core::Result<TextPosition> TextDocumentModel::position_for_line_column(std::size_t line,TextPosition column)const{
    return protect<TextPosition>([p=impl_,line,column]()->core::Result<TextPosition>{if(auto r=p->ready();!r)return std::unexpected(r.error());auto point=p->position(line,column);if(!point)return std::unexpected(point.error());return point->position;});
}
core::Result<TextDocumentCursor> TextDocumentModel::line_column_for_position(TextPosition position)const{
    return protect<TextDocumentCursor>([p=impl_,position]()->core::Result<TextDocumentCursor>{if(auto r=p->ready();!r)return std::unexpected(r.error());auto point=detail::locate(p->root,position,p->statistics);return p->position(point.line,point.column);});
}
core::Result<TextPosition> TextDocumentModel::previous_word_boundary_position(TextPosition position)const{
    return protect<TextPosition>([p=impl_,position]()->core::Result<TextPosition>{if(auto r=p->ready();!r)return std::unexpected(r.error());return p->word(position,false);});
}
core::Result<TextPosition> TextDocumentModel::next_word_boundary_position(TextPosition position)const{
    return protect<TextPosition>([p=impl_,position]()->core::Result<TextPosition>{if(auto r=p->ready();!r)return std::unexpected(r.error());return p->word(position,true);});
}
core::Result<bool> TextDocumentModel::move_cursor(std::size_t line,TextPosition column){
    return protect<bool>([p=impl_,line,column]()->core::Result<bool>{if(auto r=p->ready(true);!r)return std::unexpected(r.error());Flag guard(p->committing);auto point=p->position(line,column);if(!point)return std::unexpected(point.error());return p->set_cursor(*point);});
}
core::Result<bool> TextDocumentModel::set_cursor_position(TextPosition position){
    return protect<bool>([p=impl_,position]()->core::Result<bool>{if(auto r=p->ready(true);!r)return std::unexpected(r.error());Flag guard(p->committing);auto raw=detail::locate(p->root,position,p->statistics);auto point=p->position(raw.line,raw.column);if(!point)return std::unexpected(point.error());return p->set_cursor(*point);});
}
core::Result<bool> TextDocumentModel::move_cursor(TextCursorMove direction){
    return protect<bool>([p=impl_,direction]()->core::Result<bool>{
        if(auto r=p->ready(true);!r)return std::unexpected(r.error());if(unsigned(direction)>unsigned(TextCursorMove::word_right))return core::fail(core::ErrorCode::invalid_argument,"Unknown text cursor movement");
        Flag guard(p->committing);auto point=p->cursor;
        if(direction==TextCursorMove::document_start)point={};
        else if(direction==TextCursorMove::document_end)point=detail::locate(p->root,p->characters(),p->statistics);
        else if(direction==TextCursorMove::up){if(point.line)--point.line;}
        else if(direction==TextCursorMove::down){if(point.line+1<detail::count(p->root))++point.line;}
        else if(direction==TextCursorMove::line_start)point.column=0;
        else if(direction==TextCursorMove::line_end)point.column=detail::at(p->root,point.line,p->statistics).units;
        else if(direction==TextCursorMove::word_left||direction==TextCursorMove::word_right){auto next=p->word(point.position,direction==TextCursorMove::word_right);if(!next)return std::unexpected(next.error());point=detail::locate(p->root,*next,p->statistics);}
        else {auto index=p->index(point.line);if(!index)return std::unexpected(index.error());
            if(direction==TextCursorMove::left){if(point.column)point.column=index->previous_grapheme(std::size_t(point.column));else if(point.line){--point.line;point.column=detail::at(p->root,point.line,p->statistics).units;}}
            else {if(point.column<index->utf16_size())point.column=index->next_grapheme(std::size_t(point.column));else if(point.line+1<detail::count(p->root)){++point.line;point.column=0;}}
        }
        if(direction==TextCursorMove::up||direction==TextCursorMove::down){auto index=p->index(point.line);if(!index)return std::unexpected(index.error());point.column=std::min<TextPosition>(point.column,index->utf16_size());if(!index->byte_for_utf16(std::size_t(point.column)))point.column=index->previous_grapheme(std::size_t(point.column));}
        auto valid=p->position(point.line,point.column);if(!valid)return std::unexpected(valid.error());return p->set_cursor(*valid);
    });
}
core::Result<bool> TextDocumentModel::set_text(std::string_view text){
    return protect<bool>([p=impl_,text]()->core::Result<bool>{
        if(auto r=p->ready(true);!r)return std::unexpected(r.error());
        if(text.size()>p->options.maximum_materialized_bytes||!core::valid_utf8(text))return core::fail(core::ErrorCode::invalid_argument,"Document text must be bounded UTF-8");
        Flag guard(p->committing);auto normalized=detail::newlines(text);
        if(normalized.size()>p->options.maximum_file_bytes)return core::fail(core::ErrorCode::out_of_range,"Document text exceeds its byte budget");
        bool equal=detail::bytes(p->root)-1==normalized.size();std::size_t offset=0;
        if(equal){auto compared=detail::all(p->root,[&](const detail::Record& record)->core::Result<bool>{auto value=p->index(record);if(!value)return std::unexpected(value.error());
                if(value->text()!=std::string_view(normalized).substr(offset,value->text().size()))return false;offset+=value->text().size();
                if(offset<normalized.size()){if(normalized[offset]!='\n')return false;++offset;}return true;});
            if(!compared)return std::unexpected(compared.error());equal=*compared;
        }
        if(equal&&!p->loading)return false;
        auto tree=p->memory_tree(normalized);if(!tree)return std::unexpected(tree.error());if(p->closed.load())return core::fail(core::ErrorCode::closed,"Text read retired its model");
        const auto removed=detail::count(p->root);p->stop_load();p->root=std::move(*tree);p->cursor={};p->stamp=++p->next_stamp;++p->document_revision;p->last_error.clear();p->clear_cache();p->clear_history();
        if(auto r=p->publish({.removed_lines=removed,.inserted_lines=detail::count(p->root),.content=true,.cursor=true,.storage=true,.reset=true,.history=true,.edited=true});!r)return std::unexpected(r.error());return true;
    });
}
core::Result<bool> TextDocumentModel::replace_range(TextPosition first,TextPosition last,std::string_view text){
    return protect<bool>([p=impl_,first,last,text]()->core::Result<bool>{if(auto r=p->ready(true);!r)return std::unexpected(r.error());Flag guard(p->committing);return p->replace(first,last,text);});
}
core::Result<bool> TextDocumentModel::insert_text(std::string_view text){
    return protect<bool>([p=impl_,text]()->core::Result<bool>{if(auto r=p->ready(true);!r)return std::unexpected(r.error());Flag guard(p->committing);return p->replace(p->cursor.position,p->cursor.position,text);});
}
core::Result<bool> TextDocumentModel::insert_newline(){return insert_text("\n");}
core::Result<bool> TextDocumentModel::remove_previous_character(){
    return protect<bool>([p=impl_]()->core::Result<bool>{if(auto r=p->ready(true);!r)return std::unexpected(r.error());if(!p->cursor.position)return false;Flag guard(p->committing);
        auto start=p->cursor.position-1;if(p->cursor.column){auto value=p->index(p->cursor.line);if(!value)return std::unexpected(value.error());start=detail::start(p->root,p->cursor.line,p->statistics)+value->previous_grapheme(std::size_t(p->cursor.column));}
        return p->replace(start,p->cursor.position,{});
    });
}
core::Result<bool> TextDocumentModel::remove_next_character(){
    return protect<bool>([p=impl_]()->core::Result<bool>{if(auto r=p->ready(true);!r)return std::unexpected(r.error());if(p->cursor.position==p->characters())return false;Flag guard(p->committing);
        auto end=p->cursor.position+1;if(p->cursor.column<detail::at(p->root,p->cursor.line,p->statistics).units){auto value=p->index(p->cursor.line);if(!value)return std::unexpected(value.error());end=detail::start(p->root,p->cursor.line,p->statistics)+value->next_grapheme(std::size_t(p->cursor.column));}
        return p->replace(p->cursor.position,end,{});
    });
}
core::Result<bool> TextDocumentModel::load_file(std::filesystem::path value){
    return protect<bool>([p=impl_,value=std::move(value)]()->core::Result<bool>{
        if(auto r=p->ready(true);!r)return std::unexpected(r.error());Flag guard(p->committing);
        auto path=detail::path(value.empty()?p->file_path:value);if(!path)return std::unexpected(path.error());
        core::Result<std::shared_ptr<const TextDocumentSource>> source;
        {Flag external(p->reading);source=p->storage->open_read(*path);}
        if(p->closed.load())return core::fail(core::ErrorCode::closed,"Text storage retired its model");
        const bool missing=!source&&source.error().code==core::ErrorCode::not_found;
        if(!source&&!missing)return p->file_failure(TextDocumentEventKind::load_failed,source.error(),*path);
        if(source&&!*source)return p->file_failure(TextDocumentEventKind::load_failed,{core::ErrorCode::failed,"Text storage returned an empty source"},*path);
        const auto size=missing?0:(*source)->byte_size();if(size>p->options.maximum_file_bytes)return p->file_failure(TextDocumentEventKind::load_failed,{core::ErrorCode::out_of_range,"Text source exceeds its byte budget"},*path);
        auto empty=std::make_shared<detail::LineNode>(p->memory_record({}));
        const auto old_token=p->scheduled;const auto next_generation=p->load_generation+1;
        if(!missing){if(auto r=p->schedule(next_generation);!r)return std::unexpected(r.error());}
        else p->scheduled={};
        if(auto r=p->ready();!r){(void)p->scheduled.cancel();return std::unexpected(r.error());}
        (void)old_token.cancel();const auto removed=detail::count(p->root);
        p->load_generation=next_generation;p->loading=!missing;p->loading_source=missing?nullptr:*source;
        p->root=std::move(empty);p->file_path=*path;p->loading_path=std::move(*path);p->cursor={};p->pending_line.clear();p->pending_offset=0;p->pending_cr=false;p->finalized=false;
        p->loaded_bytes=0;p->total_bytes=size;p->last_error.clear();p->stamp=p->clean_stamp=++p->next_stamp;++p->document_revision;p->clear_cache();p->clear_history();
        if(auto r=p->publish({.removed_lines=removed,.inserted_lines=1,.content=true,.cursor=true,.storage=true,.reset=true,.history=true});!r){p->stop_load();p->last_error=r.error().message;return std::unexpected(r.error());}
        if(missing&&!p->closed.load()){if(auto r=p->publish({.kind=TextDocumentEventKind::loaded});!r)return std::unexpected(r.error());}
        return true;
    });
}
core::Result<bool> TextDocumentModel::reload_file(){return load_file();}
core::Result<bool> TextDocumentModel::cancel_load(){
    return protect<bool>([p=impl_]()->core::Result<bool>{if(auto r=p->ready(true);!r)return std::unexpected(r.error());if(!p->loading)return false;Flag guard(p->committing);auto path=p->loading_path;p->stop_load();if(auto r=p->publish({.kind=TextDocumentEventKind::load_cancelled,.path=std::move(path)});!r)return std::unexpected(r.error());return true;});
}
core::Result<bool> TextDocumentModel::save_file(std::filesystem::path value){
    return protect<bool>([p=impl_,value=std::move(value)]()->core::Result<bool>{
        if(auto r=p->ready(true);!r)return std::unexpected(r.error());if(p->loading)return core::fail(core::ErrorCode::busy,"Cannot save a partially loaded text document");
        auto path=detail::path(value.empty()?p->file_path:value);if(!path)return std::unexpected(path.error());Flag guard(p->committing);
        core::Result<std::unique_ptr<TextDocumentWrite>> opened;
        {Flag external(p->reading);opened=p->storage->begin_write(*path);}
        if(p->closed.load())return core::fail(core::ErrorCode::closed,"Text storage retired its model");
        if(!opened)return p->file_failure(TextDocumentEventKind::save_failed,opened.error(),*path);
        auto writer=std::move(*opened);if(!writer)return p->file_failure(TextDocumentEventKind::save_failed,{core::ErrorCode::failed,"Text storage returned an empty writer"},*path);
        auto source=std::make_shared<SavedSource>();detail::Tree next;std::uint64_t written=0;bool first=true;
        auto streamed=detail::visit(p->root,[&](const detail::Record& record)->core::Result<void>{
            auto index=p->index(record);if(!index)return std::unexpected(index.error());
            if(!first){auto result=writer->write("\n");if(!result)return result;++written;}first=false;
            if(p->closed.load())return core::fail(core::ErrorCode::closed,"Text writer retired its model");
            auto result=writer->write(index->text());if(!result)return result;
            if(p->closed.load())return core::fail(core::ErrorCode::closed,"Text writer retired its model");
            detail::Record saved;saved.id=record.id;saved.offset=written;saved.bytes=saved.decoded_bytes=index->text().size();saved.units=record.units;saved.fingerprint=detail::fingerprint(index->text());saved.source=source;
            next=detail::merge(std::move(next),std::make_shared<detail::LineNode>(std::move(saved)),p->statistics);written+=index->text().size();return {};
        });
        if(!streamed)return p->file_failure(TextDocumentEventKind::save_failed,streamed.error(),*path);
        auto committed=writer->commit();if(!committed)return p->file_failure(TextDocumentEventKind::save_failed,committed.error(),*path);
        if(p->closed.load())return core::fail(core::ErrorCode::closed,"Text publication retired its model");
        if(!*committed||(*committed)->byte_size()!=written)return p->file_failure(TextDocumentEventKind::save_failed,{core::ErrorCode::failed,"Text publication returned another byte range"},*path);
        source->source=std::move(*committed);p->root=std::move(next);p->file_path=std::move(*path);p->clean_stamp=p->stamp;p->loaded_bytes=p->total_bytes=written;p->last_error.clear();
        if(auto r=p->publish({.kind=TextDocumentEventKind::saved,.storage=true});!r)return std::unexpected(r.error());return true;
    });
}
core::Result<bool> TextDocumentModel::mark_clean(){
    return protect<bool>([p=impl_]()->core::Result<bool>{if(auto r=p->ready(true);!r)return std::unexpected(r.error());if(p->stamp==p->clean_stamp)return false;Flag guard(p->committing);p->clean_stamp=p->stamp;if(auto r=p->publish({});!r)return std::unexpected(r.error());return true;});
}
core::Result<bool> TextDocumentModel::clear(){return set_text({});}
core::Result<bool> TextDocumentModel::undo(){
    return protect<bool>([p=impl_]()->core::Result<bool>{if(auto r=p->ready(true);!r)return std::unexpected(r.error());if(p->undo.empty())return false;Flag guard(p->committing);
        auto entry=p->undo.back();p->redo.push_back(entry);p->undo.pop_back();p->stop_load();const auto removed=detail::count(p->root);p->root=std::move(entry.before);p->cursor=entry.cursor_before;p->stamp=entry.stamp_before;++p->document_revision;p->last_error.clear();
        if(auto r=p->publish({.removed_lines=removed,.inserted_lines=detail::count(p->root),.content=true,.cursor=true,.storage=true,.history=true,.edited=true,
            .edit=TextDocumentEdit{entry.first,entry.inserted,entry.removed,entry.stamp_after,TextEditOperation::undo,true}});!r)return std::unexpected(r.error());
        return true;
    });
}
core::Result<bool> TextDocumentModel::redo(){
    return protect<bool>([p=impl_]()->core::Result<bool>{if(auto r=p->ready(true);!r)return std::unexpected(r.error());if(p->redo.empty())return false;Flag guard(p->committing);
        auto entry=p->redo.back();p->undo.push_back(entry);p->redo.pop_back();p->stop_load();const auto removed=detail::count(p->root);p->root=std::move(entry.after);p->cursor=entry.cursor_after;p->stamp=entry.stamp_after;++p->document_revision;p->last_error.clear();
        if(auto r=p->publish({.removed_lines=removed,.inserted_lines=detail::count(p->root),.content=true,.cursor=true,.storage=true,.history=true,.edited=true,
            .edit=TextDocumentEdit{entry.first,entry.removed,entry.inserted,entry.stamp_after,TextEditOperation::redo,true}});!r)return std::unexpected(r.error());
        return true;
    });
}
core::Result<bool> TextDocumentModel::clear_history(){
    return protect<bool>([p=impl_]()->core::Result<bool>{if(auto r=p->ready(true);!r)return std::unexpected(r.error());if(p->undo.empty()&&p->redo.empty())return false;Flag guard(p->committing);p->clear_history();if(auto r=p->publish({.history=true});!r)return std::unexpected(r.error());return true;});
}
core::Result<bool> TextDocumentModel::close(){
    auto p=impl_;if(p->closed.load())return false;if(auto r=p->ready();!r)return std::unexpected(r.error());p->stop_load();p->closed.store(true);p->lifetime.request_close();return true;
}
} // namespace lvrs
