#pragma once
#include <LVRS/Motion.h>
#include <stdexcept>
#include <functional>
#include <string>
class ModalBackend final {
public:
    inline static int created=0,destroyed=0;
    std::function<void()> on_open,on_outside,on_primary,on_secondary,on_tertiary,on_canceled,on_opened,on_closed,on_close;
    std::function<void(std::string)> on_edited;
    std::function<void(double)> on_moved;
    std::function<void()> on_motion_changed;
    std::shared_ptr<lvrs::Motion> motion=std::make_shared<lvrs::Motion>();
    ModalBackend(){++created;}~ModalBackend(){++destroyed;}
    void open(){if(on_open)on_open();}void outside(){if(on_outside)on_outside();}
    void primary(){if(on_primary)on_primary();}void secondary(){if(on_secondary)on_secondary();}void tertiary(){if(on_tertiary)on_tertiary();}
    void canceled(){if(on_canceled)on_canceled();}void opened(){if(on_opened)on_opened();}void closed(){if(on_closed)on_closed();}void close(){if(on_close)on_close();}
    void edited(std::string value){if(on_edited)on_edited(std::move(value));}void moved(double value){if(on_moved)on_moved(value);}
    void reduce_motion(){auto state=motion->state();if(!state)throw std::runtime_error(state.error().message);auto result=motion->set_reduced_motion(!state->reduced_motion);if(!result)throw std::runtime_error(result.error().message);if(on_motion_changed)on_motion_changed();}
    void motion_speed(){auto state=motion->state();if(!state)throw std::runtime_error(state.error().message);auto result=motion->set_speed(state->speed==1?2:1);if(!result)throw std::runtime_error(result.error().message);if(on_motion_changed)on_motion_changed();}
};
