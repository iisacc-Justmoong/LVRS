# 네이티브 Tooltip 갤러리

두 Tooltip이 각각 실제 기원 버튼을 관찰한다. 첫 말풍선에는 유지되는 레이블·버튼과 C++ 바인딩을 두고, 두 번째는 기본 줄 바꿈 레이블을 사용한다. 원래 버튼을 눌렀을 때 꼬리·바깥 클릭 닫힘이 액션을 삼키지 않는 경로를 포함한다.

최종 실행용 self-test는 Core deadline 뒤 자동 표시, 포커스 비변경, 같은 자식/백엔드의 갱신과 액션, 기원 클릭·닫힘, 가용 영역 변경 시 닫힘 유지, 캔버스 캡처, 크기 변경과 정상 종료를 검사한다. 현재는 해당 앱과 테스트를 실행하지 않는다.

install.sh는 기본적으로 예제 내부 build/install에 복사한다. LVRS_DIR·LVUI_DIR·iiGUI_DIR·iiSDKCore_DIR·iiFileProviderNative_DIR와 Vulkan/SVG 의존성 위치를 전달할 수 있다. 전역 설치를 변경하거나 빌드 후 앱을 자동 실행하지 않는다. NativeTooltipGallery의 App/ 소스와 AOT 출력·패키지 빌드 증거를 실제 창 실행 결과와 구분한다.
