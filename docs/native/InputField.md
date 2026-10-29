# 네이티브 InputField

`lvrs::InputField`는 iiGUI의 텍스트 편집기를 보존하면서 LVRS의 22px 입력 형상과
텍스트·검색·비밀번호 모드를 제공한다. `InputFieldShape`는 `inline_rectangle`(반경 0),
`rounded`(6), `cylinder`(11)이고 `InputFieldMode`는 `text`, `search`, `password`이다.
검색 모드는 왼쪽 12px 검색 아이콘을 표시하고, 비밀번호 모드는 iiGUI의 비밀번호 입력을
사용한다. 비어 있지 않은 활성 편집기에는 지우기 버튼을 표시한다.

```cpp
lvrs::InputFieldOptions options;
options.mode = lvrs::InputFieldMode::password;
options.shape = lvrs::InputFieldShape::rounded;
options.input.placeholder = "Password";
auto field = lvrs::InputField::create(builder, parent, "password", options);
```

`editor()`는 네이티브 편집기 핸들이고 `root()`는 배치 핸들이다. `set_options`는 동일한
위젯에 새 옵션과 텍스트를 적용한다. `set_text`는 사용자 편집 이벤트를 내지 않는다.
사용자 편집과 지우기 버튼은 `edited`, 제출은 `submitted`로 전달된다. 수명과 접근은
iiGUI의 소유 스레드 및 위젯 세대 규칙을 따른다.

기존 `Widgets::input_field`도 별도 스타일을 지정하지 않으면 둥근 22px 기본값을 사용한다.
호출자가 `InputOptions::style`을 제공한 경우에는 그 설정을 유지한다. 세 모양·모드와
검색·지우기 슬롯이 필요할 때는 `InputField::create`를 사용한다.

현재 구현은 색조와 크기를 적용하며, QML 재질의 배경 흐림·입체 조명과 완전히 동일한
네이티브 렌더링은 아직 제공하지 않는다. 네이티브 iiGUI 자리 표시자 색상 및 검색 모드의
오른쪽 내측 여백도 QML 원본과 완전 일치하지 않으므로 전체 시각 상태 이식으로 표시하지 않는다.
`tests/native/input_field.cpp`는 모드 전환, 편집, 비밀번호, 지우기와 비활성·수명을 검사한다.
