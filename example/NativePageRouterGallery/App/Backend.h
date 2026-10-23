#pragma once
#include <LVRS/PageRouter.h>
#include <LVRS/Navigator.h>
#include "PageBackend.h"
class PageRouterGalleryBackend final {
public:
    std::shared_ptr<lvrs::PageRouter> router;
    std::shared_ptr<lvrs::Navigator> navigator=std::make_shared<lvrs::Navigator>();
    std::shared_ptr<PageState> pages=std::make_shared<PageState>();
    std::function<void()> on_close;
    std::function<void()> on_undo_visit;
    PageRouterGalleryBackend();
    void home();
    void report();
    void back();
    void alternate();
    void undo_visit();
    void close();
};
