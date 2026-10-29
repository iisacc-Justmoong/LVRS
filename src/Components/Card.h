#pragma once
#include <iiGUI/NativeWidgets.h>
#include <vector>
namespace lvrs {
enum class CardType {file,file_preview,folder,project,device,model,member,link};
enum class CardSize {small,medium,large};
enum class CardDetail {brief,detailed};
enum class CardDisplayState {automatic,normal,hover,selected};
struct CardRow final {
    std::string label,value,icon_name;
    std::shared_ptr<const ii::gui::RasterImage> icon;
    friend bool operator==(const CardRow&,const CardRow&)=default;
};
struct CardOptions final {
    CardType type=CardType::file;CardSize size=CardSize::medium;CardDetail detail=CardDetail::brief;CardDisplayState display_state=CardDisplayState::automatic;
    bool selected=false,enabled=true,show_description=true,show_status=true;
    std::optional<bool> selectable,show_menu,show_action;
    std::string title,description,details,metadata,status_text,summary,footnote,domain,preview_title,icon_name,menu_accessibility_name="More actions";
    std::optional<std::string> action_text,progress_label;
    std::string progress_text;
    std::vector<CardRow> rows;
    double progress=0;
    std::optional<ii::gui::WidgetColor> progress_color,status_color;
    std::shared_ptr<const ii::gui::RasterImage> icon,preview_source;
    double width=0,height=0;
};
struct CardEvents final {
    ii::runtime::Task clicked,menu_requested,action_triggered;
    std::function<void(bool)> selected_changed;
};
using CardComponent=std::shared_ptr<const ii::gui::WidgetFactory>;
[[nodiscard]] const ii::gui::WidgetFactory& card_component(const ii::gui::WidgetFactory&);
[[nodiscard]] const ii::gui::WidgetFactory& card_component(const CardComponent&);
struct CardContent final {ii::gui::WidgetFactory preview,icon;};
struct CardState final {
    bool image_card=false,preview_only=false,progress_card=false,effective_selected=false,effective_hovered=false;
    double border_width=0,content_inset=0;
    ii::gui::Size implicit_size;
    ii::gui::WidgetColor border,surface;
    ii::gui::ImageStatus preview_status=ii::gui::ImageStatus::empty;
    std::string variant_name;
};
[[nodiscard]] ii::core::Result<std::shared_ptr<const ii::gui::RasterImage>> card_icon(std::string_view,std::uint32_t pixels=64);
class Card final {
public:
    Card()=default;
    [[nodiscard]] static ii::core::Result<Card> create(ii::gui::WidgetBuilder&,ii::gui::Widget,std::string_view,CardOptions={},CardEvents={},CardContent={});
    [[nodiscard]] ii::gui::Widget root()const noexcept{return root_;}
    [[nodiscard]] ii::gui::Widget preview_root()const noexcept{return inline_;}
    [[nodiscard]] ii::core::Result<CardOptions> options()const;
    [[nodiscard]] ii::core::Result<void> set_options(CardOptions)const;
    [[nodiscard]] ii::core::Result<CardState> state()const;
    [[nodiscard]] ii::core::Result<ii::gui::Widget> preview_item()const;
    [[nodiscard]] ii::core::Result<void> set_selected(bool)const;
    [[nodiscard]] ii::core::Result<void> set_title(std::string_view)const;
    [[nodiscard]] ii::core::Result<void> set_rows(std::vector<CardRow>)const;
    [[nodiscard]] ii::core::Result<void> set_preview_source(std::shared_ptr<const ii::gui::RasterImage>)const;
    [[nodiscard]] ii::core::Result<void> set_preview_content(ii::gui::WidgetFactory)const;
    [[nodiscard]] ii::core::Result<void> set_icon_content(ii::gui::WidgetFactory)const;
    [[nodiscard]] ii::core::Result<void> set_events(CardEvents)const;
private:
    struct Impl;
    [[nodiscard]] ii::core::Result<std::shared_ptr<Impl>> lock()const;
    ii::gui::Widget root_,inline_;std::weak_ptr<Impl> state_;
};
}
