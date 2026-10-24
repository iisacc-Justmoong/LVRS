#include <LVRS/Widgets.h>
#include <iiGUI/Bindings.h>
#include <iiSDKCore/Core/Property/Observable.h>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace ii;
namespace {
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
template<class T>T take(core::Result<T> r){if(!r)throw std::runtime_error(r.error().message);return std::move(*r);}
void good(core::Result<void> r){if(!r)throw std::runtime_error(r.error().message);}
bool near(double a,double b){return std::abs(a-b)<.01;}
std::vector<lvrs::HierarchyToolbarEntry> entries(){std::vector<lvrs::HierarchyToolbarEntry> items(3);for(int i=0;i<3;++i){items[i].button_id=std::string("id-")+std::to_string(i);items[i].button.icon_name="projectStructure";items[i].item=core::Value(std::int64_t(i));}return items;}
void run(const std::string& name){
    gui::Host host({.size={800,500}});gui::WidgetMount mount;lvrs::ListToolbar list;lvrs::ListFooter footer;lvrs::HierarchyToolbar hierarchy;lvrs::ToolbarButton manual,other;unsigned clicks=0,changes=0;std::vector<std::string> order;std::optional<lvrs::ToolbarTrigger> saved;
    core::Observable<bool> enabled{true};auto bindings=std::make_shared<gui::BindingScope>();
    good(host.run({.opened=[&]{
        mount=take(host.mount_widgets([&](auto& b)->core::Result<void>{
            auto root=take(b.vstack({},"root",{.spacing=4,.alignment=gui::StackAlignment::leading}));good(b.retain(bindings));
            if(name.starts_with("list_")){
                lvrs::ListToolbarOptions o;if(name!="list_default")o.icons={"projectStructure","generaldelete","settings"};
                list=take(lvrs::ListToolbar::create(b,root,"list",o,{.icon_clicked=[&](std::size_t index,std::string source){check(index==0&&!source.empty(),"ListToolbar callback lost its slot/source");++clicks;if(name=="list_retirement")good(host.unmount_widgets());}}));
            }else if(name.starts_with("footer_")){
                lvrs::ListFooterOptions o;o.buttons[0].on_clicked=[&]{order.push_back("slot");if(name=="footer_reentrance")good(host.unmount_widgets());};
                footer=take(lvrs::ListFooter::create(b,root,"footer",o,{.button_clicked=[&](std::size_t index,const lvrs::ListFooterSlot& slot){check(index==0&&slot.button.icon_name=="addFile","Footer callback lost config");order.push_back("footer");}}));
            }else if(name=="toolbar_standalone")manual=take(lvrs::ToolbarButton::create(b,root,"standalone",{},[&]{++clicks;}));
            else{
                auto items=name.starts_with("model_")?entries():std::vector<lvrs::HierarchyToolbarEntry>{};
                if(name=="model_selected")items[1].selected=true;
                if(name=="model_hidden"){items[1].button.visible=false;items[1].selected=true;}
                if(name=="model_events"||name=="model_context"||name=="model_reentrance"||name=="model_retirement"){
                    items[1].events={{"first",core::Value(42)},{"second",core::Value("payload")}};
                    items[1].on_triggered=[&](const lvrs::ToolbarTrigger& context){order.push_back("entry");saved=context;if(name=="model_events")good(context.emit("custom",core::Value(true)));};
                }
                lvrs::HierarchyToolbarOptions o;if(name=="model_spacing"){o.width=400;o.distribute_spacing=true;}
                hierarchy=take(lvrs::HierarchyToolbar::create(b,root,"hierarchy",o,std::move(items),{
                    .active_changed=[&](const auto&){++changes;order.push_back("active");},
                    .button_triggered=[&](const auto& context){order.push_back("trigger");saved=context;if(name=="model_reentrance")good(hierarchy.set_items({}));if(name=="model_retirement")good(host.unmount_widgets());},
                    .button_event_triggered=[&](const auto& event,const auto&){if(name=="model_context"&&event.name=="throw")throw std::runtime_error("Intentional event failure");order.push_back(event.name);}}));
                manual=take(lvrs::ToolbarButton::create(b,hierarchy.root(),"manual",{.button_id=std::string("manual"),.icon_name="projectStructure"},[&]{++clicks;}));
                other=take(lvrs::ToolbarButton::create(b,hierarchy.root(),"other",{.button_id=std::string("other"),.icon_name="settings"}));
                good(hierarchy.register_button(manual,false));good(hierarchy.register_button(other));
                if(name=="toolbar_bindings")good(gui::bind_member(*bindings,manual.root(),enabled,[manual](auto,bool value)->core::Result<void>{auto o=manual.options();if(!o)return std::unexpected(o.error());o->enabled=value;return manual.set_options(std::move(*o));}));
                if(name=="toolbar_registration"){check(!hierarchy.register_button(take(lvrs::ToolbarButton::create(b,root,"outside"))),"Foreign manual child accepted");good(hierarchy.register_button(manual));}
            }
            return {};
        }));
        if(name.starts_with("list_")){
            auto s=take(list.root().snapshot());check(near(s.implicit_size.width,90)&&near(s.implicit_size.height,30),"ListToolbar natural metrics changed");auto button=take(list.button_at(0));
            if(name=="list_default"){good(button.activate());check(!take(button.snapshot()).enabled&&clicks==0,"Blank icon slot remained actionable");}
            else if(name=="list_slots"){check(!list.button_at(-1)&&!list.button_at(3)&&take(list.resolved_icon(0))==lvrs::Icons::normalize("projectStructure"),"ListToolbar index/resolution contract changed");}
            else if(name=="list_update"){auto o=take(list.options());o.icons[0]="settings";o.button_size=28;good(list.set_options(o));check(near(take(button.snapshot()).bounds.width,28)&&take(list.resolved_icon(0))==lvrs::Icons::normalize("settings"),"ListToolbar update replaced its button or lost icon metrics");}
            else if(name=="list_invalid"){auto o=take(list.options());o.icons[2]="not-an-embedded-lvrs-icon";check(!list.set_options(o)&&take(list.options()).icons[2]=="settings","Invalid icon partially committed");o=take(list.options());o.button_size=-1;check(!list.set_options(o),"Negative ListToolbar size accepted");}
            else if(name=="list_events"||name=="list_retirement"){good(button.activate());check(clicks==1,"ListToolbar event was not delivered");if(name=="list_retirement")check(!list.options(),"ListToolbar held a retired generation");else{auto o=take(list.options());o.interactive=false;good(list.set_options(o));good(button.activate());check(clicks==1&&!take(button.snapshot()).enabled,"Noninteractive ListToolbar remained actionable");}}
            else throw std::runtime_error("Unknown list toolbar case");
        }else if(name.starts_with("footer_")){
            auto s=take(footer.root().snapshot());check(near(s.implicit_size.width,88)&&near(s.implicit_size.height,26),"ListFooter default icon/menu metrics changed");auto button=take(footer.button_at(0));
            if(name=="footer_slots"){auto o=take(footer.options());o.buttons[1].button.visible=false;good(footer.set_options(o));check(near(take(footer.root().snapshot()).implicit_size.width,66)&&!take(take(footer.button_at(1)).snapshot()).visible,"Hidden footer slot kept a layout gap");}
            else if(name=="footer_update"){auto slot=take(footer.options()).buttons[0];slot.button.menu=true;slot.button.left_padding=4;slot.button.content_spacing=-2;good(footer.set_slot(0,slot));check(near(take(button.snapshot()).implicit_size.width,40),"Footer live menu update lost retained geometry");}
            else if(name=="footer_events"||name=="footer_reentrance"){good(button.activate());check(order==(name=="footer_events"?std::vector<std::string>{"slot","footer"}:std::vector<std::string>{"slot"}),"Footer callback order or retirement guard changed");}
            else if(name=="footer_invalid"){auto o=take(footer.options());o.buttons[2].button.icon_size=0;check(!footer.set_options(o)&&take(footer.options()).buttons[2].button.icon_size==18,"Invalid footer slot partially committed");check(!footer.set_slot(3,{}),"Invalid footer index accepted");}
            else if(name!="footer_default")throw std::runtime_error("Unknown footer case");
        }else if(name=="toolbar_standalone"){check(!take(manual.active()),"Standalone toolbar button is active");good(manual.root().activate());check(clicks==1,"Standalone toolbar action lost");}
        else if(name.starts_with("toolbar_")){
            check(take(hierarchy.button_count())==2&&take(manual.root().snapshot()).parent_id=="hierarchy/manual","Manual toolbar registration or default slot failed");
            if(name=="toolbar_manual"){good(manual.root().activate());check(clicks==1&&take(hierarchy.selection()).index==0&&take(manual.active()),"Manual activation did not share the native source");good(other.root().activate());check(take(hierarchy.selection()).index==1&&!take(manual.active())&&take(other.active()),"Manual active tone did not move");}
            else if(name=="toolbar_manual_id"){good(hierarchy.set_active_id(std::string("other")));check(take(hierarchy.selection()).index==1&&changes==0,"Programmatic normalization emitted activeChanged or lost ID");}
            else if(name=="toolbar_disabled"||name=="toolbar_bindings"){if(name=="toolbar_bindings")enabled.set(false);else{auto o=take(manual.options());o.enabled=false;good(manual.set_options(o));}check(take(hierarchy.selection()).index==1&&!take(hierarchy.trigger_index(0)),"Disabled manual source remained selected/actionable");}
            else if(name!="toolbar_registration")throw std::runtime_error("Unknown manual toolbar case");
        }else{
            check(take(hierarchy.button_count())==3,"Model toolbar lost its entries");
            if(name=="model_default")check(take(hierarchy.selection()).index==0&&changes==0,"Initial normalization emitted activation");
            else if(name=="model_selected")check(take(hierarchy.selection()).index==1,"Selected entry was not initial active source");
            else if(name=="model_normalize"){good(hierarchy.set_active_id(std::string("id-2")));check(take(hierarchy.selection()).index==2&&changes==0,"Stable ID normalization emitted click/activeChanged");good(hierarchy.set_active_id(std::string("missing")));check(take(hierarchy.selection()).index==2,"Unknown ID lost the existing active source");}
            else if(name=="model_events"){check(take(hierarchy.trigger_index(1)),"Model trigger rejected");check(order==std::vector<std::string>{"active","trigger","first","second","entry","custom"},"Toolbar event/callback order changed");check(saved&&saved->button_id==lvrs::ToolbarId{std::string("id-1")},"Toolbar trigger context lost source ID");}
            else if(name=="model_same_ids"){auto retained=take(hierarchy.button_at(0));auto next=entries();next[0].button.icon_name="settings";good(hierarchy.set_items(next));check(retained.snapshot().has_value()&&take(retained.snapshot()).icon_name==lvrs::Icons::normalize("settings"),"Same-ID update rebuilt or failed to refresh a model button");}
            else if(name=="model_reorder"){good(hierarchy.set_active_id(std::string("id-1")));auto old=take(hierarchy.button_at(0));auto next=entries();std::swap(next[0],next[1]);good(hierarchy.set_items(next));check(!old.snapshot()&&take(hierarchy.selection()).button_id==lvrs::ToolbarId{std::string("id-1")}&&take(hierarchy.selection()).index==0,"Reorder did not retire old nodes or retain stable active ID");}
            else if(name=="model_fallback"){auto next=entries();for(auto& e:next)e.button.enabled=false;good(hierarchy.set_items(next));check(take(hierarchy.selection()).index==0&&!take(hierarchy.trigger_index(0)),"All-disabled fallback did not match the original selection contract");for(auto& e:next){e.button.enabled=true;e.button.tone=gui::ButtonTone::disabled;}good(hierarchy.set_items(next));check(!take(take(hierarchy.button_at(0)).snapshot()).enabled,"Active fallback overrode a disabled tone");}
            else if(name=="model_hidden")check(take(hierarchy.visible_button_count())==2&&take(hierarchy.selection()).index==1&&take(hierarchy.trigger_index(1)),"Hidden model entry lost its original programmatic activation contract");
            else if(name=="model_spacing")check(near(take(hierarchy.distributed_spacing()),159),"Distributed model spacing did not use the actual viewport");
            else if(name=="model_switch"){good(hierarchy.set_items({}));check(take(hierarchy.button_count())==2&&manual.root().snapshot().has_value()&&take(manual.root().snapshot()).visible,"Returning to manual mode rebuilt or hid manual sources");good(hierarchy.set_items(entries()));check(!take(manual.root().snapshot()).visible,"Model mode did not hide the retained manual slot");}
            else if(name=="model_failure"){auto old=take(hierarchy.button_at(0));auto next=entries();next[1].button.icon_name="not-an-embedded-lvrs-icon";check(!hierarchy.set_items(next)&&old.snapshot().has_value()&&take(hierarchy.button_count())==3,"Failed model validation retired the existing toolbar");}
            else if(name=="model_context"){check(take(hierarchy.trigger_index(1))&&saved.has_value(),"Missing event context");auto failed=saved->emit("throw",{});check(!failed&&failed.error().cause,"Saved event callback exception escaped its Result boundary");good(hierarchy.set_items(entries()));check(!saved->activate(0)&&!saved->emit("late",{}),"Retired context targeted a newer model revision");}
            else if(name=="model_reentrance"){check(take(hierarchy.trigger_index(1))&&take(hierarchy.button_count())==2&&order==std::vector<std::string>{"active","trigger"},"Model replacement continued obsolete event callbacks");}
            else if(name=="model_retirement"){check(take(hierarchy.trigger_index(1))&&!hierarchy.options()&&order==std::vector<std::string>{"active","trigger"},"Retirement did not stop source callbacks safely");}
            else if(name=="model_limits"){auto next=entries();next.resize(257);check(!hierarchy.set_items(next),"Toolbar entry budget ignored");next=entries();next[0].events.resize(65);check(!hierarchy.set_items(next),"Toolbar event budget ignored");auto o=take(hierarchy.options());o.slot_size=4000;check(!hierarchy.set_options(o)&&take(hierarchy.options()).slot_size==22,"Excess aggregate extent partially committed");o=take(hierarchy.options());o.background_opacity=std::numeric_limits<double>::quiet_NaN();check(!hierarchy.set_options(o),"Invalid background opacity accepted");}
            else throw std::runtime_error("Unknown model toolbar case");
        }
        good(host.request_close());
    }}));
}
}
int main(int argc,char** argv){try{run(argc>1?argv[1]:"model_default");std::cout<<"LVRS_TOOLBARS_OK\n";}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
