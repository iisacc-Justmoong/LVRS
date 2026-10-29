# 네이티브 플랫폼 정보와 전체 런타임 프로필

`lvrs::PlatformInfo`는 기존 Platform 싱글턴의 18개 속성·9개 조회 메서드를 C++ 읽기 전용
객체로 제공한다. 현재 OS와 프로세스 아키텍처는 `iiSDKCore::PlatformProfile`에서 읽으며
Qt·QSysInfo·QVariant·QML 엔진을 요구하지 않는다. Core는 LVRS를 참조하지 않는다.

기본 생성자는 현재 Core/네이티브 빌드 환경을 읽는다. `PlatformEnvironment`를 전달하는
생성자는 별도의 타깃 문맥을 구성한다. 문맥을 조회하거나 다른 문맥을 선택해도 OS·창·
렌더러·파일이나 원래 문맥을 바꾸지 않는다. 객체는 가변 상태·구독·스레드를 소유하지 않아
서로 독립적으로 읽거나 값으로 복사할 수 있다.

macOS의 기존 `os()` 값은 `osx`, `canonicalOs()` 값은 `macos`를 유지한다. macOS·Linux·
Windows·iOS·Android·WASM의 원래 순서, 모바일/데스크톱 구분, 22개 별칭을 보존한다.
ASCII 타깃 이름은 대소문자를 구분하지 않고 Unicode 공백을 제거한다. 잘못된 UTF-8은
예외로 거절한다. 이름이 없는 predicate는 false이며 기본 프로필/백엔드 조회만 현재
문맥을 사용한다. `runtimeProfile()`의 requested에는 공백을 제거한 원래 표기를 보존한다.

프로필의 41개 필드는 Core Value 객체로 반환한다. CMake 시스템 이름·파일 확장자,
부트스트랩 MSAA·프레임·아틀라스, 모바일 윈도우 권고, 화면 분기점·내비게이션 폭·서랍
여백·전환 시간 등을 원본 값으로 제공한다. WASM은 데스크톱 분류에 속하지만 직접 실행과
적응형 애니메이션은 false이다. 임의 타깃을 일부 문자열만으로 인식하지 않는다.

`metalSupported()`·`vulkanSupported()`는 실제 링크한 네이티브 패키지의 빌드 기능이다.
원본 `backendFeatureReadyFor()`는 Metal·Vulkan의 기능 게이트만 검사하며 default/D3D11은
별도 게이트가 없다. 이 의미를 유지한다. `generationSupported`는 알려진 설정 타깃이라는
뜻이고 설치된 교차 컴파일러·원격 장치·해당 플랫폼의 실제 실행 가능성을 증명하지 않는다.
특히 네이티브 WASM 창/표시 어댑터의 완성을 의미하지 않는다. 실제 선택된 GPU와 성능은
Host/표면의 별도 실행 정보로 확인해야 한다.

아키텍처 표기는 Core의 현재 프로세스/빌드 문맥을 따른다. 혼합 ABI·에뮬레이션에서 원본
QSysInfo가 보고하는 네이티브 CPU와의 일치 여부는 플랫폼 실행 검증에 남긴다. 기본 Core
탐지를 사용하면서 이 차이를 숨기거나 모든 플랫폼의 물리 CPU를 조사했다고 보고하지 않는다.

## LVUI 연결

```lvui
import platform lvrs.PlatformInfo
Window {
    VStack {
        Label(text: platform.canonicalOs())
        Label(text: platform.runtimeProfile("ios")["adaptiveWideBreakpoint"])
    }
}
```

일반 C++ 메서드·Core Value·목록 결과를 기존 AOT 경계에 연결한다. 별도의 소스 해석기나
플랫폼별 QML 함수를 만들지 않는다. 서비스 인스턴스는 import를 소유한 활성화에 속하며,
타입만 불러오는 `import class`는 인스턴스를 만들지 않는다.

## 검증 경계

`tools/native_platform_contract.py`는 원본 전체 소스 해시를 고정한 검토용 fixture에서
7개 프로필·41개 필드·22개 별칭과 공개 속성/메서드 표면을 대조한다. fixture는 원본을
읽고 작성한 기대값이며 원본 Qt 실행 결과라고 주장하지 않는다. 원본이 변경되면 해시만
덮어쓰지 않고 전체 기대값을 다시 검토해야 한다. 빌드 시 native C++ 데이터로 생성하므로
검사 바이너리에도 런타임 JSON 파서를 추가하지 않는다.

네이티브 모델 검사 12개는 전체 필드·호스트/기능 조합·별칭·공백·미지원 타깃·문맥 독립성을,
생성 뷰 7개는 기본 import·메타데이터·선택 문맥·Static·입력 정체성·파일 비변경·퇴역을
다룬다. LVRS 실행은 전체 목표의 최종 일괄 단계에 모으고 중간 컴파일·링크와 구분한다.
서체 등록·SVG 로딩이나 FontPolicy/SvgManager 전체 API의 구현 완료로 대체하지 않는다.
