#include <LVRS/TextDocumentModel.h>
#include <iiFileProviderNative.h>
#include <algorithm>
#include <functional>
#include <iostream>
#include <map>
#include <random>
#include <stdexcept>
#include <thread>
using namespace ii;
namespace {
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
template<class T>T take(core::Result<T> result){if(!result)throw std::runtime_error(result.error().message);return std::move(*result);}
void good(core::Result<void> result){if(!result)throw std::runtime_error(result.error().message);}
struct Counters final {std::size_t reads=0,bytes=0,largest_read=0,writes=0;bool fail_read=false,fail_write=false,fail_commit=false,corrupt_read=false;std::function<void()> on_read;};
class Source final:public lvrs::TextDocumentSource{
    std::shared_ptr<const std::string> bytes_;
    std::shared_ptr<Counters> counters_;
public:
    Source(std::string value,std::shared_ptr<Counters> counters):bytes_(std::make_shared<const std::string>(std::move(value))),counters_(std::move(counters)){}
    std::uint64_t byte_size()const noexcept override{return bytes_->size();}
    core::Result<std::string> read(std::uint64_t offset,std::size_t amount)const override{
        ++counters_->reads;counters_->largest_read=std::max(counters_->largest_read,amount);
        if(auto callback=std::exchange(counters_->on_read,{}))callback();
        if(counters_->fail_read)return core::fail(core::ErrorCode::failed,"Injected source error");
        if(offset>bytes_->size())return core::fail(core::ErrorCode::out_of_range,"Invalid source offset");
        auto result=bytes_->substr(std::size_t(offset),amount);if(counters_->corrupt_read&&!result.empty())result[0]^=1;counters_->bytes+=result.size();return result;
    }
};
class MemoryStorage final:public lvrs::TextDocumentStorage,public std::enable_shared_from_this<MemoryStorage>{
public:
    std::map<std::filesystem::path,std::string> files;
    std::shared_ptr<Counters> counters=std::make_shared<Counters>();
    void put(const std::string& name,std::string bytes){files[std::filesystem::absolute(name).lexically_normal()]=std::move(bytes);}
    std::string get(const std::string& name)const{return files.at(std::filesystem::absolute(name).lexically_normal());}
    core::Result<std::shared_ptr<const lvrs::TextDocumentSource>> open_read(const std::filesystem::path& path)override{
        const auto found=files.find(path);if(found==files.end())return core::fail(core::ErrorCode::not_found,"Missing memory file");
        return std::shared_ptr<const lvrs::TextDocumentSource>(std::make_shared<Source>(found->second,counters));
    }
    core::Result<std::unique_ptr<lvrs::TextDocumentWrite>> begin_write(const std::filesystem::path&)override;
};
class Write final:public lvrs::TextDocumentWrite{
    std::shared_ptr<MemoryStorage> owner_;
    std::filesystem::path path_;
    std::string value_;
public:
    Write(std::shared_ptr<MemoryStorage> storage,std::filesystem::path path):owner_(std::move(storage)),path_(std::move(path)){}
    core::Result<void> write(std::string_view value)override{++owner_->counters->writes;if(owner_->counters->fail_write)return core::fail(core::ErrorCode::failed,"Injected write error");value_+=value;return {};}
    core::Result<std::shared_ptr<const lvrs::TextDocumentSource>> commit()override{
        if(owner_->counters->fail_commit)return core::fail(core::ErrorCode::failed,"Injected publication error");
        auto source=std::make_shared<Source>(value_,owner_->counters);owner_->files[path_]=value_;
        return std::shared_ptr<const lvrs::TextDocumentSource>(std::move(source));
    }
};
core::Result<std::unique_ptr<lvrs::TextDocumentWrite>> MemoryStorage::begin_write(const std::filesystem::path& path){return std::unique_ptr<lvrs::TextDocumentWrite>(std::make_unique<Write>(shared_from_this(),path));}
void drain(runtime::EventLoop& loop,const std::shared_ptr<lvrs::TextDocumentModel>& model){
    for(unsigned i=0;take(model->state()).loading&&i<100000;++i)(void)loop.poll({.max_callbacks=1,.max_time=std::chrono::milliseconds(100)});
    check(!take(model->state()).loading,"Text loading did not retire its Core callbacks");
    check(take(model->state()).last_error.empty(),"Text loading recorded an unexpected error");
}
void run(const std::string& name,const std::filesystem::path& build){
    runtime::EventLoop loop;
    auto storage=std::make_shared<MemoryStorage>();lvrs::TextDocumentOptions options;
    if(name=="chunked"||name=="newline_chunks"||name=="cancel"||name=="edit_cancel")options.load_chunk_size=3;
    if(name=="newline_chunks")options.load_chunk_size=4;
    if(name=="utf8_chunks")options.load_chunk_size=1;
    if(name=="long_line")options.load_chunk_size=2;
    if(name=="materialization_limit")options.maximum_materialized_bytes=4;
    if(name=="cache"){options.decoded_cache_entries=2;options.decoded_cache_bytes=2048;}
    if(name=="limits"){options.maximum_lines=2;options.maximum_line_bytes=3;options.maximum_materialized_bytes=16;}
    if(name=="history_budget")options.undo_entries=2;
    auto model=take(lvrs::TextDocumentModel::create(loop,options,storage));
    if(name=="defaults"){auto state=take(model->state());check(state.line_count==1&&state.character_count==0&&!state.dirty&&!state.loading&&state.memory_line_count==1&&take(model->text()).empty(),"Empty document contract changed");}
    else if(name=="text"||name=="lines"){check(take(model->set_text("one\r\ntwo\rthree\n")),"Text assignment was ignored");check(take(model->text())=="one\ntwo\nthree\n"&&take(model->state()).line_count==4&&take(model->line_length(2))==5,"Newline normalization or line index changed");auto row=take(model->line_at(1));check(row.index==1&&row.number==2&&row.text=="two"&&!row.file_backed,"Native row roles changed");}
    else if(name=="ranges"){take(model->set_text("α😀\n한국\nlast"));check(take(model->text_range(1,5))=="😀\n한"&&take(model->text_range(5,1))=="😀\n한","Half-open UTF-16 range changed");check(!model->text_range(2,3)&&!model->line_text(99),"Invalid line or split surrogate accepted");}
    else if(name=="cursor"){take(model->set_text("ab\n😀\nz"));take(model->move_cursor(0,1));const auto old=take(model->state());take(model->move_cursor(lvrs::TextCursorMove::down));check(take(model->state()).cursor.line==1&&take(model->state()).cursor.column==0,"Vertical movement split a surrogate pair");take(model->move_cursor(lvrs::TextCursorMove::document_end));check(take(model->state()).cursor.position==7,"Document end changed");auto after=take(model->state());check(after.document_revision==old.document_revision&&after.undo_depth==old.undo_depth,"Cursor navigation edited the document");}
    else if(name=="unicode"){take(model->set_text("e\xcc\x81👩‍👩‍👧‍👦한"));take(model->set_cursor_position(13));take(model->move_cursor(lvrs::TextCursorMove::left));check(take(model->state()).cursor.position==2,"Cursor split a ZWJ emoji");take(model->remove_previous_character());check(take(model->text())=="👩‍👩‍👧‍👦한"&&take(model->state()).cursor.position==0,"Backspace split a combining cluster");take(model->remove_next_character());check(take(model->text())=="한","Delete split a ZWJ cluster");}
    else if(name=="words"){take(model->set_text("one!\n  \n한국 e\xcc\x81 tail_2"));check(take(model->next_word_boundary_position(3))==10,"Word movement failed to cross separator-only lines");check(take(model->previous_word_boundary_position(8))==0,"Backward word movement failed across lines");take(model->move_cursor(lvrs::TextCursorMove::word_right));check(take(model->state()).cursor.position==3,"Word cursor target changed");}
    else if(name=="edits"||name=="multirange"){take(model->set_text("one\ntwo\nthree"));take(model->replace_range(2,9,"A\r\nB"));check(take(model->text())=="onA\nBhree"&&take(model->state()).cursor.line==1&&take(model->state()).cursor.column==1,"Range replacement or cursor placement changed");take(model->insert_newline());check(take(model->state()).cursor.line==2&&take(model->state()).cursor.column==0,"Return did not move to the inserted line");take(model->remove_previous_character());check(take(model->text())=="onA\nBhree","Newline merge changed content");}
    else if(name=="noop"){take(model->set_text("same"));take(model->mark_clean());const auto old=take(model->state());check(!take(model->set_text("same"))&&!take(model->replace_range(0,4,"same"))&&!take(model->insert_text("")),"Valid no-op changed content");auto state=take(model->state());check(state.revision==old.revision&&!state.dirty&&state.undo_depth==0,"No-op dirtied or revised the document");}
    else if(name=="dirty_history"||name=="history_budget"){take(model->set_text("base"));take(model->mark_clean());take(model->insert_text("1"));take(model->insert_text("2"));take(model->insert_text("3"));take(model->undo());take(model->undo());if(name=="history_budget")check(!take(model->undo())&&take(model->text())=="1base","History count budget changed");else{take(model->undo());check(take(model->text())=="base"&&!take(model->state()).dirty,"Undo lost the clean checkpoint");}take(model->redo());take(model->insert_text("X"));check(!take(model->redo())&&take(model->stats()).history_bytes<=options.undo_bytes,"Redo retirement underflowed the history budget");}
    else if(name=="chunked"||name=="newline_chunks"||name=="lossy_bom"){auto bytes=name=="lossy_bom"?std::string("\xef\xbb\xbf" "A\xff\nB",7):std::string("one\r\ntwo\rthree\n");storage->put("input.txt",bytes);take(model->load_file("input.txt"));check(take(model->state()).loading&&storage->counters->reads==0,"Loading read the whole source synchronously");(void)loop.poll({.max_callbacks=1});check(take(model->state()).loaded_byte_count<=options.load_chunk_size,"One callback exceeded the chunk budget");drain(loop,model);if(name=="lossy_bom")check(take(model->text())=="A\xef\xbf\xbd\nB","BOM or invalid UTF-8 replacement changed");else check(take(model->text())=="one\ntwo\nthree\n"&&take(model->state()).file_backed_line_count==4,"CRLF chunk indexing changed");}
    else if(name=="lazy"||name=="cache"||name=="promotion"){storage->put("input.txt","a\nb\nc\nd\ne");take(model->load_file("input.txt"));drain(loop,model);const auto baseline=take(model->stats());check(take(model->state()).memory_line_count==0&&baseline.cached_lines==0,"Loading retained decoded lines");take(model->line_text(1));const auto read=take(model->stats()).source_reads;take(model->line_text(1));check(take(model->stats()).source_reads==read&&take(model->stats()).cache_hits>baseline.cache_hits,"Repeated line query re-read its source");if(name=="promotion"){take(model->replace_range(2,3,"B"));check(take(model->state()).file_backed_line_count==4&&take(model->state()).memory_line_count==1,"Edit promoted untouched source lines");}if(name=="cache"){for(unsigned i=0;i<5;++i)take(model->line_text(i));auto stats=take(model->stats());check(stats.cached_lines<=2&&stats.cache_bytes<=2048,"Decoded line cache exceeded its budget");}}
    else if(name=="save"||name=="save_as"||name=="save_fail"){storage->put("input.txt","one\r\ntwo");take(model->load_file("input.txt"));drain(loop,model);take(model->replace_range(0,3,"ONE"));auto revision=take(model->state()).document_revision;
        if(name=="save_fail"){storage->counters->fail_commit=true;check(!model->save_file()&&storage->get("input.txt")=="one\r\ntwo"&&take(model->state()).dirty,"Failed publication replaced the destination");}
        else{take(model->save_file(name=="save_as"?"other.txt":""));check(storage->get(name=="save_as"?"other.txt":"input.txt")=="ONE\ntwo"&&!take(model->state()).dirty&&take(model->state()).file_backed_line_count==2&&take(model->state()).document_revision==revision,"Saving reset content, revision or backing records");take(model->undo());check(take(model->text())=="one\ntwo"&&take(model->state()).dirty,"Save retired the undo source snapshot");take(model->redo());check(!take(model->state()).dirty,"Redo lost the saved clean checkpoint");}}
    else if(name=="missing"){take(model->load_file("new.txt"));check(!take(model->state()).loading&&!take(model->state()).dirty&&take(model->state()).has_file_path,"Missing file was not an attached empty document");take(model->insert_text("new"));take(model->save_file());check(storage->get("new.txt")=="new","First edit could not create its attached file");}
    else if(name=="cancel"||name=="edit_cancel"){storage->put("input.txt","a\nb\nc\nd\ne");take(model->load_file("input.txt"));(void)loop.poll({.max_callbacks=1});check(take(model->state()).loading,"Fixture did not produce a partial load");if(name=="cancel")take(model->cancel_load());else take(model->insert_text("X"));auto current=take(model->text());(void)loop.poll();check(!take(model->state()).loading&&take(model->text())==current,"Retired loading callback appended stale content");}
    else if(name=="load_fail"){storage->put("input.txt","one\ntwo");storage->counters->fail_read=true;take(model->load_file("input.txt"));(void)loop.poll();check(!take(model->state()).loading&&!take(model->state()).last_error.empty(),"Source failure left loading active");}
    else if(name=="source_changed"){storage->put("input.txt","one");take(model->load_file("input.txt"));drain(loop,model);storage->counters->corrupt_read=true;check(!model->line_text(0),"Changed source bytes bypassed the retained line fingerprint");}
    else if(name=="save_write_fail"){storage->put("input.txt","old");take(model->load_file("input.txt"));drain(loop,model);take(model->insert_text("new"));storage->counters->fail_write=true;check(!model->save_file()&&storage->get("input.txt")=="old"&&take(model->state()).dirty,"Failed streaming write changed the published destination");}
    else if(name=="materialization_limit"){storage->put("input.txt","one\ntwo");take(model->load_file("input.txt"));drain(loop,model);check(!model->text()&&take(model->text_range(2,5))=="e\nt"&&take(model->line_text(1))=="two","Materialization cap prevented bounded line/range access");}
    else if(name=="utf8_chunks"){storage->put("input.txt","한😀\r\n次\n");take(model->load_file("input.txt"));drain(loop,model);check(storage->counters->largest_read==1&&take(model->text())=="한😀\n次\n","Chunk boundaries split UTF-8 or CRLF");}
    else if(name=="long_line"){storage->put("input.txt",std::string(4097,'x'));take(model->load_file("input.txt"));drain(loop,model);check(storage->counters->largest_read==2&&take(model->state()).line_count==1&&take(model->line_length(0))==4097,"Unfinished line was truncated or read outside its chunk budget");}
    else if(name=="limits"){take(model->set_text("ab\nc"));auto old=take(model->text());check(!model->set_text("four")&&!model->insert_text("\nx\ny")&&take(model->text())==old,"Rejected edit partially changed the document");}
    else if(name=="admission"){runtime::EventLoop small(1);auto owned=take(lvrs::TextDocumentModel::create(small,{},storage));take(owned->set_text("preserve"));storage->put("input.txt","next");auto occupied=small.post([]{});check(bool(occupied),"Queue fixture failed");auto rejected=owned->load_file("input.txt");check(!rejected&&rejected.error().code==core::ErrorCode::full&&take(owned->text())=="preserve"&&!take(owned->state()).loading,"Rejected load destroyed the previous document");}
    else if(name=="reschedule"){runtime::EventLoop small(1);auto owned=take(lvrs::TextDocumentModel::create(small,{.load_chunk_size=2},storage));storage->put("input.txt","a\nb\nc");take(owned->load_file("input.txt"));(void)small.poll({.max_callbacks=1});check(!take(owned->state()).loading&&!take(owned->state()).last_error.empty(),"Reschedule failure left an orphaned load");}
    else if(name=="observation"||name=="weak"||name=="reentrant"||name=="notification_failure"){unsigned calls=0;bool blocked=false;auto port=std::make_shared<lvrs::TextDocumentPort>([&](const auto& change){++calls;check(change.state.revision>1,"Observer received a pre-commit state");if(name=="reentrant"){auto result=model->set_text("nested");blocked=!result&&result.error().code==core::ErrorCode::busy;take(model->line_text(0));}if(name=="notification_failure")throw std::runtime_error("receiver");});good(model->observe(port));good(model->observe(port));if(name=="weak")port.reset();auto result=model->set_text("committed");if(name=="notification_failure")check(!result&&take(model->text())=="committed","Receiver error rolled back a committed command");else{take(std::move(result));check(calls==(name=="weak"?0u:1u),"Observer was retained or duplicated");if(name=="reentrant")check(blocked,"Receiver mutation bypassed reentrancy admission");}}
    else if(name=="retirement"){storage->put("input.txt","pending");take(model->load_file("input.txt"));std::weak_ptr weak=model;model.reset();(void)loop.poll();check(weak.expired()&&storage->counters->reads==0,"Pending read retained a retired model");}
    else if(name=="retire_in_read"){storage->put("input.txt","one");take(model->load_file("input.txt"));drain(loop,model);std::weak_ptr weak=model;storage->counters->on_read=[&]{model.reset();};auto result=model->line_text(0);check(!result&&result.error().code==core::ErrorCode::closed&&weak.expired(),"Read callback used a retired model");}
    else if(name=="wrong_owner"){core::ErrorCode code=core::ErrorCode::failed;std::thread worker([&]{auto state=model->state();if(!state)code=state.error().code;});worker.join();check(code==core::ErrorCode::wrong_executor,"Owner-thread boundary was ignored");}
    else if(name=="wrong_loop"){storage->put("input.txt","one");take(model->load_file("input.txt"));std::thread worker([&]{(void)loop.poll();});worker.join();auto state=model->state();check(!state&&state.error().code==core::ErrorCode::wrong_executor,"Foreign Core dispatch mutated the model");}
    else if(name=="large"){std::string value;for(unsigned i=0;i<100000;++i){value+=std::to_string(i);value+='\n';}storage->put("large.txt",value);take(model->load_file("large.txt"));drain(loop,model);auto before=take(model->stats());check(take(model->position_for_line_column(99999,2))>500000,"Large line index returned a wrong prefix");auto after=take(model->stats());check(after.index_visits-before.index_visits<256&&after.materializations==before.materializations,"Point lookup scanned or materialized the large document");take(model->replace_range(0,0,"first\n"));check(take(model->state()).line_count==100002&&take(model->line_text(100000))=="99999","Structural edit reset a large source index");}
    else if(name=="random"){std::mt19937 generator(713);std::string reference;for(unsigned i=0;i<500;++i){const auto a=reference.empty()?0:generator()%(reference.size()+1);const auto b=reference.empty()?0:generator()%(reference.size()+1);std::string added=i%7==0?"\n":std::string(1,char('a'+i%26));const auto first=std::min(a,b),last=std::max(a,b);reference.replace(first,last-first,added);take(model->replace_range(first,last,added));check(take(model->text())==reference,"Indexed edits diverged from the independent string oracle");}}
    else if(name=="operation_path"){storage->put("input.txt","one\ntwo");std::filesystem::path loaded;auto port=std::make_shared<lvrs::TextDocumentPort>([&](const auto& change){if(change.kind==lvrs::TextDocumentEventKind::loaded)loaded=change.path;});good(model->observe(port));take(model->load_file("input.txt"));take(model->set_file_path("rebound.txt"));drain(loop,model);check(loaded.filename()=="input.txt"&&take(model->state()).file_path.filename()=="rebound.txt","Operation path followed a rebound file property");}
    else if(name=="provider"){iiFileProvider::native::TemporaryDirectory directory(build);const auto path=directory.path()/"native-document.txt";iiFileProvider::native::File::write(path,"one\r\ntwo");auto file=take(lvrs::TextDocumentModel::create(loop));take(file->load_file(path));drain(loop,file);take(file->replace_range(0,3,"ONE"));take(file->save_file());check(iiFileProvider::native::File::read(path)=="ONE\ntwo","iiFileProvider did not publish normalized bytes");take(file->undo());check(take(file->text())=="one\ntwo","Atomic replacement invalidated the old retained file handle");}
    else throw std::runtime_error("Unknown text document case");
}
}
int main(int argc,char** argv){try{run(argc>1?argv[1]:"defaults",argc>2?argv[2]:std::filesystem::current_path());std::cout<<"TEXT_DOCUMENT_OK\n";}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
