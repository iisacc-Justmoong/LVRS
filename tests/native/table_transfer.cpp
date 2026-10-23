#include <LVRS/TableTransfer.h>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
template<class T>T take(ii::core::Result<T> result){if(!result)throw std::runtime_error(result.error().message);return std::move(*result);}
void run(std::string_view name){
    using namespace lvrs;
    if(name=="addresses"){
        check(take(table_column_name(0))=="A"&&take(table_column_name(26))=="AA"&&take(table_column_name(65535))=="CRXP","Column address encoding failed");
        check(take(table_column_index(" aa "))==26&&take(table_cell_coordinates("Bc12"))==TablePosition{11,54},"Cell address decoding failed");
        check(take(table_cell_reference({999999,65535}))=="CRXP1000000","Maximum model address failed");
        check(!table_cell_coordinates("A0")&&!table_cell_coordinates("A01")&&!table_cell_coordinates("A184467440737095516160")&&!table_column_index("A-1"),"Malformed address accepted");
        check(!table_column_name(65536)&&!table_cell_reference({1000000,0}),"Address exceeded the model domain");
    }else if(name=="tsv"){
        TableMatrix values{{std::string("one\ttwo"),std::string("a\"b"),std::string("line\r\nnext"),std::nullopt},{std::int64_t(42),true,1.5,std::string{}}};
        auto encoded=take(table_values_as_tsv(values));auto decoded=take(parse_table_tsv(encoded));
        check(decoded.size()==2&&decoded[0].size()==4&&std::get<std::string>(decoded[0][0])=="one\ttwo"&&std::get<std::string>(decoded[0][1])=="a\"b"&&std::get<std::string>(decoded[0][2])=="line\r\nnext","Quoted TSV lost values");
        check(std::get<std::string>(decoded[1][0])=="42"&&std::get<std::string>(decoded[1][1])=="true"&&std::get<std::string>(decoded[1][2])=="1.5","Typed TSV conversion failed");
        check(take(parse_table_tsv("a\tb\r\nc\td\n")).size()==2&&take(parse_table_tsv("")).front().size()==1,"TSV terminal newline/empty rules changed");
        check(std::get<std::string>(take(parse_table_tsv("\"open")).front().front())=="open"&&std::get<std::string>(take(parse_table_tsv("\"x\"tail")).front().front())=="xtail","Original permissive quote behavior changed");
        TableModel model;TableModelOptions data;data.columns={{"A","string"},{"B","string"},{"C","string"}};data.rows={{{{.value=std::string("a")},{.value=std::string("b")}}},{{{.value=std::string("c")}}}};
        take(model.set_options(std::move(data)));check(!table_range_as_tsv(model,{0,0,2,3})&&take(table_range_as_tsv(model,{0,0,1,2}))=="a\tb","Streaming range export changed missing-cell rules");
        check(!table_range_as_tsv(model,{0,0,1,2},2),"Streaming output did not stop at its byte budget");
    }else if(name=="limits"){
        check(!parse_table_tsv(std::string(65537,'x'))&&!parse_table_tsv(std::string(16*1024*1024+1,'x')),"Unbounded TSV accepted");
        check(!parse_table_tsv(std::string(65536,'\t')),"Too many transfer columns accepted");
        check(!table_values_as_tsv({{std::numeric_limits<double>::infinity()}}),"Nonfinite transfer value accepted");
        check(!table_values_as_tsv({{std::string(65537,'x')}}),"Oversized output cell accepted");
    }else if(name=="unicode"){
        auto text=take(table_values_as_tsv({{std::string("한글 🙂"),std::string("quote\"\t값")}}));
        check(std::get<std::string>(take(parse_table_tsv(text))[0][0])=="한글 🙂","UTF-8 transfer changed text");
        check(!parse_table_tsv(std::string("a\0b",3))&&!parse_table_tsv(std::string("\xc0\xaf",2)),"Invalid UTF-8/NUL transfer accepted");
    }else throw std::runtime_error("Unknown table transfer case");
}
}
int main(int argc,char** argv){try{run(argc>1?argv[1]:"tsv");std::cout<<"TABLE_TRANSFER_OK\n";}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
