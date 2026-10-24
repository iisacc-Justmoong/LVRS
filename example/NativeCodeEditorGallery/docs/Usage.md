# 네이티브 CodeEditor 갤러리

LVUI의 CodeEditor 예약어가 AOT 단계에서 C++23 `lvrs::CodeEditor`로 생성된다. iiSDKCore의 애플리케이션 진입점이 호스트·프레임·작업 종료를 소유한다. iiGUI의 자체 GPU 위젯에 네이티브 글꼴 배치 결과를 표시하며 Cocoa/Win32 편집 컨트롤은 생성하지 않는다.

기본 실행은 메모리 샘플이다. `--document PATH`를 전달하면 iiFileProvider Native를 통해 파일을 읽는다. Save를 명시적으로 눌렀을 때만 원본 경로에 게시하며 로딩 중·실패·부분 취소 상태의 저장은 거절한다. 서식 있는 TextEditor를 시연하는 예제가 아니다.

두 CodeEditor가 같은 모델을 공유한다. 각 뷰는 선택·커서·스크롤을 별도로 보유하며 다른 뷰의 편집 범위만큼 위치를 이동시킨다. undo/redo는 편집을 시작한 뷰의 이전/이후 선택을 복원하고 다른 뷰는 역방향/정방향 편집 범위에 따라 재배치한다. 뷰별 이력 메타데이터는 기본 128개이며 문서 데이터의 undo 예산과 별도이다.

설치 스크립트의 기본 목적지는 예제 내부 `build/install`이다. CMake의 `LVRS_DIR`, `LVUI_DIR`, `iiGUI_DIR`, `iiSDKCore_DIR`, `iiFileProviderNative_DIR`, Vulkan과 SVG 의존성 경로를 전달할 수 있다. 전역 SDK 설치와 이 작업 폴더의 검증 패키지는 별개이다.

등록된 self-test는 임시 fixture 생성에만 iiFileProvider를 사용한다. 파일 기반 줄 로드, 뷰 탐색의 무변경, 범위 편집, 취소·명시적 게시, 전체 문자열 복사 방지, 백그라운드 결과, 바인딩을 통한 크기 변경, 정상 종료를 검사한다. 현재 사용자 지시에 따라 실행하지 않으며 전체 LVRS 이식의 최종 일괄 검증 목록에 포함한다.
