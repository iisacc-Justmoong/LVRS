# PushButton

Figma 키보드 포커스 링은 1.5px 기본이며 기본 배율에서 3px 시작 및 11px 반경을 갖습니다. 키를 놓으면 Space와 Enter가 활성화됩니다. 톤 선택 및 기존의 모든 라벨/아이콘 크기는 변경되지 않습니다.

위치: `src/qml/components/control/buttons/PushButton.qml`

`PushButton`는 `AbstractButton`를 기반으로 구축된 독립 푸시 동작 제품군입니다. 이는 2026-09-06에서 측정된 [Figma PushButton 44:599](https://www.figma.com/design/0GkItQYSNIR0lZ3iJhfJzc/Layerd-Visual-Render-System?node-id=44-599)를 구현합니다. `LabelButton` 및 `IconButton`는 레이블 및 아이콘 사전 설정입니다.

## API

- `iconMode: false`는 라벨 변형을 선택합니다. `true`는 아이콘 변형을 선택합니다.
- `text`, `tone`, `enabled`, `method`, `methods` 및 `clicked()`는 `AbstractButton` 계약을 유지합니다. `Primary`는 기본 톤입니다.
- 아이콘 입력: `iconSource` / `url`, `iconName`, `icon.name`, `iconGlyph`, 및 `iconSize`. 기존 프로젝트 구조 대체 경로와 슈퍼샘플링이 재사용됩니다.
- 아이콘 사전 설정은 아이콘 옆의 선택적 텍스트도 지원합니다.
- 패딩, `spacing` 및 `cornerRadius`는 독립적으로 재정의 가능한 상태로 유지됩니다.

<a id="geometry"></a>

## 기하학

|변형|데스크탑 크기|패딩 L/T/R/B|간격|반경|
| --- | --- | --- | --- | --- |
|라벨, 텍스트 `Button`| 56 × 22 | 8 / 4.5 / 8 / 4.5 | 10 | 8 |
|아이콘, 악센트| 22 × 22 | 2 / 2 / 2 / 2 | 0 | 8 |
|아이콘, 기타 톤| 22 × 22 | 2 / 2 / 2 / 2 | 7 | 8 |

Figma 의 간격은 자식이 하나만 있는 변형일 때도 유지됩니다. 레이블은 기존 고정 본문 13px 중간 / 13px 라인 박스를 사용합니다. 아이콘은 18 × 18입니다. 반지름은 `Theme.radiusMd` 를, 간격은 `Theme.gap10` , `Theme.gap7` , `Theme.gapNone` 를 사용합니다. 모바일은 동일한 기하 구조와 바디 13px 라인박스를 사용하며, 레이블 56 × 22, 아이콘 22 × 22, 반지름 8, 및 레이블 수직 패딩 4.5입니다. 기본, 강조, 테두리 없음, 파괴, 비활성화 변형은 크기를 공유합니다.

<a id="usage"></a>

## 사용법

```qml
import LVRS 1.0 as LV

LV.PushButton {
    text: "Save"
    method: function(eventData) { documentController.save() }
}
LV.PushButton { iconMode: true; iconName: "add" }
```

`documentController`는 소비 애플리케이션에 속합니다. `LabelButton` 및 `IconButton`는 가져오기나 메서드 삽입을 변경하지 않고도 계속 사용할 수 있습니다.

<a id="validation"></a>

## 검증

`LVRSTests_import_api::button_family_components_contract` 는 두 가지 새 타입을 로드하고, 데스크톱과 모바일에서 두 가지 콘텐츠 모드와 모든 기존 톤을 확인하며, 4 패딩 값, 반지름, 간격, 체vron 경계, 실제 클릭 분배, 비활성화 입력 차단 여부를 확인합니다. `button_padding_matches_figma_spec` 는 기존 프리셋 이름을 동일한 업데이트된 기하학과 대조합니다. `LVRS_BUTTON_CAPTURE_DIR` 를 `build/` 하위의 디렉토리로 설정하여 렌더링된 데스크톱과 모바일 행렬을 저장합니다. `ctest --test-dir build --output-on-failure` 를 `cmake --build build` 이후 LVRS 라이브러리를 사용하여 런타임 경로에서 실행합니다.

<a id="shared-motion"></a>

## 공유 모션

`releaseOnSignal` 는 콘텐츠 모드와 LabelButton / IconButton 프리셋 모두에 기본값이 true 로 설정됩니다. 진정한 포인터, 터치 또는 키보드 릴리스는 `Motion.buttonReleaseDuration` ( 180ms 에서 1×) 탄성 복귀를 수행하며, 프레스 애니메이션보다 짧은 탭도 포함됩니다. 취소는 리바운드를 하지 않습니다. 기존 호버/프레스 채움과 키보드 포커스는 변경되지 않습니다. 감소된 동작과 `motionEnabled: false` 는 릴리스 응답을 억제합니다.

`LVRSTests_motion`의 `compact_button_release`, `compact_button_cancel_and_opt_out` 및 `compact_button_tones_and_keyboard_focus` 케이스는 즉시 및 보류 해제, 취소, 옵트아웃, 5가지 톤 모두, 키보드 포커스 및 안정적인 적중 형상을 다룹니다.

라벨과 아이콘은 하나의 컴팩트한 압축 및 탄성 릴리스를 공유합니다. 전역 속도, 모션 감소, 로컬 오버라이드 및 구성요소별 VisualCatalog 레시피는 [모션 정책](../../motion.md)를 참조하세요.
