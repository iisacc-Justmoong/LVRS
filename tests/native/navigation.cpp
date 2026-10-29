#include <LVRS/RouteMatcher.h>
#include <LVRS/RouteResolver.h>
#include <LVRS/NavigationStackModel.h>
#include <iostream>
#include <functional>
#include <stdexcept>
#include <thread>
using namespace ii;
namespace {
void check(bool condition,const char* message){if(!condition)throw std::runtime_error(message);}
template<class T>T take(core::Result<T> value){if(!value)throw std::runtime_error(value.error().message);return std::move(*value);}
void good(core::Result<void> value){if(!value)throw std::runtime_error(value.error().message);}
void run(const std::string& name){
    using lvrs::RouteMatcher;using lvrs::NavigationOperation;
    if(name=="normalize"){
        check(take(RouteMatcher::normalize_path("  reports/// \t"))=="/reports","Path normalization changed");
        check(take(RouteMatcher::normalize_path("\xc2\xa0\xe3\x80\x80"))=="/","Unicode whitespace did not normalize to root");
        check(take(RouteMatcher::normalize_path("/a//b?x=1#part"))=="/a//b?x=1#part","Matcher unexpectedly interpreted URL or internal separators");
        check(!RouteMatcher::normalize_path(std::string("/a\0b",4))&&!RouteMatcher::normalize_path("\xc0\xaf"),"Invalid path boundary accepted");
    }else if(name=="match"){
        auto matched=take(RouteMatcher::match("/reports/한글","/reports/[id]"));check(matched.matched&&matched.params.at("id")=="한글","Named parameter lost Unicode");
        matched=take(RouteMatcher::match("/logs/a/b","/logs/[...path]"));check(matched.matched&&matched.params.at("path")=="a/b","Rest parameter lost segments");
        matched=take(RouteMatcher::match("/logs","/logs/[...path]"));check(matched.matched&&matched.params.at("path").empty(),"Empty rest parameter did not match");
        check(!take(RouteMatcher::match("/reports","/reports/[id]")).matched&&!take(RouteMatcher::match("/a/b","/a")).matched,"Partial segment match accepted");
        check(!take(RouteMatcher::match("/a","/")).matched&&take(RouteMatcher::match("","/")).matched,"Root matching changed");
    }else if(name=="legacy"){
        auto m=take(RouteMatcher::match("/one/two","/[id]/[id]"));check(m.matched&&m.params.at("id")=="two","Duplicate capture did not keep the last value");
        m=take(RouteMatcher::match("/logs/a","/logs/[...rest]/ignored"));check(m.matched&&m.params.at("rest")=="a","Rest no longer terminated matching immediately");
        m=take(RouteMatcher::match("/a//b","/a/[]/b"));check(m.matched&&m.params.at("").empty(),"Empty segment/key compatibility changed");
    }else if(name=="resolver"||name=="cache"||name=="cache_budget"||name=="resolver_observation"){
        lvrs::RouteResolver resolver;take(resolver.set_route_patterns({"","/[id]","/fixed","/logs/[...path]"}));
        auto first=take(resolver.resolve("fixed/"));check(first->matched&&first->index==1&&first->params.at("id")=="fixed","Resolver did not preserve declaration order or source indices");
        if(name=="cache"){
            auto before=take(resolver.state());auto again=take(resolver.resolve(" /fixed "));auto after=take(resolver.state());
            check(first==again&&after.cache_hits==before.cache_hits+1&&after.pattern_matches==before.pattern_matches,"Cache hit rebuilt a result or rematched patterns");
            take(resolver.set_cache_capacity(0));check(take(resolver.state()).cache_capacity==16,"Cache minimum changed");
            for(unsigned i=0;i<20;++i)take(resolver.resolve("/entry"+std::to_string(i)));
            after=take(resolver.state());check(after.cached_entries==16&&after.cache_bytes<=lvrs::RouteLimits::cache_bytes,"Cache limits were not enforced");
            good(resolver.clear_cache());check(take(resolver.state()).cached_entries==0,"Cache clear retained entries");
        }else if(name=="cache_budget"){
            take(resolver.set_cache_capacity(65536));
            for(unsigned i=0;i<100;++i)take(resolver.resolve("/"+std::string(60000,'a')+std::to_string(i)));
            const auto state=take(resolver.state());check(state.cache_bytes<=lvrs::RouteLimits::cache_bytes&&state.cached_entries<100,"Cache byte budget did not evict large immutable results");
            const auto before=take(resolver.state()).revision;check(!resolver.set_route_patterns({std::string(lvrs::RouteLimits::path_bytes+1,'x')})&&take(resolver.state()).revision==before,"Invalid configuration replaced compiled routes");
        }else if(name=="resolver_observation"){
            int called=0;auto port=std::make_shared<lvrs::RouteResolverChangePort>([&](const auto& change){++called;check(change.patterns,"Resolver change flags lost");check(take(resolver.state()).revision==change.revision,"Observer saw stale resolver state");check(!resolver.set_cache_capacity(64),"Resolver accepted notification reentry");});
            good(resolver.observe(port));take(resolver.set_route_patterns({"/fixed"}));check(called==1,"Resolver change not delivered once");
            port.reset();take(resolver.set_route_patterns({"/other"}));
            auto failing=std::make_shared<lvrs::RouteResolverChangePort>([](const auto&){throw std::runtime_error("configuration receiver failed");});good(resolver.observe(failing));
            auto changed=resolver.set_route_patterns({"/committed"});check(!changed&&changed.error().cause&&take(resolver.resolve("/committed"))->matched,"Resolver receiver failure rolled back configuration");
        }else{
            take(resolver.set_route_patterns({"/fixed","/[id]"}));auto replaced=take(resolver.resolve("/fixed"));check(replaced->index==0&&replaced!=first&&first->params.at("id")=="fixed","Route replacement reused stale cache or invalidated an owned result");
            check(!take(resolver.set_route_patterns({"/fixed","/[id]"})),"Unchanged pattern list changed configuration");
            auto missing=take(resolver.resolve("/"));auto missing_again=take(resolver.resolve(""));check(missing==missing_again&&!missing->matched&&missing->index==-1&&missing->params.empty(),"Negative cache did not share its immutable result");
        }
    }else{
        lvrs::NavigationStackModel stack;
        if(name=="operations"){
            check(take(stack.state()).depth==0&&!take(stack.pop()),"Empty stack pop changed state");
            take(stack.apply_path_operation("home"));take(stack.apply_path_operation("/reports/42",core::Value{core::Value::Object{{"id",core::Value{42}}}}));
            check(take(stack.state()).depth==2&&take(stack.current_entry_descriptor()).path=="/reports/42","Push did not update current route");
            auto preview=take(stack.stack_after_path_operation("/preview",{},NavigationOperation::replace));check(preview.size()==2&&take(stack.current_entry_descriptor()).path=="/reports/42","Preview mutated the live stack");
            take(stack.apply_path_operation("/reports/43",{},NavigationOperation::replace));check(take(stack.state()).depth==2,"Replace pushed another entry");
            take(stack.pop());check(take(stack.current_entry_descriptor()).path=="/home"&&!take(stack.pop()),"Pop did not retain the root");
            take(stack.apply_path_operation("/new",{},NavigationOperation::set_root));check(take(stack.state()).depth==1&&take(stack.current_entry_descriptor()).path=="/new","Root operation did not replace stack");
            take(stack.set_path({}));check(take(stack.state()).depth==0&&take(stack.path_view()).empty(),"Explicit empty path did not clear the stack");
        }else if(name=="components"){
            auto component=std::make_shared<const int>(7);take(stack.apply_component_operation(component));
            auto current=take(stack.current_entry_descriptor());check(current.path.empty()&&current.component==component,"Compiled component identity was lost");
            auto tracking=take(stack.view_tracking_entries());check(tracking.size()==1&&tracking[0].view_id=="_component_0","Component tracking id changed");
            take(stack.apply_path_operation("/next"));take(stack.pop_to_root());check(take(stack.current_entry_descriptor()).component==component,"Root pop lost component ownership");
        }else if(name=="tracking"){
            std::vector<lvrs::NavigationEntry> entries{{.path="/same"},{.path="/same",.enabled=false},{.path="",.view_id="explicit",.disabled=true},{.path="/other",.params=core::Value{core::Value::Object{{"disabled",core::Value{true}}}}}};
            take(stack.set_path(entries));auto tracking=take(stack.view_tracking_entries());check(tracking.size()==4&&!tracking[1].enabled&&!tracking[2].enabled&&!tracking[3].enabled,"Tracking enable/disable policy changed");
            auto removed=take(stack.update_tracked_view_ids(tracking));check(removed.empty()&&take(stack.tracked_view_ids()).size()==3,"Tracked ids were not stable and unique");
            removed=take(stack.update_tracked_view_ids(std::span<const lvrs::NavigationViewEntry>{tracking.data(),1}));check(removed==std::vector<std::string>{"explicit","/other"},"Removed tracking ids lost prior order");
        }else if(name=="observation"){
            int called=0;auto port=std::make_shared<lvrs::NavigationChangePort>([&](const auto& change){++called;check(take(stack.state()).revision==change.revision&&change.path&&change.current&&change.tracking,"Observer saw inconsistent navigation state");check(!stack.apply_path_operation("/reentrant"),"Navigation mutation reentered notification");});
            good(stack.observe(port));take(stack.apply_path_operation("/one"));check(called==1,"Navigation change was not coalesced");
            port.reset();take(stack.apply_path_operation("/two"));
            auto failing=std::make_shared<lvrs::NavigationChangePort>([](const auto&){throw std::runtime_error("receiver failed");});good(stack.observe(failing));
            auto result=stack.apply_path_operation("/committed");check(!result&&result.error().cause&&take(stack.current_entry_descriptor()).path=="/committed","Receiver failure rolled back or escaped the committed Result boundary");
        }else if(name=="retirement"){
            auto model=std::make_shared<lvrs::NavigationStackModel>();auto raw=model.get();
            auto port=std::make_shared<lvrs::NavigationChangePort>([&](const auto&){model.reset();});good(raw->observe(port));
            check(take(raw->apply_path_operation("/retire"))&&!model,"Receiver could not retire the model safely");
            struct Component final {std::function<void()> on_destroy;~Component(){on_destroy();}};
            bool blocked=false;auto component=std::make_shared<Component>();component->on_destroy=[&]{auto state=stack.state();auto mutation=stack.apply_path_operation("/bad");blocked=!state&&state.error().code==core::ErrorCode::busy&&!mutation;};
            take(stack.apply_component_operation(component));component.reset();take(stack.apply_component_operation(std::make_shared<const int>(1),{},NavigationOperation::replace));
            check(blocked,"Component retirement reentered a partially committed stack");
        }else if(name=="growth"){
            std::uintptr_t previous=0;std::size_t growths=0;
            for(unsigned i=0;i<10000;++i){take(stack.apply_path_operation("/row"));const auto address=reinterpret_cast<std::uintptr_t>(take(stack.path_view()).data());if(address!=previous){++growths;previous=address;}}
            check(growths<32&&take(stack.state()).depth==10000,"Push repeatedly rebuilt or exactly reserved the entire stack");
            take(stack.pop_to_root());check(take(stack.state()).depth==1,"Large root pop lost history invariant");
        }else if(name=="ownership"){
            lvrs::RouteResolver resolver;bool rejected=false,resolver_rejected=false,pure=false;
            std::thread worker([&]{auto state=stack.state();rejected=!state&&state.error().code==core::ErrorCode::wrong_executor;auto configured=resolver.set_route_patterns({"/"});resolver_rejected=!configured&&configured.error().code==core::ErrorCode::wrong_executor;pure=take(RouteMatcher::match("/","/")).matched;});worker.join();check(rejected&&resolver_rejected&&pure,"Owner-thread models or pure matcher thread contract changed");
        }else if(name=="entry"){
            lvrs::NavigationEntry entry{.path="/native",.params=core::Value{core::Value::Object{}},.view_id="Canvas",.enabled=false};
            good(stack.validate_entry_operation(entry));check(take(stack.state()).depth==0,"Entry preflight changed history");
            take(stack.apply_entry_operation(entry));check(take(stack.path()).back()==entry&&take(stack.view_tracking_entries()).back().view_id=="Canvas"&&!take(stack.view_tracking_entries()).back().enabled,"A complete typed entry lost its identity or enabled state");
            const auto revision=take(stack.state()).revision;check(!take(stack.apply_entry_operation(entry,NavigationOperation::replace))&&take(stack.state()).revision==revision,"An identical full-entry replacement mutated history");
            entry.view_id=std::string(lvrs::NavigationLimits::view_id_bytes+1,'x');check(!stack.validate_entry_operation(entry)&&!stack.apply_entry_operation(entry)&&take(stack.state()).revision==revision,"Invalid entry identity passed admission or changed history");
        }else if(name=="limits"){
            take(stack.apply_path_operation("/safe"));auto before=take(stack.state());
            good(stack.validate_path_operation("/preview"));check(take(stack.state()).revision==before.revision,"Preflight mutated history");
            check(!stack.validate_path_operation("/bad",{},static_cast<NavigationOperation>(99))&&!stack.validate_component_operation(std::make_shared<const int>(1),core::Value{std::string(lvrs::NavigationLimits::entry_bytes,'x')}),"Preflight missed operation or parameter budgets");
            check(!stack.apply_path_operation(std::string(lvrs::RouteLimits::path_bytes+1,'a'))&&take(stack.state()).revision==before.revision,"Oversized route partially changed history");
            check(!stack.apply_path_operation("/bad",{},static_cast<NavigationOperation>(99))&&take(stack.state()).revision==before.revision,"Invalid operation partially changed history");
            core::Value value{std::string(lvrs::NavigationLimits::entry_bytes,'x')};check(!stack.apply_path_operation("/large",value)&&take(stack.state()).depth==1,"Oversized parameters partially changed history");
            auto empty=take(stack.create_path_entry(""));check(empty.path.empty()&&empty.params.kind()==core::ValueKind::object,"Empty typed path entry lost its explicit descriptor");
        }else throw std::runtime_error("Unknown native navigation case");
    }
}
}
int main(int argc,char** argv){try{run(argc>1?argv[1]:"match");std::cout<<"LVRS_NATIVE_NAVIGATION_OK\n";}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
