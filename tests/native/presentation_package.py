"""Build a fresh relocated native consumer; execution requires --execute."""
from pathlib import Path
import argparse,hashlib,json,re,shutil,subprocess,tempfile,time

def main():
    p=argparse.ArgumentParser();p.add_argument('--build',type=Path,required=True);p.add_argument('--lvui-source',type=Path,required=True);p.add_argument('--execute',action='store_true');a=p.parse_args()
    build=a.build.resolve();source=Path(__file__).resolve().parents[2];cache=dict(re.findall(r'^([^/#][^:\n]*):[^=\n]*=(.*)$',(build/'CMakeCache.txt').read_text(),re.M))
    evidence=build/'presentation-evidence';evidence.mkdir(exist_ok=True)
    steps=[]
    def run(*command):
        print('Package consumer stage: '+' '.join(map(str,command)),flush=True)
        started=time.monotonic()
        result=subprocess.run(command,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
        step={'command':list(map(str,command)),'exit_code':result.returncode,'seconds':round(time.monotonic()-started,3)}
        steps.append(step)
        (evidence/'installed-consumer-steps.json').write_text(json.dumps(steps,indent=2)+'\n')
        print('Package consumer stage completed in '+str(step['seconds'])+' seconds',flush=True)
        print(result.stdout,end='',flush=True);result.check_returncode();return result.stdout
    with tempfile.TemporaryDirectory(prefix='presentation-consumer-',dir=build) as temporary:
        root=Path(temporary);stage=root/'stage'
        run('cmake','--install',str(build),'--prefix',str(stage));relocated=root/'relocated';stage.rename(relocated)
        project=root/'project';shutil.copytree(source/'tests/native/presentation_consumer',project)
        fixtures=a.lvui_source/'tests/fixtures'
        for original,name in [('NativePresentation.lvui','Presentation.lvui'),('NativeScopedPresentation.lvui','Scoped.lvui'),('NativePresentationFixture.h','NativePresentationFixture.h')]:shutil.copyfile(fixtures/original,project/name)
        for name in ['Progress','ProgressScoped','ProgressForwarded']:shutil.copyfile(fixtures/('Native'+name+'.lvui'),project/(name+'.lvui'))
        shutil.copyfile(fixtures/'NativeProgressFixture.h',project/'NativeProgressFixture.h')
        for name in ['History','HistoryScoped']:shutil.copyfile(fixtures/('Native'+name+'.lvui'),project/(name+'.lvui'))
        shutil.copyfile(fixtures/'NativeHistoryFixture.h',project/'NativeHistoryFixture.h')
        for name in ['Registry','RegistryScoped']:shutil.copyfile(fixtures/('Native'+name+'.lvui'),project/(name+'.lvui'))
        shutil.copyfile(fixtures/'NativeRegistryFixture.h',project/'NativeRegistryFixture.h')
        shutil.copyfile(fixtures/'NativeRouteRegistry.lvui',project/'RouteRegistry.lvui')
        shutil.copyfile(fixtures/'NativeRouteRegistryFixture.h',project/'NativeRouteRegistryFixture.h')
        for name in ['HierarchyList','HierarchyItem','HierarchyCount','HierarchyDelegate']:shutil.copyfile(fixtures/('Native'+name+'.lvui'),project/(name+'.lvui'))
        shutil.copyfile(fixtures/'NativeHierarchyFixture.h',project/'NativeHierarchyFixture.h')
        shutil.copyfile(fixtures/'NativeOpacity.lvui',project/'Opacity.lvui')
        shutil.copyfile(fixtures/'NativeOpacityFixture.h',project/'NativeOpacityFixture.h')
        for name in ['Platform','PlatformSelection']:shutil.copyfile(fixtures/('Native'+name+'.lvui'),project/(name+'.lvui'))
        shutil.copyfile(fixtures/'NativePlatformFixture.h',project/'NativePlatformFixture.h')
        for name in ['ModelSource','ModelSourceSelection']:shutil.copyfile(fixtures/('Native'+name+'.lvui'),project/(name+'.lvui'))
        shutil.copyfile(fixtures/'NativeModelSourceFixture.h',project/'NativeModelSourceFixture.h')
        for name in ['Font','FontSelection']:shutil.copyfile(fixtures/('Native'+name+'.lvui'),project/(name+'.lvui'))
        shutil.copyfile(fixtures/'NativeFontFixture.h',project/'NativeFontFixture.h')
        for name in ['Svg','SvgSelection']:shutil.copyfile(fixtures/('Native'+name+'.lvui'),project/(name+'.lvui'))
        shutil.copyfile(fixtures/'NativeSvgFixture.h',project/'NativeSvgFixture.h')
        for name in ['Render','RenderSelection']:shutil.copyfile(fixtures/('Native'+name+'.lvui'),project/(name+'.lvui'))
        shutil.copyfile(fixtures/'NativeRenderFixture.h',project/'NativeRenderFixture.h')
        flags=[f'-D{name}={cache[name]}' for name in ['CMAKE_CXX_COMPILER','CMAKE_CXX_FLAGS','CMAKE_EXE_LINKER_FLAGS','CMAKE_OSX_DEPLOYMENT_TARGET','iiGUI_DIR','iiSDKCore_DIR','iiFileProviderNative_DIR','LVUI_DIR','iiColorManager_DIR','Vulkan_INCLUDE_DIR','Vulkan_LIBRARY','iiJson_DIR','iiNetwork_DIR','iiVisualCodec_DIR','OPENSSL_ROOT_DIR','OPENSSL_USE_STATIC_LIBS'] if cache.get(name)]
        target=project/'build';run('cmake','-S',str(project),'-B',str(target),'-G','Ninja','-DCMAKE_BUILD_TYPE=Release',f'-DLVRS_DIR={relocated / "lib/cmake/LVRS"}',*flags)
        run('cmake','--build',str(target),'-j','2')
        binary=target/'presentation_consumer';executed=False
        if a.execute:run(str(binary));executed=True
        headers={name:hashlib.sha256((relocated/'include/LVRS'/name).read_bytes()).hexdigest() for name in ['ViewModel.h','StateSource.h','StateModel.h','ProgressModel.h','ProgressBar.h','ModelUndoStack.h','ModelProperties.h','ViewModelRegistry.h','PageRouter.h','NavigationStackModel.h','HierarchyModel.h','HierarchyProjection.h','HierarchyController.h','HierarchyItem.h','HierarchyList.h','HierarchyDelegateContext.h','PlatformInfo.h','ModelSource.h','ModelAdapter.h','FontPolicy.h','SvgManager.h','SvgSource.h','Image.h','RenderMonitor.h']}
        for name,digest in headers.items():assert digest==hashlib.sha256((source/'src/LVRS'/name).read_bytes()).hexdigest()
        symbols=subprocess.check_output(['nm','-C',str(binary)],text=True)
        assert not re.search(r'lvui::(?:parse\(|parse_view\(|compile_native\()',symbols)
        assert 'lvui_generated::InstalledOpacity::build(' in symbols
        assert 'ii::gui::Widget::set_opacity(' in symbols
        assert 'lvui_generated::InstalledPlatform::build(' in symbols
        assert 'lvrs::PlatformInfo::runtimeProfile(' in symbols
        assert 'lvrs::PlatformEnvironment::current(' in symbols
        assert 'lvui_generated::InstalledModelSourceSelection::build(' in symbols
        assert 'lvrs::ModelSource::setSource(' in symbols
        assert 'lvrs::ModelAdapter::row(' in symbols
        assert 'lvui_generated::InstalledFontSelection::build(' in symbols
        assert 'lvrs::FontPolicy::enforceApplicationFallback(' in symbols
        assert 'ii::gui::FontDatabase::load_bundled_fonts(' in symbols
        assert 'ii::gui::FontDatabase::fold_name(' in symbols
        assert 'lvui_generated::InstalledSvgSelection::build(' in symbols
        assert 'lvrs::SvgManager::image(' in symbols
        assert 'lvrs::Image::create(' in symbols
        assert 'ii::visual::image::encode_png(' in symbols
        assert 'lvrs::RenderMonitor::attachWindow(' in symbols
        assert 'ii::gui::Host::presentation_source(' in symbols
        assert 'lvui_generated::InstalledRenderSelection::build(' in symbols
        report={'relocated':True,'compiled_linked':True,'generated_native_factories':25,'public_headers_sha256':headers,'executed':executed,'LVUI_runtime_parser':False,'native_opacity_symbol':True,'native_platform_symbol':True,'native_model_source_symbol':True,'native_font_symbol':True,'native_svg_symbol':True,'native_render_symbol':True}
        report['steps']=steps
        (evidence/'installed-consumer.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))

if __name__=='__main__':main()
