#include "HierarchyVisibility.h"
#include <algorithm>
#include <limits>
namespace lvrs::hierarchy_detail {
namespace {constexpr auto absent=std::numeric_limits<std::size_t>::max();}
void VisibilityIndex::pull(std::size_t i){
    auto& n=nodes_[i];const auto& a=nodes_[i*2];const auto& b=nodes_[i*2+1];const auto minimum=std::min(a.minimum,b.minimum);
    n.minimum=minimum+n.lazy;n.count=(a.minimum==minimum?a.count:0)+(b.minimum==minimum?b.count:0);
    n.eligible=(a.minimum==minimum?a.eligible:0)+(b.minimum==minimum?b.eligible:0);n.extent=(a.minimum==minimum?a.extent:0)+(b.minimum==minimum?b.extent:0);
}
void VisibilityIndex::build(std::size_t i,std::size_t first,std::size_t end,std::span<const Leaf> leaves){
    if(end-first==1){auto l=leaves[first];nodes_[i]={l.hidden,0,1,std::size_t(l.eligible),l.extent};return;}
    auto middle=(first+end)/2;build(i*2,first,middle,leaves);build(i*2+1,middle,end,leaves);pull(i);
}
void VisibilityIndex::reset(std::span<const Leaf> leaves){std::vector<Node> next(leaves.empty()?0:leaves.size()*4);nodes_=std::move(next);size_=leaves.size();if(size_)build(1,0,size_,leaves);}
void VisibilityIndex::add(std::size_t i,std::size_t first,std::size_t end,std::size_t begin,std::size_t finish,int delta){
    if(finish<=first||begin>=end)return;auto& n=nodes_[i];
    if(begin<=first&&end<=finish){n.minimum+=delta;n.lazy+=delta;return;}
    auto middle=(first+end)/2;add(i*2,first,middle,begin,finish,delta);add(i*2+1,middle,end,begin,finish,delta);pull(i);
}
void VisibilityIndex::add(std::size_t first,std::size_t end,int delta){if(size_&&first<end)add(1,0,size_,first,end,delta);}
void VisibilityIndex::update(std::size_t i,std::size_t first,std::size_t end,std::size_t position,double value,bool eligible_value,bool set_extent){
    auto& n=nodes_[i];if(end-first==1){if(set_extent)n.extent=value;else n.eligible=eligible_value;return;}
    auto middle=(first+end)/2;if(position<middle)update(i*2,first,middle,position,value,eligible_value,set_extent);else update(i*2+1,middle,end,position,value,eligible_value,set_extent);pull(i);
}
void VisibilityIndex::extent(std::size_t i,double value){if(i<size_)update(1,0,size_,i,value,false,true);}
void VisibilityIndex::eligible(std::size_t i,bool value){if(i<size_)update(1,0,size_,i,0,value,false);}
std::size_t VisibilityIndex::count()const{return size_&&nodes_[1].minimum==0?nodes_[1].count:0;}
std::size_t VisibilityIndex::eligible_count()const{return size_&&nodes_[1].minimum==0?nodes_[1].eligible:0;}
double VisibilityIndex::extent()const{return size_&&nodes_[1].minimum==0?nodes_[1].extent:0;}
VisibilityIndex::Node VisibilityIndex::range(std::size_t i,std::size_t first,std::size_t end,std::size_t finish,int inherited)const{
    if(first>=finish)return {};const auto& n=nodes_[i];
    if(end<=finish){if(n.minimum+inherited)return {};return {0,0,n.count,n.eligible,n.extent};}
    auto middle=(first+end)/2;auto a=range(i*2,first,middle,finish,inherited+n.lazy),b=range(i*2+1,middle,end,finish,inherited+n.lazy);
    return {0,0,a.count+b.count,a.eligible+b.eligible,a.extent+b.extent};
}
std::size_t VisibilityIndex::prefix_count(std::size_t end,bool eligible)const{if(!size_)return 0;auto n=range(1,0,size_,std::min(size_,end),0);return eligible?n.eligible:n.count;}
double VisibilityIndex::prefix_extent(std::size_t end)const{return size_?range(1,0,size_,std::min(size_,end),0).extent:0;}
bool VisibilityIndex::visible(std::size_t p)const{return p<size_&&prefix_count(p+1)>prefix_count(p);}
std::size_t VisibilityIndex::at(std::size_t rank,bool eligible)const{
    if(rank>=(eligible?eligible_count():count()))return absent;std::size_t i=1,first=0,end=size_;int inherited=0;
    while(end-first>1){inherited+=nodes_[i].lazy;const auto& left=nodes_[i*2];auto count=left.minimum+inherited==0?(eligible?left.eligible:left.count):0;auto middle=(first+end)/2;if(rank<count){i*=2;end=middle;}else{rank-=count;i=i*2+1;first=middle;}}
    return first;
}
std::size_t VisibilityIndex::at_offset(double offset)const{
    if(offset<0||offset>=extent())return absent;std::size_t i=1,first=0,end=size_;int inherited=0;
    while(end-first>1){inherited+=nodes_[i].lazy;const auto& left=nodes_[i*2];const auto sum=left.minimum+inherited==0?left.extent:0;auto middle=(first+end)/2;if(offset<sum){i*=2;end=middle;}else{offset-=sum;i=i*2+1;first=middle;}}
    return first;
}
} // namespace lvrs::hierarchy_detail
