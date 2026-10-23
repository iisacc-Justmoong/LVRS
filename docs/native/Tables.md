# 네이티브 표의 셀·행·헤더

`TableCellItem`, `TableRow`, `TableHeader`, `TableHeaderModel`의 C++23 구현을 추가했다.
원본은 `src/qml/components/control/display/`의 세 QML과
`src/backend/model/tableheadermodel.*`이다. LVUI AOT가 생성한 C++에서 `lvrs::Widgets`
또는 직접 C++ 생성기를 호출한다. iiGUI의 자체 위젯이 기하·입력·텍스트·GPU 표시를
담당하고 iiSDKCore가 앱 수명과 작업자를 소유한다. Qt·QML·소스 해석기·OS 표/입력
컨트롤은 연결하지 않는다.

별도의 [네이티브 TableModel](TableModel.md)은 값·범위 편집, 타입, 정렬, 병합,
구조 변경, 크기 조절, undo/redo와 viewport 데이터 조회를 제공한다. 모델의 구현과
테스트 코드가 추가되었으며 실행 검증은 최종 일괄 단계까지 유보한다.

현재 [네이티브 Table 화면](TableView.md)을 모델과 연결하는 구현도 추가했다. 가상
viewport와 단일 편집기, 선택/탐색/정렬, LVUI AOT의 C++ 모델 연결을 제공한다. 원본의
전체 Table 계약과 실제 실행 검증은 별도로 남아 있다.

## 셀

기본 셀은 234×24이며 왼쪽 1픽셀 구분선과 8픽셀 간격을 사용한다. body 글자와 행 높이는
원본처럼 13이다. 읽기 상태는 네이티브 말줄임표 Label, 입력 상태는 투명 자체 편집기이다.
`show_divider`, `clip_content`, 글자 역할과 색상을 지정할 수 있다. 선택 배경은 primary의
alpha에 `31/255`를 곱하며 current 테두리는 primary와 `current_border_width`를 사용한다.
두 색상은 명시적으로 바꿀 수 있다.

`text_style`은 읽기 Label의 글자 역할이다. 원본처럼 편집기 글자는 항상 body 13을 쓰며,
역할별 content 행 높이 안에서 가운데 배치한다.

`TableCellItem::create`는 root와 editor를 한 번 생성한다. `set_selected`는 편집 값을
보존한다. `set_options`는 같은 위젯에 새 데이터·외형을 적용하고 입력 override를 초기화한다.
트리 해제 후 핸들은 `closed`를 반환하며 위젯 접근은 UI 소유 스레드에서 수행한다.

검증기가 없으면 원본처럼 문자열을 허용한다. `value_type`만으로 자동 변환하지 않는다.
`TableValidator`는 accepted·표시 text·typed value를 반환한다. 거절된 입력은 마지막
accepted 값으로 되돌리고 `rejected(text, valueType)`를 보낸다. 허용된 사용자 편집은
`edited(text)`, Enter는 `submitted(text)`를 보낸다. `apply_input_result`는 직접 검증/적용할
때 사용하며 edited/submitted를 합성하지 않는다.

명시적인 `validate_table_value`는 문자열, int64 정수, 유한한 실수, bool을 지원한다.
정수 범위 초과·NaN·무한대·잘못된 형식은 거절한다. Qt/JavaScript의 Number 변환 규칙
전체와 같은 변환기라는 뜻은 아니다. 사용자 검증기로 규칙을 교체한다.

## 행과 헤더

행은 기본 세 셀, 폭 717, 셀 폭 234, 높이 24이다. 간격은 원본처럼
`max(0, (width - count * cellWidth) / (count - 1))`로 구한다. 행의 inputable 기본값과
각 셀의 optional override를 합성하고 편집/submit에 열 인덱스를 전달한다.
같은 개수의 `set_options`는 핸들을 보존한다. 다른 개수의 교체는 현재 거절하며 동적
행·열·delegate 수명은 전체 Table 모델 이식 때 구현할 대상이다.

`TableHeaderModel`은 문자열·타입·padding을 가진 값 소유 열 목록을 받는다. 양수 column
width를 반올림해 최소 폭 이상으로 제한하며 0 이하 값은 fallback을 사용한다. fallback이
0이면 `floor(tableWidth / count)`와 최소 폭의 큰 값을 사용한다. 기본 padding 8,
최소 폭 32, 헤더 높이 24와 하단 separator 1이다. 같은 옵션은 revision을 올리지 않는다.
변경 때 descriptor와 prefix 위치를 한 번 계산해 열 조회는 O(1)이다. 반환 span은 다음
옵션 교체 때 무효화되며 소유자가 동시성을 관리한다. 입력은 최대 512개, 개별 기하 8192,
합산 열 좌표 32767 이내로 제한한다.

`TableHeader`는 description 12 글자와 separator를 그리고 interactive일 때 포인터·키보드·
접근성 활성화를 `column_clicked(index, column)`으로 전달한다. 헤더 클릭 자체는 정렬하지
않는다. 데이터 정렬은 Table 모델/소비자의 책임이다.

## LVUI와 검증

```lvui
import vm TableBackend
ApplicationWindow {
    VStack {
        TableHeader(columns: ["Name", "Count"], columnWidths: [300, 120],
                    interactive: true, onColumnClicked: vm.sort)
        TableRow(cells: ["Asset", "12"], width: 420, cellWidth: 210,
                 inputable: true, onCellInputEdited: vm.rowEdited)
        TableCellItem(text: "12", inputable: true, valueType: "int",
                      valueValidator: vm.validator,
                      onInputEdited: vm.edited, onInputRejected: vm.rejected)
    }
}
```

`vm.validator`는 scope 소유 C++ backend의 public `TableValidator` 값이다. 콜백은 클릭마다
backend를 만들지 않고 같은 scope를 사용한다. AOT의 cells/columns는 정적 String 배열,
columnWidths는 정적 Number 배열이다. `examples/NativeTableGallery`는 Core entry·worker·
display frame, 편집·검증 거절·submit·헤더 활성화·크기 변경·정상 종료를 함께 검사한다.

`tests/native/table_parts.cpp`는 기하·입력·선택 GPU·검증·수명·헤더 모델을 검사한다.
표 화면의 범위 선택, 정렬·병합·구조 명령 연결, resize hit 영역, 가상 위젯 풀, delegate와
일반 반응형/상속 API는 아직 이식하지 않았다. [대응표](Parity.md)의 모델과 구성 요소는
부분 구현이며 Table 화면도 부분 이식 상태이다. 다른 OS·실제 모바일 입력 검증도 남아 있다.
