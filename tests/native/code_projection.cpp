#include <LVRS/CodeEditorDetail.h>
#include <iostream>
#include <random>
#include <stdexcept>
using namespace ii;
namespace {
void check(bool b,const char* m){if(!b)throw std::runtime_error(m);}
template<class T>T take(core::Result<T> r){if(!r)throw std::runtime_error(r.error().message);return std::move(*r);}
void compare(lvrs::code_detail::Projection& p,const std::string& expected){
    auto index=take(gui::TextIndex::create(expected));check(p.length()==index.utf16_size()&&p.range(0,p.length())==expected,"Projected content differs from the independent string oracle");
    std::vector<std::string> lines;std::size_t start=0;for(std::size_t i=0;i<expected.size();++i)if(expected[i]=='\n'){lines.push_back(expected.substr(start,i-start));start=i+1;}lines.push_back(expected.substr(start));
    check(p.lines()==lines.size(),"Projected line count changed");std::uint64_t units=0;
    for(std::size_t row=0;row<lines.size();++row){auto line=p.line(row);check(line.start==units&&line.index.text()==lines[row],"Projected prefix/preedit/suffix line mapping changed");
        auto part=take(gui::TextIndex::create(lines[row]));for(std::size_t column=0;column<=part.utf16_size();++column)if(part.byte_for_utf16(column)){auto pos=p.locate(units+column);check(pos.line==row&&pos.column==column&&pos.position==units+column,"Projected caret location differs from its rendered row");}
        units+=part.utf16_size()+1;
    }
    for(std::size_t a=0;a<=index.utf16_size();++a)if(auto begin=index.byte_for_utf16(a))for(auto b=a;b<=index.utf16_size();++b)if(auto end=index.byte_for_utf16(b))check(p.range(a,b)==expected.substr(*begin,*end-*begin),"Bounded surrounding-text range crossed the wrong source segment");
}
void compose(lvrs::code_detail::Projection& p,std::size_t a,std::size_t b,std::string text){
    lvrs::code_detail::Projection::Composition c;c.first=a;c.last=b;c.begin=take(p.model->line_column_for_position(a));c.end=take(p.model->line_column_for_position(b));c.text=take(gui::TextIndex::create(std::move(text)));c.generation=1;
    for(std::size_t i=0;i<c.text.text().size();++i)if(c.text.text()[i]=='\n')c.starts.push_back(take(c.text.utf16_for_byte(i+1)));p.composition=std::move(c);
}
void run(const std::string& name){
    runtime::EventLoop loop;auto model=take(lvrs::TextDocumentModel::create(loop));take(model->set_text("alpha\n한글 😀\nomega"));lvrs::code_detail::Projection p{.model=model,.state=take(model->state())};
    if(name=="plain")compare(p,"alpha\n한글 😀\nomega");
    else if(name=="metadata"){auto info=take(model->line_info(1));auto before=take(model->stats());auto second=take(model->line_info(1));take(model->line_column_for_position(3));take(model->position_for_line_column(1,5));auto after=take(model->stats());check(info.identity==second.identity&&info.start==6&&info.length==5&&after.source_reads==before.source_reads&&after.decoded_lines==before.decoded_lines,"Line metadata/ASCII/line-edge navigation decoded text or lost its identity");}
    else if(name=="immutable"){auto old=take(model->line_index(1));auto identity=take(model->line_info(2)).identity;take(model->replace_range(0,0,"new\n"));check(old.text()=="한글 😀"&&take(model->line_info(3)).identity==identity,"Native editor indices lost immutable sharing or untouched row identity");}
    else if(name=="one"){compose(p,7,8,"국");compare(p,"alpha\n한국 😀\nomega");}
    else if(name=="multi"){compose(p,7,8,"국\nABC");compare(p,"alpha\n한국\nABC 😀\nomega");}
    else if(name=="cross"){compose(p,3,9,"X\nY\n");compare(p,"alpX\nY\n😀\nomega");}
    else if(name=="suffix"){compose(p,6,11,"A\nB");compare(p,"alpha\nA\nB\nomega");}
    else if(name=="empty"){compose(p,0,p.state.character_count,"");compare(p,"");}
    else if(name=="newline"){compose(p,5,6,"\n\n");compare(p,"alpha\n\n한글 😀\nomega");}
    else if(name=="virtual_unchanged"){const auto before=take(model->state());compose(p,0,5,"한\n😀");compare(p,"한\n😀\n한글 😀\nomega");const auto after=take(model->state());check(before.revision==after.revision&&before.document_revision==after.document_revision&&before.undo_depth==after.undo_depth,"Preedit modified the storage model");}
    else if(name=="random"){std::mt19937 rng(27);const std::string source="alpha\n한글 😀\nomega";auto index=take(gui::TextIndex::create(source));std::vector<std::size_t> edges;for(std::size_t i=0;i<=index.utf16_size();++i)if(index.byte_for_utf16(i))edges.push_back(i);
        for(unsigned i=0;i<160;++i){auto a=edges[rng()%edges.size()],b=edges[rng()%edges.size()];if(a>b)std::swap(a,b);std::string inserted=i%4==0?"":i%4==1?"X":i%4==2?"한\n😀":"\n\nZ";compose(p,a,b,inserted);compare(p,source.substr(0,take(index.byte_for_utf16(a)))+inserted+source.substr(take(index.byte_for_utf16(b))));}}
    else throw std::runtime_error("Unknown code projection case");
}
}
int main(int argc,char** argv){try{run(argc>1?argv[1]:"plain");std::cout<<"CODE_PROJECTION_OK\n";}catch(const lvrs::code_detail::Failure& e){std::cerr<<e.error.message<<'\n';return 1;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
