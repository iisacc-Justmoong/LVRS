# 네이티브 Hierarchy 데이터와 뷰 투영

2026-10-24의 데이터 계층 단계는 빌드·패키지·정적 검증까지 기록했다. 이후 LVUI 전체
완성 목표에 따라 [컨트롤러](HierarchyController.md)와 [실제 위젯](HierarchyViews.md)을
추가하고 있으며, 아래 데이터 단계의 이력과 전체 위젯 완성 여부를 구분한다.

원본 backend/model/hierarchymodel 및 HierarchyItem.qml·HierarchyList.qml의 계약을
기준으로 Qt 없는 C++23 데이터 계층을 추가했다. 현재 범위는 HierarchyModel과
HierarchyProjection이다. 이 문서의 모델 구현을 실제 HierarchyItem·HierarchyList
위젯이나 해당 LVUI 예약어의 완성으로 해석하지 않는다. 기존 List의 단순 계층 모드는
별도로 유지한다.

## 데이터 계약

HierarchyRow는 UTF-8 key·label·아이콘 이름/글리프, 불변 RasterImage 참조, item_id,
count, depth와 활성·선택·펼침·드래그 기본값을 갖는다. 행은 전위 순서이며 처음 depth는
0이고 다음 행은 직전보다 최대 한 단계 깊을 수 있다. key는 ASCII 공백을 제거하고,
비어 있으면 음수가 아닌 item_id 또는 평면 인덱스 문자열로 채운다. label이 비면 key를
사용한다. key는 유일해야 한다. 중복 item_id 검색은 첫 행을 반환한다.

원본의 동적 QVariant/QJSValue·모델 role/column 입력을 네이티브 코어에 복제하지 않는다.
앱의 별도 어댑터가 HierarchyRow로 변환한다. 중복 key·잘못된 depth·손상된 UTF-8은
거절하며 이는 원본의 관대한 정규화와 완전 동등하지 않다. 고급 어댑터와 원본 신호 순서는
남은 작업이다.

기본 admission은 최대 100만 행·깊이 256·label 65,536바이트, 데이터 추산 256MiB,
개별 메타데이터 쿼리 추산 1MiB이다. 행/인덱스/문자열의 보수적 추산으로 허용 여부를
판단하므로 행 수 한도보다 먼저 거절될 수 있다. 실제 RSS의 하드 제한을 뜻하지 않는다.
외부 RasterImage 저장소와 호출자가 먼저 만든 입력 벡터의 메모리는 포함하지 않는다.
각 투영은 기본 256MiB의 별도 인덱스 admission을 사용한다.

set_rows는 전체 입력을 검증하고 새 인덱스를 준비한 뒤 원자적으로 교체한다.
update_row는 key·item_id·depth를 고정하고 텍스트/표시/상태 기본값만 바꾸며 트리
인덱스를 다시 만들지 않는다. 같은 데이터는 revision과 통지를 늘리지 않는다. 다만
set_rows의 동일성 판정 전 정규화·검증 비용까지 생략하는 것은 아니다.

## 인덱스와 메타데이터

전체 입력 또는 구조 이동 확정 시 O(n)으로 부모·첫 자식·형제 링크·형제/자식 수·
subtree_end와 하위 최대 깊이를 준비한다. subtree_end는 배타적 끝 인덱스이다.
key/ID 인덱스는 문자열 저장소를 소유하며 벡터 이동이나 SSO 주소에 의존하지 않는다.
메타데이터 쿼리를 하지 않는 스크롤/드래그 경로에서 경로 문자열이나 자식 key 배열을
모든 행에 대해 생성하지 않는다.

metadata(index)는 해당 행의 조상·경로·직접 자식 목록만 생성한다. 반환 크기가
쿼리 예산을 넘으면 full 오류이며 모델은 변하지 않는다. 경로 구분자는 원본과 같은
` / `이다. 데이터 모델의 metadata는 뷰 가시성을 가정하지 않는다. 표시 인덱스·보이는
자식/하위 수가 필요하면 투영의 metadata를 사용한다.

## 독립적인 펼침·높이·탐색

여러 HierarchyProjection이 같은 모델을 공유해도 펼침 override와 행 높이는 각각
독립적이다. 해당 key가 살아 있는 구조 이동/교체는 override를 유지하고 삭제된 key는
버린다. 로컬 override가 없는 행만 모델의 expanded 기본값 변경을 따른다.
show_chevron=false인 가지는 원본과 같이 자식을 숨기지 않는다.

범위 가산 인덱스는 접힌 조상 수와 보이는 행 수·활성 가능 행 수·누적 높이를 유지한다.
한 가지의 펼침/접힘은 자손 범위에 O(log n)으로 반영한다. 보이는 순번·활성 가능 순번·
누적 offset·가변 높이 hit-test도 O(log n)이다. expand_ancestors는 조상 깊이에
비례하는 구간 갱신이고, 명시적 expand_all/collapse_all은 인덱스를 한 번 재구성한다.
collapse_all의 기본값은 루트를 펼쳐 두는 원본 동작이다. 순번 선택 대상의 eligible은
enabled와 activatable이 모두 참인 보이는 행이다.

선택과 스크롤은 상위 위젯이 보유해야 하며 모델에 기록하지 않는다. 투영의 펼침·높이·
조회도 모델 revision을 바꾸지 않는다. 숨겨진 행의 offset은 앞쪽 가시 높이 합이고,
숨겨진 행의 visible_rank/eligible_rank는 not_found를 반환한다. 범위를 벗어난 순번과
offset은 out_of_range이다. 행 높이는 유한한 0 초과 8,192 이하 값이어야 한다.

## 이동 계획과 확정

plan_move는 이동할 행과 하위 트리 전체를 논리적으로 제외한 gap을 계산한다.
raw_insertion은 원래 전위 배열의 gap이며 subtree 내부 삽입은 거절한다. 깊이는 다음
행의 최소 깊이와 이전 행의 허용 깊이 사이로 제한한다. 부모와 before/after/child/root
anchor를 반환하며 하위 최대 깊이는 저장된 인덱스로 검사한다.

포인터 좌표를 받는 resolve_drag_target은 원본의 indent 반올림과 8 기본 step을 따른다.
계획 계산은 나머지 전체 벡터나 경로 배열을 복사하지 않는다. 최대 조상 깊이에 비례하는
탐색과 key 결과 생성 비용이 있다. 이는 알고리즘 경계이며 측정된 레이턴시 보장은 아니다.

commit_move는 revision과 계획 내용을 다시 검증한다. 오래된 계획은 busy, 변조된
계획은 invalid_argument, 읽기 전용 소스는 unsupported이다. 확정 시 O(n)으로 행을
복사·회전하고 인덱스를 준비한 뒤 교체한다. 포인터 hover마다 이 확정을 호출해서는
안 된다. 같은 위치·깊이의 이동은 revision을 늘리지 않는다. draggable 등의 UI 정책은
상위 입력 컨트롤러가 판단한다.

## 소유권·오류·수명

모델과 투영은 생성 스레드 소유이며 모든 메서드는 소유 스레드에서 호출한다.
공개 모델/투영 객체는 복사·이동하지 않으며 create의 shared_ptr로 동일 수명을 공유한다.
백그라운드 결과는 Core 실행기에 먼저 전달한다. 추가 이벤트 루프·작업 스레드·타이머를
생성하지 않는다. rows와 row는 빌린 span/reference이며 다음 데이터 변경까지 유효하다.
이를 작업 스레드나 통지 이후까지 저장해서는 안 된다.

Core QueryPort 관찰자는 약한 참조이고 중복 등록을 제거하며 최대 4,096개이다.
변경은 이미 적용된 상태를 통지한다. 한 수신자의 실패가 다른 수신자 전송을 막지 않지만
호출자에게 실패를 반환한다. 실패를 받았다는 이유로 확정을 재시도하지 말고 state의
revision을 확인한다. 통지 중 같은 객체의 일반 변경은 busy로 거절한다. 모델 관찰자의
수신 순서상 투영이 아직 새 revision을 받지 않았다면 투영 조회도 busy이다.

수신 중 공개 객체의 마지막 소유자가 해제되어도 진행 중 구현 상태는 통지 끝까지 유지된다.
close는 새 작업을 차단하고 한 번의 종료 통지를 전달한다. 투영만 닫아도 공유 모델은
계속 사용할 수 있다. 모델이 닫히면 연결된 투영도 닫힌다. 투영의 인덱스 재구성 실패는
faulted 상태와 오류로 노출하고 다음 소스 변경에서 재구성을 시도한다. 단순 관찰자
실패는 이미 유효하게 반영한 투영을 faulted로 만들지 않는다.

## 사용과 검증 범위

```cpp
auto model = lvrs::HierarchyModel::create();
if (!model) return std::unexpected(model.error());
auto changed = (*model)->set_rows({
    {.key="assets", .label="Assets", .expanded=true},
    {.key="image", .label="Image", .depth=1}
});
if (!changed) return std::unexpected(changed.error());
auto view = lvrs::HierarchyProjection::create(*model);
if (!view) return std::unexpected(view.error());
auto row = (*view)->at_offset(25); // image, model revision is unchanged
```

공개 헤더는 소스에 인접한 HierarchyModel.h·HierarchyProjection.h이다.
HierarchyVisibility는 내부 구현이며 설치하지 않는다. 원본의 시각 HierarchyItem·
HierarchyList, 자체 그리기, 선택/키보드/드래그/터치, 유지되는 수동 행, LVUI AOT,
전체 상속 API와 플랫폼 실행은 다음 단계에서 연결한다.

tests/native/hierarchy_model.cpp와 hierarchy_projection.cpp에 독립적인 전위 순회·
배열 splice oracle, 무작위 조작, 10만 행, 약한 관찰·재진입·종료·소유자·객체 해제의
회귀 사례를 추가했다. 실행은 작업 전체의 최종 일괄 단계까지 보류한다.
일반/ASan+UBSan 컴파일·링크, 작업용 설치, 재배치한 공개 API 소비자의 빌드·정적 검사를
실제 실행 결과와 구분한다. 사용자 전역 설치본은 변경하지 않는다.

이 데이터 단계의 일반·계측 빌드 기록은 build/native-hierarchy-model-evidence에,
재배치한 일반·계측 소비자의 공개 헤더/객체/의존성 검사는
build/native-hierarchy-model-consumer에 보관한다. 최종 정적 감사와 실행 대기 목록은
final-build-audit.json과 runtime-batch-pending.json이며 실제 실행은 모두 false이다.
모델 37개·투영 32개 사례는 네 구성에서 각각 등록할 뿐 통과로 집계하지 않는다.
