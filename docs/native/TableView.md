# 네이티브 Table 화면

`lvrs::Table`은 `TableModel`에 연결된 C++23 위젯이다. iiGUI의 가상 scroll viewport와
보관형 그리기, 단일 자체 편집기로 구성한다. Cocoa 표/입력 컨트롤, Qt, QML 런타임이나
LVUI 소스 해석기를 사용하지 않는다. `.lvui`의 `Table`은 같은 C++ 생성기를 AOT 호출한다.

**현재는 소스·문서·빌드 단계이며 실행 검증을 최종 일괄 단계까지 유보한다.** 아래는
작성한 구현 계약이다. 실제 입력·GPU 픽셀·계측·설치 소비자 실행의 통과를 뜻하지 않는다.
원본 `Table.qml`의 모든 227개 직접 노출 멤버와 상속 계약을 이식한 상태도 아니다.

## 생성과 소유

```cpp
auto model = std::make_shared<lvrs::TableModel>();
auto table = lvrs::Table::create(builder, parent, "assets",
    {.model = model, .width = 660, .height = 300});
```

모델을 생략하면 기본 4×3 데이터를 가진 새 모델을 소유한다. 모델은 shared_ptr이며
뷰는 Core QueryPort를 약하게 등록한다. 위젯 mount가 C++ 구현을 보관하고 공개 Table
핸들은 약한 세대 핸들이다. unmount 이후 UI 접근은 closed이다. 외부에서 모델을 계속
소유하면 화면 해제 후에도 데이터 API를 사용할 수 있다.

기본 폭은 717, 높이는 기본 데이터에 맞춘 121, 헤더 높이 24와 separator 1이다.
`fit_columns_to_viewport`는 생성/명시적인 옵션 변경 때 모델의 table width를 설정한다.
동일 모델을 여러 화면에 연결하면 열 기하도 공유한다. 화면마다 독립된 자동 열 폭이
필요하면 별도 모델을 사용하거나 자동 맞춤을 끄고 열 폭을 직접 관리한다.
`set_options`로 모델 객체를 교체할 수 있다. 새 모델의 소유 스레드와 구독을 먼저 검증하고
이전 weak 수신 포트를 폐기한다. 이전 모델의 후속 변경은 새 화면에 적용하지 않는다.
일반 C++ 멤버 대입을 자동으로 관찰하는 LVUI 재바인딩까지 구현했다는 뜻은 아니다.

## viewport와 그리기

수직·수평 스크롤의 논리 범위는 모델 전체 높이와 폭이다. 실제 content 위젯은 viewport
원점과 화면 크기에 머무르며 두 축의 offset과 교차하는 anchor만 조회한다. `100,000`행이라고 셀/행 수만큼 위젯을
만들지 않는다. root·header·viewport·body·editor·frame과 삭제 메뉴·두 항목의 아홉 노드를 재사용한다.
헤더/셀은 패널·네이티브 글자 목록이며 활성 편집기 하나만 실제 입력 위젯이다.

viewport와 교차하는 병합 anchor는 위쪽이나 왼쪽에 시작해도 표시 대상으로 유지한다. 큰 병합의
선택 배경·divider·테두리는 논리 기하를 화면과 먼저 교차한 뒤 작은 지역 좌표로 그린다.
원본 텍스트의 세로 중앙이 화면 밖이면 글자를 임의로 화면 중앙에 다시 배치하지 않는다.
편집 시작은 해당 중앙을 수직으로 드러낸다. 전체 셀 크기를 거대한 물리 위젯으로 만들지
않으므로 수백만 논리 픽셀의 병합도 모델 좌표와 viewport 좌표를 구분한다.

UI 상태 조회는 이미 보관한 상태를 반환하며 매번 데이터 조회/그리기 목록을 만들지 않는다.
화면 밖 셀 값만 바뀌거나 history/context/resize 상태만 바뀌면 모델 revision만 따라간다.
화면 안 값, 기하, 헤더나 구조가 바뀔 때 viewport를 다시 구성한다. GPU는 기존 iiGUI
전경/패널 재사용 계약을 유지한다. 전체 데이터 행 수와 무관한 그리기라는 뜻이며,
viewport 내부 전체 glyph atlas와 모든 국소 damage 갱신을 완성했다는 뜻은 아니다.

기하·그리기 용량은 iiGUI Canvas 한도를 공유한다. 한 화면의 텍스트와 패널이 예산을
초과하면 full/invalid 오류로 보고한다. 일부 셀을 조용히 누락하지 않는다. 최대 밀도의
모든 화면을 무제한 처리하거나 모든 플랫폼에서 같은 성능을 보장하는 구현은 아니다.

## 수평 위치와 드러내기

`set_scroll_position(x,y)`는 두 축을 한 번에 적용한다. 기존 `set_scroll_offset(y)`는
가로 위치를 유지하고 `set_horizontal_scroll_offset(x)`는 세로 위치를 유지한다.
`reveal_cell(position)`은 선택·데이터·이력을 바꾸지 않고 셀을 드러낸다. 화면보다 큰
병합 셀은 이미 교차 중이면 위치를 유지하며 완전히 벗어났을 때 시작 위치로 이동한다.
`snapshot()`은 `horizontal_scroll_offset`와 `first_visible_column`을 함께 제공한다.

가로 휠과 Shift+세로 휠을 지원한다. 헤더/아래 삽입 영역의 순회는 현재 첫 열을 Fenwick
색인으로 찾고 보이는 열까지만 진행한다. 화면 좌표와 모델 X 좌표의 차이를 헤더 정렬,
셀 hit-test, 우클릭, 크기 조절과 열 삽입에 일관되게 적용한다. 활성 resize 중에는 휠
이동을 소비하여 드래그 기준이 다른 열로 이동하지 않게 한다.

셀/헤더 텍스트는 원래 레이아웃 경계를 유지하고 `TextRun::clip`으로 실제 노출 영역만
그린다. 가로 스크롤로 잘린 글자를 화면 왼쪽부터 다시 시작하지 않는다. 텍스트 경계와
화면이 교차할 때만 float 물리 좌표로 변환한다. 기존 8192 논리 픽셀의 텍스트 레이아웃
폭 한도는 유지한다. 더 넓은 셀은 시작 위치부터 그 범위까지의 레이아웃을 유지하며,
그 너머의 모든 텍스트/말줄임을 지원한다고 주장하지 않는다. 긴 병합의 배경과
hit-test/선택/스크롤은 전체 논리 범위를 사용한다.

편집 시작은 화면 밖 열도 드러낸다. 편집 중 스크롤하여 텍스트 시작 위치가 왼쪽 화면
밖으로 나가거나 세로 중앙이 사라지면 편집기를 닫고 읽기 표시로 돌아간다. 허용된 값은
이미 모델에 기록한 상태이며 submitted를 합성하지 않는다.

## 선택, 탐색과 편집

pointer release는 셀을 선택하고 Shift는 기존 anchor에서 범위를 확장한다. 병합 셀은
anchor와 span으로 정규화한다. none/single_cell/range 모드를 제공하며 선택·스크롤은
데이터 revision이나 undo 기록을 만들지 않는다. 선택 상태와 데이터 상태를 분리한다.
방향키·Home/End·select-all·Escape는 선택을 변경하며 두 축의 범위를 드러낸다.

입력 가능한 셀의 클릭이나 선택 후 Enter는 하나의 보관한 편집기를 해당 위치에 연결한다.
검증에 성공한 사용자 편집은 모델에 쓰고 edited(row,column,text)를 보낸다. 거절된
입력은 마지막 모델 값으로 되돌리고 rejected(row,column,text,type)를 보낸다. Enter는
submitted를 보내고 Escape/명시적인 end_edit는 편집기를 닫는다. 이미 허용된 편집을
Escape만으로 되돌리지는 않으며 데이터 undo/redo를 사용한다. 프로그램 방식의 모델
변경은 사용자 edited/submitted를 합성하지 않는다.

none 모드에서도 입력 가능한 셀은 클릭해 편집할 수 있다. 단일 셀 모드의 select-all은
원본의 단일 끝 셀 선택으로 제한한다. 편집기 안의 문자 이동·선택·IME는 iiGUI의 기존
편집기 경로이며 표의 탐색이 좌우 caret 입력을 가로채지 않는다. 모든 IME/clipboard
조합의 동등성을 검증했다는 뜻은 아니다.

`sorting_enabled`이면 헤더 클릭으로 오름/내림을 번갈아 적용한다. 명시적인 sort_rows는
모델 정렬과 연결되며 rows-sorted 이벤트를 전달한다. 병합·분리·행열 삽입/삭제와 크기
변경을 모델 API로 수행하면 화면이 갱신된다. 명시적인 `merge_selection`, `split_cell`,
행·열 삽입/삭제, `undo/redo`도 같은 모델 명령을 사용한다.

열 경계와 행 경계에는 기본 6픽셀 resize hit 영역을 둔다. 드래그는 화면 밖 이동·해제를
캡처하고 모델의 최소 치수와 하나로 합친 undo 기록을 사용한다. Escape·초점 상실·숨김과
화면 해제는 마지막 허용 치수를 유지하고 소유한 resize를 끝낸다. 모델 통지 안에서
소유권을 끝내야 하는 경우 `request_end_resize`가 통지가 끝난 경계에서 상태를 정리한다.
다른 데이터 명령의 통지 재진입은 계속 busy이다. 완료 콜백은 변경된 치수를 드래그 해제 때
전달하며 모든 원본 signal 발생 순서를 재현한 상태는 아니다.

`structure_controls_visible`이면 오른쪽·아래의 기본 20픽셀 영역에서 해당 행·열 다음에
삽입한다. 표 전체 노드 크기 안에 이 영역을 확보하고 실제 body viewport와 분리한다.
보이는 행·열의 `+`만 벡터로 그리며 데이터 수만큼 버튼 객체를 만들지 않는다. 우클릭은
대상 셀과 선택을 정리한 뒤 삭제 메뉴를 연다. 마지막 열의 삭제 항목은 비활성화한다.
벡터 조작 영역의 가상 접근성 버튼과 OS resize 커서는 아직 남아 있다.

범위 선택·A1 주소·TSV·클립보드와 원자적 붙여넣기는 [표 전송 계약](TableTransfer.md)에
정리한다. 선택과 스크롤은 데이터 이력을 만들지 않으며 명시적인 데이터 명령과 구분한다.

## LVUI AOT

```lvui
import vm AssetTableBackend
ApplicationWindow {
    Table(model: vm.model, width: 660, height: 300,
          selectionMode: .RangeSelection, sortingEnabled: true,
          onCellInputEdited: vm.edited,
          onCellInputSubmitted: vm.submitted,
          onCellInputRejected: vm.rejected)
}
```

`vm.model`은 scope 소유 C++ backend의 public `shared_ptr<lvrs::TableModel>`이다.
한 scope에서 backend를 한 번 생성하며 셀·스크롤마다 backend를 다시 만들지 않는다.
행·열·데이터는 모델 API로 교체한다. AOT의 임의 rows/header role 배열과 delegate는
아직 미지원이다. 콜백은 row/column과 문자열, 선택 optional<TableRange>, 정렬 enum을
갖는 C++ 함수로 생성한다. 일반적인 C++ 멤버 자동 재바인딩의 완성을 의미하지 않는다.

`NativeDataTableGallery`는 100,000행, 타입 편집/거절, 1,200,000 offset, Core 작업자 결과,
디스플레이 프레임과 크기 변경/종료를 다루는 예제이다. 다섯 열의 합계 폭은 1,200이며
실제 viewport보다 넓다. self-test에는 500의 수평 offset과 실제 body 원점 유지 검사를 넣었다.
삽입 영역과 append/undo/redo도 제공한다. 새 self-test와 열여덟 개 Table 화면 검사, 여덟 개 iiGUI viewport 검사,
네 개 전송 검사와 네이티브 클립보드 검사, AOT 검사는 실행을 유보한다. 빌드 도구의 AOT
코드 생성과 최종 앱의 실제 실행을 구분한다.

선택 콜백에서 현재 셀을 지우거나 mount를 해제할 수 있다. 탐색은 콜백이 반환된 뒤
수명과 현재 셀을 다시 확인한 후 스크롤하며, 이미 지운 선택을 되살리지 않는다.

## 남은 계약

- 기존 8192 논리 픽셀 텍스트 레이아웃 한도보다 넓은 셀의 전체 조판·말줄임 동등성
- 벡터 조작 영역의 접근성·OS resize 커서와 모든 포인터/키보드 조합의 원본 동등성
- 셀·헤더의 가상 접근성 peer/값/행동과 전체 키보드/터치/IME 조합
- 임의 delegate·모델 role adapter, 일반 반응형/상속 API와 모든 signal 순서
- 원본 implicit sizing/외부 Flickable/overscan/스타일·모션, Windows/Linux 동작 검증과 모바일/WASM 입력 어댑터
- 실제 실행·GPU 픽셀·성능·계측·설치 소비자 실행 검증

화면 밖 열의 편집은 시작 전에 가로 위치와 세로 텍스트 중앙을 드러낸다. 전체 이식은
계속 진행하며 이 항목을 complete로 표시하지 않는다.
