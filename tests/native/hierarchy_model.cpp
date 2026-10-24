#include <LVRS/HierarchyModel.h>
#include <algorithm>
#include <iostream>
#include <random>
#include <stdexcept>
#include <thread>
#include <type_traits>
using namespace ii;
using namespace lvrs;
static_assert(!std::is_copy_constructible_v<HierarchyModel>&&!std::is_move_constructible_v<HierarchyModel>);
namespace {
void check(bool condition,const char* message){if(!condition)throw std::runtime_error(message);}
template<class T>T take(core::Result<T> value){if(!value)throw std::runtime_error(value.error().message);return std::move(*value);}
void good(core::Result<void> value){if(!value)throw std::runtime_error(value.error().message);}
std::vector<HierarchyRow> rows(){return {
    {.key="a",.label="Root",.item_id=10,.depth=0,.expanded=true},
    {.key="b",.label="Branch",.item_id=20,.depth=1,.expanded=true},
    {.key="c",.label="한글 😀",.item_id=30,.depth=2},
    {.key="d",.label="Sibling",.item_id=40,.depth=1},
    {.key="e",.label="Other",.item_id=50,.depth=0},
    {.key="f",.label="Hidden",.item_id=60,.depth=1},
    {.key="g",.label="Disabled",.item_id=70,.depth=0,.enabled=false}
};}
std::size_t end_of(const std::vector<HierarchyRow>& data,std::size_t i){auto end=i+1;while(end<data.size()&&data[end].depth>data[i].depth)++end;return end;}
void compare(const std::shared_ptr<HierarchyModel>& model,const std::vector<HierarchyRow>& expected){
    auto actual=take(model->rows());check(std::ranges::equal(actual,expected),"Preorder rows differ from the independent splice oracle");
    for(std::size_t i=0;i<expected.size();++i){
        const auto relation=take(model->relation(i));std::size_t parent=hierarchy_npos;
        if(expected[i].depth)for(auto j=i;j-->0;)if(expected[j].depth+1==expected[i].depth){parent=j;break;}
        check(relation.parent==parent&&relation.subtree_end==end_of(expected,i),"Parent/subtree index differs from the independent preorder walk");
        check(take(model->index_of_key(expected[i].key))==i,"Owned key index changed after moving short strings");
        std::vector<std::size_t> siblings,children;
        for(std::size_t j=0;j<expected.size();++j){
            if(j>i&&j<end_of(expected,i)&&expected[j].depth==expected[i].depth+1)children.push_back(j);
            if(expected[j].depth==expected[i].depth&&(parent==hierarchy_npos||(j>parent&&j<end_of(expected,parent))))siblings.push_back(j);
        }
        auto at=std::size_t(std::ranges::find(siblings,i)-siblings.begin());
        check(relation.sibling_count==siblings.size()&&relation.sibling_index==at&&relation.child_count==children.size(),"Sibling/child metadata differs from the independent walk");
        check(relation.first_child==(children.empty()?hierarchy_npos:children.front()),"First child does not match preorder");
        check(relation.previous_sibling==(at?siblings[at-1]:hierarchy_npos)&&relation.next_sibling==(at+1<siblings.size()?siblings[at+1]:hierarchy_npos),"Sibling navigation links are wrong");
    }
}
void splice(std::vector<HierarchyRow>& data,const HierarchyMovePlan& plan){
    std::vector<HierarchyRow> block(data.begin()+std::ptrdiff_t(plan.source),data.begin()+std::ptrdiff_t(plan.source_end));
    auto delta=std::int64_t(plan.depth)-block.front().depth;for(auto& row:block)row.depth=std::uint32_t(std::int64_t(row.depth)+delta);
    data.erase(data.begin()+std::ptrdiff_t(plan.source),data.begin()+std::ptrdiff_t(plan.source_end));
    data.insert(data.begin()+std::ptrdiff_t(plan.insertion),block.begin(),block.end());
}
void run(const std::string& name){
    auto model=take(HierarchyModel::create());
    if(name=="empty"){auto state=take(model->state());check(state.count==0&&state.revision==1&&state.editable&&!state.closed,"Default model state changed");check(!model->row(0)&&!model->index_of_key("missing")&&!model->relation(0),"Empty model lookup accepted");return;}
    if(name=="options"){auto options=HierarchyModelOptions{};options.maximum_rows=0;check(!HierarchyModel::create(options),"Zero row budget accepted");options={};options.maximum_depth=1025;check(!HierarchyModel::create(options),"Unbounded depth accepted");options={};options.maximum_metadata_bytes=0;check(!HierarchyModel::create(options),"Invalid metadata budget accepted");return;}
    if(name=="normalization"){auto data=std::vector<HierarchyRow>{{.key="  alpha \t",.item_id=9},{.item_id=4},{}};take(model->set_rows(data));auto out=take(model->rows());check(out[0].key=="alpha"&&out[0].label=="alpha"&&out[1].key=="4"&&out[2].key=="2","Identity/label fallback normalization changed");return;}
    auto expected=rows();take(model->set_rows(expected));
    if(name=="topology")compare(model,expected);
    else if(name=="unicode"){check(take(model->row("c")).get().label=="한글 😀","Unicode label was changed");auto row=expected[2];row.label=std::string("\xc0\x80",2);check(!model->update_row("c",row)&&take(model->row("c")).get()==expected[2],"Malformed UTF-8 mutation was committed");}
    else if(name=="duplicate"){auto invalid=expected;invalid.back().key="a";check(!model->set_rows(invalid),"Duplicate key admitted");compare(model,expected);}
    else if(name=="depth"){auto invalid=expected;invalid[0].depth=1;check(!model->set_rows(invalid),"Nonzero root admitted");invalid=expected;invalid[3].depth=4;check(!model->set_rows(invalid),"Preorder depth gap admitted");compare(model,expected);}
    else if(name=="limits"){auto limited=take(HierarchyModel::create({.maximum_rows=3}));check(!limited->set_rows(expected)&&take(limited->state()).count==0,"Row budget allowed partial commit");auto options=HierarchyModelOptions{};options.maximum_data_bytes=4096;limited=take(HierarchyModel::create(options));std::vector<HierarchyRow> huge{{.key="large",.label=std::string(5000,'x')}};check(!limited->set_rows(huge),"Text admission budget ignored");options={};options.maximum_text_bytes=2;limited=take(HierarchyModel::create(options));check(!limited->set_rows({{.key="long"}})&&take(limited->state()).count==0,"Fallback label bypassed normalized text admission");}
    else if(name=="update"){auto before=take(model->stats());expected[2].label="Changed";take(model->update_row("c",expected[2]));compare(model,expected);auto after=take(model->stats());check(after.topology_builds==before.topology_builds&&after.data_updates==before.data_updates+1,"Data-only update rebuilt topology");}
    else if(name=="identity"){auto row=expected[2];row.depth=1;check(!model->update_row("c",row),"Data-only update changed topology");row=expected[2];row.item_id=4;check(!model->update_row("c",row),"Data-only update changed identity");compare(model,expected);}
    else if(name=="noop"){auto revision=take(model->state()).revision;check(!take(model->set_rows(expected))&&!take(model->update_row("c",expected[2]))&&!take(model->set_editable(true))&&take(model->state()).revision==revision,"No-op changed revision");}
    else if(name=="metadata"){auto m=take(model->metadata(2));check(m.parent_key=="b"&&m.parent_path_label=="Root / Branch"&&m.path_label=="Root / Branch / 한글 😀"&&m.ancestor_keys==std::vector<std::string>{"a","b"}&&m.path_keys==std::vector<std::string>{"a","b","c"},"Requested path metadata is wrong");m=take(model->metadata(0));check(m.child_keys==std::vector<std::string>{"b","d"}&&m.child_count==2&&m.descendant_count==3&&!m.visible,"Data model eagerly invented view-local visibility");}
    else if(name=="metadata_budget"){auto options=HierarchyModelOptions{};options.maximum_metadata_bytes=1024;auto limited=take(HierarchyModel::create(options));std::vector<HierarchyRow> data;for(unsigned i=0;i<16;++i)data.push_back({.key=std::to_string(i),.label=std::string(128,'x'),.depth=i});take(limited->set_rows(data));auto m=limited->metadata(15);check(!m&&m.error().code==core::ErrorCode::full,"Unbounded metadata query admitted");check(take(limited->state()).count==16,"Metadata rejection damaged source");}
    else if(name=="ids"){expected[1].item_id=10;take(model->set_rows(expected));check(take(model->index_of_id(10))==0&&!model->index_of_id(-44),"First matching ID lookup changed");}
    else if(name=="before"||name=="after"||name=="child"||name=="root"||name=="subtree"){
        const auto source=name=="subtree"?"a":name=="root"?"c":"d";const auto gap=name=="before"?1:name=="after"?6:name=="child"?3:7;
        auto plan=take(model->plan_move(source,gap,name=="root"?0:name=="child"?2:1));splice(expected,plan);take(model->commit_move(plan));compare(model,expected);
    }
    else if(name=="cycle"){check(!model->plan_move("a",2,2),"Insertion inside moved subtree accepted");check(!model->plan_move("a",99,0),"Out-of-range gap accepted");compare(model,expected);}
    else if(name=="stale"){auto plan=take(model->plan_move("d",6,1));auto row=expected[0];row.label="New";take(model->update_row("a",row));auto moved=model->commit_move(plan);check(!moved&&moved.error().code==core::ErrorCode::busy,"Stale plan accepted");}
    else if(name=="tampered"){auto plan=take(model->plan_move("d",6,1));++plan.source_end;check(!model->commit_move(plan),"Tampered plan accepted");compare(model,expected);}
    else if(name=="readonly"){auto plan=take(model->plan_move("d",6,1));take(model->set_editable(false));plan=take(model->plan_move("d",6,1));auto moved=model->commit_move(plan);check(!moved&&moved.error().code==core::ErrorCode::unsupported,"Read-only source moved");}
    else if(name=="coordinates"){auto plan=take(model->resolve_drag_target("d",6,24,8,8));check(plan.depth==2,"Pointer depth rounding changed");check(!model->resolve_drag_target("d",6,std::numeric_limits<double>::quiet_NaN())&&!model->resolve_drag_target("d",6,0,-1),"Invalid pointer geometry accepted");}
    else if(name=="subtree_depth"){auto options=HierarchyModelOptions{};options.maximum_depth=2;auto limited=take(HierarchyModel::create(options));take(limited->set_rows(expected));auto plan=limited->plan_move("a",6,2);check(!plan&&plan.error().code==core::ErrorCode::out_of_range,"Moved descendants exceeded the depth budget");compare(limited,expected);}
    else if(name=="move_noop"){auto plan=take(model->plan_move("b",1,1));auto revision=take(model->state()).revision;check(!take(model->commit_move(plan))&&take(model->state()).revision==revision,"Same-position subtree move mutated source");}
    else if(name=="observe"||name=="dedup"||name=="weak"){unsigned deliveries=0;auto port=std::make_shared<HierarchyChangePort>([&](const auto& c){if(c.kind==HierarchyChangeKind::data){++deliveries;check(c.first==2&&c.end==3&&take(model->state()).revision==c.revision,"Notification did not expose committed state");}});good(model->observe(port));if(name=="dedup")good(model->observe(port));if(name=="weak")port.reset();auto row=expected[2];row.label="New";take(model->update_row("c",row));check(deliveries==(name=="weak"?0u:1u),"Weak observer or dedup policy failed");if(port)(void)port->close();}
    else if(name=="reentrant"){bool rejected=false;auto port=std::make_shared<HierarchyChangePort>([&](const auto& c){if(c.kind!=HierarchyChangeKind::data)return;auto changed=model->set_editable(false);rejected=!changed&&changed.error().code==core::ErrorCode::busy;});good(model->observe(port));auto row=expected[2];row.label="New";take(model->update_row("c",row));check(rejected&&take(model->state()).editable,"Notification allowed source reentry");(void)port->close();}
    else if(name=="fanout"){unsigned healthy=0;auto bad=std::make_shared<HierarchyChangePort>([](const auto&){throw std::runtime_error("receiver failure");});auto port=std::make_shared<HierarchyChangePort>([&](const auto&){++healthy;});good(model->observe(bad));good(model->observe(port));auto row=expected[2];row.label="Committed";auto updated=model->update_row("c",row);check(!updated&&updated.error().code==core::ErrorCode::failed&&healthy==1&&take(model->row("c")).get().label=="Committed","Receiver failure hid a committed mutation or skipped a healthy observer");(void)bad->close();(void)port->close();}
    else if(name=="closed"){unsigned closed=0;auto port=std::make_shared<HierarchyChangePort>([&](const auto& c){if(c.kind==HierarchyChangeKind::closed)++closed;});good(model->observe(port));good(model->close());good(model->close());check(closed==1&&take(model->state()).closed&&!model->rows()&&!model->set_editable(false),"Closed model admitted work or delivered twice");(void)port->close();}
    else if(name=="close_in_notify"){unsigned closed=0;auto first=std::make_shared<HierarchyChangePort>([&](const auto& c){if(c.kind==HierarchyChangeKind::data)good(model->close());});auto last=std::make_shared<HierarchyChangePort>([&](const auto& c){if(c.kind==HierarchyChangeKind::closed)++closed;else throw std::runtime_error("stale delivery after close");});good(model->observe(first));good(model->observe(last));auto row=expected[2];row.label="Close";take(model->update_row("c",row));check(closed==1,"Terminal close was not delivered during reentry");(void)first->close();(void)last->close();}
    else if(name=="lifetime"){auto raw=model.get();bool released=false;auto port=std::make_shared<HierarchyChangePort>([&](const auto& c){if(c.kind==HierarchyChangeKind::data){model.reset();released=true;}});good(model->observe(port));auto row=expected[2];row.label="Release";take(raw->update_row("c",row));check(released&&!model,"Observer release did not destroy the public model safely");(void)port->close();}
    else if(name=="owner"){bool rejected=false;std::thread worker([&]{auto r=model->state();auto m=model->set_rows({});rejected=!r&&!m&&r.error().code==core::ErrorCode::wrong_executor&&m.error().code==core::ErrorCode::wrong_executor;});worker.join();check(rejected,"Wrong owner accessed the source");compare(model,expected);}
    else if(name=="large"){std::vector<HierarchyRow> data;for(std::size_t i=0;i<100000;++i)data.push_back({.key=std::to_string(i),.depth=std::uint32_t(i%100?1:0)});take(model->set_rows(std::move(data)));auto before=take(model->stats());for(unsigned i=0;i<1000;++i)take(model->resolve_drag_target("0",100000,8));auto after=take(model->stats());check(after.move_plans-before.move_plans==1000&&after.topology_builds==before.topology_builds&&after.metadata_queries==before.metadata_queries,"Drag planning rebuilt tree/path metadata");check(take(model->relation(0)).subtree_end==100,"Large subtree boundary changed");}
    else if(name=="random"){std::mt19937 random(521);for(unsigned attempt=0;attempt<600;++attempt){const auto source=random()%expected.size(),gap=random()%(expected.size()+1);if(gap>source&&gap<end_of(expected,source))continue;auto plan=model->plan_move(expected[source].key,gap,random()%5);if(!plan){check(plan.error().code==core::ErrorCode::out_of_range,"Valid preorder gap failed planning");continue;}splice(expected,*plan);take(model->commit_move(*plan));compare(model,expected);}}
    else throw std::runtime_error("Unknown hierarchy model case");
}
}
int main(int argc,char** argv){try{run(argc>1?argv[1]:"topology");std::cout<<"HIERARCHY_MODEL_OK\n";}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
