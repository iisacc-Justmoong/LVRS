#pragma once
#include <LVRS/Tooltip.h>
#include <iiSDKCore/Core/Property/Observable.h>
#include <iiSDKCore/Runtime/Application.h>
struct TooltipSession final:std::enable_shared_from_this<TooltipSession>{
    ii::core::Observable<std::string> help{"Native retained Tooltip content"},detail{"Hover or focus the origin; the bubble keeps its own interactive content."},status{"Ready"},background{"Background: pending"};
    ii::core::Observable<double> width{700},maximum{320},delay{80};
    ii::core::Observable<lvrs::TooltipPlacement> side{lvrs::TooltipPlacement::above};
    ii::core::Observable<ii::gui::Rect> area{ii::gui::Rect{12,40,736,468}};
    ii::runtime::TaskScope jobs;
    unsigned opens=0,closes=0,clicks=0;
    bool background_done=false,retired=false;
    void start(ii::runtime::Application&);
    void retire();
};
class TooltipBackend final {
public:
    inline static unsigned created=0,retired=0;
    std::shared_ptr<TooltipSession> session=std::make_shared<TooltipSession>();
    ii::core::Observable<std::string>& help=session->help;
    ii::core::Observable<std::string>& detail=session->detail;
    ii::core::Observable<std::string>& status=session->status;
    ii::core::Observable<std::string>& background=session->background;
    ii::core::Observable<double>& width=session->width;
    ii::core::Observable<double>& maximum=session->maximum;
    ii::core::Observable<double>& delay=session->delay;
    ii::core::Observable<lvrs::TooltipPlacement>& side=session->side;
    ii::core::Observable<ii::gui::Rect>& area=session->area;
    std::function<void()> on_close;
    TooltipBackend(){++created;}
    ~TooltipBackend(){session->retire();++retired;}
    TooltipBackend(const TooltipBackend&)=delete;
    TooltipBackend& operator=(const TooltipBackend&)=delete;
    void opened();void closed();void origin();void action();void close();
};
