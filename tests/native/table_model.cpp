#include <LVRS/TableModel.h>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <thread>

using namespace ii;
namespace {
void check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
template<class T> T take(core::Result<T> result) {
    if (!result) throw std::runtime_error(result.error().message);
    return std::move(*result);
}
void good(core::Result<void> result) {
    if (!result) throw std::runtime_error(result.error().message);
}
lvrs::TableModelOptions fixture() {
    lvrs::TableModelOptions options;
    options.columns = {{"Name", "string"}, {"Count", "int"}, {"Ready", "bool"}};
    options.rows = {
        {{{.value = std::string("Beta")}, {.value = std::int64_t(20)}, {.value = false}}},
        {{{.value = std::string("Alpha")}, {.value = std::int64_t(3)}, {.value = true}}},
        {{{.value = std::string("Gamma")}, {.value = std::int64_t(3)}, {.value = false}}}
    };
    options.column_widths = {200, 100, 80};
    options.row_heights = {24, 30, 40};
    return options;
}
void values() {
    lvrs::TableModel model;
    take(model.set_options(fixture()));
    check(model.row_count() == 3 && model.column_count() == 3, "Typed dimensions lost");
    check(take(model.cell_text({1, 0})) == "Alpha", "String value lost");
    take(model.set_cell_value({1, 1}, std::string("42")));
    check(std::get<std::int64_t>(*take(model.cell_raw_value({1, 1}))) == 42, "Integer edit not typed");
    auto revision = take(model.state()).revision;
    check(!model.set_cell_value({1, 1}, std::string("9223372036854775808")), "Overflow accepted");
    check(take(model.state()).revision == revision, "Rejected edit changed revision");
    take(model.set_cell_value({0, 2}, std::string(" YES ")));
    check(take(model.cell_text({0, 2})) == "true", "Source boolean spelling lost");
    check(!model.set_cell_value({0, 1}, std::numeric_limits<double>::infinity()), "Nonfinite typed input accepted");
    check(!model.cell_at({99, 0}) && !model.header_at(9), "Invalid index returned data");
    auto options = fixture();
    options.inputable = true;
    options.rows[0].inputable = false;
    options.rows[0].cells[1].inputable = true;
    take(model.set_options(options));
    check(!take(model.cell_inputable({0, 0})) && take(model.cell_inputable({0, 1})) && take(model.cell_inputable({1, 0})), "Input override precedence lost");
}
void range() {
    lvrs::TableModel model;
    take(model.set_options(fixture()));
    auto before = take(model.state());
    check(!model.set_range_values({0, 0}, {{std::string("Changed"), std::string("bad")}}), "Partial invalid range accepted");
    check(take(model.cell_text({0, 0})) == "Beta" && take(model.state()).revision == before.revision, "Invalid range partially committed");
    take(model.set_range_values({0, 0}, {{std::string("One"), std::int64_t(7)}, {std::string("Two"), std::int64_t(8)}}));
    check(take(model.state()).undo_depth == 1, "Range split into multiple undo commands");
    auto copied = take(model.range_values({0, 0, 2, 2}));
    check(copied.size() == 2 && std::get<std::int64_t>(*copied[1][1]) == 8, "Range extraction changed shape/value");
    take(model.undo());
    check(take(model.cell_text({1, 0})) == "Alpha" && take(model.cell_text({0, 1})) == "20", "Range undo incomplete");
    take(model.redo());
    check(take(model.cell_text({1, 0})) == "Two", "Range redo failed");
    check(!model.set_range_values({0, 0}, {{}}) && !model.range_values({0, 0, 0, 1}), "Empty range accepted");
}
void geometry() {
    lvrs::TableModel model;
    take(model.set_options(fixture()));
    check(take(model.column_x(2)) == 300 && take(model.row_y(2)) == 54 && model.total_body_height() == 94, "Prefix geometry wrong");
    check(take(model.row_at_y(24)) == std::optional<std::size_t>(1) && take(model.column_at_x(299)) == std::optional<std::size_t>(1), "Coordinate index lookup chose wrong boundary");
    check(take(model.position_at(300, 54)) == std::optional(lvrs::TablePosition{2, 2}) && !take(model.position_at(380, 94)), "Point hit did not use half-open table bounds");
    check(!take(model.row_at_y(-1)) && !model.column_at_x(std::nan("")), "Invalid point coordinate accepted");
    take(model.set_row_height(1, 48));
    check(take(model.row_y(2)) == 72 && model.total_body_height() == 112, "Height update did not update prefix");
    take(model.set_column_width(0, 250));
    check(take(model.column_x(2)) == 350, "Width update did not update prefix");
    auto cell = take(model.cell_geometry({2, 2}));
    check(cell.x == 350 && cell.y == 72 && cell.width == 80 && cell.height == 40, "Cell geometry diverged from axis index");
    auto revision = take(model.state()).revision;
    check(!take(model.set_column_width(0, 250)) && take(model.state()).revision == revision, "Equal geometry recorded a mutation");
    check(!model.set_row_height(0, std::nan("")) && !model.column_x(3), "Invalid geometry accepted");
    auto options = fixture();
    options.column_widths.clear(); options.table_width = 600;
    take(model.set_options(options));
    take(model.insert_row(1));
    auto depth = take(model.state()).undo_depth;
    take(model.set_table_width(900));
    check(take(model.column_width(0)) == 300 && take(model.state()).undo_depth == depth, "Viewport resize reset data history");
    take(model.undo());
    check(model.row_count() == 3 && take(model.column_width(0)) == 300 && model.options().table_width == 900, "Structure undo restored an obsolete viewport width");
    take(model.redo());
    check(model.row_count() == 4 && take(model.column_width(0)) == 300, "Structure redo lost the current automatic geometry");
}
void merges() {
    lvrs::TableModel model;
    take(model.set_options(fixture()));
    take(model.merge_cells({0, 0, 2, 2}));
    auto revision = take(model.state()).revision;
    check(!take(model.merge_cells({0, 0, 2, 2})) && take(model.state()).revision == revision, "Equal merge created a new command");
    check(take(model.merge_anchor({1, 1})) == lvrs::TablePosition{0, 0}, "Merge anchor lookup wrong");
    check(take(model.position_at(250, 30)) == std::optional(lvrs::TablePosition{0, 0}) && take(model.position_at(250, 30, false)) == std::optional(lvrs::TablePosition{1, 1}), "Merged hit did not distinguish anchor and raw row");
    auto geometry = take(model.cell_geometry({0, 0}));
    check(geometry.width == 300 && geometry.height == 54 && geometry.row_span == 2 && geometry.column_span == 2, "Merged geometry wrong");
    check(take(model.cell_geometry({1, 1})).covered, "Covered cell exposed as an anchor");
    check(!model.set_range_values({0, 0}, {{std::string("Blocked")}}), "Merged range paste accepted");
    take(model.split_cell({1, 1}));
    check(!take(model.cell_geometry({1, 1})).covered && take(model.cell_text({1, 1})) == "3", "Split destroyed original covered value");
    take(model.undo());
    check(take(model.cell_geometry({1, 1})).covered, "Merge undo failed");
    take(model.merge_cells({1, 1, 2, 2}));
    check(!take(model.cell_geometry({0, 0})).covered && take(model.cell_geometry({0, 0})).row_span == 1, "Overlapping merge failed to split old region");
    auto options = model.options();
    options.merges.push_back({1, 2, 1, 1});
    revision = take(model.state()).revision;
    check(!model.set_options(options) && take(model.state()).revision == revision, "Overlapping imported merges partially replaced model");
}
void sorting() {
    lvrs::TableModel model;
    auto options = fixture();
    options.rows.push_back({{{.value = std::string("Missing")}, {.value = std::nullopt}, {.value = true}}});
    options.row_heights.push_back(50);
    take(model.set_options(options));
    take(model.sort_rows(1, lvrs::TableSortOrder::ascending));
    check(take(model.cell_text({0, 0})) == "Alpha" && take(model.cell_text({1, 0})) == "Gamma", "Stable numeric ordering failed");
    check(take(model.row_height(0)) == 30 && take(model.row_height(1)) == 40, "Sort detached row heights");
    check(take(model.cell_text({3, 0})) == "Missing", "Ascending sort did not keep empty values last");
    take(model.sort_rows(1, lvrs::TableSortOrder::descending));
    check(take(model.cell_text({0, 0})) == "Beta" && take(model.cell_text({3, 0})) == "Missing", "Descending numeric/empty order failed");
    take(model.undo());
    check(take(model.cell_text({0, 0})) == "Alpha", "Sort undo failed");
}
void structure() {
    lvrs::TableModel model;
    take(model.set_options(fixture()));
    take(model.merge_cells({0, 0, 2, 2}));
    take(model.insert_row(1));
    check(model.row_count() == 4 && take(model.cell_text({1, 1})) == "0" && take(model.row_height(2)) == 30, "Inserted row defaults/height order wrong");
    check(!take(model.cell_geometry({0, 0})).covered && take(model.cell_geometry({0, 0})).row_span == 1, "Structure edit retained stale merge coordinates");
    take(model.undo());
    check(model.row_count() == 3 && take(model.cell_geometry({1, 1})).covered, "Structure undo did not restore merge state");
    take(model.insert_column(1));
    check(model.column_count() == 4 && take(model.cell_text({0, 1})) == "0" && take(model.cell_text({0, 2})) == "20", "Inserted column lost data/default type");
    take(model.delete_column(1));
    check(take(model.cell_text({0, 1})) == "20" && take(model.column_width(1)) == 100, "Column delete detached widths/data");
    take(model.delete_row(0));
    check(take(model.cell_text({0, 0})) == "Alpha" && take(model.row_height(0)) == 30, "Row delete detached heights/data");
    while (model.column_count() > 1) take(model.delete_column(0));
    check(!model.delete_column(0), "Deleted final column");
    check(!model.insert_row(model.row_count() + 1), "Out-of-range insertion accepted");
}
void resizing() {
    lvrs::TableModel model;
    take(model.set_options(fixture()));
    take(model.begin_column_resize(1, 100));
    take(model.update_column_resize(120));
    take(model.update_column_resize(145));
    check(take(model.state()).undo_depth == 1 && take(model.column_width(1)) == 145, "Resize drag created multiple undo commands");
    check(!model.insert_row(0) && !model.undo() && !model.begin_row_resize(0, 0), "Concurrent operation corrupted active resize");
    take(model.end_resize());
    take(model.undo());
    check(take(model.column_width(1)) == 100, "Resize undo failed to restore start width");
    take(model.redo());
    check(take(model.column_width(1)) == 145, "Resize redo failed");
    take(model.begin_row_resize(0, 0));
    take(model.update_row_resize(-1000));
    check(take(model.row_height(0)) == 18, "Row resize ignored minimum");
    take(model.end_resize());
    check(!model.update_row_resize(10) && !model.begin_column_resize(99, 0), "Invalid resize state accepted");
    bool released=false;int ended=0;
    auto receiver=std::make_shared<lvrs::TableChangePort>([&](const lvrs::TableChange& change){
        if(change.geometry&&!released){released=true;check(!model.end_resize(),"Ordinary resize completion reentered notification");take(model.request_end_resize());check(take(model.state()).resizing_column.has_value(),"Resize release was applied inside its change callback");}
        else if(change.resize_state&&!take(model.state()).resizing_column&&!take(model.state()).resizing_row)++ended;
    });
    good(model.observe(receiver));take(model.begin_column_resize(0,0));take(model.update_column_resize(10));
    check(released&&ended==1&&!take(model.state()).resizing_column,"Deferred resize ownership did not finish after notification");
    take(model.set_cell_value({0,1},std::int64_t(42)));
}
void viewport() {
    lvrs::TableModel model;
    auto options = fixture();
    const auto repeated_row = options.rows.front();
    options.rows.assign(100000, repeated_row);
    options.row_heights.clear();
    options.merges = {{10, 0, 100, 2}};
    take(model.set_options(std::move(options)));
    auto cells = take(model.visible_cells(50 * 24, 52 * 24, 100, 380));
    check(cells.size() == 3 && cells[0].position == lvrs::TablePosition{10, 0}, "Viewport lost merged anchor above top");
    check(cells[0].geometry.y == 240 && cells[0].geometry.height == 2400 && cells[1].position == lvrs::TablePosition{50, 2}, "Viewport returned unrelated or covered cells");
    auto tail = take(model.visible_cells(99999 * 24, 100000 * 24));
    check(tail.size() == 3 && tail[0].index == 299798, "Deep viewport global anchor index wrong");
    take(model.set_row_height(0, 48));
    auto shifted = take(model.visible_cells(100000 * 24, 100001 * 24));
    check(shifted.size() == 3 && shifted[0].position.row == 99999, "Deep viewport did not follow resize index");
    check(take(model.visible_cells(-100, -10)).empty(), "Negative viewport returned cells");
    check(!model.visible_cells(0, std::nan("")), "Nonfinite viewport accepted");
}
void history() {
    lvrs::TableModel model;
    take(model.set_options(fixture()));
    take(model.set_undo_limit(2));
    for (int value = 1; value <= 3; ++value) take(model.set_cell_value({0, 1}, std::int64_t(value)));
    check(take(model.state()).undo_depth == 2, "Undo limit ignored");
    take(model.undo());
    take(model.undo());
    check(take(model.cell_text({0, 1})) == "1" && !take(model.undo()), "Undo did not stop at limit");
    take(model.redo());
    take(model.set_cell_value({0, 1}, std::int64_t(10)));
    check(!take(model.redo()), "New branch retained obsolete redo");
    take(model.clear_history());
    check(take(model.state()).undo_depth == 0, "History clear failed");
    take(model.set_undo_limit(0));
    take(model.set_cell_value({0, 1}, std::int64_t(11)));
    check(take(model.state()).undo_depth == 0, "Disabled history retained edit");
}
void observation() {
    lvrs::TableModel model;
    take(model.set_options(fixture()));
    int notifications = 0;
    auto receiver = std::make_shared<lvrs::TableChangePort>([&](const lvrs::TableChange& change) {
        ++notifications;
        check(change.cells && change.affected == lvrs::TableRange{0, 1, 1, 1}, "Cell change lost affected range");
        auto nested = model.set_cell_value({0, 1}, std::int64_t(100));
        check(!nested && nested.error().code == core::ErrorCode::busy, "Notification mutation recursively committed");
        check(std::get<std::int64_t>(*take(model.cell_raw_value({0, 1}))) == 42, "Notification observed uncommitted data");
    });
    good(model.observe(receiver));
    good(model.observe(receiver));
    take(model.set_cell_value({0, 1}, std::int64_t(42)));
    check(notifications == 1, "Receiver registered twice");
    receiver.reset();
    take(model.set_cell_value({0, 1}, std::int64_t(43)));
    check(notifications == 1, "Dead receiver retained");
    auto failure = std::make_shared<lvrs::TableChangePort>([](const auto&) { throw std::runtime_error("receiver failure"); });
    good(model.observe(failure));
    check(!model.set_cell_value({0, 1}, std::int64_t(44)) && take(model.cell_text({0, 1})) == "44", "Notification error rolled back committed model");
    core::ErrorCode code = core::ErrorCode::failed;
    std::thread worker([&] { auto result = model.set_cell_value({0, 1}, std::int64_t(45)); if (!result) code = result.error().code; });
    worker.join();
    check(code == core::ErrorCode::wrong_executor, "Foreign-thread mutation accepted");
}
void context() {
    lvrs::TableModel model;
    take(model.set_options(fixture()));
    take(model.set_context_cell({1, 1}));
    auto actions = take(model.context_menu({1, 1}));
    check(actions.size() == 3 && actions[1].divider && actions[0].event_name == "table.deleteRow", "Context action contract differs");
    take(model.trigger_context_action(lvrs::TableContextAction::delete_row, {1, 1}));
    check(model.row_count() == 2 && !take(model.state()).context_cell, "Context delete did not clear target");
    take(model.undo());
    check(take(model.cell_text({1, 0})) == "Alpha", "Context undo lost deleted row");
}
void invalid() {
    lvrs::TableModel model;
    auto before = take(model.state());
    auto options = fixture();
    options.rows[2].cells[2].value = std::string("bad\0text", 8);
    check(!model.set_options(options) && take(model.state()).revision == before.revision, "Invalid final cell partially replaced model");
    options = fixture();
    options.row_heights = {std::nan("")};
    check(!model.set_options(options), "Invalid row height accepted");
    options = fixture();
    options.columns[0].trailing_icon = "unknown-icon";
    check(!model.set_options(options) && take(model.state()).revision == before.revision,
          "Invalid header icon changed the model");
    options = fixture();
    options.merges = {{0, 0, std::numeric_limits<std::size_t>::max(), 1}};
    check(!model.set_options(options), "Overflowing merge range accepted");
    options = fixture();
    options.rows.clear(); options.row_heights.clear();
    take(model.set_options(options));
    check(take(model.visible_cells(0, 100)).empty() && !model.cell_at({0, 0}), "Empty model exposed a body cell");
    take(model.insert_row(0));
    check(take(model.cell_text({0, 1})) == "0", "Empty model could not create a typed row");
    options = fixture();
    options.rows[0].cells.clear();
    options.rows[1].cells.resize(1);
    take(model.set_options(options));
    check(take(model.visible_cells(0, 94)).size() == 7 && !model.set_cell_value({0, 1}, std::int64_t(7)), "Ragged/placeholder row contract changed");
}
}
int main(int argc, char** argv) {
    try {
        const std::string name = argc > 1 ? argv[1] : "values";
        if (name == "values") values(); else if (name == "range") range();
        else if (name == "geometry") geometry(); else if (name == "merges") merges();
        else if (name == "sorting") sorting(); else if (name == "structure") structure();
        else if (name == "resizing") resizing(); else if (name == "viewport") viewport();
        else if (name == "history") history(); else if (name == "observation") observation();
        else if (name == "context") context(); else if (name == "invalid") invalid();
        else throw std::runtime_error("Unknown table model case");
        std::cout << "LVRS_TABLE_MODEL_OK " << name << '\n';
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
