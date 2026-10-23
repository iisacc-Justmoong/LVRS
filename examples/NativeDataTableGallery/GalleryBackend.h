#pragma once
#include <LVRS/Table.h>
#include <stdexcept>
class DataTableBackend final {
public:
    inline static int created=0,destroyed=0;
    std::shared_ptr<lvrs::TableModel> model=std::make_shared<lvrs::TableModel>();
    std::function<void(std::size_t,std::size_t,std::string)> on_edited,on_submitted;
    std::function<void(std::size_t,std::size_t,std::string,std::string)> on_rejected;
    std::function<void()> on_close;
    std::function<void(std::string)> on_operation;
    DataTableBackend(){
        ++created;lvrs::TableModelOptions data;data.inputable=true;
        data.columns={{"Asset","string"},{"Count","int"},{"Ready","bool"},{"Collection","string"},{"Notes","string"}};data.column_widths={300,180,180,220,320};
        lvrs::TableRowData row{{{.value=std::string("Asset")},{.value=std::int64_t(12)},{.value=true},{.value=std::string("Library")},{.value=std::string("Native horizontal viewport")}}};
        data.rows.assign(100000,row);
        for(std::size_t i=0;i<data.rows.size();++i)data.rows[i].cells[0].value="Asset "+std::to_string(i);
        if(auto result=model->set_options(std::move(data));!result)throw std::runtime_error(result.error().message);
    }
    ~DataTableBackend(){++destroyed;}
    void edited(std::size_t row,std::size_t column,std::string value){if(on_edited)on_edited(row,column,std::move(value));}
    void submitted(std::size_t row,std::size_t column,std::string value){if(on_submitted)on_submitted(row,column,std::move(value));}
    void rejected(std::size_t row,std::size_t column,std::string value,std::string type){if(on_rejected)on_rejected(row,column,std::move(value),std::move(type));}
    void close(){if(on_close)on_close();}
    void range_changed(lvrs::TableRange range){report("Pasted "+std::to_string(range.row_count)+" rows");}
    void column_resized(std::size_t column,double width){report("Column "+std::to_string(column)+": "+std::to_string(int(width)));}
    void row_resized(std::size_t row,double height){report("Row "+std::to_string(row)+": "+std::to_string(int(height)));}
    void append_row(){check(model->insert_row(model->row_count()));report("Appended row");}
    void undo(){check(model->undo());report("Undo");}
    void redo(){check(model->redo());report("Redo");}
private:
    void report(std::string message){if(on_operation)on_operation(std::move(message));}
    static void check(ii::core::Result<bool> result){if(!result)throw std::runtime_error(result.error().message);}
};
