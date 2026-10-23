#import <AppKit/AppKit.h>
#include <LVRS/Table.h>
#include <iiGUI/Desktop.h>
#include <iostream>
#include <stdexcept>
using namespace ii;
namespace {
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
void good(core::Result<void> value){if(!value){if(value.error().cause)std::rethrow_exception(value.error().cause);throw std::runtime_error(value.error().message);}}
template<class T>T take(core::Result<T> value){if(!value)throw std::runtime_error(value.error().message);return std::move(*value);}
NSView* surface(NSView* view){if([view.identifier isEqualToString:@"iiGUI.widget.surface"])return view;for(NSView* child in view.subviews)if(auto found=surface(child))return found;return nil;}
void run(){
    // Preserve all advertised types/items, including content outside iiGUI's API.
    NSMutableArray<NSPasteboardItem*>* saved=[NSMutableArray new];
    for(NSPasteboardItem* old in NSPasteboard.generalPasteboard.pasteboardItems){auto copy=[NSPasteboardItem new];for(NSPasteboardType type in old.types)if(auto data=[old dataForType:type])[copy setData:data forType:type];[saved addObject:copy];}
    const auto restore=[&]{[NSPasteboard.generalPasteboard clearContents];if(saved.count)[NSPasteboard.generalPasteboard writeObjects:saved];};
    try{
        auto model=std::make_shared<lvrs::TableModel>();lvrs::TableModelOptions data;
        data.inputable=true;data.columns={{"Name","string"},{"Count","int"},{"Ready","bool"}};
        data.rows={{{{.value=std::string("Alpha")},{.value=std::int64_t(12)},{.value=true}}}};take(model->set_options(std::move(data)));
        gui::Host host({.title="LVRS Table Clipboard",.size={700,400}});lvrs::Table table;gui::WidgetMount mount;int ranges=0;
        good(host.run({.opened=[&]{
            mount=take(host.mount_widgets([&](gui::WidgetBuilder& b)->core::Result<void>{table=take(lvrs::Table::create(b,{},"table",{.model=model,.width=620,.height=260},{.range_values_changed=[&](lvrs::TableRange){++ranges;}}));return {};}));
            take(table.select_range({0,0},{0,2}));good(table.root().focus());NSView* view=nil;
            for(NSWindow* window in NSApp.windows)if([window.title isEqualToString:@"LVRS Table Clipboard"]){view=surface(window.contentView);break;}
            check(view!=nil,"Native table surface missing");[(id)view copy:nil];
            check(take(gui::read_clipboard()).text=="Alpha\t12\ttrue","Native copy command did not reach table selection");
            good(gui::write_clipboard({.text="Beta\t42\tfalse"}));[(id)view paste:nil];
            check(take(model->cell_text({0,0}))=="Beta"&&take(model->cell_text({0,1}))=="42"&&ranges==1,"Native paste command did not validate a typed matrix");
            take(table.select_cell({0,1}));good(table.begin_edit());[(id)view selectAll:nil];good(gui::write_clipboard({.text="77"}));[(id)view paste:nil];
            check(take(model->cell_text({0,1}))=="77"&&ranges==1,"Text-editor paste was redirected into a table matrix");
            [(id)view selectAll:nil];[(id)view copy:nil];check(take(gui::read_clipboard()).text=="77","Editor text selection copy regressed");
            good(host.request_close());
        }}));
        restore();
    }catch(...){restore();throw;}
}
}
int main(){try{run();std::cout<<"TABLE_NATIVE_CLIPBOARD_OK\n";}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
