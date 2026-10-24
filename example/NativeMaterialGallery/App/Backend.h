#pragma once
#include <LVRS/MaterialSurface.h>
#include <iiSDKCore/Core/Property/Observable.h>
struct MaterialModel final {
    ii::core::Observable<ii::gui::WidgetColor> accent{ii::gui::WidgetColor{0,.4,1,1}};
    ii::core::Observable<lvrs::MaterialDensity> density{lvrs::MaterialDensity::glass25};
    ii::core::Observable<bool> blur{true};
    ii::core::Observable<double> width{420};
    ii::core::Observable<std::string> status{"Background result: pending"};
    std::shared_ptr<ii::gui::MotionPreferences> motion=std::make_shared<ii::gui::MotionPreferences>();
};
class MaterialBackend final {
public:
    inline static unsigned created=0,retired=0;
    std::shared_ptr<MaterialModel> model=std::make_shared<MaterialModel>();
    ii::core::Observable<ii::gui::WidgetColor>& accent=model->accent;
    ii::core::Observable<lvrs::MaterialDensity>& density=model->density;
    ii::core::Observable<bool>& blur=model->blur;
    ii::core::Observable<double>& width=model->width;
    ii::core::Observable<std::string>& status=model->status;
    std::shared_ptr<ii::gui::MotionPreferences> motion=model->motion;
    std::function<void()> on_close;
    MaterialBackend(){++created;}~MaterialBackend(){++retired;}
    MaterialBackend(const MaterialBackend&)=delete;MaterialBackend& operator=(const MaterialBackend&)=delete;
    void change_accent();void change_density();void change_blur();void close();
};
