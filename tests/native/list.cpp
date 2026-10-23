#include <LVRS/Widgets.h>
#include <algorithm>
#include <array>
#include <iostream>
#include <stdexcept>
#include <thread>
using namespace ii;
namespace {
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
template<class T>T take(core::Result<T> value){if(!value){if(value.error().cause)std::rethrow_exception(value.error().cause);throw std::runtime_error(value.error().message);}return std::move(*value);}
void good(core::Result<void> value){if(!value){if(value.error().cause)std::rethrow_exception(value.error().cause);throw std::runtime_error(value.error().message);}}
void model(){
    lvrs::ListModel model;take(model.set_rows({{"a","Group"},{"b","Child","a"},{"c","Other"}}));
    const auto original=std::vector<lvrs::ListRowData>(model.rows().begin(),model.rows().end());const auto revision=model.changes().revision();
    check(take(model.select("b"))&&model.changes().revision()==revision,"Selection changed list data revision");
    take(model.set_expanded("a",false));check(take(model.visible_rows()).size()==2&&std::ranges::equal(original,model.rows()),"Expansion altered source rows");
    check(!model.move("a","b"),"Cyclic reparenting accepted");check(std::ranges::equal(original,model.rows()),"Rejected move altered rows");
    check(!model.move("a","a"),"Self-parenting accepted");
    check(!model.set_rows({{"duplicate","A"},{"duplicate","B"}}),"Duplicate identifiers accepted");
    take(model.rename("b","새 이름"));check(take(model.row("b")).text=="새 이름","Unicode rename failed");
    take(model.move("c","","a"));check(model.rows().front().key=="c","Sibling order did not change");
    take(model.set_expanded("a",true));auto visible=take(model.visible_rows());check(visible.size()==3&&visible[2].depth==1,"Tree depth/expansion lost");
    take(model.set_rows({{"a","Remaining"}}));check(model.selection().get().empty(),"Removed selection retained a stale ID");
    bool rejected=false;std::thread other([&]{auto result=model.select("a");rejected=!result&&result.error().code==core::ErrorCode::wrong_executor;});other.join();check(rejected,"List model accepted a foreign thread");
}
void view(bool large){
    auto model=std::make_shared<lvrs::ListModel>();std::vector<lvrs::ListRowData> rows;
    const auto count=large?20000:3;for(int i=0;i<count;++i)rows.push_back({"row-"+std::to_string(i),"Layer "+std::to_string(i)});take(model->set_rows(std::move(rows)));
    gui::Host host({.size={720,520}});lvrs::List list;int renamed=0;
    good(host.run({.opened=[&]{
        auto mount=take(host.mount_widgets([&](gui::WidgetBuilder& b)->core::Result<void>{
            lvrs::Widgets widgets(b);auto card=take(widgets.app_card({},"card",{.width=500,.height=400}));
            list=take(lvrs::List::create(b,card,"layers",{.model=model,.width=460,.height=320,.hierarchy=true},
                {.renamed=[&](std::string key,std::string text){check(key=="row-0"&&text=="Renamed","Typed rename payload lost");++renamed;}}));return {};
        }));
        auto state=take(list.snapshot());const auto snapshots=take(mount.snapshot());
        const auto list_nodes=std::ranges::count_if(snapshots,[](const auto& node){return node.id=="layers"||node.id.starts_with("layers.");});
        check(list_nodes<10&&state.painted_rows<30,"List allocated or painted all rows");
        if(large){good(list.set_scroll_offset(280000));state=take(list.snapshot());check(state.scroll_offset>0&&state.painted_rows<30,"Deep virtual scroll failed");}
        else{
            take(model->select("row-0"));good(list.begin_rename());auto editor=take(mount.find("layers.editor"));
            good(mount.dispatch_input({.kind=gui::WidgetInputKind::key_down,.key=gui::WidgetKey::select_all}));
            good(mount.dispatch_input({.kind=gui::WidgetInputKind::text,.text="Renamed"}));
            good(mount.dispatch_input({.kind=gui::WidgetInputKind::key_down,.key=gui::WidgetKey::enter}));
            check(renamed==1&&!take(list.snapshot()).editing&&take(model->row("row-0")).text=="Renamed","Reusable list editor failed");
        }
        check(take(take(mount.find("card")).snapshot()).drawing_panels==1,"Native card background did not follow layout");
        good(host.set_content_size({850,600}));check(take(list.snapshot()).painted_rows<40,"Resize broke virtual drawing");
        good(host.unmount_widgets());take(model->rename("row-0","After detach"));check(!list.snapshot(),"List model kept a retired view alive");good(host.request_close());
    }}));
}
void hidden_surface(){
    gui::Host host({.size={400,300}});gui::Widget card;
    good(host.run({.opened=[&]{
        auto mount=take(host.mount_widgets([&](gui::WidgetBuilder& b)->core::Result<void>{
            lvrs::Widgets w(b);auto parent=take(w.vstack({},"parent"));good(parent.set_visible(false));
            card=take(w.app_card(parent,"hidden-card",{.width=300,.height=200}));
            good(parent.set_visible(true));return {};
        }));
        (void)take(host.capture_canvas());
        check(take(card.snapshot()).drawing_panels==1,"Previously hidden card did not paint on its first visible layout");
        good(host.request_close());
    }}));
}

void appearance(){
    gui::Host host({.size={320,240}});good(host.run({.opened=[&]{
        auto model=std::make_shared<lvrs::ListModel>();take(model->set_rows({{"a","Layer 1"}}));take(model->select("a"));
        lvrs::List list;auto m=take(host.mount_widgets([&](gui::WidgetBuilder& b)->core::Result<void>{
            list=take(lvrs::List::create(b,{},"layers",{.model=model,.width=260,.height=140,.row_height=20,.background={36./255,38./255,37./255,1},.selection_color={36./255,50./255,77./255,1},.selection_radius=0,.font_size=11}));
            check(!lvrs::List::create(b,{},"bad-font",{.font_size=0}),"Invalid list font accepted");
            check(!lvrs::List::create(b,{},"bad",{.selection_radius=-1}),"Negative selection radius accepted");return {};}));
        auto image=take(host.capture_canvas());const auto logical=take(host.snapshot()).size;auto r=take(list.root().snapshot()).bounds;
        auto rgb=[&](double x,double y){const auto i=(std::size_t((r.y+y)/logical.height*image.height)*image.width+std::size_t((r.x+x)/logical.width*image.width))*4;return std::array<unsigned,3>{image.rgba[i],image.rgba[i+1],image.rgba[i+2]};};
        check(rgb(200,10)==std::array<unsigned,3>{36,50,77},"Authored selection color changed");
        check(rgb(200,80)==std::array<unsigned,3>{36,38,37},"Authored list background changed");
        good(host.request_close());
    }}));
}

}
int main(int argc,char** argv){try{check(argc==2,"Expected case");std::string test=argv[1];if(test=="model")model();else if(test=="view"){view(false);appearance();hidden_surface();}else if(test=="virtual")view(true);else throw std::runtime_error("Unknown list test");std::cout<<"LIST_OK "<<test<<'\n';return 0;}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
