<a id="hierarchy-ux-completion-plan-lvrs"></a>

# 계층 구조 UX 완료 계획(LVRS)

<a id="status"></a>

## 상태
- 시작됨: 2026-03-15
- 모드: 증분, 계약 우선

<a id="stage-1-interaction-semantics-started"></a>

## 스테이지 1. 상호작용 의미론(시작됨)
- `HierarchyDropMode` 추가: 이전/이후/하위/루트
- `HierarchyMoveIntent` 계약 추가

<a id="stage-2-structural-operations-started"></a>

## 스테이지 2. 구조적 작업(시작)
- 의도에 따른 결정적 이동 애플리케이션 추가
- 형제 순서를 유지하고 순환을 방지합니다.

<a id="stage-3-visibility-engine-started"></a>

## 스테이지 3. 가시성 엔진(시작됨)
- 노드 + 확장 상태에서 표시되는 행 투영 추가

<a id="stage-4-selectionfocus-state-started"></a>

## 스테이지 4. 선택/초점 상태(시작됨)
- `HierarchySelectionState` 계약 추가

<a id="stage-5-keyboardinline-edit-contract-planned"></a>

## 스테이지 5. 키보드/인라인 편집 계약(예정)
- 작업 열거형 및 상태 전환 정의

<a id="stage-6-transactionundo-redo-started"></a>

## 스테이지 6. 트랜잭션/실행 취소-다시 실행(시작됨)
- 이동 의도를 위한 명령 스택 추가

<a id="stage-7-syncconflict-baseline-planned"></a>

## 스테이지 7. 동기화/충돌 기준(예정)
- 버전 메타데이터 계약 및 충돌 표시 추가

<a id="stage-8-performance-gate-planned"></a>

## 스테이지 8. 퍼포먼스 게이트(예정)
- 기준 성능 테스트 대상 추가(10,000개 노드)

<a id="stage-9-quality-lock-started"></a>

## 스테이지 9. 품질 잠금(시작됨)
- 이동/가시성/실행 취소 경로에 대한 단위 테스트 추가

<a id="stage-10-consumer-api--sample-planned"></a>

## 스테이지 10. 컨슈머 API + 샘플(예정)
- 백엔드 API에 컨트롤러 노출
- 최소한의 소비자 예시 추가
