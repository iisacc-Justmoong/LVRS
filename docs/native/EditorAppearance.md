# Vincent 편집기 외형 지원

ListOptions는 인코딩된 sRGB의 background, selection_color, text_color와 selection_radius를 받는다. 기존 기본값은 유지한다. 색 채널은 유한한 0..1, 반경은 0..128 범위이다. 색상은 그리기 직전에 선형광으로 변환하며 선택 및 외형 변경은 문서 데이터를 수정하지 않는다. tests/native/list.cpp의 view 검사는 20px 행, 직각 선택 영역과 지정한 배경·선택 픽셀을 검증한다.

ColorPickerButtonOptions.circular은 iiGUI의 원형 색상 버튼을 전달한다. 공통 모델·클릭·키보드·접근성 계약을 그대로 사용한다.

ListOptions.font_size는 1..256의 글자 크기를 받는다. 기본 13과 기존 행 간격을 보존하며 큰 글자는 행 높이 안에서 줄 높이를 조정한다. ColorPickerButtonOptions.border_opacity는 원형 테두리의 0..1 불투명도를 전달한다. 기본 1을 유지한다.

숨겨진 부모 아래에서 생성하는 AppCard 등 표면 구성 요소는 크기가 아직 0일 수 있다. SurfaceDetail.paint는 이때 빈 그리기를 보관하고 처음 보이는 배치에서 배경을 그린다. list의 view 검사는 숨겨진 라이선스 카드 생성과 표시 전환을 재현한다. 목록의 가상화 검사는 목록 하위 위젯 수를 기준으로 하며, 카드에 부가 요소가 늘어도 목록의 제한을 정확히 검증한다.
