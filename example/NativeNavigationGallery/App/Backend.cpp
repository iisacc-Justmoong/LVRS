#include "Backend.h"
#include <stdexcept>

NavigationGalleryBackend::NavigationGalleryBackend(){
    ++created;
    check(resolver.set_route_patterns({"/","/reports/[id]","/logs/[...path]"}));
    root();
}
NavigationGalleryBackend::~NavigationGalleryBackend(){++destroyed;}
void NavigationGalleryBackend::report(){navigate("/reports/42",lvrs::NavigationOperation::push);}
void NavigationGalleryBackend::replace_report(){navigate("/reports/43",lvrs::NavigationOperation::replace);}
void NavigationGalleryBackend::logs(){navigate("/logs/build/native",lvrs::NavigationOperation::push);}
void NavigationGalleryBackend::back(){check(model->pop());}
void NavigationGalleryBackend::root(){navigate("/",lvrs::NavigationOperation::set_root);}
void NavigationGalleryBackend::close(){if(on_close)on_close();}
void NavigationGalleryBackend::navigate(std::string_view path,lvrs::NavigationOperation operation){
    auto result=resolver.resolve(path);
    if(!result)throw std::runtime_error(result.error().message);
    if(!(*result)->matched)throw std::runtime_error("Gallery route was not found");
    ii::core::Value::Object params;
    for(const auto& [key,value]:(*result)->params)params.emplace(key,ii::core::Value{value});
    check(model->apply_path_operation(path,ii::core::Value{std::move(params)},operation));
}
void NavigationGalleryBackend::check(ii::core::Result<bool> result){
    if(!result)throw std::runtime_error(result.error().message);
}
