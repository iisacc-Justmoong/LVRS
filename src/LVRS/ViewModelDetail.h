#pragma once
#include "ViewModel.h"
#include "PresentationDetail.h"
namespace lvrs {
struct ViewModel::Impl final:presentation_detail::Signals<ViewModelChange> {
    ModelProperties::DescriptorSignal descriptor_signal;
    ii::core::Observable<std::string> key{""},display_name{""},error{""};
    ii::core::Observable<bool> busy{false};
    ii::core::Observable<ii::core::Value::Object> metadata{{}};
    template<class T>ii::core::Result<bool> assign(ii::core::Observable<T>& source,T value,ViewModelField field){
        if(auto r=writable();!r)return std::unexpected(r.error());if(source.get()==value)return false;
        presentation_detail::Flag guard{changing};std::optional<ii::core::Error> failure;const auto previous_revision=source.revision();
        try{source.set(std::move(value));}catch(...){failure=ii::core::Error{ii::core::ErrorCode::failed,source.revision()==previous_revision?"ViewModel property update failed before commit":"ViewModel property observation failed after commit",std::current_exception()};if(source.revision()==previous_revision)return std::unexpected(*failure);}
        {presentation_detail::Flag notifying_guard{notifying};send({field},failure);}
        if(auto result=descriptor_signal.notify();!result&&!failure)failure=std::move(result.error());
        if(failure)return std::unexpected(*failure);return true;
    }
    ii::core::Result<bool> assign_error(std::string value){return assign(error,presentation_detail::normalized_text(value),ViewModelField::error);}
    ii::core::Value::Object snapshot()const{
        readable();return {{"key",ii::core::Value(key.get())},{"displayName",ii::core::Value(display_name.get())},{"busy",ii::core::Value(busy.get())},
            {"error",ii::core::Value(error.get())},{"hasError",ii::core::Value(!error.get().empty())},{"metadata",ii::core::Value(metadata.get())}};
    }
};
}
