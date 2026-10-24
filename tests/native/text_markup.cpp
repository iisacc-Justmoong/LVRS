#include <LVRS/TextMarkup.h>
#include <iostream>
#include <stdexcept>
using lvrs::TextMarkup;
namespace {
void check(bool v,const char* m){if(!v)throw std::runtime_error(m);}
std::string take(ii::core::Result<std::string> r){if(!r)throw std::runtime_error(r.error().message);return std::move(*r);}
void run(const std::string& name){
    if(name=="newlines")check(take(TextMarkup::normalize("a\r\nb\rc<br/>d<P>e</p>"))=="a\nb\nc\nd\ne\n","Newline/tag normalization changed");
    else if(name=="tags")check(take(TextMarkup::normalize("<B>a</b> <strong>b</strong> <i>c</I> <em>d</em> <code>e</code>"))=="**a** **b** *c* *d* `e`","Legacy inline tag conversion changed");
    else if(name=="unknown")check(take(TextMarkup::normalize("<span color='red'>x</span> <> <b>open"))=="x <> open","Unsupported/unmatched tag handling changed");
    else if(name=="html")check(take(TextMarkup::render_html("**bold** __also__ *em* _also_\n`<b>code</b>`"))=="<strong>bold</strong> <strong>also</strong> <em>em</em> <em>also</em><br/><code>**code**</code>","Inline HTML rendering changed");
    else if(name=="escaping")check(take(TextMarkup::render_html("& \" <script>literal</script>"))=="&amp; &quot; literal","Escaped literal text became HTML markup");
    else if(name=="code")check(take(TextMarkup::render_html("**`a*b`** `**plain**`"))=="<strong><code>a*b</code></strong> <code>**plain**</code>","Code spans were interpreted as emphasis");
    else if(name=="placeholder")check(take(TextMarkup::render_html("{{{CODE_0}}} `safe`"))=="{{{CODE_0}}} <code>safe</code>","Literal text collided with an internal code placeholder");
    else if(name=="plain")check(take(TextMarkup::render_plain_text("<b>한글</b> **x**"))=="**한글** **x**","Plain rendering removed legacy markdown markers");
    else if(name=="empty")check(take(TextMarkup::render_html("")).empty()&&take(TextMarkup::normalize("<b></b>"))=="****","Empty markup changed");
    else if(name=="invalid")check(!TextMarkup::normalize(std::string("\xc0\xaf",2)),"Invalid UTF-8 accepted");
    else if(name=="large"){std::string text;for(unsigned i=0;i<10000;++i)text+="**a** `b`\n";auto html=take(TextMarkup::render_html(text));check(html.size()>text.size()&&html.find("<strong>a</strong> <code>b</code><br/>")==0,"Large markup transform changed");}
    else throw std::runtime_error("Unknown markup case");
}
}
int main(int argc,char** argv){try{run(argc>1?argv[1]:"tags");std::cout<<"TEXT_MARKUP_OK\n";}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
