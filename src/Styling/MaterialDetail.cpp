#include <LVRS/MaterialDetail.h>
#include <LVRS/SurfaceDetail.h>
#include <iiSDKCore/Core/Text/Number.h>
#include <array>
#include <algorithm>
#include <cmath>
namespace lvrs::material_detail {
using namespace ii;
namespace {
std::string number(double x){return core::format_decimal(x);}
std::string escaped(std::string_view s){std::string out;for(char c:s)switch(c){case '&':out+="&amp;";break;case '<':out+="&lt;";break;case '>':out+="&gt;";break;case '"':out+="&quot;";break;default:out+=c;}return out;}
std::string document(double w,double h,std::string body,double x=0,double y=0){return "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\""+number(w)+"\" height=\""+number(h)+"\" viewBox=\""+number(x)+" "+number(y)+" "+number(w)+" "+number(h)+"\">"+body+"</svg>";}
core::Result<std::shared_ptr<const gui::RasterImage>> raster(std::string source,double width,double height,double scale){
    if(!std::isfinite(scale)||scale<=0||scale>8)return core::fail(core::ErrorCode::invalid_argument,"Invalid Material backing scale");
    const auto w=std::ceil(width*scale),h=std::ceil(height*scale);if(w<1||h<1||w>gui::maximum_image_dimension||h>gui::maximum_image_dimension||w*h>gui::maximum_image_pixels)return core::fail(core::ErrorCode::out_of_range,"Material raster dimensions exceed the native image budget");return gui::svg_image(source,std::uint32_t(w),std::uint32_t(h));
}
std::string path(std::string_view d,std::string fill,std::string extra={}){return "<path d=\""+escaped(d)+"\" fill=\""+fill+"\" "+extra+"/>";}
std::string rgb(gui::WidgetColor c){return "rgb("+number(c.red*255)+","+number(c.green*255)+","+number(c.blue*255)+")";}
std::string gradients(double w,double h,double pad,double intense,double faint,double alpha){
    struct Spot{double x,y,rx,ry;bool strong;};const std::array<Spot,8> spots{{{w/2-129,h/2-28,200,200,true},{w/2+136,h/2+62,180,180,false},{48,32,200,200,true},{w-64,40,260,150,false},{0,h/2,140,300,false},{w,h/2,160,300,true},{w/2,h,380,150,false},{72,h-40,180,180,true}}};
    std::string defs="<defs>",body;for(unsigned i=0;i<spots.size();++i){auto spot=spots[i];const auto strength=(spot.strong?intense:faint)*alpha;const auto id="gradient"+std::to_string(i);defs+="<radialGradient id=\""+id+"\" gradientUnits=\"userSpaceOnUse\" cx=\"0\" cy=\"0\" r=\"1\" gradientTransform=\"translate("+number(spot.x)+" "+number(spot.y)+") scale("+number(spot.rx)+" "+number(spot.ry)+")\">";for(auto [at,factor]:{std::pair{0.,1.},std::pair{.38,.55},std::pair{.72,.13},std::pair{1.,0.}})defs+="<stop offset=\""+number(at)+"\" stop-color=\"white\" stop-opacity=\""+number(strength*factor)+"\"/>";defs+="</radialGradient>";body+="<rect x=\""+number(-pad)+"\" y=\""+number(-pad)+"\" width=\""+number(w+2*pad)+"\" height=\""+number(h+2*pad)+"\" fill=\"url(#"+id+")\"/>";}return document(w+2*pad,h+2*pad,defs+"</defs>"+body,-pad,-pad);
}
}
core::Result<Prepared> prepare(const MaterialOptions& o,MaterialVariant variant,const gui::WidgetAppearance& appearance){
    if(unsigned(variant)>2||(o.density&&unsigned(*o.density)>1)||unsigned(o.backdrop)>2||!surface_detail::text(o.outline_path)||!surface_detail::metric(o.width)||!surface_detail::metric(o.height))return core::fail(core::ErrorCode::invalid_argument,"Invalid Material variant, density, source, outline or extent");
    for(auto value:{o.primary_color,o.color,o.border_color})if(value&&!surface_detail::color(*value))return core::fail(core::ErrorCode::invalid_argument,"Invalid Material color");if(!surface_detail::color(o.backdrop_base_color))return core::fail(core::ErrorCode::invalid_argument,"Invalid Material base color");
    for(auto value:{o.tint_opacity,o.inner_highlight_opacity,o.shadow_opacity})if(value&&(!std::isfinite(*value)||*value<0||*value>1))return core::fail(core::ErrorCode::invalid_argument,"Invalid Material opacity");
    for(double value:{o.intense_opacity,o.faint_opacity})if(!std::isfinite(value)||value<0||value>1)return core::fail(core::ErrorCode::invalid_argument,"Invalid Material radial opacity");
    for(auto value:{o.blur_radius,o.radius,o.shadow_radius})if(value&&!surface_detail::metric(*value))return core::fail(core::ErrorCode::invalid_argument,"Invalid Material radius");
    for(double value:{o.border_width})if(!surface_detail::metric(value))return core::fail(core::ErrorCode::invalid_argument,"Invalid Material border");
    for(double value:{o.inner_highlight_offset_y,o.shadow_offset_y.value_or(0)})if(!std::isfinite(value)||std::abs(value)>8192)return core::fail(core::ErrorCode::invalid_argument,"Invalid Material offset");
    if(!o.outline_path.empty()){auto check=gui::svg_image(document(1,1,path(o.outline_path,"white")),1,1);if(!check)return std::unexpected(check.error());}
    Prepared p;auto& s=p.state;s.variant=variant;s.density=o.density.value_or(variant==MaterialVariant::window?MaterialDensity::dense75:MaterialDensity::glass25);const bool dense=s.density==MaterialDensity::dense75;
    s.tint_opacity=o.tint_opacity.value_or(variant==MaterialVariant::window?.5:dense?.75:.25);const auto configured_blur=o.blur_radius.value_or(dense?64:16);s.blur_radius=o.blur_enabled?std::clamp(std::ceil(configured_blur),2.,64.):0;s.capture_padding=std::clamp(std::ceil(configured_blur),2.,64.);
    s.radius=o.radius.value_or(variant==MaterialVariant::window?16:12);s.inner_highlight_opacity=o.inner_highlight_opacity.value_or(dense?.06:.14);s.shadow_radius=std::clamp(std::ceil(o.shadow_radius.value_or(variant==MaterialVariant::window?48:24)),2.,64.);s.shadow_offset_y=o.shadow_offset_y.value_or(variant==MaterialVariant::window?16:8);s.shadow_opacity=o.shadow_opacity.value_or(variant==MaterialVariant::window?.30:.22);
    s.border=o.border_color.value_or(gui::WidgetColor{1,1,1,dense?.12:.22});s.implicit_size=variant==MaterialVariant::panel?gui::Size{380,204}:variant==MaterialVariant::window?gui::Size{500,272}:gui::Size{};
    p.target.tint=o.color.value_or(variant==MaterialVariant::window?gui::WidgetColor{11./255,11./255,11./255,1}:gui::WidgetColor{20./255,20./255,20./255,1});p.target.tint.alpha*=s.tint_opacity;p.target.primary=o.primary_color.value_or(appearance.primary);p.target.base=o.backdrop_base_color;
    return p;
}
core::Result<std::shared_ptr<const gui::WidgetDrawing>> paint(const MaterialOptions& o,const Prepared& prepared,const Colors& colors,const gui::WidgetPaintState& state,RasterCache& cache){
    auto drawing=std::make_shared<gui::WidgetDrawing>();const auto w=state.size.width,h=state.size.height,scale=state.backing_scale;const auto& p=prepared.state;if(w<=0||h<=0)return drawing;
    const auto radius=std::min({p.radius,w/2,h/2});const bool geometry=cache.size!=state.size||cache.scale!=scale||cache.path!=o.outline_path;const bool gradient=geometry||cache.padding!=p.capture_padding||cache.intense!=o.intense_opacity||cache.faint!=o.faint_opacity||cache.primary_alpha!=colors.primary.alpha;
    auto mask=cache.mask,radials=cache.gradient,stroke=cache.stroke;
    if(geometry){mask.reset();if(!o.outline_path.empty()){auto result=raster(document(w,h,path(o.outline_path,"white")),w,h,scale);if(!result)return std::unexpected(result.error());mask=*result;++cache.masks;}}
    if(gradient){radials.reset();if(colors.primary.alpha>0&&(o.intense_opacity>0||o.faint_opacity>0)){auto result=raster(gradients(w,h,p.capture_padding,o.intense_opacity,o.faint_opacity,colors.primary.alpha),w+2*p.capture_padding,h+2*p.capture_padding,scale);if(!result)return std::unexpected(result.error());radials=*result;++cache.gradients;}}
    if(geometry||cache.border!=p.border||cache.border_width!=o.border_width){stroke.reset();if(mask&&o.border_width>0&&p.border.alpha>0){auto result=raster(document(w,h,path(o.outline_path,"none","stroke=\""+rgb(p.border)+"\" stroke-opacity=\""+number(p.border.alpha)+"\" stroke-width=\""+number(o.border_width)+"\" stroke-linejoin=\"round\"")),w,h,scale);if(!result)return std::unexpected(result.error());stroke=*result;++cache.strokes;}}
    cache.size=state.size;cache.scale=scale;cache.path=o.outline_path;cache.radius=radius;cache.padding=p.capture_padding;cache.intense=o.intense_opacity;cache.faint=o.faint_opacity;cache.primary_alpha=colors.primary.alpha;cache.border=p.border;cache.border_width=o.border_width;cache.mask=mask;cache.gradient=radials;cache.stroke=stroke;
    gui::BackdropMaterial material{.bounds={0,0,float(w),float(h)},.radius=float(radius),.blur_radius=float(p.blur_radius),.tint=surface_detail::linear(colors.tint),.border=surface_detail::linear(p.border),.border_width=mask?0:float(std::min({o.border_width,w/2,h/2})),.source=o.backdrop==MaterialBackdrop::content?gui::BackdropSource::content:gui::BackdropSource::previous_layers,.mask=mask,.underlay=radials,.underlay_bounds={float(-p.capture_padding),float(-p.capture_padding),float(w+2*p.capture_padding),float(h+2*p.capture_padding)},.base=surface_detail::linear(colors.base),.capture=o.backdrop!=MaterialBackdrop::none,.inner_highlight_opacity=float(p.inner_highlight_opacity),.inner_highlight_offset_y=float(o.inner_highlight_offset_y)};
    if(radials){auto tint=surface_detail::linear(colors.primary);tint.alpha=1;material.underlay_tint=tint;}
    drawing->backdrop=std::move(material);if(p.shadow_opacity>0)drawing->shadow=gui::BoxShadow{.bounds={0,float(p.shadow_offset_y),float(w),float(h)},.radius=float(radius),.blur_radius=float(p.shadow_radius),.color={0,0,0,float(p.shadow_opacity)},.mask=mask};if(stroke)drawing->images.push_back({.bounds={0,0,float(w),float(h)},.image=stroke});return drawing;
}
}
