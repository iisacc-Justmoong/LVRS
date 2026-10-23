#pragma once
#include <LVRS/TableCellItem.h>
#include <LVRS/TableHeaderModel.h>
class TableBackend final {
public:
    inline static int created=0,destroyed=0;
    lvrs::TableValidator validator=lvrs::validate_table_value;
    std::function<void(std::string)> on_edited,on_submitted;
    std::function<void(std::string,std::string)> on_rejected;
    std::function<void(std::size_t,std::string)> on_row_edited,on_row_submitted;
    std::function<void(std::size_t,const lvrs::TableHeaderColumn&)> on_column;
    std::function<void()> on_close;
    TableBackend(){++created;}~TableBackend(){++destroyed;}
    void edited(std::string s){if(on_edited)on_edited(std::move(s));}void submitted(std::string s){if(on_submitted)on_submitted(std::move(s));}
    void rejected(std::string s,std::string t){if(on_rejected)on_rejected(std::move(s),std::move(t));}
    void rowEdited(std::size_t i,std::string s){if(on_row_edited)on_row_edited(i,std::move(s));}void rowSubmitted(std::size_t i,std::string s){if(on_row_submitted)on_row_submitted(i,std::move(s));}
    void columnClicked(std::size_t i,const lvrs::TableHeaderColumn& c){if(on_column)on_column(i,c);}void close(){if(on_close)on_close();}
};
