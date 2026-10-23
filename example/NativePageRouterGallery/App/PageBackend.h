#pragma once
#include <LVRS/PageRouter.h>
#include <LVRS/Navigator.h>
#include <iiSDKCore/Core/Property/Observable.h>
class PageBackend final {
public:
    inline static int created=0,retired=0;
    ii::core::Observable<std::string> title{"Native page"},context{"Ready"};
    std::shared_ptr<lvrs::Navigator> navigator;
    ii::core::Observable<std::string> href{"/reports/42"};
    ii::core::Observable<bool> replace{false},underline{true};
    ii::core::Observable<ii::gui::WidgetColor> link_color{ii::gui::WidgetColor{.08120492845773697,.5099760293960571,1,1}};
    PageBackend();
    ~PageBackend();
    void report();
    void back();
};
struct PageState final {std::weak_ptr<PageBackend> home;};
