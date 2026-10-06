<a id="popover"></a>

# 팝오버

<a id="purpose"></a>

## 목적

`Popover`는 표준 [Glass 25 소재](Materials.md)를 사용하는 구성 가능한 Qt 빠른 컨트롤 팝업입니다. 이는 애플리케이션 기본 색상을 상속하고 전경 내용을 불투명하게 유지합니다.

## API

일반 팝업 API 가 적용됩니다: `open()`, `close()`, `x`, `y`, `width`, `height`, `contentItem`, `contentChildren` 및 해제 정책. 추가 속성은 `primaryColor`, `cornerRadius`, `surfaceColor`, `surfaceOpacity`, `backdropSource`, `backdropBackground` 및 읽기 전용 `material` 입니다.

기본값은 비모달, 디밍 없음, 포커스 활성화, 12px 패딩, 8px 여백 및 이스케이프/외부 프레스 해제입니다. 항목 팝업 유형은 캡처된 애플리케이션 레이어를 동일한 장면 그래프에 유지합니다.

<a id="usage"></a>

## 사용법

```qml
LV.Popover {
    id: details
    x: 80; y: 120
    width: 320; height: 180
    contentItem: DetailsView {}
}
// details.open()
```

<a id="how-it-works"></a>

## 동작 원리

`PanelMaterial` 은 배경에 25% 중성 톤, 16px 확산 및 현재 기본 원형 색상을 제공합니다. 애플리케이션 콘텐츠와 배경은 팝업 오버레이와 별도로 캡처됩니다. 커스텀 호스트의 경우 안전한 자매 캡처 소스를 제공하거나 톤이 된 원형 표면을 위해 null 로 두십시오.

Figma: [팝오버](https://www.figma.com/design/0GkItQYSNIR0lZ3iJhfJzc?node-id=997-24). VisualCatalog: `popover`; 테스트: `LVRSTests_materials`.

<a id="shared-motion"></a>

## 공유 모션

패널은 리바운드를 통해 92%에서 확장되고 해제되면 사라집니다. 전역 속도, 모션 감소, 로컬 오버라이드 및 구성요소별 VisualCatalog 레시피는 [모션 정책](../../motion.md)를 참조하세요.
