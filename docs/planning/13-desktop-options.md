# 데스크톱 기술 대안 및 결정 게이트
상태: 대안 비교 기록. 현재 기본 방향은 사용자 위임에 따른 C++ + Qt(ADR-008)다. 세부 기술·성능은 미검증이며 아래 비교는 선택 근거와 재검토 대안이다. 구현/실측 없음.

## 확정 요구와 해석 충돌
사용자 확정은 Windows/macOS 설치형 제품이다. '웹앱은 아니고'를 곧바로 웹 기술 UI 금지·승인 또는 OS 기본 컨트롤만 사용하라는 뜻으로 확장하지 않는다.
원고는 비웹 UI와 Qt 우선안을 강하게 권고하지만 이는 조사자의 제안이다. JOBS의 UI 기술 결정으로 승격하지 않는다. Qt Widgets/Avalonia를 우선 비교하고 Tauri/Electron은 요구 해석·성능·접근성 확인이 필요한 웹 기술 UI 대안으로 둔다.

## 후보 비교
| 후보 | 제안 이유 | 미검증·비용·운영 조건 |
|---|---|---|
| Qt Widgets | 모델/뷰/델리게이트를 통한 WBS·표·간트의 공통 편집 모델 평가 | C++ 역량·메모리/스레드 안전성·간트 직접 구현·IME·접근성·모듈별 라이선스 |
| Avalonia | .NET/C#/XAML 생태계와 자체 렌더링, 팀 역량에 따른 대항 후보 | 가상화·GC 중 응답·간트 동기화·macOS 메뉴/인쇄/IME. 코어 MIT와 TreeDataGrid/Parcel 등의 현행 상용 조건 분리 |
| Tauri | Rust 백엔드와 OS WebView, 설치형 대안 | Windows WebView2/macOS WKWebView 차이, 오프라인 런타임·접근성·IME·웹 UI 요구 적합성 |
| Electron | Chromium 포함으로 렌더 환경을 앱 버전과 관리 | Chromium/Node 패치 운영·메모리·설치 크기·대형 그리드·접근성. 미측정 성능 순위 금지 |

Qt/Avalonia도 모든 UI가 OS 기본 컨트롤인 것은 아니다. 설치 파일 생성만으로 오프라인·복구·전문 편집 요구 충족을 선언하지 않는다.
기술·버전·컨트롤·라이선스 확정 전 공식 조건을 다시 고정한다. 특정 상용 구성요소 구매나 인증서 구매는 승인되지 않았다.

## 공통 구조 제안
문서 모델·stable ID·명령/undo 기록은 하나로 관리하고 표/트리/간트는 가시 행 매핑·선택·행높이·스크롤을 공유한다. 화면 픽셀은 날짜 원본이 아니다. 드래그 미리보기와 확정 계산을 분리한다.
계산 코어는 UI 컨트롤·현지 문자열·OS 현재 시간대와 독립한다. 입력revision/engine/profile/calendar/tzdb를 고정하고 오래된 결과는 새 편집에 적용하지 않는다.
Qt+C++/Rust core, Avalonia+.NET core 등은 검증 후보다. 언어가 정확성을 자동 보장하지 않는다. 긴 계산·파일 교환은 UI 밖에서 취소·진행·제한을 제공하고, 프로세스 격리는 직렬화 비용과 장애 격리를 측정해 결정한다.
SQLite는 저장 후보일 뿐 확정이 아니다. 첨부·부분읽기·감사·암호화·이관·단일파일/공유 요구와 비교한다. WAL DB를 열린 상태에서 파일 하나만 복사해 정상 백업으로 간주하지 않는다. 네트워크 파일 공유를 DB 협업으로 대체하지 않는다.
저장/자동복구/백업/승인기준선/undo는 독립 개념이다. 원자적 교체나 DB 트랜잭션만으로 복구가 완성되지 않는다. 복구본은 문서ID·revision·checksum·완료 여부와 함께 원본 보존 미리보기를 제공한다.
앱 롤백과 문서 schema 롤백은 다르다. 저장 중단·디스크 부족·권한 거부·이동식 디스크 제거·동기화 충돌·중복 실행·이관 중단을 각각 검증한다.
협업·서버·공유자원·포트폴리오는 선택 서비스/어댑터 확장 후보로 유지한다. 현재 서버 구축은 하지 않는다.

## 기술 선택 게이트
| ID | 입력과 시험 | 검증 증거·통과 방향 |
|---|---|---|
| ARCH-G0 | OS/CPU 최소버전·오프라인·팀역량·라이선스·UI요구 해석 | 제품 요구 승인. Intel/ARM 지원을 임의 확정하지 않음 |
| ARCH-G1 | 같은 WBS 1만/10만·관계 데이터, 고정열·접기·간트확대·붙여넣기·계층이동·undo | 편집 정확성·선택보존과 중앙값/P95/메모리·프레임. 원고의 100ms/33ms는 실측 없는 제안이며 제품 목표값 채택은 보류. 100만은 스트레스 후보 |
| ARCH-G2 | 계산 F01~12, 달력/DST·관계·제약·실적·취소·stale결과 | 같은 입력의 Windows/macOS 정규화 결과·진단 일치, 필드별 오차 근거 |
| ARCH-G3 | 저장/복구/이관의 강제종료·용량/권한 오류 | 성공으로 보고한 저장 보존, 복구 가능한revision·미복구 변경 명시. 무손실 절대보장 금지 |
| ARCH-G4 | 키보드 전체흐름·IME·NVDA/Narrator/VoiceOver·고대비·고DPI | 포인터 없이 핵심 과업 완료, 간트 의미와 오류 읽기/조작. 프레임워크 지원만으로 통과 금지 |
| ARCH-G5 | OS별 clean machine 설치→오프라인실행→파일연결→업데이트중단→복구→제거 | 지원매트릭스별 증거·사용자문서보존, 자동/수동업데이트. 현재 배포/서명 실행 미승인 |
| ARCH-G6 | UI/그리드/간트/포맷SDK/리포트/폰트/배포도구 BOM | 3년 비용 시나리오와 지원중단 대안. 견적 전 가격·인력·기간 확정 금지 |

필수 정확성·복구·접근성 실패를 평균 점수로 상쇄하지 않는다. 요구 적합성→필수 게이트→팀역량/유지비 순으로 결정한다. 기본 방향은 C++ + Qt로 선택됐지만 어느 후보도 검증 게이트를 통과한 것은 아니다.

## 원고와 출처
Library libfile_6cf9e72efe1c8191ac59d789c3e5c415, file_0000000098a881f6be7e8de742987fc7, 전달 버전0, 22,256bytes. 공식 read 1~142줄 전체 검토. 공식 helper 원본 전송은 download failed로 미완료.
아래는 원고의 공식 출처이며 이번 통합에서 개별 웹페이지 최신 조건을 다시 전수 검증한 것은 아니다.
- [Qt 모델/뷰](https://doc.qt.io/qt-6/model-view-programming.html), [Qt 접근성](https://doc.qt.io/qt-6/accessible-qwidget.html), [Qt 라이선스](https://doc.qt.io/qt-6/licensing.html)
- [Avalonia](https://docs.avaloniaui.net/docs/welcome), [TreeDataGrid](https://docs.avaloniaui.net/controls/data-display/structured-data/treedatagrid/), [Avalonia 접근성](https://docs.avaloniaui.net/docs/app-development/accessibility)
- [Tauri 아키텍처](https://v2.tauri.app/concept/architecture/), [Tauri 웹뷰](https://v2.tauri.app/reference/webview-versions/)
- [Electron 프로세스](https://www.electronjs.org/docs/latest/tutorial/process-model), [Electron 보안](https://www.electronjs.org/docs/latest/tutorial/security)
- [SQLite WAL](https://sqlite.org/wal.html), [백업](https://sqlite.org/backup.html), [QSaveFile](https://doc.qt.io/qt-6/qsavefile.html)


## 언어 질문에 대한 기록
사용자가 C#/Rust/C++를 비교 질문한 당시의 선택은 미확정이었다. 이 상태는 현재 ADR-008로 대체됐다. 당시 ADR-P06에 따라 C++/Qt 잠정 후보와 C#/Avalonia 대안을 같은 fixture로 비교하고, Rust는 코어/GUI 생태계의 별도 평가 후보로 둔다. 초기 다중 언어 혼합을 피하는 기본안은 제안이며 언어 자체가 계산 정확성·성능을 보장하지 않는다.

## 최신 결정에 따른 상태 갱신
사용자 위임으로 C++ + Qt가 기본 방향으로 선택됐다(ADR-008). 위 후보 미확정 비교는 결정 당시의 검토 근거이며 현재 선택 상태를 대체하지 않는다. Qt/Avalonia 동시 구현이나 초기 Rust 혼합은 하지 않는다. 세부 모듈·버전·라이선스는 게이트 검토 후 결정하고 실패 시 대안을 재검토한다.
