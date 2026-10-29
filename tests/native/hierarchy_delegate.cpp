#include <LVRS/HierarchyList.h>
#include <iiGUI/WidgetState.h>
#include <iostream>
#include <thread>
using namespace ii;
namespace {
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
template<class T>T take(core::Result<T> value){if(!value){if(value.error().cause)std::rethrow_exception(value.error().cause);throw std::runtime_error(value.error().message);}return std::move(*value);}
void good(core::Result<void> value){if(!value){if(value.error().cause)std::rethrow_exception(value.error().cause);throw std::runtime_error(value.error().message);}}
struct Surface final {
    std::shared_ptr<gui::detail::WidgetState> tree=std::make_shared<gui::detail::WidgetState>([](runtime::Task task){task();});
    gui::Widget host=take(tree->content_host({},"host",{400,240}));~Surface(){tree->close();}
};
lvrs::HierarchyItemOptions source(){lvrs::HierarchyItemOptions value;value.row.key="one";value.row.label="One";return value;}
struct Fixture final {
    Surface surface;lvrs::HierarchyDelegateContext context;lvrs::HierarchyItem row;
    std::vector<core::ObservableSubscription> leases;
    Fixture(lvrs::HierarchyItemEvents inherited={},lvrs::HierarchyItemEvents own={},lvrs::HierarchyCountFactory count={}){
        context=take(lvrs::HierarchyDelegateContext::create(source(),std::move(inherited)));
        good(surface.host.set_content([&](auto& builder)->core::Result<void>{row=take(context.create_item(builder,{},"item",source(),std::move(own),std::move(count)));return {};}));
    }
    void bind(){
        const auto render=[this]{auto value=take(context.options());value.left_padding=4;value.row.label="View "+value.row.label;good(context.apply(value));};
        core::ObservableReadScope::collect([&](auto read){leases.push_back(read.subscribe(render));},render);
    }
};
void defaults(){auto context=take(lvrs::HierarchyDelegateContext::create(source()));check(take(context.options()).row.key=="one"&&!take(context.state()).has_item&&!context.item(),"A delegate fabricated a product or lost source state");}
void invalid(){auto value=source();value.width=0;check(!lvrs::HierarchyDelegateContext::create(value),"Invalid geometry entered a delegate");value=source();value.row.key.clear();check(!lvrs::HierarchyDelegateContext::create(value),"A delegate accepted an unstable empty key");}
void product(){Fixture f;check(take(f.context.state()).has_item&&take(take(f.context.item()).options()).row.label=="One","A native delegate did not publish its own row product");auto result=f.surface.host.set_content([&](auto& b)->core::Result<void>{auto next=f.context.create_item(b,{},"second",source());return next?core::Result<void>{}:std::unexpected(next.error());});check(!result&&f.row.root().snapshot(),"A second product was admitted or destroyed the published row");}
void live(){Fixture f;f.bind();auto next=source();next.row.label="Changed";next.row.selected=true;take(f.context.update_source(next));auto value=take(f.row.options());check(value.row.label=="View Changed"&&value.row.selected&&value.left_padding==4&&take(f.context.state()).applied_revision==take(f.context.state()).revision,"A source update froze custom style or logical row state");}
void normalize(){Fixture f;auto value=take(f.context.options());value.row.key="wrong";value.row.item_id=8;value.row.depth=9;value.width=999;value.row_height=90;value.row.selected=true;value.row.expanded=true;value.row.label="Custom";good(f.context.apply(value));value=take(f.row.options());check(value.row.key=="one"&&value.row.item_id==-1&&value.row.depth==0&&value.width==200&&value.row_height==20&&!value.row.selected&&!value.row.expanded&&value.row.label=="Custom","Delegate styling replaced list-owned identity or geometry");}
void capabilities(){auto initial=source();initial.row.enabled=initial.row.activatable=initial.row.draggable=false;auto context=take(lvrs::HierarchyDelegateContext::create(initial));Surface s;lvrs::HierarchyItem row;good(s.host.set_content([&](auto& b)->core::Result<void>{row=take(context.create_item(b,{},"item",source()));return {};}));auto value=take(row.options());check(!value.row.enabled&&!value.row.activatable&&!value.row.draggable,"Delegate styling enabled a source-disabled capability");}
void retry(){Fixture f;auto next=source();next.row.label="Later";auto result=f.context.update_source(next);check(!result&&take(f.row.options()).row.label=="One"&&take(f.context.state()).applied_revision!=take(f.context.state()).revision,"An unrendered source revision was silently acknowledged");f.bind();check(take(f.row.options()).row.label=="View Later","A rejected source revision could not be rendered later");take(f.context.update_source(next));}
void legacy(){Surface s;auto context=take(lvrs::HierarchyDelegateContext::create(source()));lvrs::HierarchyItem row;good(s.host.set_content([&](auto& b)->core::Result<void>{auto value=source();value.left_padding=4;value.icon_size=0;row=take(lvrs::HierarchyItem::create(b,{},"item",value));good(context.adopt_legacy(row));return {};}));auto next=source();next.row.label="New";next.row.selected=true;next.right_padding=12;take(context.update_source(next));auto value=take(row.options());check(value.left_padding==4&&value.icon_size==0&&value.right_padding==12&&value.row.label=="New"&&value.row.selected,"Legacy delegate overrides were overwritten or inherited state was frozen");}
void identity(){Fixture f;auto next=source();next.row.key="other";auto result=f.context.update_source(next);check(!result&&result.error().code==core::ErrorCode::invalid_argument&&take(f.context.options()).row.key=="one","A delegate source changed stable keys");}
void reentrant(){Fixture f;bool blocked=false;core::ObservableReadScope::collect([&](auto read){f.leases.push_back(read.subscribe([&]{auto result=f.context.update_source(source());blocked=!result&&result.error().code==core::ErrorCode::busy;good(f.context.apply(take(f.context.options())));}));},[&]{take(f.context.options());});auto next=source();next.row.label="New";take(f.context.update_source(next));check(blocked,"Delegate notification admitted recursive source mutation");}
void events(){std::vector<std::string> order;Fixture f({.activated=[&]{order.push_back("source");}}, {.activated=[&]{order.push_back("view");}});good(f.row.activate());check(order==std::vector<std::string>{"source","view"},"Delegate activation replaced or duplicated the list command");}
void event_error(){int own=0;Fixture f({.activated=[]{throw std::runtime_error("Source event failed");}}, {.activated=[&]{++own;}});auto result=f.row.activate();check(!result&&own==1,"An inherited event error discarded the user callback");}
void event_retirement(){Surface s;int own=0;auto context=take(lvrs::HierarchyDelegateContext::create(source(),{.activated=[&]{good(s.host.clear_content());}}));lvrs::HierarchyItem row;good(s.host.set_content([&](auto& b)->core::Result<void>{row=take(context.create_item(b,{},"item",source(),{.activated=[&]{++own;}}));return {};}));good(row.activate());check(own==0&&take(context.state()).closed,"A retired delegate continued its user callback");}
void retirement(){Fixture f;good(f.surface.host.clear_content());check(take(f.context.state()).closed&&!f.context.options()&&!f.context.item()&&!f.context.apply(source()),"Widget retirement retained an active delegate context");}
void ownership(){Fixture f;bool blocked=false;std::thread other([&]{auto result=f.context.options();blocked=!result&&result.error().code==core::ErrorCode::wrong_executor;});other.join();check(blocked,"Delegate input crossed its owner executor");}
void close(){Fixture f;good(f.context.close());good(f.context.close());check(take(f.context.state()).closed&&!f.context.update_source(source())&&f.row.root().snapshot(),"Closing the view context destroyed someone else's widget or admitted source updates");}
}
int main(int argc,char** argv){try{const std::string name=argc>1?argv[1]:"defaults";if(name=="defaults")defaults();else if(name=="invalid")invalid();else if(name=="product")product();else if(name=="live")live();else if(name=="normalize")normalize();else if(name=="capabilities")capabilities();else if(name=="retry")retry();else if(name=="legacy")legacy();else if(name=="identity")identity();else if(name=="reentrant")reentrant();else if(name=="events")events();else if(name=="event_error")event_error();else if(name=="event_retirement")event_retirement();else if(name=="retirement")retirement();else if(name=="ownership")ownership();else if(name=="close")close();else throw std::runtime_error("Unknown hierarchy delegate case");std::cout<<"Native hierarchy delegate "<<name<<" passed\n";}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
