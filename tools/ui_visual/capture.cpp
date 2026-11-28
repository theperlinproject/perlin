// Production JSON UI resolver/layout/scene/text/canvas, with a standalone GLX test backend.
#include "client/ui/JsonUi.hpp"
#include "client/ui/UiLayout.hpp"
#include "client/ui/SceneEmit.hpp"
#include "client/ui/draw/SceneView.hpp"
#include "client/systems/text/McFont.hpp"
#include "client/ui/HudData.hpp"
#include "util/FileIo.hpp"
#include "glx_minimal.hpp"
#include <png.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <chrono>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <cstdlib>
#include <cmath>
namespace fs=std::filesystem;
using namespace perlin;
using namespace perlin::ui;
using namespace perlin::ui::draw;
struct PixelImage { int w=0,h=0; std::vector<uint8_t> rgba; };
static PixelImage ReadPng(const fs::path& path) {
    png_image p{}; p.version=PNG_IMAGE_VERSION;
    if (!png_image_begin_read_from_file(&p,path.c_str())) return {};
    if (!p.width || !p.height || p.width>8192 || p.height>8192) { png_image_free(&p); return {}; }
    p.format=PNG_FORMAT_RGBA;
    PixelImage out{static_cast<int>(p.width),static_cast<int>(p.height),std::vector<uint8_t>(PNG_IMAGE_SIZE(p))};
    if (!png_image_finish_read(&p,nullptr,out.rgba.data(),0,nullptr)) { png_image_free(&p); return {}; }
    png_image_free(&p); return out;
}
static void SavePng(const fs::path& path,int w,int h,const std::vector<uint8_t>& rgb) {
    png_image p{}; p.version=PNG_IMAGE_VERSION; p.width=w; p.height=h; p.format=PNG_FORMAT_RGB;
    if (!png_image_write_to_file(&p,path.c_str(),0,rgb.data(),0,nullptr)) throw std::runtime_error(p.message);
}
struct PackTextures : TextureSource {
    std::vector<fs::path> packs; int scale;
    std::unordered_map<std::string,TexInfo> cache;
    std::unordered_map<std::string,PixelImage> pixels;
    std::unordered_map<std::string,TextureSidecar> metadata;
    std::unordered_set<std::string> missing;
    PackTextures(std::vector<fs::path> p,int s):packs(std::move(p)),scale(s) {}
    int Device() const override { return scale; }
    fs::path Find(std::string rel) const {
        fs::path p(rel);
        if (p.is_absolute() || std::ranges::any_of(p,[](const fs::path& c){return c=="..";})) return {};
        for (const auto& root:packs) if (fs::is_regular_file(root/p)) return root/p;
        return {};
    }
    PixelImage& Image(const std::string& key) {
        if (const auto it=pixels.find(key);it!=pixels.end()) return it->second;
        PixelImage image;
        if (key=="mc:white") image={1,1,{255,255,255,255}};
        else if (key.starts_with("mcfont:")) { const auto& font=McFont::Instance(); image={font.AtlasWidth(),font.AtlasHeight(),font.Atlas()}; }
        else { const fs::path p(key); const auto path=Find(p.extension().empty()?key+".png":key); if (!path.empty()) image=ReadPng(path); }
        if (!image.w) missing.insert(key);
        return pixels.emplace(key,std::move(image)).first->second;
    }
    TextureSidecar Sidecar(const std::string& key) override {
        if (const auto it=metadata.find(key);it!=metadata.end()) return it->second;
        fs::path rel(key);rel.replace_extension(".json"); TextureSidecar result;
        const auto path=Find(rel.generic_string());
        if (!path.empty()) { json::Value v;std::string error;if(json::Value::Parse(file::Read(path),v,error))result=ParseTextureSidecar(v); }
        metadata[key]=result; return result;
    }
    TexInfo Get(const std::string& key) override {
        if (const auto it=cache.find(key);it!=cache.end()) return it->second;
        auto& image=Image(key);if(!image.w)return {};
        auto rgba=image.rgba; for(size_t i=0;i<rgba.size();i+=4)for(int c=0;c<3;++c)rgba[i+c]=(rgba[i+c]*rgba[i+3]+127)/255;
        GLuint id=0;glGenTextures(1,&id);glBindTexture(GL_TEXTURE_2D,id);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
        glPixelStorei(GL_UNPACK_ALIGNMENT,1);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,image.w,image.h,0,GL_RGBA,GL_UNSIGNED_BYTE,rgba.data());
        return cache.emplace(key,TexInfo{id,image.w,image.h}).first->second;
    }
};
struct Data : UiData {
    json::Value bindings=json::Value::MakeObject(),collections=json::Value::MakeObject();
    bool Global(const std::string& key,json::Value& out) const override {if(auto* v=bindings.Find(key)){out=*v;return true;}return false;}
    int CollectionSize(const std::string& key) const override {auto* v=collections.Find(key);return v&&v->IsArray()?static_cast<int>(v->array.size()):0;}
    bool CollectionValue(const std::string& key,int index,const std::string& binding,json::Value& out) const override {
        auto* v=collections.Find(key);if(!v||!v->IsArray()||index<0||static_cast<size_t>(index)>=v->array.size())return false;
        if(auto* p=v->array[index].Find(binding)){out=*p;return true;}return false;
    }
};
struct DisplayWindow {
    Display* d=nullptr;XVisualInfo* vis=nullptr;GLXContext ctx=nullptr;Window win=0;Colormap cm=0;
    DisplayWindow(int w,int h) {
        d=XOpenDisplay(nullptr);if(!d)throw std::runtime_error("No X display; use xvfb-run");
        int attrs[]={GLX_RGBA,GLX_RED_SIZE,8,GLX_GREEN_SIZE,8,GLX_BLUE_SIZE,8,0};
        vis=glXChooseVisual(d,DefaultScreen(d),attrs);if(!vis)throw std::runtime_error("No GLX visual");
        const Window root=RootWindow(d,vis->screen);cm=XCreateColormap(d,root,vis->visual,AllocNone);
        XSetWindowAttributes swa{};swa.colormap=cm;swa.event_mask=StructureNotifyMask|ExposureMask;swa.border_pixel=0;
        win=XCreateWindow(d,root,0,0,w,h,0,vis->depth,InputOutput,vis->visual,CWColormap|CWEventMask|CWBorderPixel,&swa);
        XStoreName(d,win,"Perlin JSON UI visual capture");XMapWindow(d,win);XSync(d,False);
        ctx=glXCreateContext(d,vis,nullptr,True);if(!ctx||!glXMakeCurrent(d,win,ctx))throw std::runtime_error("GLX context failed");
        glDrawBuffer(GL_FRONT);glReadBuffer(GL_FRONT);
        std::cout<<"GL_RENDERER="<<glGetString(GL_RENDERER)<<" GL_VERSION="<<glGetString(GL_VERSION)<<" window="<<win<<"\n";
    }
    ~DisplayWindow(){if(d){if(ctx){glXMakeCurrent(d,0,nullptr);glXDestroyContext(d,ctx);}if(win)XDestroyWindow(d,win);if(cm)XFreeColormap(d,cm);if(vis)XFree(vis);XCloseDisplay(d);}}
};
static void Draw(const Rendered& r,int gw,int gh,int scale) {
    glViewport(0,0,gw*scale,gh*scale);glMatrixMode(GL_PROJECTION);glLoadIdentity();glOrtho(0,gw,gh,0,-1,1);
    glMatrixMode(GL_MODELVIEW);glLoadIdentity();glDisable(GL_DITHER);glDisable(GL_DEPTH_TEST);glDisable(GL_SCISSOR_TEST);
    glClearColor(0,0,0,1);glClear(GL_COLOR_BUFFER_BIT);glEnable(GL_TEXTURE_2D);glEnable(GL_BLEND);glBlendFunc(GL_ONE,GL_ONE_MINUS_SRC_ALPHA);
    for(const auto& b:r.batches){if(b.glint)continue;
        if(b.scissor){glEnable(GL_SCISSOR_TEST);glScissor(b.rect[0],gh*scale-b.rect[1]-b.rect[3],b.rect[2],b.rect[3]);}else glDisable(GL_SCISSOR_TEST);
        glBindTexture(GL_TEXTURE_2D,b.texture);glBegin(GL_TRIANGLES);
        for(uint32_t idx:b.indices){const auto& v=b.vertices.at(idx);const auto c=v.abgr;glColor4ub(c&255,(c>>8)&255,(c>>16)&255,c>>24);glTexCoord2f(v.u,v.v);glVertex2f(v.x,v.y);}glEnd();
    }
    glDisable(GL_SCISSOR_TEST);glFinish();if(glGetError()!=0)throw std::runtime_error("OpenGL submission failed");
}
int main(int argc,char** argv) try {
    if(argc<7){std::cerr<<"usage: ui_visual_capture <namespace.screen> <guiW> <guiH> <integer-scale> <output.png> <pack root (highest priority)> [lower packs...]\n";return 2;}
    const int gw=std::stoi(argv[2]),gh=std::stoi(argv[3]),scale=std::stoi(argv[4]);
    if(gw<1||gh<1||scale<1||scale>8||gw>4096/scale||gh>4096/scale)throw std::runtime_error("Invalid viewport");
    std::vector<fs::path> roots;std::vector<std::string> uiRoots;
    for(int i=6;i<argc;++i){roots.emplace_back(argv[i]);uiRoots.push_back((roots.back()/"ui").string());}
    DisplayWindow display(gw*scale,gh*scale);PackTextures textures(roots,scale);
    for(const char* name:{"font/default8.png","textures/font/default8.png"}){const auto path=textures.Find(name);if(path.empty())continue;const auto img=ReadPng(path);if(img.w&&McFont::Instance().LoadSheet(img.rgba.data(),img.w,img.h))break;}
    std::cout<<"bitmap_font="<<(McFont::Instance().Builtin()?"Perlin_builtin_fallback":"pack_sheet")<<"\n";
    UiLibrary lib;lib.SetRoots(uiRoots);if(!lib.Scan())throw std::runtime_error("No pack UI definitions");BuildReport report;auto vars=EngineRootVariables();Data data;
    if(const char* path=std::getenv("PERLIN_UI_VISUAL_DATA")){json::Value state;std::string error;if(!json::Value::Parse(file::Read(path),state,error))throw std::runtime_error("Invalid data JSON: "+error);if(auto* v=state.Find("bindings"))data.bindings=*v;if(auto* v=state.Find("collections"))data.collections=*v;if(auto* v=state.Find("variables");v&&v->IsObject())for(const auto& [k,val]:v->object)vars[k]=val;}
    auto tree=BuildTree(lib,argv[1],vars,report);if(!tree)throw std::runtime_error("Screen not found");
    UiServices svc;svc.textureSize=[&](const std::string& k,int& w,int& h){auto& t=textures.Image(k);w=t.w;h=t.h;return w>0&&h>0;};
    svc.textureBaseSize=[&](const std::string& k,int& w,int& h){auto meta=textures.Sidecar(k);w=meta.baseW;h=meta.baseH;return w>0&&h>0;};
    svc.textWidth=[](const std::string& str,float s,bool){return McFont::Instance().Width(str,s);};svc.lineHeight=[](float s,bool){return 9*s;};svc.localize=[](const std::string& s){return s;};
    auto layout=EvaluateTree(*tree,data,svc,gw,gh);if(!layout)throw std::runtime_error("Layout failed");
    SceneEmitter emitter;auto doc=emitter.Emit(*layout,gw,gh,EmitLayout::Screen);Rendered rendered;RenderScene(doc.scene,{},textures,gw,gh,rendered);Draw(rendered,gw,gh,scale);XSync(display.d,False);
    const int w=gw*scale,h=gh*scale;std::vector<uint8_t> rgb(static_cast<size_t>(w)*h*3);
    glPixelStorei(GL_PACK_ALIGNMENT,1);glReadPixels(0,0,w,h,GL_RGB,GL_UNSIGNED_BYTE,rgb.data());
    for(int y=0;y<h/2;++y)std::swap_ranges(rgb.begin()+static_cast<size_t>(y)*w*3,rgb.begin()+static_cast<size_t>(y+1)*w*3,rgb.begin()+static_cast<size_t>(h-1-y)*w*3);
    SavePng(argv[5],w,h,rgb);std::ofstream(std::string(argv[5])+".scene.txt")<<DumpScene(doc);
    size_t verts=0;for(const auto& b:rendered.batches)verts+=b.vertices.size();
    std::cout<<"nodes="<<report.nodes<<" batches="<<rendered.batches.size()<<" vertices="<<verts<<" missing_controls="<<report.missingControls.size()<<" unresolved_vars="<<report.unresolvedVariables.size()<<" missing_textures="<<textures.missing.size()<<" custom_renderers="<<emitter.UnknownRenderers().size()<<"\n";
    for(const auto& k:textures.missing)std::cerr<<"missing texture: "<<k<<"\n";for(const auto& k:report.missingControls)std::cerr<<"missing control: "<<k<<"\n";for(const auto& k:report.unresolvedVariables)std::cerr<<"unresolved variable: "<<k<<"\n";for(const auto& k:emitter.UnknownRenderers())std::cerr<<"unsupported custom renderer: "<<k<<"\n";
    if(const char* hold=std::getenv("PERLIN_UI_VISUAL_HOLD_MS"))std::this_thread::sleep_for(std::chrono::milliseconds(std::clamp(std::atoi(hold),0,30000)));
    return report.missingControls.empty()&&textures.missing.empty()&&emitter.UnknownRenderers().empty()?0:3;
} catch(const std::exception& ex){std::cerr<<ex.what()<<"\n";return 1;}
