# 표 주소와 TSV 데이터 전송

`TableTransfer.h/.cpp`는 Qt·OS 위젯 없이 표 주소와 텍스트 전송을 제공한다.
`Table` 화면과 C++ 소비자가 같은 코덱을 사용한다. 새 검사 코드는 컴파일·링크하고,
실행은 작업 마지막의 일괄 검증까지 유보한다.

## 주소와 한도

`table_column_name/index`는 0 기반 열과 A, Z, AA 형식을 변환한다.
`table_cell_reference/coordinates`는 0 기반 `TablePosition`과 A1 형식을 변환한다.
앞뒤 ASCII 공백과 소문자 열 이름을 허용하며 행 0, 선행 0 행 번호, 비 ASCII 열 문자,
숫자 overflow와 모델 범위를 벗어난 주소를 거절한다. JavaScript의 모든 문자열 변환과
Unicode 대문자 확장을 복제하지 않는다.

`TableLimits`는 모델의 최대 1,000,000행·65,536열·16,000,000셀과 셀 문자열 최대
65,536바이트를 정의한다. 한 번의 TSV 전송은 최대 16 MiB·1,000,000셀이다.
데스크톱 클립보드 경로는 iiGUI의 1 MiB 텍스트 한도를 사용한다.

## TSV 계약

```cpp
auto matrix = lvrs::parse_table_tsv("Asset\t42\ttrue\n");
auto text = lvrs::table_range_as_tsv(model, {0, 0, 10, 3});
```

탭·CR·LF·인용부호가 있는 셀을 인용하고 내부 인용부호는 두 번 쓴다.
파서는 CRLF를 하나의 행 구분자로 처리하고 마지막 빈 행 하나를 제거한다.
인용된 셀 내부의 줄바꿈은 보존한다. 원본 `Table.qml`처럼 닫히지 않은 인용부호와
닫힌 인용부호 뒤의 문자를 허용한다. 엄격한 CSV 파서를 대체하는 규칙은 아니다.
UTF-8이 잘못되었거나 NUL이 있는 입력은 거절한다.

TSV 출력은 문자열·정수·유한 실수·bool의 표시 값을 사용하며 결측 값은 빈 문자열이다.
입력은 문자열 행렬이다. 대상 열의 타입 변환과 전체 명령 검증은 `TableModel`이 담당한다.
타입·결측 여부를 왕복 보존하는 저장 형식은 아니며 원본 RAW나 파일 journal의 대체재도 아니다.

`table_values_as_tsv`는 이미 소유한 값 행렬을 변환한다. `table_range_as_tsv`는 모델의
셀을 하나씩 읽어 출력 한도에서 멈추므로 선택 전체의 이중 배열을 먼저 복사하지 않는다.
기존 모델의 `range_values`와 같이 선택 직사각형 안에 저장되지 않은 셀이 있으면 거절한다.
`selection_values`는 호출자가 전체 값 행렬을 명시적으로 요구하는 별도 API이다.

## 화면 연결

`Table::select_range/row/column/all`, `selection_as_tsv`, `set_range_values`,
`paste_tsv/paste_selection_tsv`, `copy_selection/paste_from_clipboard`를 제공한다.
붙여넣기는 한 모델 명령과 한 undo 항목을 사용한다. 잘못된 행렬은 부분 적용하지 않는다.
허용된 변경에는 `range_values_changed`를 전달하고 유효한 끝 셀까지 선택을 갱신한다.
이는 사용자 단일 셀 편집의 `edited`·`submitted`를 합성하지 않는다.

macOS의 Copy/Paste 명령은 입력 필드가 활성화되면 기존 문자 선택 경로를 사용하고,
그 외에는 C++ 입력 처리기로 전달한다. Paste는 `WidgetInput.content`에 클립보드 값을
보관한다. 기본 `clipboard_enabled`는 true이며 이 옵션은 사용자 단축키 경로를 제어한다.
명시적인 C++ 데이터 명령을 권한 검사로 제한하는 옵션은 아니다. Cut·셀 접근성 전체와
모든 플랫폼의 혼합 입력 계약은 별도 이식 항목이다.

공개 Table 명령의 사용자 콜백 예외는 `failed`와 예외 정보로 전달한다. 이미 반영한
데이터나 선택을 자동으로 되돌리지 않는다. 실패를 미적용으로 간주해 반복 호출하면 안 된다.
실제 클립보드 검사는 원래의 모든 항목·광고 타입을 보관하고 검사 후 복원하도록 작성했다.
현재 이 검사를 실행하지 않았으며 실제 입력·클립보드·계측 통과를 주장하지 않는다.
