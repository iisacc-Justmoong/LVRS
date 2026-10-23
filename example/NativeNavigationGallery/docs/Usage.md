# 구성과 사용

이 예제는 네이티브 LVRS·iiGUI·iiSDKCore·LVUI의 설치 패키지가 필요하다.
소스 트리의 헤더나 빌드 라이브러리를 직접 추가하지 않고 CMake 패키지를 찾는다.
의존성을 재배치한 prefix를 `CMAKE_PREFIX_PATH`로 제공할 수 있다.

출력 디렉터리는 예제의 `build/`이다. `install.sh`의 첫 인자는 설치 위치이며 생략하면
`build/install`이다. 이후 인자는 CMake 구성 옵션으로 전달한다. 이 스크립트는 앱이나
테스트를 자동으로 실행하지 않는다. CMake에서 Qt5·Qt6 검색을 비활성화한다.

앱은 경로 매개변수·나머지 경로·push·replace·back·root를 보여 준다. 버튼은 scope 소유
C++ backend의 메서드 참조이며 Core 포트가 표시 레이블을 갱신한다. 백그라운드 결과와
프레임 콜백, 창 크기 변경과 종료도 iiSDKCore 앱에 연결한다.

`--self-test`는 `tests/CMakeLists.txt`에 등록했다. 현재 구현 단계에서는 실행하지 않는다.
최종 일괄 검증에서 다섯 탐색 통지, backend 생성/소멸 1회, 작업자 결과, 프레임 도달,
창 크기 변경과 정상 종료를 함께 확인한다. 실제 경로별 페이지 생성·폐기·전환과 별도
페이지 backend scope는 상위 LVRS의 PageRouter 구현 범위에 남아 있다.
