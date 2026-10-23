#include <LVRS/Widgets.h>
#include <LVRS/TableHeaderModel.h>
#include <cmath>
#include <iostream>
#include <stdexcept>
using namespace ii;
namespace {
void check(bool b,const char* s){if(!b)throw std::runtime_error(s);}
template<class T>T take(core::Result<T> r){if(!r)throw std::runtime_error(r.error().message);return std::move(*r);}
void good(core::Result<void> r){if(!r)throw std::runtime_error(r.error().message);}
void model(){
    lvrs::TableHeaderModel m;lvrs::TableHeaderModelOptions o{.width=717,.column_widths={100.4,90.6,0},.columns={{"Name","string"},{"Count","int"},{"Status","bool"}}};good(m.set_options(o));
    auto d=m.descriptors();check(d.size()==3&&d[0].width==100&&d[1].width==91&&d[2].width==239&&d[2].x==191&&d[0].padding==8,"Header geometry differs from source rounding/automatic widths");
    auto rev=m.revision();good(m.set_options(o));check(m.revision()==rev,"Equal header options rebuilt descriptors");
    o.column_widths[0]=1;good(m.set_options(o));check(m.descriptors()[0].width==32,"Header minimum width ignored");
    o.width=std::nan("");check(!m.set_options(o)&&m.descriptors()[0].width==32,"Invalid header replaced valid descriptors");
    check(!m.descriptor_at(99),"Out-of-range header exposed data");
}
void run(std::string name){
    gui::Host host({.size={900,620}});gui::WidgetMount mount;lvrs::TableCellItem cell;lvrs::TableRow row;lvrs::TableHeader header;int edited=0,submitted=0,rejected=0,columns=0;std::string value;std::weak_ptr<int> retired;
    good(host.run({.opened=[&]{mount=take(host.mount_widgets([&](gui::WidgetBuilder& b)->core::Result<void>{
        auto root=take(b.vstack({},"root",{.spacing=12,.padding=20,.alignment=gui::StackAlignment::leading}));auto owner=std::make_shared<int>(1);retired=owner;
        cell=take(lvrs::TableCellItem::create(b,root,"cell",{.text="12",.inputable=true,.value_type="int",.validator=lvrs::validate_table_value},{.edited=[&,owner](std::string s){++edited;value=std::move(s);},.submitted=[&](std::string s){++submitted;value=std::move(s);},.rejected=[&](std::string s,std::string t){++rejected;check(t=="int"&&!s.empty(),"Rejection payload lost");}}));
        row=take(lvrs::TableRow::create(b,root,"row",{.inputable=true},{.edited=[&](std::size_t i,std::string s){check(i==1,"Row index lost");value=std::move(s);++edited;}}));
        header=take(lvrs::TableHeader::create(b,root,"header",{.interactive=true},{.column_clicked=[&](std::size_t i,const lvrs::TableHeaderColumn& c){check(i==1&&c.text=="Column","Column event lost source");++columns;}}));return {};
    },lvrs::Theme{}.appearance()));
    if(name=="geometry"){
        auto r=take(cell.root().snapshot()).bounds,e=take(cell.editor().snapshot()).bounds;check(r.width==234&&r.height==24&&e.x==r.x+9&&e.y==r.y+5.5&&e.height==13,"Native cell metrics differ");
        auto a=take(take(row.cell(0)).root().snapshot()).bounds,c=take(take(row.cell(2)).root().snapshot()).bounds;check(a.width==234&&std::abs(c.x-a.x-483)<.01,"Row spacing differs");
        check(take(header.root().snapshot()).bounds.height==25&&take(take(header.column(1)).snapshot()).bounds.width==239,"Header metrics differ");
    }else if(name=="editing"){
        good(cell.editor().focus());good(mount.dispatch_input({.kind=gui::WidgetInputKind::key_down,.key=gui::WidgetKey::select_all}));good(mount.dispatch_input({.kind=gui::WidgetInputKind::text,.text="45"}));good(mount.dispatch_input({.kind=gui::WidgetInputKind::key_down,.key=gui::WidgetKey::enter}));
        check(edited==1&&submitted==1&&value=="45"&&std::get<std::int64_t>(take(cell.snapshot()).typed_value)==45,"Typed cell edit/submit failed");
        good(mount.dispatch_input({.kind=gui::WidgetInputKind::key_down,.key=gui::WidgetKey::select_all}));good(mount.dispatch_input({.kind=gui::WidgetInputKind::text,.text="bad"}));check(rejected==1&&edited==1&&take(cell.editor().snapshot()).text=="45"&&!take(cell.snapshot()).input_accepted,"Rejected edit changed accepted value");
        auto input=take(row.cell(1)).editor();good(input.focus());good(mount.dispatch_input({.kind=gui::WidgetInputKind::key_down,.key=gui::WidgetKey::select_all}));good(mount.dispatch_input({.kind=gui::WidgetInputKind::text,.text="Row edit"}));check(edited==2&&value=="Row edit","Row edit forwarding failed");
        good(take(header.column(1)).activate());check(columns==1,"Header activation missing");
    }else if(name=="selection"){
        auto before=take(host.capture_canvas());good(cell.set_selected(true,true));auto after=take(host.capture_canvas());check(before.rgba!=after.rgba,"Cell selection/current border did not reach GPU");
        good(cell.set_selected(false,false));check(take(cell.snapshot()).input_result=="12","Selection changed cell value");
    }else if(name=="mutations"){
        good(cell.set_options({.text="Styled",.text_style=gui::TextRole::header,.inputable=true}));check(take(cell.editor().snapshot()).font_size==13&&take(take(mount.find("cell.label")).snapshot()).font_size==17,"Readonly text role changed the original body-font editor contract");
        auto old=take(take(row.cell(1)).editor().snapshot()).id;lvrs::TableRowOptions o{.width=600,.cell_width=200,.inputable=true};o.cells[1].text="Replacement";good(row.set_options(o));check(take(take(row.cell(1)).editor().snapshot()).id==old&&take(take(row.cell(1)).snapshot()).input_result=="Replacement"&&edited==0,"Row replacement rebuilt handles or emitted edit");
        o.width=570;o.cells[2].text=std::string("bad\0text",8);check(!row.set_options(o)&&take(row.root().snapshot()).bounds.width==600,"Invalid last cell partially changed row geometry");
        lvrs::TableHeaderOptions h{.interactive=true};h.layout.columns[1].text="Updated";h.layout.column_widths={180,200,240};good(header.set_options(h));check(take(take(header.column(1)).snapshot()).accessibility_name=="Updated"&&take(take(header.column(2)).snapshot()).bounds.x-take(header.root().snapshot()).bounds.x==380,"Header geometry/accessible name not updated");
    }else if(name=="validator_lifetime"){
        lvrs::TableCellOptions o{.text="12",.inputable=true};o.validator=[&](std::string_view value,std::string_view type){auto nested=cell.apply_input_result(value);check(!nested&&nested.error().code==core::ErrorCode::busy,"Recursive validation not rejected");return lvrs::validate_table_value(value,type);};good(cell.set_options(o));check(take(cell.apply_input_result("42"))=="42","Validator did not recover after reentry rejection");
        o.validator=[&](std::string_view value,std::string_view){good(host.unmount_widgets());return lvrs::TableValidation{true,std::string(value),std::string(value)};};good(cell.set_options(o));auto r=cell.apply_input_result("45");check(!r&&r.error().code==core::ErrorCode::closed&&retired.expired(),"Validator removal retained or mutated detached cell");
    }else if(name=="validation"){
        check(!cell.set_options({.width=0})&&!cell.set_options({.cell_height=std::nan("")}),"Invalid cell geometry accepted");
        check(!row.cell(3)&&!row.set_options({.cells={}}),"Invalid row index or structural replacement accepted");
        check(!header.set_options({.layout={.columns={{"one","string"}}}}),"Structural header replacement accepted");
        auto a=lvrs::validate_table_value("9223372036854775808","int"),b=lvrs::validate_table_value("nan","number"),c=lvrs::validate_table_value("true","bool");check(!a.accepted&&!b.accepted&&c.accepted&&std::get<bool>(c.value),"Value validation overflow or nonfinite handling failed");
    }else if(name=="lifetime"){
        check(!retired.expired(),"Cell callback not retained");good(host.unmount_widgets());check(retired.expired()&&!cell.snapshot()&&!row.cell(0)&&!header.column(0),"Detached cell retained callbacks or returned handles");
    }else throw std::runtime_error("Unknown native table test");
    good(host.request_close());}}));check(!cell.snapshot(),"Cell handle survived Host shutdown");
}
}
int main(int n,char** v){try{std::string name=n>1?v[1]:"geometry";if(name=="model")model();else run(name);std::cout<<"LVRS_TABLE_PARTS_OK\n";}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
