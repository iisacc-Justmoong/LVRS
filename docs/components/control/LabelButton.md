# LabelButton

위치: `src/qml/components/control/buttons/LabelButton.qml`

`LabelButton`는 텍스트 전용 [PushButton](PushButton.md) 사전 설정입니다.

<a id="purpose"></a>

## 목적

- 컴팩트한 고정 높이 텍스트 작업 버튼을 제공합니다.
- 시각적 노이즈를 줄이면서 기본 버튼의 톤 동작을 유지합니다.

## API

`PushButton`와 해당 레이블 모드 기본값에서 상속됨:

- 기본 톤: `Primary` (`Kind=accent`)
- 주입 방법 API: `method`, `methods`, `invokeMethods(...)`
- 고정 높이: `Theme.iconSm + Theme.gap2 * 2`(`22` 데스크탑, `44` 모바일)
- `horizontalPadding: Theme.gap8`(`8` 데스크톱, `16` 모바일)
- 수직 패딩은 고정된 프레임과 바디 라인 높이에서 파생됩니다. (`4.5` 데스크톱, `15.5` 모바일)
- `cornerRadius: Theme.radiusMd`(`8` 데스크톱, `16` 모바일)
- `spacing: Theme.gap10`(`10` 데스크탑, 단일 라벨은 간격을 소비하지 않음)

<a id="figma-visual-contract"></a>

## Figma 시각적 계약

- 출처: `44:599`, `Type=LabelButton`.
- 텍스트 `Button`를 사용하면 데스크탑 프레임은 `56 x 22`입니다. 텍스트 경계는 `x=8, y=4.5, 40 x 13`입니다.
- 타이포그래피는 Pretendard 본문 `13px Medium / 13px`이며 모바일에서는 `13px`로 유지됩니다.
- 해당 모바일 토큰 구성은 `72 x 44`입니다.
- 모든 5 톤은 동일한 기하학을 유지합니다.

<a id="usage"></a>

## 사용법

```qml
import LVRS 1.0 as LV

LV.LabelButton {
    text: "Save"
    method: function(eventData) {
        save()
    }
}
```

<a id="how-it-works"></a>

## 동작 원리

- `iconMode: false`와 함께 `PushButton` 라벨 콘텐츠를 사용합니다.
- 레이블 너비를 전체 논리 픽셀로 반올림한 다음 가로 패딩을 추가합니다.
- Figma 호환 컴팩트 계약에 잠긴 암시적/명시적 높이를 유지합니다.

<a id="practical-notes"></a>

## 실용적인 참고 사항

- 조밀한 도구 모음과 짧은 작업 레이블에는 `LabelButton`를 사용하세요.
- 아이콘이 많은 작업의 경우 레이블 잘림 압력을 피하기 위해 `IconButton`를 선호합니다.

## FAQ

Q. `LabelButton`가 더 큰 텍스트에도 고정된 컴팩트 높이를 유지하는 이유는 무엇입니까?   A. 이것은 조밀한 도구 모음 리듬을 위해 설계된 것입니다. 긴 라벨은 단축되거나 더 큰 버튼 변형으로 이동되어야 합니다.

Q. 반경/패딩을 변경할 수 있나요?   A. 예, 상속된 레이아웃 속성은 재정의 가능하지만 버튼 패밀리 사전 설정과의 시각적 패리티가 깨질 수 있습니다.

<a id="mistake-patterns"></a>

## 실수 패턴

- 장황한 문장 길이의 라벨을 컴팩트한 버튼에 넣는 것,
- 형제 버튼 전체에 사용자 정의 패딩을 혼합하고 행 정렬을 잃습니다.

<a id="validation-checklist"></a>

## 검증 체크리스트

- 활성화/비활성화 상태에서 톤 대비가 허용됩니다.
- 컴팩트한 레이아웃에서 텍스트가 많이 생략되는 것을 방지할 수 있을 만큼 충분히 짧은 텍스트를 유지합니다.
- 행 일관성을 위해 형제 버튼 높이가 일치합니다.

<a id="shared-motion"></a>

## 공유 모션

시각적 변환이 압축되고 반환될 때 레이블은 중앙에 유지됩니다. 전역 속도, 모션 감소, 로컬 오버라이드 및 구성요소별 VisualCatalog 레시피는 [모션 정책](../../motion.md)를 참조하세요.
