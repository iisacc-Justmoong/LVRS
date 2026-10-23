#pragma once
#include <LVRS/Motion.h>
#include <functional>
#include <stdexcept>
#include <string>
class SheetBackend final {
public:
    inline static int created=0,destroyed=0;
    std::shared_ptr<lvrs::Motion> motion=std::make_shared<lvrs::Motion>();
    std::function<void()> on_desktop,on_mobile,on_opened,on_closed,on_apply,on_close,on_motion_changed;
    std::function<void(std::string)> on_edited;
    SheetBackend(){++created;}~SheetBackend(){++destroyed;}
    void desktop(){if(on_desktop)on_desktop();}void mobile(){if(on_mobile)on_mobile();}
    void opened(){if(on_opened)on_opened();}void closed(){if(on_closed)on_closed();}
    void apply(){if(on_apply)on_apply();}void close(){if(on_close)on_close();}
    void edited(std::string text){if(on_edited)on_edited(std::move(text));}
    void reduce_motion(){auto state=motion->state();if(!state)throw std::runtime_error(state.error().message);auto r=motion->set_reduced_motion(!state->reduced_motion);if(!r)throw std::runtime_error(r.error().message);if(on_motion_changed)on_motion_changed();}
    void motion_speed(){auto state=motion->state();if(!state)throw std::runtime_error(state.error().message);auto r=motion->set_speed(state->speed==1?2:1);if(!r)throw std::runtime_error(r.error().message);if(on_motion_changed)on_motion_changed();}
};
