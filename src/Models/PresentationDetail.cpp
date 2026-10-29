#include <LVRS/PresentationDetail.h>
#include <LVRS/RoutingDetail.h>
#include <iiSDKCore/Core/Text/Number.h>
#include <iiSDKCore/Core/Text/Text.h>
#include <cmath>
namespace lvrs::presentation_detail {
std::string normalized_text(std::string_view text){
    if(!ii::core::valid_utf8(text))throw std::invalid_argument("Presentation text requires UTF-8");
    return std::string(routing_detail::trim(text));
}
std::vector<std::string> ordered_keys(const ii::core::Value::Object& values){
    std::vector<std::pair<std::u16string,std::string>> ordered;ordered.reserve(values.size());
    for(const auto& [key,value]:values){(void)value;auto utf16=ii::core::to_utf16(key);if(!utf16)throw std::invalid_argument("State keys require UTF-8");ordered.emplace_back(std::move(*utf16),key);}
    std::sort(ordered.begin(),ordered.end());std::vector<std::string> result;result.reserve(ordered.size());for(auto& [code,key]:ordered){(void)code;result.push_back(std::move(key));}return result;
}
ii::core::Value::Object normalized_values(const ii::core::Value::Object& values){
    ii::core::Value::Object result;for(const auto& key:ordered_keys(values)){auto normalized=normalized_text(key);if(!normalized.empty())result.insert_or_assign(std::move(normalized),values.at(key));}return result;
}
double number(const ii::core::Value& value,double fallback){
    return std::visit([fallback](const auto& data)->double{
        using T=std::remove_cvref_t<decltype(data)>;
        if constexpr(std::is_arithmetic_v<T>){const double n=static_cast<double>(data);return std::isfinite(n)?n:fallback;}
        else if constexpr(std::same_as<T,std::string>||std::same_as<T,ii::core::Bytes>){
            const auto text=[&]{if constexpr(std::same_as<T,std::string>)return std::string_view(data);else return std::string_view(reinterpret_cast<const char*>(data.data()),data.size());}();
            if(!ii::core::valid_utf8(text))return fallback;auto normalized=routing_detail::trim(text);if(normalized.starts_with('+'))normalized.remove_prefix(1);
            auto n=ii::core::parse_decimal(normalized);return n&&std::isfinite(*n)?*n:fallback;
        }else return fallback;
    },value.storage());
}
bool fuzzy_equal(double a,double b){return std::abs(a-b)*1e12<=std::min(std::abs(a),std::abs(b));}
}
