#include "ViewModelDetail.h"
namespace lvrs {
ViewModel::ViewModel():impl_(std::make_shared<Impl>()){impl_->descriptor_signal=descriptor_signal();}
ViewModel::~ViewModel(){impl_->closed=true;impl_->receivers.clear();}
std::string ViewModel::key()const{auto m=impl_;m->readable();return m->key.get();}
std::string ViewModel::display_name()const{auto m=impl_;m->readable();return m->display_name.get();}
bool ViewModel::busy()const{auto m=impl_;m->readable();return m->busy.get();}
std::string ViewModel::error()const{auto m=impl_;m->readable();return m->error.get();}
bool ViewModel::has_error()const{auto m=impl_;m->readable();return !m->error.get().empty();}
ii::core::Value::Object ViewModel::metadata()const{auto m=impl_;m->readable();return m->metadata.get();}
ii::core::Value::Object ViewModel::snapshot()const{auto m=impl_;return m->snapshot();}
std::optional<ii::core::Value> ViewModel::read_property(std::string_view name)const{
    using ii::core::Value;
    if(name=="key")return Value(key());
    if(name=="displayName")return Value(display_name());
    if(name=="busy")return Value(busy());
    if(name=="error")return Value(error());
    if(name=="hasError")return Value(has_error());
    if(name=="metadata")return Value(metadata());
    auto m=impl_;m->readable();return {};
}
ii::core::Result<bool> ViewModel::write_property(std::string_view name,ii::core::Value value){
    auto m=impl_;if(auto ready=m->writable();!ready)return std::unexpected(ready.error());
    const auto accepted=[](ii::core::Result<bool> result)->ii::core::Result<bool>{if(!result)return std::unexpected(result.error());return true;};
    if(name=="key"||name=="displayName"||name=="error"){
        auto text=value.get<std::string>();if(!text)return ii::core::fail(ii::core::ErrorCode::invalid_argument,"ViewModel property requires text");
        return accepted(name=="key"?set_key(text->get()):name=="displayName"?set_display_name(text->get()):set_error(text->get()));
    }
    if(name=="busy"){
        auto flag=value.get<bool>();if(!flag)return ii::core::fail(ii::core::ErrorCode::invalid_argument,"ViewModel busy requires Bool");
        return accepted(set_busy(flag->get()));
    }
    if(name=="metadata"){
        auto object=value.get<ii::core::Value::Object>();if(!object)return ii::core::fail(ii::core::ErrorCode::invalid_argument,"ViewModel metadata requires Object");
        return accepted(set_metadata(object->get()));
    }
    return false;
}
ii::core::Value::Object ViewModel::model_descriptor()const{
    using ii::core::Value;auto result=snapshot();auto key=result.extract("key");result.emplace("viewModelKey",std::move(key.mapped()));
    result.emplace("className",Value("ViewModel"));result.emplace("viewModel",Value(true));result.emplace("stateModel",Value(false));return result;
}
ii::core::Result<bool> ViewModel::set_key(std::string value){auto m=impl_;if(auto r=m->writable();!r)return std::unexpected(r.error());return presentation_detail::attempt([&]{return m->assign(m->key,presentation_detail::normalized_text(value),ViewModelField::key);});}
ii::core::Result<bool> ViewModel::set_display_name(std::string value){auto m=impl_;if(auto r=m->writable();!r)return std::unexpected(r.error());return presentation_detail::attempt([&]{return m->assign(m->display_name,presentation_detail::normalized_text(value),ViewModelField::display_name);});}
ii::core::Result<bool> ViewModel::set_busy(bool value){auto m=impl_;return m->assign(m->busy,value,ViewModelField::busy);}
ii::core::Result<bool> ViewModel::set_error(std::string value){auto m=impl_;if(auto r=m->writable();!r)return std::unexpected(r.error());return presentation_detail::attempt([&]{return m->assign_error(std::move(value));});}
ii::core::Result<bool> ViewModel::set_metadata(ii::core::Value::Object value){auto m=impl_;if(auto r=m->writable();!r)return std::unexpected(r.error());return presentation_detail::attempt([&]{(void)ii::core::Value(value);return m->assign(m->metadata,std::move(value),ViewModelField::metadata);});}
ii::core::Result<bool> ViewModel::clear_error(){auto m=impl_;return m->assign_error("");}
ii::core::Result<void> ViewModel::observe(std::shared_ptr<const ViewModelChangePort> receiver){auto m=impl_;return m->observe(std::move(receiver));}
}
