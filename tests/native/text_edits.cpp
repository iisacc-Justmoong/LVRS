#include <LVRS/TextDocumentModel.h>
#include <iostream>
#include <stdexcept>
using namespace ii;
namespace {
void check(bool b,const char* m){if(!b)throw std::runtime_error(m);}
template<class T>T take(core::Result<T> r){if(!r)throw std::runtime_error(r.error().message);return std::move(*r);}
void good(core::Result<void> r){if(!r)throw std::runtime_error(r.error().message);}
void run(const std::string& name){
    using namespace lvrs;
    if(name=="insert"){TextDocumentEdit e{.first=3,.inserted=2};check(take(map_text_position(3,e,TextPositionAffinity::before))==3&&take(map_text_position(3,e))==5&&take(map_text_position(8,e))==10,"Insertion affinity or suffix mapping changed");}
    else if(name=="remove"){TextDocumentEdit e{.first=3,.removed=4};check(take(map_text_position(2,e))==2&&take(map_text_position(5,e))==3&&take(map_text_position(7,e))==3&&take(map_text_position(10,e))==6,"Deletion mapping left positions inside removed text");}
    else if(name=="replace"){TextDocumentEdit e{.first=2,.removed=4,.inserted=3};check(take(map_text_position(4,e,TextPositionAffinity::before))==2&&take(map_text_position(4,e))==5&&take(map_text_position(6,e,TextPositionAffinity::before))==5&&take(map_text_position(10,e))==9,"Replacement edge mapping changed");}
    else if(name=="selection"){check(take(map_text_selection({3,8},{.first=3,.inserted=2}))==TextDocumentSelection{5,10},"Selection start did not exclude a boundary insertion");check(take(map_text_selection({3,8},{.first=8,.inserted=2}))==TextDocumentSelection{3,8},"Selection end included a boundary insertion");}
    else if(name=="reversed"){auto e=TextDocumentEdit{.first=2,.removed=4,.inserted=1};check(take(map_text_selection({4,3},e))==TextDocumentSelection{3,3}&&take(map_text_selection({8,3},e))==TextDocumentSelection{5,3},"Reverse selection collapsed incorrectly or changed direction");}
    else if(name=="collapsed"){check(take(map_text_selection({3,3},{.first=3,.inserted=2}))==TextDocumentSelection{5,5},"Collapsed caret did not follow an insertion");}
    else if(name=="large"){constexpr auto n=std::uint64_t(1)<<40;check(take(map_text_position(n+50,{.first=n,.removed=10,.inserted=20}))==n+60,"Large logical positions were narrowed to widget coordinates");}
    else if(name=="overflow"){check(!map_text_position(UINT64_MAX,{.first=UINT64_MAX-10,.removed=5,.inserted=10})&&!map_text_position(0,{.first=UINT64_MAX,.inserted=1})&&!map_text_position(1,{},static_cast<TextPositionAffinity>(4)),"Invalid/overflowing edit mapping was admitted");}
    else{
        runtime::EventLoop loop;TextDocumentOptions options;if(name=="no_history")options.undo_entries=0;
        auto model=take(TextDocumentModel::create(loop,options));take(model->set_text("alpha\n한글 😀\nomega"));
        std::vector<TextDocumentChange> changes;auto receiver=std::make_shared<TextDocumentPort>([&](const auto& c){changes.push_back(c);});good(model->observe(receiver));
        if(name=="metadata"||name=="history"||name=="no_history"){
            take(model->replace_range(6,11,"X\r\nY"));check(changes.size()==1&&changes[0].edit&&changes[0].edit->first==6&&changes[0].edit->removed==5&&changes[0].edit->inserted==3&&changes[0].edit->transaction&&changes[0].edit->operation==TextEditOperation::replace,"Edit notification lost exact normalized UTF-16 units");
            auto id=changes[0].edit->transaction;
            if(name=="no_history")check(!changes[0].edit->undoable&&!take(model->undo()),"Unretained edit advertised view undo history");
            else if(name=="history"){take(model->undo());auto u=*changes.back().edit;check(u.transaction==id&&u.operation==TextEditOperation::undo&&u.first==6&&u.removed==3&&u.inserted==5,"Undo did not publish the inverse of its original transaction");take(model->redo());auto r=*changes.back().edit;check(r.transaction==id&&r.operation==TextEditOperation::redo&&r.removed==5&&r.inserted==3,"Redo changed transaction identity or range");}
        }
        else if(name=="no_mutation"){auto before=take(model->state());take(model->set_cursor_position(3));take(model->mark_clean());for(const auto& c:changes)check(!c.edit,"Navigation/clean checkpoint advertised a text edit");check(take(model->state()).document_revision==before.document_revision,"Navigation changed content revision");}
        else if(name=="noop"){check(!take(model->replace_range(0,5,"alpha"))&&changes.empty(),"No-op replacement created an edit transaction");}
        else if(name=="rejected"){check(!model->replace_range(9,10,"x")&&!model->replace_range(0,1,std::string("\xff",1))&&changes.empty(),"Scalar-splitting or invalid UTF-8 edit was committed");}
        else if(name=="fanout"){unsigned delivered=0;auto broken=std::make_shared<TextDocumentPort>([](const auto&){throw std::runtime_error("Receiver failure");});auto healthy=std::make_shared<TextDocumentPort>([&](const auto&){++delivered;});good(model->observe(broken));good(model->observe(healthy));auto result=model->replace_range(0,1,"Q");check(!result&&delivered==1&&take(model->text()).starts_with("Qlpha"),"A failed receiver starved a healthy view or rolled back committed data");}
        else throw std::runtime_error("Unknown text edit case");
    }
}
}
int main(int argc,char** argv){try{run(argc>1?argv[1]:"metadata");std::cout<<"TEXT_EDITS_OK\n";}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
