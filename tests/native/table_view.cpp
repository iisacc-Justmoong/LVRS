#include <LVRS/Table.h>
#include <iostream>
#include <stdexcept>
using namespace ii;
namespace {
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
template<class T>T take(core::Result<T> result){if(!result)throw std::runtime_error(result.error().message);return std::move(*result);}
void good(core::Result<void> result){if(!result)throw std::runtime_error(result.error().message);}
void run(const std::string& name){
    auto model=std::make_shared<lvrs::TableModel>();lvrs::TableModelOptions data;
    data.columns={{"Asset","string"},{"Count","int"},{"Ready","bool"}};data.column_widths={220,180,160};data.inputable=true;
    data.rows={{{{.value=std::string("Alpha")},{.value=std::int64_t(12)},{.value=true}}},{{{.value=std::string("Beta")},{.value=std::int64_t(3)},{.value=false}}}};
    if(name=="virtual"||name=="merged"){const auto repeated_row=data.rows.front();data.rows.assign(100000,repeated_row);}
    if(name=="merged")data.merges={{10,0,10000,2}};
    if(name=="horizontal_merged"){data.merges={{0,0,1,3}};data.inputable=false;}
    if(name=="horizontal_pixels"){
        data.columns={{"ABCDEFGHIJKLMNOPQRSTUVWXYZ · stable header origin","string"}};data.column_widths={1000};data.inputable=false;
        data.rows={{{{.value=std::string("ABCDEFGHIJKLMNOPQRSTUVWXYZ · stable glyph origin · ABCDEFGHIJKLMNOPQRSTUVWXYZ")}}}};
    }
    if(name=="horizontal_large"){
        data.columns.resize(4096,{"Wide","string"});data.column_widths.assign(4096,1000000000.);
        for(auto& row:data.rows)row.cells.resize(4096,{.value=std::string("Wide")});
    }
    take(model->set_options(std::move(data)));
    gui::Host host({.size={700,500}});gui::WidgetMount mount;lvrs::Table table;int edited=0,submitted=0,rejected=0,selection=0,ranges=0,columns_resized=0,rows_resized=0;
    bool clear_on_selection=false,unmount_on_selection=false,throw_on_selection=false;
    good(host.run({.opened=[&]{
        mount=take(host.mount_widgets([&](gui::WidgetBuilder& builder)->core::Result<void>{
            table=take(lvrs::Table::create(builder,{},"table",{.model=model,.width=name.starts_with("horizontal")?260.:620.,.height=300,.sorting_enabled=true,.fit_columns_to_viewport=!name.starts_with("horizontal")},
                {.edited=[&](std::size_t r,std::size_t c,std::string value){check(r==0&&c==1&&value=="42","Edited payload lost");++edited;},
                 .submitted=[&](std::size_t r,std::size_t c,std::string value){check(r==0&&c==1&&value=="42","Submit payload lost");++submitted;},
                 .rejected=[&](std::size_t,std::size_t,std::string,std::string){++rejected;},
                 .selection_changed=[&](std::optional<lvrs::TableRange>){
                     ++selection;
                     if(throw_on_selection)throw std::runtime_error("Selection callback failure");
                     if(clear_on_selection){clear_on_selection=false;take(table.clear_selection());}
                     else if(unmount_on_selection){unmount_on_selection=false;good(host.unmount_widgets());}
                 },.range_values_changed=[&](lvrs::TableRange){++ranges;},
                 .column_resized=[&](std::size_t index,double width){check(index==(name=="horizontal"?1:0)&&width==(name=="horizontal"?205:285),"Column resize completion payload lost");++columns_resized;},
                 .row_resized=[&](std::size_t index,double height){check(index==0&&height==49,"Row resize completion payload lost");++rows_resized;}}));return {};
        }));
        auto viewport=take(table.viewport().snapshot());
        if(name=="virtual"){
            check(viewport.content_height==2400000&&take(mount.snapshot()).size()<16,"Native table allocated one widget per row/cell");
            good(table.viewport().set_scroll_offset(24));
            check(take(table.snapshot()).first_visible_row==1&&take(mount.snapshot()).size()<16,"Viewport callback must redraw without rewriting its scroll configuration");
            good(table.set_scroll_offset(1200000));auto state=take(table.snapshot());
            check(state.first_visible_row>=49999&&state.visible_cells<50,"Viewport did not query the deep logical rows");
            check(take(table.body().snapshot()).bounds.y==viewport.bounds.y,"Deep scroll became a huge physical coordinate");
            take(model->set_cell_value({50000,1},std::int64_t(99)));check(take(table.snapshot()).model_revision==take(model->state()).revision,"Model change did not reach retained table");
            take(host.capture_canvas());
            auto before=take(host.graphics_info()).scene_renders;take(model->set_cell_value({90000,1},std::int64_t(17)));
            check(take(host.graphics_info()).scene_renders==before,"Offscreen data edit rebuilt visible GPU scene");
        }else if(name=="horizontal"){
            const auto revision=take(model->state()).revision;
            take(table.select_cell({0,2}));good(table.reveal_cell({0,2}));
            check(take(table.snapshot()).horizontal_scroll_offset==300&&take(model->state()).revision==revision,"Horizontal reveal changed model state or missed the column");
            good(table.set_horizontal_scroll_offset(0));take(table.select_cell({0,0}));good(table.root().focus());
            good(mount.dispatch_input({.kind=gui::WidgetInputKind::key_down,.key=gui::WidgetKey::end}));
            check(take(table.snapshot()).current==lvrs::TablePosition{0,2}&&take(table.snapshot()).horizontal_scroll_offset==300,"Keyboard End did not reveal the last column");
            good(table.set_scroll_position(240,0));check(take(table.snapshot()).first_visible_column==1,"Horizontal query started at column zero");
            check(take(table.body().snapshot()).bounds.x==viewport.bounds.x,"Logical horizontal scroll translated the retained body");
            const auto r=take(table.root().snapshot()).bounds;
            const auto click=[&](double x,double y){good(mount.dispatch_input({.kind=gui::WidgetInputKind::pointer_down,.x=r.x+x,.y=r.y+y}));good(mount.dispatch_input({.kind=gui::WidgetInputKind::pointer_up,.x=r.x+x,.y=r.y+y}));};
            click(50,37);check(take(table.snapshot()).current==lvrs::TablePosition{0,1}&&take(table.snapshot()).editing,"Scrolled pointer did not edit the logical column");good(table.end_edit());
            good(table.set_horizontal_scroll_offset(0));take(table.select_cell({0,1}));good(table.begin_edit());
            check(take(table.snapshot()).horizontal_scroll_offset==140&&take(table.editor().snapshot()).visible,"Offscreen editor was not revealed");
            good(table.set_horizontal_scroll_offset(300));check(!take(table.snapshot()).editing&&!take(table.editor().snapshot()).visible&&take(table.body().snapshot()).drawing_texts>0,"Scrolling editor origin out of view left a blank or active editor");good(table.end_edit());
            good(table.set_horizontal_scroll_offset(220));const auto depth=take(model->state()).undo_depth;
            good(mount.dispatch_input({.kind=gui::WidgetInputKind::pointer_down,.x=r.x+180,.y=r.y+10}));
            good(mount.dispatch_input({.kind=gui::WidgetInputKind::pointer_move,.x=r.x+205,.y=r.y+10}));
            good(mount.dispatch_input({.kind=gui::WidgetInputKind::pointer_up,.x=r.x+205,.y=r.y+10}));
            check(take(model->column_width(1))==205&&columns_resized==1&&take(model->state()).undo_depth==depth+1,"Scrolled resize used the physical column index");
            click(70,10);check(take(table.snapshot()).sort_column==1&&take(model->cell_text({0,0}))=="Beta","Scrolled header targeted the wrong sort column");
            good(mount.dispatch_input({.kind=gui::WidgetInputKind::context_request,.x=r.x+30,.y=r.y+37}));
            check(take(model->state()).context_cell==lvrs::TablePosition{0,1},"Scrolled context menu targeted the wrong column");good(table.context_menu().close_popup());
            good(table.set_options({.model=model,.width=260,.height=300,.fit_columns_to_viewport=false,.structure_controls_visible=true}));click(70,290);
            check(model->column_count()==4&&take(model->column_type(3))=="bool","Scrolled structure gutter inserted at the wrong column");
        }else if(name=="horizontal_pixels"){
            auto before=take(host.capture_canvas());const auto scale=take(host.snapshot()).backing_scale;good(table.set_horizontal_scroll_offset(50));auto after=take(host.capture_canvas());
            const auto x0=std::size_t((viewport.bounds.x+20)*scale),y0=std::size_t((viewport.bounds.y+5)*scale),shift=std::size_t(50*scale);
            std::size_t ink=0;
            for(std::size_t y=y0;y<y0+std::size_t(14*scale);++y)for(std::size_t x=x0;x<x0+std::size_t(160*scale);++x){
                const auto old=(y*before.width+x+shift)*4,current=(y*after.width+x)*4;
                for(unsigned c=0;c<4;++c)check(std::abs(int(before.rgba[old+c])-int(after.rgba[current+c]))<=1,"Horizontal crop reshaped or moved glyph origin");
                ink+=after.rgba[current]>80;
            }
            check(ink>0,"Horizontal crop comparison had no text ink");
        }else if(name=="horizontal_merged"){
            const auto revision=take(model->state()).revision;good(table.set_horizontal_scroll_offset(250));
            check(take(table.snapshot()).visible_cells==3&&take(table.snapshot()).first_visible_column==1,"Merged anchor outside horizontal viewport was lost");
            take(host.capture_canvas());good(table.reveal_cell({0,2}));
            check(take(table.snapshot()).horizontal_scroll_offset==250&&take(model->state()).revision==revision,"Merged reveal jumped despite visible intersection or changed history");
        }else if(name=="horizontal_large"){
            const auto count=take(mount.snapshot()).size();const auto revision=take(model->state()).revision;
            good(table.set_scroll_position(4000000000000.,0));auto state=take(table.snapshot());
            check(state.first_visible_column==4000&&state.visible_cells==2&&take(mount.snapshot()).size()==count,"Deep horizontal view allocated or queried offscreen columns");
            check(take(table.body().snapshot()).bounds.x==viewport.bounds.x&&take(model->state()).revision==revision,"Large logical X became physical geometry or document state");
            check(!table.reveal_cell({10,0})&&take(table.snapshot()).horizontal_scroll_offset==4000000000000.,"Invalid reveal partially moved viewport");
            take(host.capture_canvas());const auto renders=take(host.graphics_info()).scene_renders;take(model->set_cell_value({0,0},std::string("Offscreen")));
            check(take(host.graphics_info()).scene_renders==renders,"Offscreen horizontal edit rebuilt the visible scene");
            good(table.set_horizontal_scroll_offset(1e15));take(model->delete_column(4095));
            check(take(table.snapshot()).horizontal_scroll_offset==model->total_width()-viewport.bounds.width,"Column deletion did not clamp horizontal viewport");
        }else if(name=="editing"){
            take(table.select_cell({0,1}));good(table.begin_edit());
            good(mount.dispatch_input({.kind=gui::WidgetInputKind::key_down,.key=gui::WidgetKey::select_all}));good(mount.dispatch_input({.kind=gui::WidgetInputKind::text,.text="42"}));
            good(mount.dispatch_input({.kind=gui::WidgetInputKind::key_down,.key=gui::WidgetKey::enter}));
            check(edited==1&&submitted==1&&take(model->cell_text({0,1}))=="42","Native table edit did not commit to model");
            good(table.begin_edit());good(mount.dispatch_input({.kind=gui::WidgetInputKind::key_down,.key=gui::WidgetKey::select_all}));good(mount.dispatch_input({.kind=gui::WidgetInputKind::text,.text="bad"}));
            check(rejected==1&&take(table.editor().snapshot()).text=="42","Rejected native table input changed accepted value");
            good(table.end_edit());
        }else if(name=="selection"){
            auto before=take(model->state()).revision;take(table.select_cell({0,0}));take(table.select_cell({1,2},true));
            auto state=take(table.snapshot());check(state.selection==std::optional(lvrs::TableRange{0,0,2,3})&&selection==2,"Range selection failed");
            check(take(model->state()).revision==before,"Pure table selection mutated model/document state");
            good(table.root().focus());good(mount.dispatch_input({.kind=gui::WidgetInputKind::key_down,.key=gui::WidgetKey::home}));
            check(take(table.snapshot()).current==std::optional(lvrs::TablePosition{1,0}),"Native table keyboard navigation failed");
            take(table.sort_rows(1,lvrs::TableSortOrder::ascending));check(take(model->cell_text({0,0}))=="Beta","Header/model sort failed");
            good(table.set_options({.model=model,.width=620,.height=300,.selection_mode=lvrs::TableSelectionMode::none}));
            good(mount.dispatch_input({.kind=gui::WidgetInputKind::key_down,.key=gui::WidgetKey::right}));check(!take(table.snapshot()).current,"NoSelection navigation created a current cell");
        }else if(name=="merged"){
            good(table.set_scroll_offset(50000));auto state=take(table.snapshot());check(state.first_visible_row==10,"Merged anchor above viewport was lost");
            take(host.capture_canvas());
            take(table.select_cell({10010,1}));check(!take(table.is_cell_selected({11,1})),"Covered-cell selection used the covered coordinate as its merge origin");
        }else if(name=="transfer"){
            take(table.select_range({0,0},{1,2}));check(take(table.selection_as_tsv())=="Alpha\t12\ttrue\nBeta\t3\tfalse","Selection TSV did not preserve typed source values");
            auto before=take(model->state()).undo_depth;take(table.paste_tsv({0,0},"Gamma\t42\tfalse\nDelta\t9\ttrue"));
            check(ranges==1&&edited==0&&take(model->state()).undo_depth==before+1,"Range paste did not use one model command");
            check(!table.paste_tsv({0,0},"Changed\t11\ttrue\nBad\tnot-an-int\tfalse")&&take(model->cell_text({0,0}))=="Gamma","Rejected matrix partially changed data");
            good(table.root().focus());good(mount.dispatch_input({.kind=gui::WidgetInputKind::text,.content=std::make_shared<const gui::ClipboardContent>(gui::ClipboardContent{.text="Epsilon\t55\ttrue"})}));
            check(take(model->cell_text({0,0}))=="Epsilon"&&take(model->cell_text({0,1}))=="55"&&ranges==2,"Clipboard content did not reach native range paste");
            take(table.undo());check(take(model->cell_text({0,0}))=="Gamma","Paste undo lost original values");
        }else if(name=="resizing"||name=="resize_lifetime"||name=="resize_notification"){
            auto r=take(table.root().snapshot()).bounds;const auto depth=take(model->state()).undo_depth;
            good(mount.dispatch_input({.kind=gui::WidgetInputKind::pointer_down,.x=r.x+220,.y=r.y+10}));
            good(mount.dispatch_input({.kind=gui::WidgetInputKind::pointer_move,.x=r.x+260,.y=r.y+10}));
            if(name=="resize_notification"){
                bool armed=true;
                auto port=std::make_shared<lvrs::TableChangePort>([&](const lvrs::TableChange& change){if(armed&&change.geometry){armed=false;good(host.unmount_widgets());}});
                good(model->observe(port));take(model->update_column_resize(r.x+280));
                check(!take(model->state()).resizing_column&&!table.snapshot(),"View retirement inside notification left an active resize");
                take(model->set_cell_value({0,1},std::int64_t(99)));
            }else if(name=="resize_lifetime"){
                good(host.unmount_widgets());check(!take(model->state()).resizing_column&&columns_resized==0,"Retired table left the shared model resize locked");
                take(model->set_cell_value({0,1},std::int64_t(99)));
            }else{
                good(mount.dispatch_input({.kind=gui::WidgetInputKind::pointer_move,.x=r.x+285,.y=r.y+10}));
                good(mount.dispatch_input({.kind=gui::WidgetInputKind::pointer_up,.x=690,.y=490}));
                check(take(model->column_width(0))==285&&columns_resized==1&&take(model->state()).undo_depth==depth+1,"Column drag was not captured/coalesced");
                good(mount.dispatch_input({.kind=gui::WidgetInputKind::pointer_down,.x=r.x+100,.y=r.y+49}));
                good(mount.dispatch_input({.kind=gui::WidgetInputKind::pointer_move,.x=r.x+100,.y=r.y+74}));
                good(mount.dispatch_input({.kind=gui::WidgetInputKind::pointer_up,.x=r.x+100,.y=r.y+74}));
                check(take(model->row_height(0))==49&&rows_resized==1,"Row drag did not reach the model");
                take(table.undo());take(table.undo());check(take(model->column_width(0))==220&&take(model->row_height(0))==24,"Resize undo did not restore dimensions");
                good(mount.dispatch_input({.kind=gui::WidgetInputKind::pointer_down,.x=r.x+220,.y=r.y+10}));
                good(mount.dispatch_input({.kind=gui::WidgetInputKind::key_down,.key=gui::WidgetKey::escape}));
                good(mount.dispatch_input({.kind=gui::WidgetInputKind::pointer_up,.x=r.x+100,.y=r.y+37}));
                check(!take(table.snapshot()).editing&&!take(table.snapshot()).current,"Canceled resize release activated a cell");
            }
        }else if(name=="structures"){
            good(table.set_options({.model=model,.width=620,.height=300,.structure_controls_visible=true}));auto r=take(table.root().snapshot()).bounds;
            const auto click=[&](double x,double y){good(mount.dispatch_input({.kind=gui::WidgetInputKind::pointer_down,.x=r.x+x,.y=r.y+y}));good(mount.dispatch_input({.kind=gui::WidgetInputKind::pointer_up,.x=r.x+x,.y=r.y+y}));};
            click(610,37);click(100,290);check(model->row_count()==3&&model->column_count()==4,"Structure gutters did not insert after the target");
            take(table.undo());take(table.undo());take(table.select_range({0,0},{1,1}));take(table.merge_selection());check(take(model->cell_geometry({0,0})).row_span==2,"Selection merge did not reach the model");
            take(table.split_cell({0,0}));check(take(model->cell_geometry({0,0})).row_span==1&&take(mount.snapshot()).size()<16,"Structure commands allocated a widget per cell");
        }else if(name=="menus"){
            auto r=take(table.root().snapshot()).bounds;good(mount.dispatch_input({.kind=gui::WidgetInputKind::context_request,.x=r.x+100,.y=r.y+37}));
            check(take(table.context_menu().snapshot()).opened&&take(model->state()).context_cell==lvrs::TablePosition{0,0},"Context request did not retain the target cell");
            good(take(mount.find("table.delete-column")).activate());check(model->column_count()==2&&!take(table.context_menu().snapshot()).opened,"Context column command did not close/delete");
            take(table.delete_column(1));good(table.open_context_menu({0,0},r.x+100,r.y+37));check(!take(take(mount.find("table.delete-column")).snapshot()).enabled,"Last column deletion remained enabled");
            good(table.context_menu().close_popup());
        }else if(name=="replacement"){
            auto next=std::make_shared<lvrs::TableModel>();lvrs::TableModelOptions options;options.columns={{"Next","string"}};options.rows={{{{.value=std::string("Next")}}}};take(next->set_options(std::move(options)));
            take(table.select_cell({1,2}));good(table.set_options({.model=next,.width=620,.height=300}));const auto revision=take(table.snapshot()).model_revision;
            take(model->set_cell_value({0,1},std::int64_t(99)));check(take(table.snapshot()).model_revision==revision&&take(table.snapshot()).visible_cells==1,"Old model subscription survived replacement");
            take(next->set_cell_value({0,0},std::string("New")));check(take(table.snapshot()).model_revision==take(next->state()).revision,"Replacement model was not observed");
        }else if(name=="failure"){
            throw_on_selection=true;auto failed=table.select_range({0,0},{1,2});check(!failed&&failed.error().code==core::ErrorCode::failed&&failed.error().cause,"Public callback exception escaped the Result contract");
            throw_on_selection=false;take(table.clear_selection());check(!take(table.snapshot()).selection,"Table could not recover after callback failure");
        }else if(name=="reentrant"){
            take(table.select_cell({0,1}));good(table.root().focus());clear_on_selection=true;
            good(mount.dispatch_input({.kind=gui::WidgetInputKind::key_down,.key=gui::WidgetKey::right}));
            check(!take(table.snapshot()).current,"Selection callback clear was overwritten by navigation");
            take(table.select_cell({0,1}));good(table.root().focus());unmount_on_selection=true;
            good(mount.dispatch_input({.kind=gui::WidgetInputKind::key_down,.key=gui::WidgetKey::right}));
            check(!table.snapshot(),"Selection callback did not retire table mount");
        }else if(name=="lifetime"){
            good(host.unmount_widgets());check(!table.snapshot()&&!table.root().snapshot(),"Table handle survived unmount");
            take(model->set_cell_value({0,1},std::int64_t(99)));check(take(model->cell_text({0,1}))=="99","Detached table retained a failing model receiver");
        }else throw std::runtime_error("Unknown native table view case");
        good(host.request_close());
    }}));
}
}
int main(int argc,char** argv){try{run(argc>1?argv[1]:"virtual");std::cout<<"LVRS_TABLE_VIEW_OK\n";}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
