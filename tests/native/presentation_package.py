"""Build a fresh relocated native consumer; execution requires --execute."""
from pathlib import Path
import argparse,hashlib,json,os,re,subprocess,time
import ii_file_provider as provider
from package_workspace import checked_paths, consumer_build, redirect_manifest, validate_install_dirs

def main():
    p=argparse.ArgumentParser();p.add_argument('--build',type=Path,required=True);p.add_argument('--workspace',type=Path,required=True);p.add_argument('--lvui-source',type=Path,required=True);p.add_argument('--execute',action='store_true');a=p.parse_args()
    source=Path(__file__).resolve().parents[2];build,root=checked_paths(source,a.build,a.workspace)
    cache_text=(build/'CMakeCache.txt').read_text();validate_install_dirs(cache_text)
    cache=dict(re.findall(r'^([^/#][^:\n]*):[^=\n]*=(.*)$',cache_text,re.M))
    evidence=root/'evidence';provider.mkdir(evidence,exist_ok=False)
    environment={key:value for key,value in os.environ.items() if key.upper()!='DESTDIR'}
    steps=[]
    def run(*command):
        print('Package consumer stage: '+' '.join(map(str,command)),flush=True)
        started=time.monotonic()
        result=subprocess.run(command,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,env=environment)
        step={'command':list(map(str,command)),'exit_code':result.returncode,'seconds':round(time.monotonic()-started,3)}
        steps.append(step)
        provider.write_text(evidence/'installed-consumer-steps.json',json.dumps(steps,indent=2)+'\n')
        print('Package consumer stage completed in '+str(step['seconds'])+' seconds',flush=True)
        print(result.stdout,end='',flush=True);result.check_returncode();return result.stdout
    manifest=build/'install_manifest.txt';before=manifest.read_bytes() if manifest.is_file() else None
    script=root/'cmake_install.cmake'
    provider.write_text(script,redirect_manifest((build/'cmake_install.cmake').read_text(),build,root))
    with consumer_build(build,root.name,'presentation') as target:
        stage=root/'stage'
        run('cmake','-DCMAKE_INSTALL_LOCAL_ONLY=ON',f'-DCMAKE_INSTALL_PREFIX={stage}',
            f'-DCMAKE_INSTALL_CONFIG_NAME={cache.get("CMAKE_BUILD_TYPE") or "Release"}','-P',str(script))
        relocated=root/'relocated';stage.rename(relocated)
        project=root/'project';provider.copytree(source/'tests/native/presentation_consumer',project)
        fixtures=a.lvui_source/'tests/fixtures'
        for original,name in [('NativePresentation.lvui','Presentation.lvui'),('NativeScopedPresentation.lvui','Scoped.lvui'),('NativePresentationFixture.h','NativePresentationFixture.h')]:provider.copyfile(fixtures/original,project/name,overwrite=False)
        for name in ['Progress','ProgressScoped','ProgressForwarded']:provider.copyfile(fixtures/('Native'+name+'.lvui'),project/(name+'.lvui'),overwrite=False)
        provider.copyfile(fixtures/'NativeProgressFixture.h',project/'NativeProgressFixture.h',overwrite=False)
        for name in ['History','HistoryScoped']:provider.copyfile(fixtures/('Native'+name+'.lvui'),project/(name+'.lvui'),overwrite=False)
        provider.copyfile(fixtures/'NativeHistoryFixture.h',project/'NativeHistoryFixture.h',overwrite=False)
        for name in ['Registry','RegistryScoped']:provider.copyfile(fixtures/('Native'+name+'.lvui'),project/(name+'.lvui'),overwrite=False)
        provider.copyfile(fixtures/'NativeRegistryFixture.h',project/'NativeRegistryFixture.h',overwrite=False)
        provider.copyfile(fixtures/'NativeRouteRegistry.lvui',project/'RouteRegistry.lvui',overwrite=False)
        provider.copyfile(fixtures/'NativeRouteRegistryFixture.h',project/'NativeRouteRegistryFixture.h',overwrite=False)
        for name in ['HierarchyList','HierarchyItem','HierarchyCount','HierarchyDelegate']:provider.copyfile(fixtures/('Native'+name+'.lvui'),project/(name+'.lvui'),overwrite=False)
        provider.copyfile(fixtures/'NativeHierarchyFixture.h',project/'NativeHierarchyFixture.h',overwrite=False)
        provider.copyfile(fixtures/'NativeOpacity.lvui',project/'Opacity.lvui',overwrite=False)
        provider.copyfile(fixtures/'NativeOpacityFixture.h',project/'NativeOpacityFixture.h',overwrite=False)
        for name in ['Platform','PlatformSelection']:provider.copyfile(fixtures/('Native'+name+'.lvui'),project/(name+'.lvui'),overwrite=False)
        provider.copyfile(fixtures/'NativePlatformFixture.h',project/'NativePlatformFixture.h',overwrite=False)
        for name in ['ModelSource','ModelSourceSelection']:provider.copyfile(fixtures/('Native'+name+'.lvui'),project/(name+'.lvui'),overwrite=False)
        provider.copyfile(fixtures/'NativeModelSourceFixture.h',project/'NativeModelSourceFixture.h',overwrite=False)
        for name in ['Font','FontSelection']:provider.copyfile(fixtures/('Native'+name+'.lvui'),project/(name+'.lvui'),overwrite=False)
        provider.copyfile(fixtures/'NativeFontFixture.h',project/'NativeFontFixture.h',overwrite=False)
        for name in ['Svg','SvgSelection']:provider.copyfile(fixtures/('Native'+name+'.lvui'),project/(name+'.lvui'),overwrite=False)
        provider.copyfile(fixtures/'NativeSvgFixture.h',project/'NativeSvgFixture.h',overwrite=False)
        for name in ['Render','RenderSelection']:provider.copyfile(fixtures/('Native'+name+'.lvui'),project/(name+'.lvui'),overwrite=False)
        provider.copyfile(fixtures/'NativeRenderFixture.h',project/'NativeRenderFixture.h',overwrite=False)
        flags=[f'-D{name}={cache[name]}' for name in ['CMAKE_CXX_COMPILER','CMAKE_CXX_FLAGS','CMAKE_EXE_LINKER_FLAGS','CMAKE_OSX_DEPLOYMENT_TARGET','iiGUI_DIR','iiSDKCore_DIR','iiFileProviderNative_DIR','LVUI_DIR','iiColorManager_DIR','Vulkan_INCLUDE_DIR','Vulkan_LIBRARY','iiJson_DIR','iiNetwork_DIR','iiVisualCodec_DIR','OPENSSL_ROOT_DIR','OPENSSL_USE_STATIC_LIBS'] if cache.get(name)]
        run('cmake','-S',str(project),'-B',str(target),'-G','Ninja','-DCMAKE_BUILD_TYPE=Release',f'-DLVRS_DIR={relocated / "lib/cmake/LVRS"}',*flags)
        run('cmake','--build',str(target),'-j','2')
        binary=target/'presentation_consumer';executed=False
        if a.execute:run(str(binary));executed=True
        headers={name:hashlib.sha256((relocated/'include/LVRS'/name).read_bytes()).hexdigest() for name in ['ViewModel.h','StateSource.h','StateModel.h','ProgressModel.h','ProgressBar.h','ModelUndoStack.h','ModelProperties.h','ViewModelRegistry.h','PageRouter.h','NavigationStackModel.h','HierarchyModel.h','HierarchyProjection.h','HierarchyController.h','HierarchyItem.h','HierarchyList.h','HierarchyDelegateContext.h','PlatformInfo.h','ModelSource.h','ModelAdapter.h','FontPolicy.h','SvgManager.h','SvgSource.h','Image.h','RenderMonitor.h']}
        areas=('Overlays','Components','Hierarchy','Lists','Tables','Text','Models','Navigation','Monitoring','Platform','Styling','Media','Controls')
        for name,digest in headers.items():
            matches=[source/'src'/area/name for area in areas if (source/'src'/area/name).is_file()]
            assert len(matches)==1 and digest==hashlib.sha256(matches[0].read_bytes()).hexdigest(), 'installed header mismatch: '+name
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
        provider.write_text(evidence/'installed-consumer.json',json.dumps(report,indent=2)+'\n');print(json.dumps(report))
    after=manifest.read_bytes() if manifest.is_file() else None
    if before!=after:raise RuntimeError('The project installation manifest changed during the test')

if __name__=='__main__':main()
