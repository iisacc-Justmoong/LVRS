#pragma once
#include "ProgressModel.h"
#include "Motion.h"
#include <concepts>
#include <memory>
#include <stdexcept>
#include <type_traits>
namespace lvrs {
enum class ProgressSize {large,regular};
enum class ProgressShape {round_rect,cylinder};
// Declarative sources keep their existing shared ownership. An inline scoped
// source borrows its activation owner; the control never owns that activation.
template<class Owner,class T> requires std::derived_from<T,StateSource>
std::weak_ptr<StateSource> progress_source(const std::shared_ptr<Owner>&,const std::shared_ptr<T>& source){return source;}
template<class Owner,class T> requires std::derived_from<T,StateSource>
std::weak_ptr<StateSource> progress_source(const std::shared_ptr<Owner>&,const std::weak_ptr<T>& source){return source;}
template<class Owner,class T> requires std::derived_from<T,StateSource>
std::weak_ptr<StateSource> progress_source(const std::shared_ptr<Owner>& owner,T& source){return std::shared_ptr<StateSource>(owner,&source);}
template<class Owner>std::weak_ptr<StateSource> progress_source(const std::shared_ptr<Owner>&,std::nullptr_t){return {};}
template<class T>ProgressSize progress_size(T value){
    static_assert(std::same_as<T,ProgressSize>||(std::integral<T>&&!std::same_as<T,bool>),"ProgressBar size requires ProgressSize or an integer");
    if(value==static_cast<T>(0))return ProgressSize::large;
    if(value==static_cast<T>(1))return ProgressSize::regular;
    throw std::invalid_argument("Unknown ProgressBar size");
}
template<class T>ProgressShape progress_shape(T value){
    static_assert(std::same_as<T,ProgressShape>||(std::integral<T>&&!std::same_as<T,bool>),"ProgressBar shape requires ProgressShape or an integer");
    if(value==static_cast<T>(0))return ProgressShape::round_rect;
    if(value==static_cast<T>(1))return ProgressShape::cylinder;
    throw std::invalid_argument("Unknown ProgressBar shape");
}
struct ProgressBarOptions final {
    ProgressValues values;
    ProgressStateKeys state_keys;
    std::weak_ptr<StateSource> state_model;
    ProgressSize size=ProgressSize::large;
    ProgressShape shape=ProgressShape::round_rect;
    double width=100;
    std::optional<double> height;
    double large_height=6,regular_height=3,corner_radius=100;
    ii::gui::WidgetColor track_color{0,0,0,13./255};
    std::optional<ii::gui::WidgetColor> fill_color;
    bool motion_enabled=true;
    std::shared_ptr<Motion> motion;
    std::optional<double> displayed_start,displayed_progress;
};
struct ProgressBarState final {
    ProgressSnapshot progress;
    bool using_state_model=false,animating=false;
    std::uint64_t state_revision=0;
    double bar_height=6,displayed_start=0,displayed_progress=0;
    ii::gui::Size implicit_size{100,6};
};
// One retained C++ control. Geometry/paint use the allocated native bounds;
// source and motion updates neither reconstruct the widget nor mutate files.
class ProgressBar final {
public:
    ProgressBar()=default;
    [[nodiscard]] static ii::core::Result<ProgressBar> create(ii::gui::WidgetBuilder&,ii::gui::Widget,std::string_view,ProgressBarOptions={});
    [[nodiscard]] ii::gui::Widget root()const noexcept{return root_;}
    [[nodiscard]] ii::core::Result<ProgressBarOptions> options()const;
    [[nodiscard]] ii::core::Result<ProgressBarState> state()const;
    [[nodiscard]] ii::core::Result<std::shared_ptr<const ii::gui::WidgetDrawing>> drawing(ii::gui::Size)const;
    [[nodiscard]] ii::core::Result<std::shared_ptr<ProgressModel>> model()const;
    [[nodiscard]] ii::core::Result<void> set_options(ProgressBarOptions)const;
    [[nodiscard]] ii::core::Result<double> state_number(std::string_view,double fallback)const;
    [[nodiscard]] ii::core::Result<double> normalized_value(double)const;
    [[nodiscard]] ii::core::Result<double> resolved_radius(double width,double height)const;
private:
    struct Impl;
    [[nodiscard]] ii::core::Result<std::shared_ptr<Impl>> lock()const;
    ii::gui::Widget root_;
    std::weak_ptr<Impl> state_;
};
}
