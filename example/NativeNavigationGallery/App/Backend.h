#pragma once
#include <LVRS/NavigationStackModel.h>
#include <LVRS/RouteResolver.h>
#include <functional>
class NavigationGalleryBackend final {
public:
    inline static int created=0,destroyed=0;
    std::shared_ptr<lvrs::NavigationStackModel> model=std::make_shared<lvrs::NavigationStackModel>();
    lvrs::RouteResolver resolver;
    std::function<void()> on_close;
    NavigationGalleryBackend();
    ~NavigationGalleryBackend();
    void report();
    void replace_report();
    void logs();
    void back();
    void root();
    void close();
private:
    void navigate(std::string_view,lvrs::NavigationOperation);
    static void check(ii::core::Result<bool>);
};
