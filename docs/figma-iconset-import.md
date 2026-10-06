<a id="figma-iconset-svg-import"></a>

# Figma Iconset SVG 가져오기

2026-09-19 스냅샷의 LVRS [아이콘셋 페이지](https://www.figma.com/design/0GkItQYSNIR0lZ3iJhfJzc/Layerd-Visual-Render-System?node-id=198-2) 는 2,863 개의 아이콘을 포함합니다: 2,861 컴포넌트 및 2 독립 벡터입니다. 모든 아이콘은 `resources/iconset/` 에 직접 저장되며, 카테고리 디렉토리는 유지되지 않습니다.

원본 SVG 아트워크는 Figma 의 네이티브 SVG 내보내기를 통해 나오며, 경로 재구성, 색상 재조정 또는 콘텐츠 중복 제거 없이 수행됩니다. 초기 614 플러그인 API 내보내기는 해당 네이티브 내보내기와 바이트 동일했습니다. 2026-09-20 에서 모든 2,863 아이콘셋 SVG 와 13 보조 SVG 는 루트에서 표준화되었습니다: `width="100%"`, `height="100%"`, `preserveAspectRatio="xMidYMid meet"`, 그리고 가시 아트워크 중앙에 정렬된 정사각형 `viewBox` 입니다. 경로 데이터 및 모든 하위 요소는 변경되지 않았습니다. 원본 뷰포트 크기는 가시 오버플로우가 더 큰 정사각형을 필요로 하지 않는 한 유지됩니다. 범위는 Qt SVG 를 8 픽셀당 SVG 단위로 샘플링하며, 스트로크 및 임베디드 이미지 알파를 포함합니다. 이는 기하학적 가시 범위의 중앙 정렬이며, 비대칭 형태의 주관적 광학적 가중치가 아닙니다.

Figma 의 네이티브 내보내기는 카테고리 디렉토리를 작성하며, macOS 에서 대소문자만 다른 이름을 덮어쓸 수 있습니다. `nodesTest`/`nodestest` 및 `weChat`/`wechat` 쌍은 두 버전을 모두 보존하기 위해 개별적으로 내보내졌습니다. 기존 표준 조회 이름 `nodestest.svg` 와 `weChat.svg` 는 계속 사용 가능하며, 다른 변형은 `nodesTest-1.svg` 와 `wechat-1.svg` 를 사용합니다. 평탄화 과정에서 생성된 다른 파일명 충돌은 중복된 리터럴 접미사 이름으로 덮어쓰거나 동일한 아트워크를 삭제하지 않고 다음 사용 가능한 숫자 접미사를 받습니다.

[figma-iconset-manifest.json](figma-iconset-manifest.json) 는 모든 최종 파일명, 네이티브 내보내기 파일명, 원래 내보내기 차원, 그리고 현재 SHA-256 를 기록합니다. `preResponsiveSha256` 는 루트 정규화 이전의 해시를 보존합니다. 2 정규화된 내보내기도 원래 네이티브 SHA-256 를 기록합니다. 44 소스 그룹은 완전한 2,863파일 결과와 대조되어 조정되었습니다. 그룹화는 완전성만 확인하기 위해 사용되며 리소스 레이아웃의 일부가 아닙니다.

기존 CMake 리소스 발견은 이 파일들을 자동으로 포함합니다. `LVRSTests_svg_manager::figma_iconset_resources_are_complete_and_renderable` 는 고유한 평탄 파일명, 패키징된 리소스 존재 여부, SHA-256 식별자, 유효한 SVG 파싱, 그리고 모든 아이콘에 대한 비어 있지 않은 Qt 래스터화를 확인합니다. `all_svg_icons_scale_and_center` 는 모든 리소스 SVG 의 반응형 루트와 18, 36, 72, 144 픽셀에서의 가시 영역을 확인하여 래스터 안티앨리어싱 허용도를 허용합니다.

새로 가져온 자산을 정규화하려면 저장소 루트에서 실행하십시오.

```sh
cmake -S . -B build -DLVRS_BUILD_TESTS=ON
cmake --build build --target LVRSNormalizeSvgIcons
./build/tests/LVRSNormalizeSvgIcons .
```

결과 SVG 와 manifest 변경 사항을 검토한 다음, 아래의 회귀 테스트를 실행합니다. 노멀라이저는 명시적인 유지 관리 대상이며, 일반 빌드의 일부가 아닙니다. 또한 SVG 해시를 `tests/fixtures/figma-contract.json` 에서 새로고칩니다. 샘플링 원점은 1/8단위 격자에 맞춰지므로 반복적인 노멀라이저화는 멱등적입니다. PNG 를 포함하는 2 SVG 는 이미지에 기반한 상태로 유지되며, 그 SVG 뷰포트 를 조정해도 추가적인 래스터 디테일이 생성되지 않습니다.

네이티브 `smartSelect.svg` 와 `translateObject.svg` 에는 전체 범위의 SVG 패턴에 의해 참조되는 내장 PNG 이 포함되어 있습니다. Qt 6.8.3 은 `<use> element ... in wrong context` 를 보고하며 해당 참조를 비어 있게 렌더링합니다. 이 2 내보내기는 간접 패턴을 직접 `<image>` 로 대체하여 동일한 18 × 18 뷰박스를 덮습니다. 내장 PNG 데이터는 알파 채널을 포함하여 바이트 동일하며, 어떤 아트워크도 다시 생성되지 않습니다. 두 가지 노멀라이저 파일은 모두 독립적인 SVG 로 유지됩니다.

```sh
cmake --build build --target LVRSTests_svg_manager LVRSTests_figma_parity --parallel 1
ctest --test-dir build --output-on-failure -R 'LVRSTests_(svg_manager|figma_parity)$'
```

이 스냅샷은 Iconset 페이지를 다루고 있습니다. 이 가져오기 전에 이미 삭제된 14 보조 컨트롤 SVG(스테퍼, 체크박스, 드롭다운 상태 변형 및 입력 검색 문자)는 해당 페이지에 없고 삭제된 상태로 유지됩니다.

최종 검증은 macOS 에서 Qt 6.8.3 로 수행되었으며, LVRS 빌드가 성공했고 `LVRSTests_svg_manager` 와 `LVRSTests_figma_parity` 가 통과했습니다 ( 2/2 ). 이는 SVG 로딩, 리소스 해시, 그리고 모든 2,863 가져온 파일에 대한 가시적인 래스터 출력을 포함합니다. 더 넓은 `LVRSTests_import_api` 실행은 87 통과, 6 실패, 1 스킵을 보고했으며, 모든 6 실패한 사례는 위에서 나열된 이미 삭제된 보조 제어 리소스를 필요로 합니다. 그 삭제들은 이미 삭제된 보조 제어 리소스 대신 아무런 알림 없이 복원되었습니다.

2026-09-20에 대한 반응형 루트 검증: 대상 CTest 스위트 모두 통과하며, 4 래스터 사이즈의 모든 2,876 SVG 와 패키징된 아이콘셋 해시를 포함합니다. 기존 네이티브 RHI 전용 하이라이트 사례는 오프스크린 테스트에서 건너뛰며, 이전에 삭제된 보조 컨트롤에 대한 경고는 이번 변경 사항 바깥에 유지됩니다.

<a id="control-specific-variants"></a>

## 컨트롤별 변형

기존 14 컨트롤 SVG 변형 (스텝퍼, 드롭다운, 체크박스 및 검색) 은 2,863 가져온 아이콘과 함께 유지됩니다. Figma 스냅샷 매니페스트는 가져온 아이콘만 카운트하며, 배포된 컨트롤이 참조하는 자산으로 대체해서는 안 됩니다. 모든 2,890 리소스 SVG, 13 기타 보조 자산을 포함하여 반응형 루트를 유지합니다. `control_variant_resources_are_packaged` 는 컴파일된 QML 리소스 번들 내의 14 변형을 검증하며, API 스위트는 실제 컨트롤을 검증합니다.
