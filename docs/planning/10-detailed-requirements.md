# 337개 세부 요구사항 추적 초안

공식 Library read 전체 본문으로 원본 행을 확인했다. 원본 bytes 다운로드는 미완료다. 원본 필드는 09-source-traceability.json에 그대로 보존했다. 아래 입력/출력/예외와 단계 매핑은 JOBS 설계 제안이며 행별 의미 검토와 실행 증거는 미완료다. 전체 제품 기능 전수식별·동등성·PRD 완료를 뜻하지 않는다.

## REQ-MP-D001 · 프로젝트 생성과 템플릿
- 원본: MP-D001 / Microsoft Project 제품군 / D / 데스크톱 작업 구조와 일정 엔진. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 기존 계획·팀·배정을 템플릿으로 재사용
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '프로젝트 생성과 템플릿' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 기존 계획·팀·배정을 템플릿으로 재사용의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 템플릿 복사 범위 정의 필요 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 템플릿 생성 후 원본과 새 프로젝트를 독립 보존. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-02 / J-01 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S04](https://learn.microsoft.com/en-us/office365/servicedescriptions/project-online-service-description/project-online-desktop-client-service-description)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X008, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D002 · 작업 CRUD와 일괄 입력
- 원본: MP-D002 / Microsoft Project 제품군 / D / 데스크톱 작업 구조와 일정 엔진. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 표·작업 폼에서 작업 삽입 및 편집
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '작업 CRUD와 일괄 입력' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 표·작업 폼에서 작업 삽입 및 편집의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 여러 작업 삽입 후 ID와 참조가 일관됨. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-02 / J-01 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S58](https://support.microsoft.com/en-us/project/project-help)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D003 · 요약 작업과 하위 작업
- 원본: MP-D003 / Microsoft Project 제품군 / D / 데스크톱 작업 구조와 일정 엔진. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 들여쓰기/내어쓰기로 계층 구성
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '요약 작업과 하위 작업' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 들여쓰기/내어쓰기로 계층 구성의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: WBS와 종속성은 별개 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 이동 후 부모 집계와 계층 번호 재계산. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-02 / J-01 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S06](https://support.microsoft.com/en-au/project/create-wbs-codes-in-project-desktop), [MS-S58](https://support.microsoft.com/en-us/project/project-help)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D004 · 프로젝트 요약 행
- 원본: MP-D004 / Microsoft Project 제품군 / D / 데스크톱 작업 구조와 일정 엔진. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 전체 프로젝트 집계 행 표시
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '프로젝트 요약 행' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 전체 프로젝트 집계 행 표시의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 일정·작업량·비용의 최상위 집계 검증. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-02 / J-01 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S58](https://support.microsoft.com/en-us/project/project-help)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D005 · 자동 Outline Number
- 원본: MP-D005 / Microsoft Project 제품군 / D / 데스크톱 작업 구조와 일정 엔진. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 계층 위치에 따라 숫자 번호 자동 변경
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '자동 Outline Number' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 계층 위치에 따라 숫자 번호 자동 변경의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 작업 이동 시 1.1 등 번호를 재생성. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-02 / J-01 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S06](https://support.microsoft.com/en-au/project/create-wbs-codes-in-project-desktop)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D006 · 사용자 WBS 코드
- 원본: MP-D006 / Microsoft Project 제품군 / D / 데스크톱 작업 구조와 일정 엔진. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 접두사·문자 유형·길이·구분자·고유성 설정
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '사용자 WBS 코드' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 접두사·문자 유형·길이·구분자·고유성 설정의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 자동 outline 번호와 구분 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 코드 마스크와 중복 검사 동작. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-02 / J-01 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S06](https://support.microsoft.com/en-au/project/create-wbs-codes-in-project-desktop)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D007 · WBS 재번호
- 원본: MP-D007 / Microsoft Project 제품군 / D / 데스크톱 작업 구조와 일정 엔진. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 이동/삭제 후 명시적으로 코드 재번호 가능
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'WBS 재번호' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 이동/삭제 후 명시적으로 코드 재번호 가능의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: WBS는 이동 시 자동 재번호 아님 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 외부 참조 보존과 재번호를 별도 명령 처리. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-02 / J-01 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S06](https://support.microsoft.com/en-au/project/create-wbs-codes-in-project-desktop)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D008 · 마일스톤
- 원본: MP-D008 / Microsoft Project 제품군 / D / 데스크톱 작업 구조와 일정 엔진. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 주요 일정 지점을 마일스톤으로 표시
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '마일스톤' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 주요 일정 지점을 마일스톤으로 표시의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 0기간 및 마일스톤 표시·필터 시험. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-02 / J-01 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S04](https://learn.microsoft.com/en-us/office365/servicedescriptions/project-online-service-description/project-online-desktop-client-service-description), [MS-S58](https://support.microsoft.com/en-us/project/project-help)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D009 · 반복 작업
- 원본: MP-D009 / Microsoft Project 제품군 / D / 데스크톱 작업 구조와 일정 엔진. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 일·주·월·년 주기, 종료 횟수/날짜, 별도 달력
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '반복 작업' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 일·주·월·년 주기, 종료 횟수/날짜, 별도 달력의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 반복 인스턴스 배정/평준화 별도 규칙 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 월간 회의 생성과 휴일 예외 시험. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-02 / J-01 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S07](https://support.microsoft.com/en-us/project/create-recurring-tasks)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X004, microsoft:MP-X005, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D010 · 작업 분할
- 원본: MP-D010 / Microsoft Project 제품군 / D / 데스크톱 작업 구조와 일정 엔진. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 중단과 재시작을 분리 구간으로 표시
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '작업 분할' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 중단과 재시작을 분리 구간으로 표시의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 구간 분할 후 총 작업량과 완료일 비교. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-02 / J-01 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S58](https://support.microsoft.com/en-us/project/project-help)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D011 · 자동 일정
- 원본: MP-D011 / Microsoft Project 제품군 / D / 데스크톱 작업 구조와 일정 엔진. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 기간·연결·제약·달력·자원으로 날짜 재계산
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '자동 일정' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 기간·연결·제약·달력·자원으로 날짜 재계산의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 일정 엔진 핵심 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 선행 작업 변경이 영향 범위에 전파됨. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-02 / J-01 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S05](https://support.microsoft.com/en-us/project/how-project-schedules-tasks-behind-the-scenes)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X005, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D012 · 수동 일정과 미정 정보
- 원본: MP-D012 / Microsoft Project 제품군 / D / 데스크톱 작업 구조와 일정 엔진. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 날짜/기간에 미정 텍스트 허용; 자동 이동과 다름
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '수동 일정과 미정 정보' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 날짜/기간에 미정 텍스트 허용; 자동 이동과 다름의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 수동 작업은 달력/제약 동작이 다름 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 미정 값과 수동 고정 날짜를 데이터 손실 없이 보존. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-02 / J-01 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S05](https://support.microsoft.com/en-us/project/how-project-schedules-tasks-behind-the-scenes)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X002, microsoft:MP-X005, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D013 · 수동 자동 전환
- 원본: MP-D013 / Microsoft Project 제품군 / D / 데스크톱 작업 구조와 일정 엔진. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 자동 전환 시 유효하지 않은 텍스트를 기본값으로 해석
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '수동 자동 전환' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 자동 전환 시 유효하지 않은 텍스트를 기본값으로 해석의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 세부 변환 정책 실제품 재현 필요 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 전환 전후 차이와 경고를 명시. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-02 / J-01 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S05](https://support.microsoft.com/en-us/project/how-project-schedules-tasks-behind-the-scenes)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X002, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D014 · 시작일 기준과 종료일 기준 일정
- 원본: MP-D014 / Microsoft Project 제품군 / D / 데스크톱 작업 구조와 일정 엔진. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 순방향/역방향 일정, ASAP/ALAP 기본 제약 차이
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '시작일 기준과 종료일 기준 일정' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 순방향/역방향 일정, ASAP/ALAP 기본 제약 차이의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 역방향 평준화 지연 처리 주의 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 동일 네트워크를 양방향 계산. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-02 / J-01 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S05](https://support.microsoft.com/en-us/project/how-project-schedules-tasks-behind-the-scenes)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X004, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D015 · FS SS FF SF 종속성
- 원본: MP-D015 / Microsoft Project 제품군 / D / 데스크톱 작업 구조와 일정 엔진. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 네 가지 선후행 관계; 복수 선행/후행
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'FS SS FF SF 종속성' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 네 가지 선후행 관계; 복수 선행/후행의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: SS/FF는 반드시 같은 날짜라는 뜻 아님 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 네 유형별 날짜 부등식 시험 통과. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-02 / J-01 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S10](https://support.microsoft.com/en-gb/project/link-tasks-in-a-project)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D016 · Lead와 Lag
- 원본: MP-D016 / Microsoft Project 제품군 / D / 데스크톱 작업 구조와 일정 엔진. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 양/음 기간과 비율로 지연/중첩 설정
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'Lead와 Lag' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 양/음 기간과 비율로 지연/중첩 설정의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 종속성·달력 필요 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 2일 지연과 -50% 중첩을 구분 계산. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-03 / J-03 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S11](https://support.microsoft.com/en-au/project/add-lead-or-lag-time-to-a-task)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X005, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D017 · 연결 편집과 자동 연결
- 원본: MP-D017 / Microsoft Project 제품군 / D / 데스크톱 작업 구조와 일정 엔진. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 연결 추가·해제 및 삽입 작업 자동 연결 옵션
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '연결 편집과 자동 연결' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 연결 추가·해제 및 삽입 작업 자동 연결 옵션의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 수동 작업 링크 갱신 옵션 별도 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 중간 작업 삽입 시 옵션별 연결 그래프 확인. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-02 / J-01 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S10](https://support.microsoft.com/en-gb/project/link-tasks-in-a-project)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X002, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D018 · 8종 날짜 제약
- 원본: MP-D018 / Microsoft Project 제품군 / D / 데스크톱 작업 구조와 일정 엔진. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: ASAP ALAP SNET FNET SNLT FNLT MSO MFO
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '8종 날짜 제약' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: ASAP ALAP SNET FNET SNLT FNLT MSO MFO의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 날짜 직접 입력의 제약 생성 주의 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 각 제약의 허용 날짜와 충돌 시험. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-03 / J-03 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S12](https://support.microsoft.com/en-us/project/set-a-task-start-or-finish-date-constraint-for-a-task)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D019 · 제약과 연결 우선순위
- 원본: MP-D019 / Microsoft Project 제품군 / D / 데스크톱 작업 구조와 일정 엔진. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 제약 준수 옵션으로 충돌 시 동작 달라짐
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '제약과 연결 우선순위' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 제약 준수 옵션으로 충돌 시 동작 달라짐의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 실제품 경계 사례 필요 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 불가능한 일정 충돌을 숨기지 않고 설명. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-03 / J-03 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S05](https://support.microsoft.com/en-us/project/how-project-schedules-tasks-behind-the-scenes), [MS-S10](https://support.microsoft.com/en-gb/project/link-tasks-in-a-project)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D020 · Deadline
- 원본: MP-D020 / Microsoft Project 제품군 / D / 데스크톱 작업 구조와 일정 엔진. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 목표일 표시 및 지연 지시자; 총여유에 영향 가능
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'Deadline' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 목표일 표시 및 지연 지시자; 총여유에 영향 가능의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 단순 알림으로만 모델링하면 불완전 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: Deadline과 Must Finish On을 구별. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-02 / J-01 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S13](https://support.microsoft.com/en-us/project/deadline-task-field)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D021 · Fixed Units Work Duration
- 원본: MP-D021 / Microsoft Project 제품군 / D / 데스크톱 작업 구조와 일정 엔진. 근거 상태: partial; 기능 확인과 함께 에디션·조건·세부 동작 재검증 필요.
- 동작: 작업량·기간·배정 단위 중 고정값에 따라 재계산
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'Fixed Units Work Duration' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 작업량·기간·배정 단위 중 고정값에 따라 재계산의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 수동 작업 적용 설명은 문서 간 충돌 · 재검증. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 3유형×3변경 항목 계산 회귀시험. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-02 / J-01 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S08](https://support.microsoft.com/en-us/project/change-the-task-type-for-more-accurate-scheduling), [MS-S05](https://support.microsoft.com/en-us/project/how-project-schedules-tasks-behind-the-scenes)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X002, microsoft:MP-X003, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D022 · Effort driven
- 원본: MP-D022 / Microsoft Project 제품군 / D / 데스크톱 작업 구조와 일정 엔진. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 최초 배정 후 인원 변경 시 총 작업량을 유지·재분배
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'Effort driven' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 최초 배정 후 인원 변경 시 총 작업량을 유지·재분배의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: Fixed Work는 항상 적용; 요약/삽입 프로젝트 제외 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 인원 추가 시 작업량 보존 여부를 옵션별 검증. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-02 / J-01 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S09](https://support.microsoft.com/en-us/project/change-the-effort-driven-setting-for-task-types)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X003, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D023 · 기간 단위와 예상 기간
- 원본: MP-D023 / Microsoft Project 제품군 / D / 데스크톱 작업 구조와 일정 엔진. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 분·시간·일·주·월 등 기간 입력
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '기간 단위와 예상 기간' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 분·시간·일·주·월 등 기간 입력의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: elapsed duration 전체 규칙은 MP-X 참조 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 표시 단위와 근무시간 환산을 분리. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-02 / J-01 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S63](https://support.microsoft.com/en-us/project/change-a-task-duration)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D024 · CPM 임계경로
- 원본: MP-D024 / Microsoft Project 제품군 / D / 데스크톱 작업 구조와 일정 엔진. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 프로젝트 종료를 좌우하는 작업 경로 표시
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'CPM 임계경로' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 프로젝트 종료를 좌우하는 작업 경로 표시의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 달력·제약·완료 상태 필요 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 비임계 지연과 임계 지연의 종료 영향 비교. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-03 / J-03 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S14](https://support.microsoft.com/en-us/project/manage-your-project-s-critical-path)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X005, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D025 · 복수 및 교차 프로젝트 임계경로
- 원본: MP-D025 / Microsoft Project 제품군 / D / 데스크톱 작업 구조와 일정 엔진. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 독립 네트워크별 경로 및 통합 경로 분석
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '복수 및 교차 프로젝트 임계경로' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 독립 네트워크별 경로 및 통합 경로 분석의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: Master/외부 연결 필요 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 하위 프로젝트 임계경로와 전체 경로 비교. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-03 / J-03 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S14](https://support.microsoft.com/en-us/project/manage-your-project-s-critical-path), [MS-S25](https://support.microsoft.com/en-us/project/link-projects-to-create-a-master-project-in-project-desktop)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X016, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D026 · 총여유와 자유여유
- 원본: MP-D026 / Microsoft Project 제품군 / D / 데스크톱 작업 구조와 일정 엔진. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 후속 또는 프로젝트 종료에 영향 없는 여유 표시
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '총여유와 자유여유' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 후속 또는 프로젝트 종료에 영향 없는 여유 표시의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: Deadline 영향 포함 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 기준 네트워크의 두 여유값 독립 검산. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-03 / J-03 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S05](https://support.microsoft.com/en-us/project/how-project-schedules-tasks-behind-the-scenes), [MS-S13](https://support.microsoft.com/en-us/project/deadline-task-field)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D027 · Task Inspector
- 원본: MP-D027 / Microsoft Project 제품군 / D / 데스크톱 작업 구조와 일정 엔진. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 작업 날짜를 결정한 요인과 오류 설명
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'Task Inspector' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 작업 날짜를 결정한 요인과 오류 설명의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 세부 원인 분류는 재현 필요 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 날짜가 이동한 이유를 영향 요소로 표시. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-02 / J-01 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S04](https://learn.microsoft.com/en-us/office365/servicedescriptions/project-online-service-description/project-online-desktop-client-service-description)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D028 · Task Path 분석
- 원본: MP-D028 / Microsoft Project 제품군 / D / 데스크톱 작업 구조와 일정 엔진. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 선택 작업과 연결된 경로를 강조
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'Task Path 분석' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 선택 작업과 연결된 경로를 강조의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 세부 edition·UI 검증 필요 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 선행/후행 및 driving 경로 시험. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-02 / J-01 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S04](https://learn.microsoft.com/en-us/office365/servicedescriptions/project-online-service-description/project-online-desktop-client-service-description)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D029 · 작업 비활성화
- 원본: MP-D029 / Microsoft Project 제품군 / Pro / 데스크톱 작업 구조와 일정 엔진. 근거 상태: partial; 기능 확인과 함께 에디션·조건·세부 동작 재검증 필요.
- 동작: 계획에 남기되 일정과 자원 계산에서 제외
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '작업 비활성화' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 계획에 남기되 일정과 자원 계산에서 제외의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: Standard 제외; 완료 보관과 다른 개념 · 조건. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 비활성 전후 계산 차이, 재활성 원상복원. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-02 / J-01 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S28](https://support.microsoft.com/en-us/project/inactivate-a-task)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D030 · 기본 프로젝트 달력
- 원본: MP-D030 / Microsoft Project 제품군 / D / 달력과 자원. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 표준·24시간·야간교대 등 기본 달력 사용
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '기본 프로젝트 달력' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 표준·24시간·야간교대 등 기본 달력 사용의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 근무시간과 표시상 일 환산 분리 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 작업일/휴일에 따른 종료일 비교. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-03 / J-03 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S62](https://support.microsoft.com/en-us/project/set-the-general-working-days-and-times-for-a-project)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X005, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D031 · 자원 달력
- 원본: MP-D031 / Microsoft Project 제품군 / D / 달력과 자원. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 자원별 가용 근무시간과 비근무 예외
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '자원 달력' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 자원별 가용 근무시간과 비근무 예외의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 수동 일정은 다르게 처리 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 휴가 자원 배정 시 일정 변경 확인. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-03 / J-03 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S68](https://support.microsoft.com/en-us/project/set-a-unique-schedule-for-a-specific-resource), [MS-S05](https://support.microsoft.com/en-us/project/how-project-schedules-tasks-behind-the-scenes)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X002, microsoft:MP-X005, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D032 · 작업 달력과 자원 무시 옵션
- 원본: MP-D032 / Microsoft Project 제품군 / D / 달력과 자원. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 작업/자원 달력의 교집합 또는 자원 달력 무시
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '작업 달력과 자원 무시 옵션' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 작업/자원 달력의 교집합 또는 자원 달력 무시의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 야간 작업·주간 자원 충돌 사례 시험. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-03 / J-03 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S05](https://support.microsoft.com/en-us/project/how-project-schedules-tasks-behind-the-scenes)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X005, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D033 · 휴일 휴가와 비근무시간
- 원본: MP-D033 / Microsoft Project 제품군 / D / 달력과 자원. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 달력에 휴가·휴일 예외를 반영
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '휴일 휴가와 비근무시간' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 달력에 휴가·휴일 예외를 반영의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 반복 예외 세부 규칙 추가 재현 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 달력 변경의 영향 범위를 표시. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-03 / J-03 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S58](https://support.microsoft.com/en-us/project/project-help)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X005, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D034 · Work Material Cost 자원
- 원본: MP-D034 / Microsoft Project 제품군 / D / 달력과 자원. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 인력/장비성 작업 자원, 재료, 비용 자원 구분
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'Work Material Cost 자원' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 인력/장비성 작업 자원, 재료, 비용 자원 구분의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: Generic은 별도 분류 속성 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 세 자원의 단위·비용·달력 특성 분리. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-06 / J-05 / 단계 C; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S20](https://support.microsoft.com/en-us/project/enter-costs-for-resources), [MS-S64](https://support.microsoft.com/en-us/project/add-resources-to-your-project)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X005, microsoft:MP-X020, microsoft:MP-X023. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D035 · Generic 자원과 실명 대체
- 원본: MP-D035 / Microsoft Project 제품군 / D/Pro / 달력과 자원. 근거 상태: partial; 기능 확인과 함께 에디션·조건·세부 동작 재검증 필요.
- 동작: 초기 일반 자원으로 계획; 전사 팀 구성 지원
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'Generic 자원과 실명 대체' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 초기 일반 자원으로 계획; 전사 팀 구성 지원의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 전사 Build Team은 Pro · 조건. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 역할 수요와 실명 배정을 연결. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-06 / J-05 / 단계 C; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S04](https://learn.microsoft.com/en-us/office365/servicedescriptions/project-online-service-description/project-online-desktop-client-service-description), [MS-S64](https://support.microsoft.com/en-us/project/add-resources-to-your-project)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X005, microsoft:MP-X010, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D036 · Max Units와 배정 단위
- 원본: MP-D036 / Microsoft Project 제품군 / D / 달력과 자원. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 부분/전일/복수 인력 용량, 배정 단위 입력
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'Max Units와 배정 단위' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 부분/전일/복수 인력 용량, 배정 단위 입력의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 달력과 자원 가용기간 필요 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 50%·100%·300% 용량 계산. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-06 / J-05 / 단계 C; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S64](https://support.microsoft.com/en-us/project/add-resources-to-your-project)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X003, microsoft:MP-X005, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D037 · 자원 배정 생성과 교체
- 원본: MP-D037 / Microsoft Project 제품군 / D / 달력과 자원. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 작업과 자원의 배정 및 작업량 관리
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '자원 배정 생성과 교체' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 작업과 자원의 배정 및 작업량 관리의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 교체 세부 규칙 실제품 확인 필요 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 교체 시 일정·실적 보존 정책 검증. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-06 / J-05 / 단계 C; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S64](https://support.microsoft.com/en-us/project/add-resources-to-your-project)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X005, microsoft:MP-X008, microsoft:MP-X014, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D038 · 시간 구간별 작업량
- 원본: MP-D038 / Microsoft Project 제품군 / D / 달력과 자원. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: Task/Resource Usage에서 시간대별 배정 값 표시
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '시간 구간별 작업량' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: Task/Resource Usage에서 시간대별 배정 값 표시의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: task/resource/assignment grain 분리 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 일·주·월 집계와 총량 일치. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-06 / J-05 / 단계 C; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S21](https://support.microsoft.com/en-us/project/update-work-on-a-project), [MS-S31](https://support.microsoft.com/en-us/project/available-fields-reference)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X005, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D039 · Work Contour
- 원본: MP-D039 / Microsoft Project 제품군 / D / 달력과 자원. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 배정 기간에 작업량을 비선형 분포
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'Work Contour' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 배정 기간에 작업량을 비선형 분포의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 배정 단위 기능 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: front/back-loaded 및 수동 분포 총량 확인. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-06 / J-05 / 단계 C; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S15](https://support.microsoft.com/en-gb/project/work-contour-assignment-field)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X003, microsoft:MP-X005, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D040 · 과다배정과 가용성 표시
- 원본: MP-D040 / Microsoft Project 제품군 / D / 달력과 자원. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 자원 과다배정 표시 및 사용량 확인
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '과다배정과 가용성 표시' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 자원 과다배정 표시 및 사용량 확인의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 평준화 감도 단위에 의존 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 동일 시각 중복 용량과 하루 총량 구별. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-06 / J-05 / 단계 C; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S16](https://support.microsoft.com/en-us/project/distribute-project-work-evenly-level-resource-assignments)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X004, microsoft:MP-X005, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D041 · 자원 자동 평준화
- 원본: MP-D041 / Microsoft Project 제품군 / D / 달력과 자원. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 작업 지연/분할로 과다배정 완화; 사람 교체 아님
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '자원 자동 평준화' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 작업 지연/분할로 과다배정 완화; 사람 교체 아님의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 자재/비용 자원 평준화 제외 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 용량 충돌 해소와 종료일 변화 비교. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-06 / J-06 / 단계 D; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S16](https://support.microsoft.com/en-us/project/distribute-project-work-evenly-level-resource-assignments)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X004, microsoft:MP-X005, microsoft:MP-X020, microsoft:MP-X023. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D042 · 평준화 범위와 감도
- 원본: MP-D042 / Microsoft Project 제품군 / D / 달력과 자원. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 선택 작업/자원/전체, 날짜 범위, 감도 설정
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '평준화 범위와 감도' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 선택 작업/자원/전체, 날짜 범위, 감도 설정의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 분·일·주 단위 충돌 탐지 비교. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-06 / J-06 / 단계 D; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S16](https://support.microsoft.com/en-us/project/distribute-project-work-evenly-level-resource-assignments)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X004, microsoft:MP-X005, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D043 · 평준화 우선순위와 순서
- 원본: MP-D043 / Microsoft Project 제품군 / D / 달력과 자원. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: ID/표준/우선순위 및 프로젝트 우선순위
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '평준화 우선순위와 순서' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: ID/표준/우선순위 및 프로젝트 우선순위의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 우선순위 1000은 평준화 제외 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 중요 작업 보호와 순서 결과 확인. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-06 / J-06 / 단계 D; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S16](https://support.microsoft.com/en-us/project/distribute-project-work-evenly-level-resource-assignments)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X004, microsoft:MP-X005, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D044 · 여유 내 평준화와 분할 옵션
- 원본: MP-D044 / Microsoft Project 제품군 / D / 달력과 자원. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 여유 내 제한, 개별 배정 이동, 잔여 작업 분할
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '여유 내 평준화와 분할 옵션' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 여유 내 제한, 개별 배정 이동, 잔여 작업 분할의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 수동/proposed 포함 옵션 존재 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 종료일 유지가 불가능하면 실패 사유 표시. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-03 / J-03 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S16](https://support.microsoft.com/en-us/project/distribute-project-work-evenly-level-resource-assignments)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X002, microsoft:MP-X004, microsoft:MP-X005, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D045 · 평준화 취소와 결과 시각화
- 원본: MP-D045 / Microsoft Project 제품군 / D / 달력과 자원. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: Clear Leveling 및 Leveling Gantt
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '평준화 취소와 결과 시각화' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: Clear Leveling 및 Leveling Gantt의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 원래 일정과 평준화 결과 비교/복원. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-06 / J-06 / 단계 D; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S16](https://support.microsoft.com/en-us/project/distribute-project-work-evenly-level-resource-assignments)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X004, microsoft:MP-X005, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D046 · 파일 기반 공유 Resource Pool
- 원본: MP-D046 / Microsoft Project 제품군 / D / 달력과 자원. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 서버 없이 여러 파일에서 공유 자원·충돌 분석
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '파일 기반 공유 Resource Pool' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 서버 없이 여러 파일에서 공유 자원·충돌 분석의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 전사 풀과 별도 기능 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 두 프로젝트의 같은 자원 중복 수요 탐지. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-06 / J-05 / 단계 C; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S26](https://support.microsoft.com/en-us/project/create-and-share-a-resource-pool-in-project-desktop)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X005, microsoft:MP-X018, microsoft:MP-X020, microsoft:MP-X021. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D047 · Team Planner
- 원본: MP-D047 / Microsoft Project 제품군 / Pro / 달력과 자원. 근거 상태: partial; 기능 확인과 함께 에디션·조건·세부 동작 재검증 필요.
- 동작: 자원 행별 작업·미배정·미일정 작업 표시
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'Team Planner' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 자원 행별 작업·미배정·미일정 작업 표시의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 완료 작업 이동 제한 · 조건. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 드래그로 시점/담당 변경 시 일정 일관성 유지. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-06 / J-05 / 단계 C; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S27](https://support.microsoft.com/en-us/project/view-your-team-s-work-with-team-planner)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X005, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D048 · Team Planner 과다배정 예방
- 원본: MP-D048 / Microsoft Project 제품군 / Pro / 달력과 자원. 근거 상태: partial; 기능 확인과 함께 에디션·조건·세부 동작 재검증 필요.
- 동작: 이동 충돌 시 다른 작업 자동 이동 옵션
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'Team Planner 과다배정 예방' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 이동 충돌 시 다른 작업 자동 이동 옵션의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 단순 Kanban과 다름 · 조건. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 옵션 on/off 결과 비교 및 변경 내역 표시. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-06 / J-05 / 단계 C; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S27](https://support.microsoft.com/en-us/project/view-your-team-s-work-with-team-planner)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X005, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D049 · Budget 자원
- 원본: MP-D049 / Microsoft Project 제품군 / D / 달력과 자원. 근거 상태: partial; 기능 확인과 함께 에디션·조건·세부 동작 재검증 필요.
- 동작: 자원에 Budget 속성 설정 가능
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'Budget 자원' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 자원에 Budget 속성 설정 가능의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 요약 배정/예산 기간별 동작 추가 확인 필요 · 재검증. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 예산과 일반 실적 자원을 구분. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-06 / J-05 / 단계 C; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S64](https://support.microsoft.com/en-us/project/add-resources-to-your-project)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X005, microsoft:MP-X009, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D050 · 표준 및 초과근무 요율
- 원본: MP-D050 / Microsoft Project 제품군 / D / 비용과 성과 측정. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 자원 작업량과 요율로 비용 계산
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '표준 및 초과근무 요율' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 자원 작업량과 요율로 비용 계산의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 초과근무는 단순 추가 용량과 다름 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 표준/초과 작업량 분리 원가 검산. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-09 / 단계 D; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S20](https://support.microsoft.com/en-us/project/enter-costs-for-resources)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X005, microsoft:MP-X008, microsoft:MP-X020, microsoft:MP-X023. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D051 · Per use 비용
- 원본: MP-D051 / Microsoft Project 제품군 / D / 비용과 성과 측정. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 배정당 사용 비용과 시간 요율 병행
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'Per use 비용' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 배정당 사용 비용과 시간 요율 병행의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 2회 배정 시 적용 단위 확인. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-09 / 단계 D; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S20](https://support.microsoft.com/en-us/project/enter-costs-for-resources)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X008, microsoft:MP-X020, microsoft:MP-X023. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D052 · 고정 비용
- 원본: MP-D052 / Microsoft Project 제품군 / D / 비용과 성과 측정. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 작업/프로젝트 요약에 고정비 입력
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '고정 비용' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 작업/프로젝트 요약에 고정비 입력의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 자원 비용과 중복 없이 집계. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-09 / 단계 D; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S20](https://support.microsoft.com/en-us/project/enter-costs-for-resources)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X008, microsoft:MP-X020, microsoft:MP-X023. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D053 · 비용 자원 배정
- 원본: MP-D053 / Microsoft Project 제품군 / D / 비용과 성과 측정. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 출장비 등 작업량과 독립인 비용 항목
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '비용 자원 배정' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 출장비 등 작업량과 독립인 비용 항목의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 비용의 날짜 분포가 일정에 영향 가능 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 여러 비용 자원을 한 작업에 합산. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-09 / 단계 D; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S20](https://support.microsoft.com/en-us/project/enter-costs-for-resources)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X008, microsoft:MP-X020, microsoft:MP-X023. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D054 · 자재 단가와 수량
- 원본: MP-D054 / Microsoft Project 제품군 / D / 비용과 성과 측정. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 측정 단위별 단가·사용 비용
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '자재 단가와 수량' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 측정 단위별 단가·사용 비용의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 단위와 수량 기반 재료비 검산. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-09 / 단계 D; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S20](https://support.microsoft.com/en-us/project/enter-costs-for-resources)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X008, microsoft:MP-X020, microsoft:MP-X023. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D055 · 요율표와 유효일
- 원본: MP-D055 / Microsoft Project 제품군 / D / 비용과 성과 측정. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 자원별 5개 요율표, 표별 요율 변경
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '요율표와 유효일' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 자원별 5개 요율표, 표별 요율 변경의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 유효일 없이 단가 변경 시 과거 원가 영향 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 승급일 전후·배정별 요율표 비용 비교. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-09 / 단계 D; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S20](https://support.microsoft.com/en-us/project/enter-costs-for-resources), [MS-S30](https://support.microsoft.com/en-gb/project/specifications-for-microsoft-project)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X008, microsoft:MP-X020, microsoft:MP-X023. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D056 · 비용 발생 방식
- 원본: MP-D056 / Microsoft Project 제품군 / D / 비용과 성과 측정. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 시작/끝/비례 배분
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '비용 발생 방식' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 시작/끝/비례 배분의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 자원/고정비 설정을 구분 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 동일 총비용의 기간별 발생 모양 비교. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-09 / 단계 D; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S20](https://support.microsoft.com/en-us/project/enter-costs-for-resources)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X008, microsoft:MP-X020, microsoft:MP-X023. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D057 · 실비 자동 계산과 수동 입력
- 원본: MP-D057 / Microsoft Project 제품군 / D / 비용과 성과 측정. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 실제 작업량 기반 계산 또는 자동 계산 해제
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '실비 자동 계산과 수동 입력' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 실제 작업량 기반 계산 또는 자동 계산 해제의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 회계 원장 수준 보호 기능 아님 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 실적 재계산 시 수동 실비 보존 정책 시험. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-09 / 단계 D; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S20](https://support.microsoft.com/en-us/project/enter-costs-for-resources)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X002, microsoft:MP-X008, microsoft:MP-X020, microsoft:MP-X023. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D058 · 기준선 11개
- 원본: MP-D058 / Microsoft Project 제품군 / D / 비용과 성과 측정. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 시작·종료·기간·작업량·비용 스냅샷
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '기준선 11개' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 시작·종료·기간·작업량·비용 스냅샷의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 전사 기준선 권한 별도 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: Baseline~Baseline10의 독립 저장/비교. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-04 / J-07 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S17](https://support.microsoft.com/en-us/project/set-and-save-a-baseline), [MS-S60](https://support.microsoft.com/en-us/project/create-or-update-a-baseline-or-an-interim-plan-in-project-desktop)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X008, microsoft:MP-X017, microsoft:MP-X018, microsoft:MP-X020, microsoft:MP-X023. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D059 · 선택 작업 기준선과 롤업
- 원본: MP-D059 / Microsoft Project 제품군 / D / 비용과 성과 측정. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 전체 또는 선택 작업 기준선; 요약 집계 선택
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '선택 작업 기준선과 롤업' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 전체 또는 선택 작업 기준선; 요약 집계 선택의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 새 하위 작업 추가 후 부모 기준선 검증. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-04 / J-07 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S17](https://support.microsoft.com/en-us/project/set-and-save-a-baseline)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X008, microsoft:MP-X017, microsoft:MP-X020, microsoft:MP-X023. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D060 · Interim Plan 10개
- 원본: MP-D060 / Microsoft Project 제품군 / D / 비용과 성과 측정. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 날짜만 저장하는 중간 계획
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'Interim Plan 10개' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 날짜만 저장하는 중간 계획의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 비용/작업량 스냅샷 아님 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 날짜 스냅샷과 full baseline을 구분. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-09 / 단계 D; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S60](https://support.microsoft.com/en-us/project/create-or-update-a-baseline-or-an-interim-plan-in-project-desktop)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X008, microsoft:MP-X020, microsoft:MP-X023. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D061 · 기준선 및 일정 편차
- 원본: MP-D061 / Microsoft Project 제품군 / D / 비용과 성과 측정. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 기준선 대비 현재 날짜·원가·작업량 비교
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '기준선 및 일정 편차' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 기준선 대비 현재 날짜·원가·작업량 비교의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 기본 variance는 원래 Baseline 사용 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 기준선 미설정 및 선택 기준선 문제 명시. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-04 / J-07 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S60](https://support.microsoft.com/en-us/project/create-or-update-a-baseline-or-an-interim-plan-in-project-desktop)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X008, microsoft:MP-X017, microsoft:MP-X020, microsoft:MP-X023. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D062 · 실제 시작 종료
- 원본: MP-D062 / Microsoft Project 제품군 / D / 비용과 성과 측정. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 작업 실적 날짜 입력
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '실제 시작 종료' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 작업 실적 날짜 입력의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 요약 실적은 하위 집계 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 이미 시작한 작업의 비현실적 과거 이동 방지. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-09 / 단계 D; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S21](https://support.microsoft.com/en-us/project/update-work-on-a-project), [MS-S10](https://support.microsoft.com/en-gb/project/link-tasks-in-a-project)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X006, microsoft:MP-X008, microsoft:MP-X020, microsoft:MP-X023. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D063 · 실제 잔여 기간
- 원본: MP-D063 / Microsoft Project 제품군 / D / 비용과 성과 측정. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 실기간/잔여기간과 기간 완료율 연동
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '실제 잔여 기간' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 실기간/잔여기간과 기간 완료율 연동의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 기간=실제+잔여 관계 검산. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-09 / 단계 D; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S21](https://support.microsoft.com/en-us/project/update-work-on-a-project)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X008, microsoft:MP-X016, microsoft:MP-X020, microsoft:MP-X023. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D064 · 실제 잔여 작업량
- 원본: MP-D064 / Microsoft Project 제품군 / D / 비용과 성과 측정. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 실작업/잔여작업과 작업 완료율 연동
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '실제 잔여 작업량' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 실작업/잔여작업과 작업 완료율 연동의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: duration %와 work % 구분 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 작업량=실제+잔여 관계 검산. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-09 / 단계 D; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S21](https://support.microsoft.com/en-us/project/update-work-on-a-project)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X008, microsoft:MP-X016, microsoft:MP-X020, microsoft:MP-X023. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D065 · 시간대별 실적 입력
- 원본: MP-D065 / Microsoft Project 제품군 / D / 비용과 성과 측정. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 배정별 일/주/월 실제 작업량 입력
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '시간대별 실적 입력' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 배정별 일/주/월 실제 작업량 입력의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 수동 일정 작업의 시간대별 actual 제한 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 시간대별 합계와 배정·작업 합계 일치. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-05 / J-08 / 단계 C; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S21](https://support.microsoft.com/en-us/project/update-work-on-a-project)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X002, microsoft:MP-X005, microsoft:MP-X006, microsoft:MP-X008, microsoft:MP-X020, microsoft:MP-X023. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D066 · Status Date
- 원본: MP-D066 / Microsoft Project 제품군 / D / 비용과 성과 측정. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 보고 기준일과 실제/잔여 작업 배치 옵션
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'Status Date' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 보고 기준일과 실제/잔여 작업 배치 옵션의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 시간대별 actual 입력에는 다른 규칙 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 기준일 전후 실적·잔여 이동 규칙 시험. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-09 / 단계 D; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S22](https://support.microsoft.com/en-us/project/set-the-status-date-for-project-reporting)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X005, microsoft:MP-X008, microsoft:MP-X020, microsoft:MP-X023. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D067 · EVM PV EV AC
- 원본: MP-D067 / Microsoft Project 제품군 / D / 비용과 성과 측정. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 기준선·기준일·실적을 바탕으로 획득가치 분석
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'EVM PV EV AC' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 기준선·기준일·실적을 바탕으로 획득가치 분석의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 기준선·상태일·비용 필수 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 작은 수치 예제로 PV/EV/AC 검산. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-10 / 단계 D; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S18](https://support.microsoft.com/en-us/project/earned-value-analysis-for-the-rest-of-us)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X006, microsoft:MP-X008, microsoft:MP-X017, microsoft:MP-X020, microsoft:MP-X023. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D068 · EVM 물리적 완료율
- 원본: MP-D068 / Microsoft Project 제품군 / D / 비용과 성과 측정. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: % Complete 또는 직접 입력 Physical % 선택
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'EVM 물리적 완료율' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: % Complete 또는 직접 입력 Physical % 선택의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 작업별 EV method 가능 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 기간 완료율과 물리 완료율을 독립 유지. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-05 / J-08 / 단계 C; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S19](https://support.microsoft.com/en-us/project/change-complete-type-for-earned-value-analysis)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X008, microsoft:MP-X020, microsoft:MP-X023. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D069 · EVM 편차와 지수
- 원본: MP-D069 / Microsoft Project 제품군 / D / 비용과 성과 측정. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: CV SV CPI SPI EAC VAC 등 필드/보고
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'EVM 편차와 지수' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: CV SV CPI SPI EAC VAC 등 필드/보고의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 수식·EAC 전제 전수 검증은 추가 과제 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 지수의 분모 0와 기준일 처리 명시. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-10 / 단계 D; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S18](https://support.microsoft.com/en-us/project/earned-value-analysis-for-the-rest-of-us), [MS-S31](https://support.microsoft.com/en-us/project/available-fields-reference)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X007, microsoft:MP-X008, microsoft:MP-X019, microsoft:MP-X020, microsoft:MP-X023. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D070 · 비교 프로젝트 보고서
- 원본: MP-D070 / Microsoft Project 제품군 / D / 비용과 성과 측정. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 두 파일의 작업·자원 차이 보고
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '비교 프로젝트 보고서' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 두 파일의 작업·자원 차이 보고의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: **배정 비교는 포함하지 않음** · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 추가/삭제/수정 구분과 원본 보존. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-09 / 단계 D; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S61](https://support.microsoft.com/en-us/project/compare-two-versions-of-a-project)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X008, microsoft:MP-X013, microsoft:MP-X020, microsoft:MP-X021, microsoft:MP-X023. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D071 · Gantt 및 Tracking Gantt
- 원본: MP-D071 / Microsoft Project 제품군 / D / 화면 보고서 파일과 확장. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 계층 작업과 기간·연결·기준선 표시
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'Gantt 및 Tracking Gantt' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 계층 작업과 기간·연결·기준선 표시의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 표 편집과 막대가 동일 모델을 반영. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-14 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S01](https://www.microsoft.com/en-us/microsoft-365/project/microsoft-project-enterprise-plans-and-pricing), [MS-S60](https://support.microsoft.com/en-us/project/create-or-update-a-baseline-or-an-interim-plan-in-project-desktop)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X008, microsoft:MP-X013, microsoft:MP-X017, microsoft:MP-X020, microsoft:MP-X021, microsoft:MP-X022. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D072 · Network Diagram
- 원본: MP-D072 / Microsoft Project 제품군 / D / 화면 보고서 파일과 확장. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 작업 네트워크와 연결 표시
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'Network Diagram' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 작업 네트워크와 연결 표시의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: FS/SS/FF/SF와 lag 라벨 확인. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-14 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S01](https://www.microsoft.com/en-us/microsoft-365/project/microsoft-project-enterprise-plans-and-pricing), [MS-S58](https://support.microsoft.com/en-us/project/project-help)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X013, microsoft:MP-X020, microsoft:MP-X021, microsoft:MP-X022. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D073 · Calendar와 Task Sheet
- 원본: MP-D073 / Microsoft Project 제품군 / D / 화면 보고서 파일과 확장. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 달력/표 관점의 일정 검토
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'Calendar와 Task Sheet' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 달력/표 관점의 일정 검토의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 같은 작업이 모든 뷰에서 일치. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-14 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S01](https://www.microsoft.com/en-us/microsoft-365/project/microsoft-project-enterprise-plans-and-pricing)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X005, microsoft:MP-X013, microsoft:MP-X020, microsoft:MP-X021, microsoft:MP-X022. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D074 · Resource Sheet Graph Usage
- 원본: MP-D074 / Microsoft Project 제품군 / D / 화면 보고서 파일과 확장. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 자원·시간대별 수요와 배정 표시
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'Resource Sheet Graph Usage' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 자원·시간대별 수요와 배정 표시의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: task/resource/assignment 집계 검산. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-14 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S31](https://support.microsoft.com/en-us/project/available-fields-reference), [MS-S58](https://support.microsoft.com/en-us/project/project-help)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X005, microsoft:MP-X013, microsoft:MP-X020, microsoft:MP-X021, microsoft:MP-X022. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D075 · Timeline
- 원본: MP-D075 / Microsoft Project 제품군 / D / 화면 보고서 파일과 확장. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 주요 작업/마일스톤 중심 타임라인
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'Timeline' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 주요 작업/마일스톤 중심 타임라인의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 선택 항목의 독립 표현과 일정 연동. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-14 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S04](https://learn.microsoft.com/en-us/office365/servicedescriptions/project-online-service-description/project-online-desktop-client-service-description)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X013, microsoft:MP-X020, microsoft:MP-X021, microsoft:MP-X022. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D076 · 분할 화면과 폼
- 원본: MP-D076 / Microsoft Project 제품군 / D / 화면 보고서 파일과 확장. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 여러 관점으로 선택 작업 상세 확인
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '분할 화면과 폼' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 여러 관점으로 선택 작업 상세 확인의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 일부 시간대별 뷰 조합 제한 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 상하 뷰 선택 연동 및 편집 일관성. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-14 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S27](https://support.microsoft.com/en-us/project/view-your-team-s-work-with-team-planner), [MS-S58](https://support.microsoft.com/en-us/project/project-help)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X005, microsoft:MP-X013, microsoft:MP-X020, microsoft:MP-X021, microsoft:MP-X022. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D077 · 필터 정렬 그룹
- 원본: MP-D077 / Microsoft Project 제품군 / D / 화면 보고서 파일과 확장. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 조건별 필터·정렬·다중 그룹
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '필터 정렬 그룹' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 조건별 필터·정렬·다중 그룹의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 그룹 합계가 원본 데이터와 일치. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-14 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S04](https://learn.microsoft.com/en-us/office365/servicedescriptions/project-online-service-description/project-online-desktop-client-service-description)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X013, microsoft:MP-X020, microsoft:MP-X021, microsoft:MP-X022. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D078 · 사용자 필드 수식 지시자
- 원본: MP-D078 / Microsoft Project 제품군 / D / 화면 보고서 파일과 확장. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 사용자 필드와 계산식·그래픽 상태 표시
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '사용자 필드 수식 지시자' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 사용자 필드와 계산식·그래픽 상태 표시의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 전사 필드는 서버 계층 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 데이터 타입·수식·요약 집계 정책 검증. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-14 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S04](https://learn.microsoft.com/en-us/office365/servicedescriptions/project-online-service-description/project-online-desktop-client-service-description), [MS-S31](https://support.microsoft.com/en-us/project/available-fields-reference)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X007, microsoft:MP-X013, microsoft:MP-X019, microsoft:MP-X020, microsoft:MP-X021, microsoft:MP-X022. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D079 · 내장 보고서
- 원본: MP-D079 / Microsoft Project 제품군 / D / 화면 보고서 파일과 확장. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 프로젝트·자원·비용·EVM·burndown 보고서
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '내장 보고서' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 프로젝트·자원·비용·EVM·burndown 보고서의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 보고서 종류는 원문 목록 참조 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 보고값이 기준 데이터와 일치. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-14 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S24](https://support.microsoft.com/en-us/project/create-a-project-report), [MS-S59](https://support.microsoft.com/en-us/project/pick-the-right-report-in-project)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X008, microsoft:MP-X013, microsoft:MP-X020, microsoft:MP-X021, microsoft:MP-X022, microsoft:MP-X023. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D080 · 맞춤 보고서
- 원본: MP-D080 / Microsoft Project 제품군 / D / 화면 보고서 파일과 확장. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 차트·표·텍스트·이미지와 데이터 필터 조합
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '맞춤 보고서' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 차트·표·텍스트·이미지와 데이터 필터 조합의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 변경 후 보고서 자동 갱신. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-14 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S24](https://support.microsoft.com/en-us/project/create-a-project-report)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X013, microsoft:MP-X020, microsoft:MP-X021, microsoft:MP-X022. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D081 · 보고서 복사와 Organizer
- 원본: MP-D081 / Microsoft Project 제품군 / D / 화면 보고서 파일과 확장. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 보고서 복사, 전역 템플릿으로 재사용
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '보고서 복사와 Organizer' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 보고서 복사, 전역 템플릿으로 재사용의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 보고서/달력/뷰 등 자산 복사 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 템플릿 간 이동 시 참조 필드 검사. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-14 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S24](https://support.microsoft.com/en-us/project/create-a-project-report), [MS-S58](https://support.microsoft.com/en-us/project/project-help)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X005, microsoft:MP-X013, microsoft:MP-X020, microsoft:MP-X021, microsoft:MP-X022. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D082 · 인쇄 및 PDF XPS
- 원본: MP-D082 / Microsoft Project 제품군 / D / 화면 보고서 파일과 확장. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 페이지 출력과 고정 레이아웃 내보내기
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '인쇄 및 PDF XPS' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 페이지 출력과 고정 레이아웃 내보내기의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: PDF 재가져오기는 지원 아님 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: Gantt 가로 분할·범례·날짜 범위 QA. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-14 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S23](https://support.microsoft.com/en-us/project/file-formats-supported-by-project-desktop), [MS-S58](https://support.microsoft.com/en-us/project/project-help)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X013, microsoft:MP-X020, microsoft:MP-X021, microsoft:MP-X022. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D083 · MPP 및 MPT
- 원본: MP-D083 / Microsoft Project 제품군 / D / 화면 보고서 파일과 확장. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 네이티브 계획과 템플릿 저장/열기
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'MPP 및 MPT' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 네이티브 계획과 템플릿 저장/열기의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: JOBS가 MPP 동일성을 이미 가진다는 뜻 아님 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 왕복 변환 후 일정·배정·기준선 손실 보고. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-14 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S23](https://support.microsoft.com/en-us/project/file-formats-supported-by-project-desktop)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X013, microsoft:MP-X020, microsoft:MP-X021, microsoft:MP-X022. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D084 · XML 가져오기 내보내기
- 원본: MP-D084 / Microsoft Project 제품군 / D / 화면 보고서 파일과 확장. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 구조화 프로젝트 데이터 교환
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'XML 가져오기 내보내기' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 구조화 프로젝트 데이터 교환의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 스키마/버전별 필드 전수 확인 필요 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: ID·달력·연결·자원·실적 왕복 비교. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-15 / 단계 F; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S23](https://support.microsoft.com/en-us/project/file-formats-supported-by-project-desktop)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X007, microsoft:MP-X013, microsoft:MP-X019, microsoft:MP-X020, microsoft:MP-X021, microsoft:MP-X022. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D085 · Excel 및 CSV TXT
- 원본: MP-D085 / Microsoft Project 제품군 / D / 화면 보고서 파일과 확장. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: Excel 가져오기 및 표 필드 중심 내보내기
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'Excel 및 CSV TXT' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: Excel 가져오기 및 표 필드 중심 내보내기의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: Excel/CSV는 전체 프로젝트 보존 포맷 아님 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 매핑·타입·날짜·단위·문자 인코딩 시험. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-14 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S04](https://learn.microsoft.com/en-us/office365/servicedescriptions/project-online-service-description/project-online-desktop-client-service-description), [MS-S23](https://support.microsoft.com/en-us/project/file-formats-supported-by-project-desktop)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X007, microsoft:MP-X013, microsoft:MP-X019, microsoft:MP-X020, microsoft:MP-X021, microsoft:MP-X022. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D086 · Master와 Subproject
- 원본: MP-D086 / Microsoft Project 제품군 / D / 화면 보고서 파일과 확장. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 연결된 하위 프로젝트 또는 독립 복사 삽입
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'Master와 Subproject' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 연결된 하위 프로젝트 또는 독립 복사 삽입의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 중복 자원은 자동 통합되지 않음 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 읽기전용/양방향 연결/독립 복사를 구분. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-14 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S25](https://support.microsoft.com/en-us/project/link-projects-to-create-a-master-project-in-project-desktop)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X013, microsoft:MP-X016, microsoft:MP-X020, microsoft:MP-X021, microsoft:MP-X022. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D087 · 프로젝트 간 연결
- 원본: MP-D087 / Microsoft Project 제품군 / D / 화면 보고서 파일과 확장. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 외부 작업 의존성과 통합 일정
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '프로젝트 간 연결' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 외부 작업 의존성과 통합 일정의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 전체 임계경로와 resource pool 결합 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 파일 이동·삭제·읽기권한 실패 처리. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-14 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S10](https://support.microsoft.com/en-gb/project/link-tasks-in-a-project), [MS-S14](https://support.microsoft.com/en-us/project/manage-your-project-s-critical-path)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X008, microsoft:MP-X013, microsoft:MP-X016, microsoft:MP-X020, microsoft:MP-X021, microsoft:MP-X022. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D088 · Undo와 입력 편의
- 원본: MP-D088 / Microsoft Project 제품군 / D / 화면 보고서 파일과 확장. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 다단계 취소, 자동완성, 열/서식 조정
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'Undo와 입력 편의' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 다단계 취소, 자동완성, 열/서식 조정의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 대량 편집 취소 후 모델 동일성 검사. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-14 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S04](https://learn.microsoft.com/en-us/office365/servicedescriptions/project-online-service-description/project-online-desktop-client-service-description)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X013, microsoft:MP-X020, microsoft:MP-X021, microsoft:MP-X022. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D089 · Task Board와 Sprint
- 원본: MP-D089 / Microsoft Project 제품군 / Sub / 화면 보고서 파일과 확장. 근거 상태: partial; 기능 확인과 함께 에디션·조건·세부 동작 재검증 필요.
- 동작: 보드 상태·백로그·스프린트와 Gantt 병행
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'Task Board와 Sprint' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 보드 상태·백로그·스프린트와 Gantt 병행의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 영구 Standard/Pro 2024는 별도 확인 · 조건. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 보드 상태 변경이 일정 상세를 삭제하지 않음. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-14 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S29](https://support.microsoft.com/en-us/project/use-task-boards-in-microsoft-project-online-desktop-client)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X013, microsoft:MP-X020, microsoft:MP-X021, microsoft:MP-X022. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D090 · 애자일 보고서와 카드
- 원본: MP-D090 / Microsoft Project 제품군 / Sub / 화면 보고서 파일과 확장. 근거 상태: partial; 기능 확인과 함께 에디션·조건·세부 동작 재검증 필요.
- 동작: 스프린트/작업 보고, 카드 추가 필드
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '애자일 보고서와 카드' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 스프린트/작업 보고, 카드 추가 필드의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 카드 custom field 최대 5개 문서 · 조건. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: sprint별 작업·작업량 합계 일치. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-14 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S29](https://support.microsoft.com/en-us/project/use-task-boards-in-microsoft-project-online-desktop-client)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X007, microsoft:MP-X013, microsoft:MP-X019, microsoft:MP-X020, microsoft:MP-X021, microsoft:MP-X022. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D091 · VBA 객체 모델
- 원본: MP-D091 / Microsoft Project 제품군 / D / 화면 보고서 파일과 확장. 근거 상태: partial; 기능 확인과 함께 에디션·조건·세부 동작 재검증 필요.
- 동작: 프로젝트·작업·자원·달력 객체 자동화
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'VBA 객체 모델' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 프로젝트·작업·자원·달력 객체 자동화의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 바이너리/매크로 호환은 별도 제품 과제 · 조건. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 가져온 매크로의 지원/미지원 경계를 보고. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-14 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S54](https://learn.microsoft.com/en-us/office/vba/project/concepts/project-object-model)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X005, microsoft:MP-X013, microsoft:MP-X020, microsoft:MP-X021, microsoft:MP-X022. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-D092 · Office task pane add in
- 원본: MP-D092 / Microsoft Project 제품군 / D / 화면 보고서 파일과 확장. 근거 상태: partial; 기능 확인과 함께 에디션·조건·세부 동작 재검증 필요.
- 동작: HTML/JS 기반 Windows Project 확장
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'Office task pane add in' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: HTML/JS 기반 Windows Project 확장의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: Office JS Project 지원은 제한적 · 조건. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 호스트 이벤트와 외부 연동 권한 시험. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-14 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S55](https://learn.microsoft.com/office/dev/add-ins/project/create-a-project-add-in-that-uses-rest-with-an-on-premises-odata-service)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X013, microsoft:MP-X020, microsoft:MP-X021, microsoft:MP-X022. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-E001 · Project Center와 전사 프로젝트
- 원본: MP-E001 / Microsoft Project 제품군 / SE/PWA-L / 전사 PWA 협업과 운영. 근거 상태: partial; 기능 확인과 함께 에디션·조건·세부 동작 재검증 필요.
- 동작: 프로젝트·배정·실적을 중앙에서 관리
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'Project Center와 전사 프로젝트' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 프로젝트·배정·실적을 중앙에서 관리의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 웹 편집 상세 동등성 재현 필요 · 조건. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 권한에 따라 전사/담당 프로젝트 목록 분리. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-13 / 단계 C; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S65](https://learn.microsoft.com/en-us/project/determine-project-management-requirements-in-project-server-2016), [MS-S58](https://support.microsoft.com/en-us/project/project-help)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X006, microsoft:MP-X014, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-E002 · Desktop 연결과 게시
- 원본: MP-E002 / Microsoft Project 제품군 / Pro+SE / 전사 PWA 협업과 운영. 근거 상태: partial; 기능 확인과 함께 에디션·조건·세부 동작 재검증 필요.
- 동작: Desktop 계획을 서버에 게시·갱신
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'Desktop 연결과 게시' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: Desktop 계획을 서버에 게시·갱신의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: Standard 단독 기능 아님 · 조건. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 개인 초안과 게시본의 가시성을 분리. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-13 / 단계 C; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S01](https://www.microsoft.com/en-us/microsoft-365/project/microsoft-project-enterprise-plans-and-pricing), [MS-S65](https://learn.microsoft.com/en-us/project/determine-project-management-requirements-in-project-server-2016)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X014, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-E003 · Check out in 및 초안 충돌
- 원본: MP-E003 / Microsoft Project 제품군 / PWA-L / 전사 PWA 협업과 운영. 근거 상태: partial; 기능 확인과 함께 에디션·조건·세부 동작 재검증 필요.
- 동작: 편집 초안과 게시 상태, 충돌 해결 흐름
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'Check out in 및 초안 충돌' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 편집 초안과 게시 상태, 충돌 해결 흐름의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 2013 아키텍처 동작; SE 세부 재검증. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 동시 편집과 게시 충돌에서 변경 손실 방지. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-13 / 단계 C; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S57](https://learn.microsoft.com/en-us/office/client-developer/project/project-server-2013-architecture)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X014, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-E004 · 전사 자원 풀
- 원본: MP-E004 / Microsoft Project 제품군 / SE / 전사 PWA 협업과 운영. 근거 상태: partial; 기능 확인과 함께 에디션·조건·세부 동작 재검증 필요.
- 동작: 공통 자원과 전사 배정을 사용
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '전사 자원 풀' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 공통 자원과 전사 배정을 사용의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 자원 식별자·역할·조직 모델 필요 · 조건. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 같은 사람이 프로젝트별 복제되지 않음. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-13 / 단계 C; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S65](https://learn.microsoft.com/en-us/project/determine-project-management-requirements-in-project-server-2016)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X014, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-E005 · 전사 팀 구성
- 원본: MP-E005 / Microsoft Project 제품군 / Pro+SE/PWA-L / 전사 PWA 협업과 운영. 근거 상태: partial; 기능 확인과 함께 에디션·조건·세부 동작 재검증 필요.
- 동작: enterprise resources를 프로젝트 팀으로 추가
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '전사 팀 구성' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: enterprise resources를 프로젝트 팀으로 추가의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 연결·권한 필요 · 조건. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 가용 역할 검색 후 팀/배정 구분. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-13 / 단계 C; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S64](https://support.microsoft.com/en-us/project/add-resources-to-your-project)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X014, microsoft:MP-X018, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-E006 · Resource Engagement 요청
- 원본: MP-E006 / Microsoft Project 제품군 / Pro+PWA-L / 전사 PWA 협업과 운영. 근거 상태: partial; 기능 확인과 함께 에디션·조건·세부 동작 재검증 필요.
- 동작: 기간·시간 또는 비율로 자원 협약 요청
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'Resource Engagement 요청' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 기간·시간 또는 비율로 자원 협약 요청의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: Standard 제외; SE 버전 문구 노후 · 재검증. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 요청/변경/재요청 상태 전이를 기록. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-13 / 단계 C; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S35](https://support.microsoft.com/en-us/project/overview-resource-engagements)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X012, microsoft:MP-X014, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-E007 · 자원 협약 승인 반려 변경
- 원본: MP-E007 / Microsoft Project 제품군 / PWA-L / 전사 PWA 협업과 운영. 근거 상태: partial; 기능 확인과 함께 에디션·조건·세부 동작 재검증 필요.
- 동작: 관리자가 수요·용량을 검토해 승인/수정
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '자원 협약 승인 반려 변경' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 관리자가 수요·용량을 검토해 승인/수정의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 승인자 권한 필요 · 조건. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 대체 자원·시간·기간 변경 이력 보존. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-13 / 단계 C; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S35](https://support.microsoft.com/en-us/project/overview-resource-engagements)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X013, microsoft:MP-X014, microsoft:MP-X018, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-E008 · 협약 초과 경고
- 원본: MP-E008 / Microsoft Project 제품군 / Pro+PWA-L / 전사 PWA 협업과 운영. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 협약 범위를 벗어난 배정에 지시자 표시
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '협약 초과 경고' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 협약 범위를 벗어난 배정에 지시자 표시의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: **승인 없는 배정을 강제 차단하지 않음** · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 협약 내/외 배정을 구분 경고. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-13 / 단계 C; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S35](https://support.microsoft.com/en-us/project/overview-resource-engagements)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X013, microsoft:MP-X014, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-E009 · 전사 용량 계획
- 원본: MP-E009 / Microsoft Project 제품군 / SE / 전사 PWA 협업과 운영. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 자원 수요와 가용 용량·과부족 분석
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '전사 용량 계획' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 자원 수요와 가용 용량·과부족 분석의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 자원 달력·전사 배정 필요 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 프로젝트·역할·기간별 수요 용량 비교. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-13 / 단계 C; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S32](https://www.microsoft.com/en-us/microsoft-365/project/enterprise-project-server), [MS-S34](https://learn.microsoft.com/en-us/project/analyzing-portfolios)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X005, microsoft:MP-X014, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-E010 · Timesheet
- 원본: MP-E010 / Microsoft Project 제품군 / SE / 전사 PWA 협업과 운영. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 직원/계약자의 기간별 작업 시간 제출
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'Timesheet' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 직원/계약자의 기간별 작업 시간 제출의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 급여·청구 시스템 자체를 뜻하지 않음 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 정규/초과·프로젝트/비프로젝트 시간을 분리. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-13 / 단계 C; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S65](https://learn.microsoft.com/en-us/project/determine-project-management-requirements-in-project-server-2016)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X014, microsoft:MP-X020, microsoft:MP-X022. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-E011 · 시간 분류와 회계 기간
- 원본: MP-E011 / Microsoft Project 제품군 / SE / 전사 PWA 협업과 운영. 근거 상태: partial; 기능 확인과 함께 에디션·조건·세부 동작 재검증 필요.
- 동작: 시간/회계 기간·달력·감사 고려를 포함
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '시간 분류와 회계 기간' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 시간/회계 기간·달력·감사 고려를 포함의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 세부 관리자 옵션 재현 필요 · 조건. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 회계월·보고주기·분류별 합계 검산. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-13 / 단계 C; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S65](https://learn.microsoft.com/en-us/project/determine-project-management-requirements-in-project-server-2016)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X005, microsoft:MP-X014, microsoft:MP-X020, microsoft:MP-X023. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-E012 · 시간과 진도 Single Entry
- 원본: MP-E012 / Microsoft Project 제품군 / PWA-L / 전사 PWA 협업과 운영. 근거 상태: partial; 기능 확인과 함께 에디션·조건·세부 동작 재검증 필요.
- 동작: 시간표와 작업 진도 또는 단일 입력 모드
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '시간과 진도 Single Entry' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 시간표와 작업 진도 또는 단일 입력 모드의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 일정 반영 승인 옵션 존재 · 재검증. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 한 번 입력된 실적의 중복 집계 방지. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-13 / 단계 C; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S43](https://support.microsoft.com/en-us/project/use-team-resources-in-project-online)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X013, microsoft:MP-X014, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-E013 · 시간표 제출 회수와 위임
- 원본: MP-E013 / Microsoft Project 제품군 / PWA-L / 전사 PWA 협업과 운영. 근거 상태: partial; 기능 확인과 함께 에디션·조건·세부 동작 재검증 필요.
- 동작: 제출·회수 및 대리 입력 흐름
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '시간표 제출 회수와 위임' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 제출·회수 및 대리 입력 흐름의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 현재 SE 세부 승인지원 확인 필요 · 재검증. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 제출 이후 수정 권한과 위임자 이력 검증. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-13 / 단계 C; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S58](https://support.microsoft.com/en-us/project/project-help)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X013, microsoft:MP-X014, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-E014 · 시간표 보존과 삭제
- 원본: MP-E014 / Microsoft Project 제품군 / SE / 전사 PWA 협업과 운영. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 기간별 시간표 영구 삭제 관리
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '시간표 보존과 삭제' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 기간별 시간표 영구 삭제 관리의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 원문은 삭제 후 복구 불가 명시 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 삭제 권한·보존 정책·되돌리기 경계를 명시. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-13 / 단계 C; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S66](https://learn.microsoft.com/en-us/project/delete-timesheets-in-project-server)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X014, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-E015 · 위험 이슈 문서 작업 연결
- 원본: MP-E015 / Microsoft Project 제품군 / SE/PWA-L / 전사 PWA 협업과 운영. 근거 상태: partial; 기능 확인과 함께 에디션·조건·세부 동작 재검증 필요.
- 동작: 프로젝트 사이트의 위험·이슈·문서·산출물 연결
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '위험 이슈 문서 작업 연결' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 프로젝트 사이트의 위험·이슈·문서·산출물 연결의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: SharePoint 의존; 정량 Monte Carlo 아님 · 조건. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 위험 항목의 담당·상태와 작업 링크 추적. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-13 / 단계 C; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S65](https://learn.microsoft.com/en-us/project/determine-project-management-requirements-in-project-server-2016), [MS-S58](https://support.microsoft.com/en-us/project/project-help)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X014, microsoft:MP-X016, microsoft:MP-X020, microsoft:MP-X023. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-E016 · 알림과 리마인더
- 원본: MP-E016 / Microsoft Project 제품군 / PWA-L / 전사 PWA 협업과 운영. 근거 상태: partial; 기능 확인과 함께 에디션·조건·세부 동작 재검증 필요.
- 동작: 이메일 알림 및 리마인더 운영
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '알림과 리마인더' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 이메일 알림 및 리마인더 운영의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 2013 운영 문서; SE 전달 경로 재검증. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 일정/승인/지연 알림의 수신 범위 시험. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-13 / 단계 C; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S44](https://learn.microsoft.com/en-us/project/operational-policies-in-project-server-2013)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X014, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-E017 · 수요 접수와 프로젝트 제안
- 원본: MP-E017 / Microsoft Project 제품군 / SE/PWA-L / 전사 PWA 협업과 운영. 근거 상태: partial; 기능 확인과 함께 에디션·조건·세부 동작 재검증 필요.
- 동작: 제안을 모아 승인 과정으로 진행
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '수요 접수와 프로젝트 제안' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 제안을 모아 승인 과정으로 진행의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: SE 개요+구버전 상세; 엔진 재검증. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 접수부터 승인·반려·완료 추적. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-13 / 단계 C; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S32](https://www.microsoft.com/en-us/microsoft-365/project/enterprise-project-server), [MS-S42](https://learn.microsoft.com/en-us/project/using-workflow-for-demand-management-in-project-server)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X013, microsoft:MP-X014, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-E018 · Project Detail Pages
- 원본: MP-E018 / Microsoft Project 제품군 / PWA-L / 전사 PWA 협업과 운영. 근거 상태: partial; 기능 확인과 함께 에디션·조건·세부 동작 재검증 필요.
- 동작: 단계별 프로젝트 데이터 입력 화면
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'Project Detail Pages' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 단계별 프로젝트 데이터 입력 화면의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: SharePoint 웹파트·필드 필요 · 재검증. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 단계마다 필수/읽기전용 필드 검증. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-13 / 단계 C; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S42](https://learn.microsoft.com/en-us/project/using-workflow-for-demand-management-in-project-server)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X007, microsoft:MP-X014, microsoft:MP-X016, microsoft:MP-X019, microsoft:MP-X020, microsoft:MP-X022. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-E019 · 단계와 Phase
- 원본: MP-E019 / Microsoft Project 제품군 / PWA-L / 전사 PWA 협업과 운영. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: stage를 phase로 조직하고 workflow로 순서 정의
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '단계와 Phase' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: stage를 phase로 조직하고 workflow로 순서 정의의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: phase 분류와 실행순서를 구분 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 필수 자료 없이 다음 단계 이동 방지. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-13 / 단계 C; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S42](https://learn.microsoft.com/en-us/project/using-workflow-for-demand-management-in-project-server)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X011, microsoft:MP-X014, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-E020 · Enterprise Project Types
- 원본: MP-E020 / Microsoft Project 제품군 / PWA-L / 전사 PWA 협업과 운영. 근거 상태: partial; 기능 확인과 함께 에디션·조건·세부 동작 재검증 필요.
- 동작: workflow·단계·부서·사이트 템플릿 결합
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'Enterprise Project Types' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: workflow·단계·부서·사이트 템플릿 결합의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 현재 SE workflow 런타임 재검증. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 유형별 서로 다른 승인 흐름 적용. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-13 / 단계 C; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S42](https://learn.microsoft.com/en-us/project/using-workflow-for-demand-management-in-project-server)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X011, microsoft:MP-X014, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-E021 · 전사 사용자 필드
- 원본: MP-E021 / Microsoft Project 제품군 / SE / 전사 PWA 협업과 운영. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 프로젝트·작업·자원별 타입과 필수값
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '전사 사용자 필드' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 프로젝트·작업·자원별 타입과 필수값의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 로컬 필드와 별도 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 조직 전체에서 같은 의미/타입 사용. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-13 / 단계 C; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S39](https://learn.microsoft.com/en-us/project/add-or-edit-enterprise-custom-fields-in-project-server)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X007, microsoft:MP-X014, microsoft:MP-X019, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-E022 · 수식 롤업과 지시자
- 원본: MP-E022 / Microsoft Project 제품군 / SE / 전사 PWA 협업과 운영. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 요약 계산·배정 전파·그래픽 지시자
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '수식 롤업과 지시자' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 요약 계산·배정 전파·그래픽 지시자의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 복잡한 수식의 성능 영향 별도 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 계층 집계와 개별 예외값 검산. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-13 / 단계 C; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S39](https://learn.microsoft.com/en-us/project/add-or-edit-enterprise-custom-fields-in-project-server)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X007, microsoft:MP-X014, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-E023 · 계층 Lookup Table
- 원본: MP-E023 / Microsoft Project 제품군 / SE / 전사 PWA 협업과 운영. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 코드 마스크·계층·순서와 선택값 관리
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '계층 Lookup Table' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 코드 마스크·계층·순서와 선택값 관리의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 저장 후 변경 제약 존재 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 표준 용어·코드와 다중선택 검증. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-13 / 단계 C; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S39](https://learn.microsoft.com/en-us/project/add-or-edit-enterprise-custom-fields-in-project-server), [MS-S40](https://learn.microsoft.com/en-us/project/add-or-edit-enterprise-custom-lookup-tables-in-project-server)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X014, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-E024 · 부서별 필드와 Workflow 통제
- 원본: MP-E024 / Microsoft Project 제품군 / SE / 전사 PWA 협업과 운영. 근거 상태: partial; 기능 확인과 함께 에디션·조건·세부 동작 재검증 필요.
- 동작: 부서 연결, 필수값 또는 workflow 제어
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '부서별 필드와 Workflow 통제' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 부서 연결, 필수값 또는 workflow 제어의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: field visibility를 보안 전체로 오인 금지 · 조건. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 부서/단계별 노출과 편집 규칙 검사. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-13 / 단계 C; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S39](https://learn.microsoft.com/en-us/project/add-or-edit-enterprise-custom-fields-in-project-server)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X007, microsoft:MP-X011, microsoft:MP-X014, microsoft:MP-X019, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-E025 · 두 가지 권한 모드
- 원본: MP-E025 / Microsoft Project 제품군 / SE / 전사 PWA 협업과 운영. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: SharePoint 또는 Project permission mode
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '두 가지 권한 모드' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: SharePoint 또는 Project permission mode의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 모드 전환은 보안 설정 삭제 유발 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 기능·객체 접근 범위를 모드별 명시. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-16 / 단계 C; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S36](https://learn.microsoft.com/en-us/project/plan-user-access-in-project-server)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X014, microsoft:MP-X016, microsoft:MP-X018, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-E026 · 역할별 그룹과 권한
- 원본: MP-E026 / Microsoft Project 제품군 / SE / 전사 PWA 협업과 운영. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 그룹·전역/범주 권한·권한 템플릿
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '역할별 그룹과 권한' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 그룹·전역/범주 권한·권한 템플릿의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: Project permission mode · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 허용/거부 우선순위와 최소권한 시험. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-16 / 단계 C; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S37](https://learn.microsoft.com/en-us/project/create-security-groups-in-project-server)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X014, microsoft:MP-X018, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-E027 · 보안 범주와 RBS
- 원본: MP-E027 / Microsoft Project 제품군 / SE / 전사 PWA 협업과 운영. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 프로젝트·자원·뷰 집합 및 RBS 기반 필터
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '보안 범주와 RBS' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 프로젝트·자원·뷰 집합 및 RBS 기반 필터의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: Project permission mode · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 조직 계층별 가시성과 관리범위 검증. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-16 / 단계 C; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S38](https://learn.microsoft.com/en-us/project/manage-categories-in-project-server)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X014, microsoft:MP-X020, microsoft:MP-X022. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-E028 · Active Directory 동기화
- 원본: MP-E028 / Microsoft Project 제품군 / SE / 전사 PWA 협업과 운영. 근거 상태: partial; 기능 확인과 함께 에디션·조건·세부 동작 재검증 필요.
- 동작: 보안 그룹을 AD 그룹과 동기화
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'Active Directory 동기화' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 보안 그룹을 AD 그룹과 동기화의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 수동 변경은 다음 동기화로 덮일 수 있음 · 조건. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 퇴사·부서이동 시 권한 변화를 검증. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-13 / 단계 C; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S37](https://learn.microsoft.com/en-us/project/create-security-groups-in-project-server)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X002, microsoft:MP-X014, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-E029 · 대리 권한
- 원본: MP-E029 / Microsoft Project 제품군 / SE / 전사 PWA 협업과 운영. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: Project permission mode에서 사용자 delegation
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '대리 권한' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: Project permission mode에서 사용자 delegation의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: SharePoint mode와 차이 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 기간·대리자·행위자 감사 구분. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-16 / 단계 C; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S36](https://learn.microsoft.com/en-us/project/plan-user-access-in-project-server)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X014, microsoft:MP-X016, microsoft:MP-X018, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-E030 · Enterprise Global 표준화
- 원본: MP-E030 / Microsoft Project 제품군 / SE / 전사 PWA 협업과 운영. 근거 상태: partial; 기능 확인과 함께 에디션·조건·세부 동작 재검증 필요.
- 동작: 전사 global template로 공통 설정 사용
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'Enterprise Global 표준화' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 전사 global template로 공통 설정 사용의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 세부 포함 자산 목록 추가 확인 · 조건. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 공통 뷰/필드 변경의 배포 영향 시험. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-13 / 단계 C; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S65](https://learn.microsoft.com/en-us/project/determine-project-management-requirements-in-project-server-2016)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X014, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-E031 · 관리 작업과 개인정보 정리
- 원본: MP-E031 / Microsoft Project 제품군 / SE / 전사 PWA 협업과 운영. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 개인 데이터 export/redact 및 캐시·보관본 처리
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '관리 작업과 개인정보 정리' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 개인 데이터 export/redact 및 캐시·보관본 처리의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: SharePoint 데이터 정리는 별도 절차 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 삭제 후 연동 캐시에서 정보 부활 방지. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-13 / 단계 C; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S67](https://learn.microsoft.com/en-us/project/delete-user-data-project-server-subscription-edition)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X014, microsoft:MP-X016, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-E032 · 보고 데이터 연동
- 원본: MP-E032 / Microsoft Project 제품군 / SE / 전사 PWA 협업과 운영. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: OData 보고 서비스 제거; pjrep 보고 스키마 대안
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '보고 데이터 연동' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: OData 보고 서비스 제거; pjrep 보고 스키마 대안의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: SQL snapshots/SSIS; 비보고 스키마 접근 지양 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 제품별 지원 연동 경로를 선택·검증. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-13 / 단계 C; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S41](https://learn.microsoft.com/en-us/project/deprecated-removed-features-project-server-subscription-edition)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X014, microsoft:MP-X015, microsoft:MP-X016, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-E033 · CSOM PSI와 이벤트 확장
- 원본: MP-E033 / Microsoft Project 제품군 / PWA-L / 전사 PWA 협업과 운영. 근거 상태: partial; 기능 확인과 함께 에디션·조건·세부 동작 재검증 필요.
- 동작: 프로젝트·작업·자원 등 API 및 이벤트 확장
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'CSOM PSI와 이벤트 확장' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 프로젝트·작업·자원 등 API 및 이벤트 확장의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: CSOM은 PSI 전체 대체 아님; SE 세부 재검증. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: API별 지원 엔터티와 비동기 완료 검증. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-13 / 단계 C; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S56](https://learn.microsoft.com/en-us/office/client-developer/project/what-the-csom-does-and-does-not-do), [MS-S57](https://learn.microsoft.com/en-us/office/client-developer/project/project-server-2013-architecture)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X010, microsoft:MP-X014, microsoft:MP-X015, microsoft:MP-X019, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-E034 · 전략 Business Driver
- 원본: MP-E034 / Microsoft Project 제품군 / SE / 전사 포트폴리오와 프로그램. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 조직 목표 정의·우선순위와 프로젝트 영향 평가
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '전략 Business Driver' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 조직 목표 정의·우선순위와 프로젝트 영향 평가의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 전략 값은 입력·평가 근거 필요 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 같은 프로젝트를 목표 가중치별 비교. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-12 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S33](https://learn.microsoft.com/en-us/project/portfolio-analysis-overview)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-E035 · 프로젝트 우선순위
- 원본: MP-E035 / Microsoft Project 제품군 / SE / 전사 포트폴리오와 프로그램. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: business driver 또는 custom field로 순위 산정
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '프로젝트 우선순위' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: business driver 또는 custom field로 순위 산정의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 정확한 알고리즘 동등성 추가 시험 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 입력 변화가 순위에 미치는 영향 설명. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-12 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S34](https://learn.microsoft.com/en-us/project/analyzing-portfolios)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-E036 · 예산 제약 포트폴리오 선택
- 원본: MP-E036 / Microsoft Project 제품군 / SE / 전사 포트폴리오와 프로그램. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 비용 한도 안에서 전략 가치와 선정 집합 분석
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '예산 제약 포트폴리오 선택' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 비용 한도 안에서 전략 가치와 선정 집합 분석의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 비용·우선순위·후보 프로젝트 필요 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 예산 감소/증가에 따른 선택안 비교. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-03 / J-03 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S34](https://learn.microsoft.com/en-us/project/analyzing-portfolios)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X020, microsoft:MP-X023. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-E037 · 강제 포함 제외
- 원본: MP-E037 / Microsoft Project 제품군 / SE / 전사 포트폴리오와 프로그램. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 규제상 필수 과제 등을 force in/out
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '강제 포함 제외' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 규제상 필수 과제 등을 force in/out의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 이유 코드 사용자화 가능 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 강제 포함 시 다른 과제와 예산 충돌 표시. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-12 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S34](https://learn.microsoft.com/en-us/project/analyzing-portfolios)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-E038 · 시나리오 저장 비교 확정
- 원본: MP-E038 / Microsoft Project 제품군 / SE / 전사 포트폴리오와 프로그램. 근거 상태: partial; 기능 확인과 함께 에디션·조건·세부 동작 재검증 필요.
- 동작: 비용/자원 시나리오 저장·비교·commit
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '시나리오 저장 비교 확정' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 비용/자원 시나리오 저장·비교·commit의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: commit의 후속 workflow 효과 재검증 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: what-if와 승인 결과를 구분 보존. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-12 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S34](https://learn.microsoft.com/en-us/project/analyzing-portfolios)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X011, microsoft:MP-X020, microsoft:MP-X023. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-E039 · 기간별 자원 제약 분석
- 원본: MP-E039 / Microsoft Project 제품군 / SE / 전사 포트폴리오와 프로그램. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 역할·기간·부서/RBS로 수요와 용량 분석
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '기간별 자원 제약 분석' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 역할·기간·부서/RBS로 수요와 용량 분석의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 자원 배정과 계획기간/단위 필요 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 같은 예산안의 역할별 부족량 제시. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-03 / J-03 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S34](https://learn.microsoft.com/en-us/project/analyzing-portfolios)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-E040 · 채용 시나리오
- 원본: MP-E040 / Microsoft Project 제품군 / SE / 전사 포트폴리오와 프로그램. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 자원 추가에 따른 포트폴리오 변화/필요 역할 보고
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '채용 시나리오' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 자원 추가에 따른 포트폴리오 변화/필요 역할 보고의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 분석은 실배정 자체를 변경하지 않음 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 추가 인원수 대비 선정안 비교. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-12 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S34](https://learn.microsoft.com/en-us/project/analyzing-portfolios)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-E041 · 시작 시기 시나리오
- 원본: MP-E041 / Microsoft Project 제품군 / SE / 전사 포트폴리오와 프로그램. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 프로젝트 시점 이동의 자원 영향 실험
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '시작 시기 시나리오' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 프로젝트 시점 이동의 자원 영향 실험의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 분석 시작일은 실제 시작일과 별개 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 시나리오 변경이 실행 계획을 덮지 않음. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-12 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S34](https://learn.microsoft.com/en-us/project/analyzing-portfolios)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-E042 · Frontier와 산점도
- 원본: MP-E042 / Microsoft Project 제품군 / SE / 전사 포트폴리오와 프로그램. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 비용-가치, 효율경계, 자원 과부족 보고
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'Frontier와 산점도' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 비용-가치, 효율경계, 자원 과부족 보고의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 시나리오 모델 필요 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 선정안과 차트 지표가 일치. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-12 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S34](https://learn.microsoft.com/en-us/project/analyzing-portfolios)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X020, microsoft:MP-X023. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-E043 · 프로그램 관리
- 원본: MP-E043 / Microsoft Project 제품군 / SE / 전사 포트폴리오와 프로그램. 근거 상태: partial; 기능 확인과 함께 에디션·조건·세부 동작 재검증 필요.
- 동작: 관련 프로젝트를 통합 목적·효익 관점에서 관리
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '프로그램 관리' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 관련 프로젝트를 통합 목적·효익 관점에서 관리의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 정량 benefit realization의 상세 엔터티 미확인 · 조건. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 프로그램과 프로젝트 지표 연결. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-12 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S32](https://www.microsoft.com/en-us/microsoft-365/project/enterprise-project-server)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X014, microsoft:MP-X016, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-P001 · 개인 작업 모음과 협업 작업
- 원본: MP-P001 / Microsoft Project 제품군 / PB/P1/P3 / Planner 기본과 프리미엄 비교층. 근거 상태: partial; 기능 확인과 함께 에디션·조건·세부 동작 재검증 필요.
- 동작: My Day/My Tasks/Assigned to me와 파일·체크리스트·라벨
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '개인 작업 모음과 협업 작업' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: My Day/My Tasks/Assigned to me와 파일·체크리스트·라벨의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 세부 premium 편집권 별도 · 조건. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 개인 할일과 팀 프로젝트의 같은 작업을 연결. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-13 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S47](https://www.microsoft.com/en-us/microsoft-365/planner/microsoft-planner-business-plans-and-pricing)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X020, microsoft:MP-X021. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-P002 · Grid Board Charts
- 원본: MP-P002 / Microsoft Project 제품군 / PB/P1/P3 / Planner 기본과 프리미엄 비교층. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 표·카드·상태 차트 및 필터/그룹
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'Grid Board Charts' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 표·카드·상태 차트 및 필터/그룹의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 뷰별 수량과 작업 상태 일치. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-13 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S45](https://support.microsoft.com/en-us/planner/compare-microsoft-planner-basic-vs-premium-plans)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-P003 · 실시간 공동작업과 그룹
- 원본: MP-P003 / Microsoft Project 제품군 / PB/P1/P3 / Planner 기본과 프리미엄 비교층. 근거 상태: partial; 기능 확인과 함께 에디션·조건·세부 동작 재검증 필요.
- 동작: 웹/Teams 협업과 M365 그룹 접근
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '실시간 공동작업과 그룹' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 웹/Teams 협업과 M365 그룹 접근의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 게스트/보안 세부 정책 미검증 · 조건. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 동시 변경·멤버 삭제 영향 시험. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-13 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S47](https://www.microsoft.com/en-us/microsoft-365/planner/microsoft-planner-business-plans-and-pricing), [MS-S52](https://learn.microsoft.com/en-us/microsoftteams/manage-tasks-app)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X014, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-P004 · 일정 템플릿
- 원본: MP-P004 / Microsoft Project 제품군 / PB/P1/P3 / Planner 기본과 프리미엄 비교층. 근거 상태: partial; 기능 확인과 함께 에디션·조건·세부 동작 재검증 필요.
- 동작: basic/premium 및 custom plan template 제공
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '일정 템플릿' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: basic/premium 및 custom plan template 제공의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 템플릿 권한/배포 범위 미검증 · 조건. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 템플릿과 개별 계획의 독립성 시험. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-13 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S47](https://www.microsoft.com/en-us/microsoft-365/planner/microsoft-planner-business-plans-and-pricing)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X018, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-P005 · 요약 하위 작업과 마일스톤
- 원본: MP-P005 / Microsoft Project 제품군 / P1/P3 / Planner 기본과 프리미엄 비교층. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 계층 작업과 주요 시점
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '요약 하위 작업과 마일스톤' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 계층 작업과 주요 시점의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: Desktop WBS 코드와 동등성 미확인 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 요약 날짜와 완료율 집계 시험. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-13 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S46](https://support.microsoft.com/en-us/planner/teams/advanced-capabilities-with-premium-plans-in-planner), [MS-S47](https://www.microsoft.com/en-us/microsoft-365/planner/microsoft-planner-business-plans-and-pricing)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-P006 · Timeline Gantt
- 원본: MP-P006 / Microsoft Project 제품군 / P1/P3 / Planner 기본과 프리미엄 비교층. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 웹 Gantt에서 작업과 의존성 시각화
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'Timeline Gantt' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 웹 Gantt에서 작업과 의존성 시각화의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: Desktop Gantt 전체 기능 아님 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 날짜 변경 후 연결 관계 유지. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-13 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S46](https://support.microsoft.com/en-us/planner/teams/advanced-capabilities-with-premium-plans-in-planner), [MS-S47](https://www.microsoft.com/en-us/microsoft-365/planner/microsoft-planner-business-plans-and-pricing)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X020, microsoft:MP-X022. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-P007 · 기본 FS 의존성
- 원본: MP-P007 / Microsoft Project 제품군 / P1/P3 / Planner 기본과 프리미엄 비교층. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 작업 간 finish-to-start 생성
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '기본 FS 의존성' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 작업 간 finish-to-start 생성의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: PB에서 보기와 생성권을 구분 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 선행 변경 시 후행 재계산. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-03 / J-03 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S47](https://www.microsoft.com/en-us/microsoft-365/planner/microsoft-planner-business-plans-and-pricing)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-P008 · 고급 의존성과 Lead Lag
- 원본: MP-P008 / Microsoft Project 제품군 / P3 / Planner 기본과 프리미엄 비교층. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: FS SS FF SF와 lead/lag
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '고급 의존성과 Lead Lag' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: FS SS FF SF와 lead/lag의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: Plan 1과 라이선스 차이 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 네 관계와 단위별 지연 계산 시험. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-03 / J-03 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S47](https://www.microsoft.com/en-us/microsoft-365/planner/microsoft-planner-business-plans-and-pricing), [MS-S49](https://techcommunity.microsoft.com/blog/plannerblog/advanced-project-planning-with-microsoft-planner-dependencies-and-critical-path-/4168235)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X005, microsoft:MP-X008, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-P009 · 임계경로
- 원본: MP-P009 / Microsoft Project 제품군 / P3 / Planner 기본과 프리미엄 비교층. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 계획 Timeline의 critical path 표시
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '임계경로' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 계획 Timeline의 critical path 표시의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: Desktop의 복수/교차 CPM과 동등성 미확인 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 종료를 결정하는 경로 변경 확인. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-03 / J-03 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S47](https://www.microsoft.com/en-us/microsoft-365/planner/microsoft-planner-business-plans-and-pricing), [MS-S49](https://techcommunity.microsoft.com/blog/plannerblog/advanced-project-planning-with-microsoft-planner-dependencies-and-critical-path-/4168235)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X020, microsoft:MP-X022. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-P010 · People view
- 원본: MP-P010 / Microsoft Project 제품군 / P1/P3 / Planner 기본과 프리미엄 비교층. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 팀원 과다/과소 배정과 작업량 표시
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'People view' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 팀원 과다/과소 배정과 작업량 표시의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 자동 resource leveling이라고 단정 금지 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 담당별 작업량 분포와 이동 확인. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-13 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S45](https://support.microsoft.com/en-us/planner/compare-microsoft-planner-basic-vs-premium-plans), [MS-S47](https://www.microsoft.com/en-us/microsoft-365/planner/microsoft-planner-business-plans-and-pricing)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-P011 · Assignments view
- 원본: MP-P011 / Microsoft Project 제품군 / P3 / Planner 기본과 프리미엄 비교층. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 기간별 자원 effort 분포 편집
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'Assignments view' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 기간별 자원 effort 분포 편집의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: Desktop 전체 원가 엔진과 구분 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 일/주/월 배정량 총계 일치. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-13 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S46](https://support.microsoft.com/en-us/planner/teams/advanced-capabilities-with-premium-plans-in-planner), [MS-S47](https://www.microsoft.com/en-us/microsoft-365/planner/microsoft-planner-business-plans-and-pricing)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X003, microsoft:MP-X020, microsoft:MP-X023. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-P012 · 사용자 근무 주 달력
- 원본: MP-P012 / Microsoft Project 제품군 / P1/P3 / Planner 기본과 프리미엄 비교층. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 계획별 근무 요일과 일정 재계산
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '사용자 근무 주 달력' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 계획별 근무 요일과 일정 재계산의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 자원별/작업별 달력 동등성 별도 확인 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 비근무 요일을 건너뛴 일정 시험. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-03 / J-03 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S46](https://support.microsoft.com/en-us/planner/teams/advanced-capabilities-with-premium-plans-in-planner)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X005, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-P013 · 목표와 작업 연결
- 원본: MP-P013 / Microsoft Project 제품군 / P1/P3 / Planner 기본과 프리미엄 비교층. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: goal을 정의하고 관련 작업 연결
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '목표와 작업 연결' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: goal을 정의하고 관련 작업 연결의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: Server 전략 최적화와 다른 층 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 목표별 작업 진행 집계. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-13 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S46](https://support.microsoft.com/en-us/planner/teams/advanced-capabilities-with-premium-plans-in-planner)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-P014 · Backlog와 Sprint
- 원본: MP-P014 / Microsoft Project 제품군 / P1/P3 / Planner 기본과 프리미엄 비교층. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 애자일 백로그 및 스프린트 관리
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'Backlog와 Sprint' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 애자일 백로그 및 스프린트 관리의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: sprint 이동·미배정 backlog 추적. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-13 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S45](https://support.microsoft.com/en-us/planner/compare-microsoft-planner-basic-vs-premium-plans), [MS-S47](https://www.microsoft.com/en-us/microsoft-365/planner/microsoft-planner-business-plans-and-pricing)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X014, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-P015 · Custom field와 조건부 색상
- 원본: MP-P015 / Microsoft Project 제품군 / P1/P3 / Planner 기본과 프리미엄 비교층. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 텍스트/날짜/숫자/Yes No/선택 필드; 최대 10개
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'Custom field와 조건부 색상' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 텍스트/날짜/숫자/Yes No/선택 필드; 최대 10개의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 전사 계산 필드와 같지 않음 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 타입별 검증과 조건부 색 적용. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-13 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S46](https://support.microsoft.com/en-us/planner/teams/advanced-capabilities-with-premium-plans-in-planner)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X007, microsoft:MP-X019, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-P016 · Task History
- 원본: MP-P016 / Microsoft Project 제품군 / P3 / Planner 기본과 프리미엄 비교층. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 일정 영향 등 작업 변경 내역 조회
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'Task History' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 일정 영향 등 작업 변경 내역 조회의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 완전 불변 감사로그라고 단정 금지 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 변경자·전후 값·연쇄변경 추적성 시험. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-13 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S46](https://support.microsoft.com/en-us/planner/teams/advanced-capabilities-with-premium-plans-in-planner), [MS-S47](https://www.microsoft.com/en-us/microsoft-365/planner/microsoft-planner-business-plans-and-pricing)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-P017 · Task conversation
- 원본: MP-P017 / Microsoft Project 제품군 / PB/P1/P3 / Planner 기본과 프리미엄 비교층. 근거 상태: partial; 기능 확인과 함께 에디션·조건·세부 동작 재검증 필요.
- 동작: 작업 단위 협업 대화
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'Task conversation' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 작업 단위 협업 대화의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: premium은 Teams 채널 조건 문서; 최신 rollout 재확인 · 조건. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 작업과 대화·멘션 연결. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-13 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S45](https://support.microsoft.com/en-us/planner/compare-microsoft-planner-basic-vs-premium-plans), [MS-S46](https://support.microsoft.com/en-us/planner/teams/advanced-capabilities-with-premium-plans-in-planner)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-P018 · Baseline
- 원본: MP-P018 / Microsoft Project 제품군 / P3 / Planner 기본과 프리미엄 비교층. 근거 상태: partial; 기능 확인과 함께 에디션·조건·세부 동작 재검증 필요.
- 동작: 계획 상태를 기준 스냅샷으로 저장
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'Baseline' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 계획 상태를 기준 스냅샷으로 저장의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 2024 발표는 1개·MPP 기준선 수입 불가; 현재 한계 재확인 · 재검증. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 계획/작업 기준값의 비교와 재설정 시험. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-04 / J-07 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S47](https://www.microsoft.com/en-us/microsoft-365/planner/microsoft-planner-business-plans-and-pricing), [MS-S50](https://techcommunity.microsoft.com/blog/projectblog/boost-your-project%E2%80%99s-success-with-the-baseline-feature-in-the-new-microsoft-plan/4031865)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X008, microsoft:MP-X017, microsoft:MP-X020, microsoft:MP-X021. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-P019 · Portfolio와 Roadmap
- 원본: MP-P019 / Microsoft Project 제품군 / P3 / Planner 기본과 프리미엄 비교층. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 여러 premium plan의 주요 날짜·상태·작업을 통합
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'Portfolio와 Roadmap' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 여러 premium plan의 주요 날짜·상태·작업을 통합의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: basic/ADO/Project Online 연결 미지원; P1은 읽기 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 계획 갱신과 포트폴리오 표시 동기화. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-13 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S48](https://support.microsoft.com/en-us/planner/manage-multiple-plans-with-portfolios-in-microsoft-planner)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X016, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-P020 · Excel Export
- 원본: MP-P020 / Microsoft Project 제품군 / PB/P1/P3 / Planner 기본과 프리미엄 비교층. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 계획을 Excel로 내보내기
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'Excel Export' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 계획을 Excel로 내보내기의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: native MPP 완전 왕복과 다름 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 필드·날짜·행 수와 내보낸 시점 기록. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-13 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S45](https://support.microsoft.com/en-us/planner/compare-microsoft-planner-basic-vs-premium-plans)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X020, microsoft:MP-X021. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-P021 · Graph Basic API
- 원본: MP-P021 / Microsoft Project 제품군 / PB / Planner 기본과 프리미엄 비교층. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: Microsoft Graph에서 basic plan/task 관리
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'Graph Basic API' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: Microsoft Graph에서 basic plan/task 관리의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: premium plan/task는 해당 API 미지원 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 권한·etag·충돌 처리 시험. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-15 / 단계 F; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S51](https://learn.microsoft.com/en-us/graph/api/resources/planner-overview?view=graph-rest-1.0)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X014, microsoft:MP-X015, microsoft:MP-X019, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-P022 · Dataverse와 Schedule API
- 원본: MP-P022 / Microsoft Project 제품군 / P1/P3 / Planner 기본과 프리미엄 비교층. 근거 상태: partial; 기능 확인과 함께 에디션·조건·세부 동작 재검증 필요.
- 동작: premium 데이터 저장 및 일정 API 기반 확장
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'Dataverse와 Schedule API' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: premium 데이터 저장 및 일정 API 기반 확장의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: Power Platform 환경·라이선스·버전 확인 필요 · 조건. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 일정 엔터티 변경 후 재계산/비동기 완료 검증. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-15 / 단계 F; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S52](https://learn.microsoft.com/en-us/microsoftteams/manage-tasks-app), [MS-S53](https://learn.microsoft.com/en-us/dynamics365/project-operations/project-management/schedule-api-preview)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X015, microsoft:MP-X019, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-P023 · AI 계획과 작업 지원
- 원본: MP-P023 / Microsoft Project 제품군 / 별도 Copilot / Planner 기본과 프리미엄 비교층. 근거 상태: partial; 기능 확인과 함께 에디션·조건·세부 동작 재검증 필요.
- 동작: AI 계획/작업 생성·실행과 상태 보고 기능
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'AI 계획과 작업 지원' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: AI 계획/작업 생성·실행과 상태 보고 기능의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: Planner Agent/Project Manager agent 명칭·preview 혼재 · 재검증. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 사용자 검토·권한·변경 추적·승인 경계 정의. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-13 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S46](https://support.microsoft.com/en-us/planner/teams/advanced-capabilities-with-premium-plans-in-planner), [MS-S47](https://www.microsoft.com/en-us/microsoft-365/planner/microsoft-planner-business-plans-and-pricing)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X013, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-MP-P024 · Basic 전용 차이
- 원본: MP-P024 / Microsoft Project 제품군 / PB / Planner 기본과 프리미엄 비교층. 근거 상태: confirmed; 공식 문서에서 기능 확인; 실제품 실행 시험 전.
- 동작: 공식 비교표에 recurring task·Outlook calendar·Loop 등 차이 명시
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'Basic 전용 차이' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 공식 비교표에 recurring task·Outlook calendar·Loop 등 차이 명시의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: premium이 basic의 완전 상위집합 아님 · 문서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: upgrade 시 소실/변경 기능을 사전 경고. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-13 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [MS-S45](https://support.microsoft.com/en-us/planner/compare-microsoft-planner-basic-vs-premium-plans)
- 확인과제 연결: microsoft:MP-X001, microsoft:MP-X020. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-001 · EPS 계층으로 프로젝트 조직화
- 원본: ORA-001 / Oracle Primavera 제품군 / P6 / 구조와 기준정보. 근거 상태: partial; 공식 목차 또는 개요 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: EPS 계층으로 프로젝트 조직화
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'EPS 계층으로 프로젝트 조직화' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: EPS 계층으로 프로젝트 조직화의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: OBS·프로젝트 상태. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 부서·지역별 트리 이동 후 집계와 접근 범위 검증. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-02 / J-01 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S03](https://docs.oracle.com/cd/G18296_01/English/User_Guides/p6_pro_user/toc.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U10. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-002 · OBS 책임 조직과 권한 연결
- 원본: ORA-002 / Oracle Primavera 제품군 / P6 / 구조와 기준정보. 근거 상태: partial; 공식 비교표 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: OBS 책임 조직과 권한 연결
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'OBS 책임 조직과 권한 연결' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: OBS 책임 조직과 권한 연결의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: OC는 OBS와 동일 모델 아님. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 같은 WBS를 책임자/비책임자로 조회해 권한 차이 검증. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-16 / 단계 C; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S03](https://docs.oracle.com/cd/G18296_01/English/User_Guides/p6_pro_user/toc.htm), [OR-S07](https://docs.oracle.com/cd/E80480_01/English/admin/p6_eppm_migration_guide/246408.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U12. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-003 · 프로젝트 내부 WBS 계층
- 원본: ORA-003 / Oracle Primavera 제품군 / P6, OC-S / 구조와 기준정보. 근거 상태: partial; 공식 목차 또는 개요 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 프로젝트 내부 WBS 계층
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '프로젝트 내부 WBS 계층' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 프로젝트 내부 WBS 계층의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 계층 순환 방지. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 이동·복사·삭제/재배정 후 활동 수와 합계 보존. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-02 / J-01 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S03](https://docs.oracle.com/cd/G18296_01/English/User_Guides/p6_pro_user/toc.htm), [OR-S08](https://docs.oracle.com/cd/E80480_01/English/user_guides/schedule_management_user_guide/toc.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U10. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-004 · WBS 가중 마일스톤
- 원본: ORA-004 / Oracle Primavera 제품군 / P6 / 구조와 기준정보. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: WBS 가중 마일스톤
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'WBS 가중 마일스톤' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: WBS 가중 마일스톤의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: OC scope의 Rules of Credit와 동일 객체 아님. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 가중 합으로 진척 재현. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-02 / J-01 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S77](https://docs.oracle.com/cd/G18296_01/English/User_Guides/p6_pro_user/using_weighted_milestones_in_the_wbs.htm), [OR-S40](https://docs.oracle.com/cd/E80480_01/English/user_guides/scope_management_user_guide/89290.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-005 · Workspace 계층과 공유 사전 상속
- 원본: ORA-005 / Oracle Primavera 제품군 / OC / 구조와 기준정보. 근거 상태: partial; 공식 비교표 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: Workspace 계층과 공유 사전 상속
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'Workspace 계층과 공유 사전 상속' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: Workspace 계층과 공유 사전 상속의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: P6 EPS와 의미 차이. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 하위 workspace에서 허용된 기준정보만 사용. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-02 / J-01 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S06](https://docs.oracle.com/cd/E80480_01/English/admin/p6_eppm_migration_guide/246409.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-006 · 프로젝트·WBS·활동 템플릿 재사용
- 원본: ORA-006 / Oracle Primavera 제품군 / P6, OC / 구조와 기준정보. 근거 상태: partial; 공식 목차 또는 개요 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 프로젝트·WBS·활동 템플릿 재사용
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '프로젝트·WBS·활동 템플릿 재사용' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 프로젝트·WBS·활동 템플릿 재사용의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 복사와 링크 구분. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 복제 시 고유 ID와 공유/로컬 데이터 정책 검증. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-02 / J-01 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S03](https://docs.oracle.com/cd/G18296_01/English/User_Guides/p6_pro_user/toc.htm), [OR-S08](https://docs.oracle.com/cd/E80480_01/English/user_guides/schedule_management_user_guide/toc.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U10. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-007 · 활동·프로젝트·자원 코드와 계층 값
- 원본: ORA-007 / Oracle Primavera 제품군 / P6, OC / 구조와 기준정보. 근거 상태: partial; 공식 비교표 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 활동·프로젝트·자원 코드와 계층 값
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '활동·프로젝트·자원 코드와 계층 값' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 활동·프로젝트·자원 코드와 계층 값의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: Global/EPS/Project 또는 workspace 범위. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 코드 기준 필터·그룹·분류 결과 일치. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-02 / J-01 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S03](https://docs.oracle.com/cd/G18296_01/English/User_Guides/p6_pro_user/toc.htm), [OR-S07](https://docs.oracle.com/cd/E80480_01/English/admin/p6_eppm_migration_guide/246408.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U10. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-008 · UDF 및 Configured Fields
- 원본: ORA-008 / Oracle Primavera 제품군 / P6, OC / 구조와 기준정보. 근거 상태: partial; 공식 비교표 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: UDF 및 Configured Fields
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'UDF 및 Configured Fields' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: UDF 및 Configured Fields의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: OC의 수식 기반 확장은 P6 UDF와 차이. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 자료형·필수값·필터·표시·수식 오류 검증. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-02 / J-01 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S07](https://docs.oracle.com/cd/E80480_01/English/admin/p6_eppm_migration_guide/246408.htm), [OR-S08](https://docs.oracle.com/cd/E80480_01/English/user_guides/schedule_management_user_guide/toc.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-009 · 전역·프로젝트·자원 달력
- 원본: ORA-009 / Oracle Primavera 제품군 / P6 / 구조와 기준정보. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 전역·프로젝트·자원 달력
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '전역·프로젝트·자원 달력' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 전역·프로젝트·자원 달력의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 달력 할당과 활동 타입 연결. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 서로 다른 달력으로 동일 기간의 날짜 결과 검증. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-03 / J-03 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S09](https://docs.oracle.com/cd/G18296_01/English/User_Guides/p6_pro_user/calendars.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U05, oracle:U10. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-010 · 근무시간·휴일·예외·개인 휴가
- 원본: ORA-010 / Oracle Primavera 제품군 / P6, OC-S / 구조와 기준정보. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 근무시간·휴일·예외·개인 휴가
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '근무시간·휴일·예외·개인 휴가' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 근무시간·휴일·예외·개인 휴가의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: OC resource calendar는 workspace calendar 할당 방식. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 반일 휴무·야간·휴일 연쇄 사례 계산. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-03 / J-03 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S09](https://docs.oracle.com/cd/G18296_01/English/User_Guides/p6_pro_user/calendars.htm), [OR-S07](https://docs.oracle.com/cd/E80480_01/English/admin/p6_eppm_migration_guide/246408.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-011 · 상위 달력 변경 전파
- 원본: ORA-011 / Oracle Primavera 제품군 / P6 / 구조와 기준정보. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 상위 달력 변경 전파
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '상위 달력 변경 전파' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 상위 달력 변경 전파의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: OC 상속은 별도 비교 필요. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: global 변경과 로컬 예외의 보존/전파 검증. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-03 / J-03 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S09](https://docs.oracle.com/cd/G18296_01/English/User_Guides/p6_pro_user/calendars.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U05. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-012 · 시간 단위 환산과 표시 설정
- 원본: ORA-012 / Oracle Primavera 제품군 / P6, OC / 구조와 기준정보. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 시간 단위 환산과 표시 설정
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '시간 단위 환산과 표시 설정' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 시간 단위 환산과 표시 설정의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 계산 내부 단위와 UI 단위 분리. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 1일=8h와 10h 달력에서 입력·표시 차이 검증. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-02 / J-01 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S09](https://docs.oracle.com/cd/G18296_01/English/User_Guides/p6_pro_user/calendars.htm), [OR-S08](https://docs.oracle.com/cd/E80480_01/English/user_guides/schedule_management_user_guide/toc.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-013 · 통화·환율·표시 통화
- 원본: ORA-013 / Oracle Primavera 제품군 / P6, OC / 구조와 기준정보. 근거 상태: partial; 공식 비교표 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 통화·환율·표시 통화
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '통화·환율·표시 통화' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 통화·환율·표시 통화의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: OC 시간대별 환율; P6와 방식 대조 필요. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 원통화 보존·보고 통화 환산·반올림 검증. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-02 / J-01 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S07](https://docs.oracle.com/cd/E80480_01/English/admin/p6_eppm_migration_guide/246408.htm), [OR-S08](https://docs.oracle.com/cd/E80480_01/English/user_guides/schedule_management_user_guide/toc.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-014 · 회계기간·보고주기
- 원본: ORA-014 / Oracle Primavera 제품군 / P6, OC-S / 구조와 기준정보. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 회계기간·보고주기
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '회계기간·보고주기' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 회계기간·보고주기의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: P6 Financial Period와 OC Reporting Cycle 차이. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 기간 경계의 실적 귀속과 잠금 정책 검증. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-02 / J-01 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S07](https://docs.oracle.com/cd/E80480_01/English/admin/p6_eppm_migration_guide/246408.htm), [OR-S23](https://docs.oracle.com/cd/G18296_01/English/User_Guides/p6_pro_user/store_period_performance.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U10. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-015 · 자원·역할·단위·단가 사전
- 원본: ORA-015 / Oracle Primavera 제품군 / P6, OC / 구조와 기준정보. 근거 상태: partial; 공식 비교표 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 자원·역할·단위·단가 사전
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '자원·역할·단위·단가 사전' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 자원·역할·단위·단가 사전의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 중앙 사전/프로젝트 사전. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 사전 변경의 기존 assignment 영향 제어. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-02 / J-01 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S04](https://docs.oracle.com/cd/E80480_01/English/admin/p6_eppm_migration_guide/246423.htm), [OR-S08](https://docs.oracle.com/cd/E80480_01/English/user_guides/schedule_management_user_guide/toc.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U10. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-016 · 위치·좌표와 지리 정보
- 원본: ORA-016 / Oracle Primavera 제품군 / P6, OC / 구조와 기준정보. 근거 상태: partial; 공식 비교표 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 위치·좌표와 지리 정보
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '위치·좌표와 지리 정보' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 위치·좌표와 지리 정보의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: OC 지도와 P6 위치 필드 동일시 금지. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 좌표 기반 프로젝트 조회. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-02 / J-01 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S07](https://docs.oracle.com/cd/E80480_01/English/admin/p6_eppm_migration_guide/246408.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-017 · 활동 타입 6종
- 원본: ORA-017 / Oracle Primavera 제품군 / P6, OC-S / 일정 엔진과 편집. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 활동 타입 6종
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '활동 타입 6종' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 활동 타입 6종의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 같은 duration 입력에도 계산 차이. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: Task/Resource dependent, LOE, 시작/완료 milestone, WBS summary 별 날짜 검증. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-03 / J-03 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S10](https://docs.oracle.com/cd/G18296_01/English/User_Guides/p6_pro_user/activity_types.htm), [OR-S08](https://docs.oracle.com/cd/E80480_01/English/user_guides/schedule_management_user_guide/toc.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U05, oracle:U10. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-018 · 기간 타입과 units·units/time 연동
- 원본: ORA-018 / Oracle Primavera 제품군 / PP, PC, OC-S / 일정 엔진과 편집. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 기간 타입과 units·units/time 연동
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '기간 타입과 units·units/time 연동' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 기간 타입과 units·units/time 연동의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 자원 배정 계산 정책. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: Fixed Units·Fixed Units/Time·두 Fixed Duration 조합별 값 변경 테스트. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-03 / J-03 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S11](https://docs.oracle.com/cd/G18296_01/English/User_Guides/p6_pro_user/define_activity_duration_types.htm), [OR-S08](https://docs.oracle.com/cd/E80480_01/English/user_guides/schedule_management_user_guide/toc.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U05, oracle:U10, oracle:U11. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-019 · FS·SS·FF·SF 선후행 관계
- 원본: ORA-019 / Oracle Primavera 제품군 / P6, OC-S / 일정 엔진과 편집. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: FS·SS·FF·SF 선후행 관계
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'FS·SS·FF·SF 선후행 관계' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: FS·SS·FF·SF 선후행 관계의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 순환 관계 진단. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 4종 관계 각각 양/음 lag와 달력 교차 사례 검증. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-03 / J-03 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S12](https://docs.oracle.com/cd/G18296_01/English/User_Guides/p6_pro_user/relationships.htm), [OR-S16](https://docs.oracle.com/cd/E80480_01/English/user_guides/schedule_management_user_guide/88257.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U05, oracle:U10. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-020 · 프로젝트 간 외부 관계
- 원본: ORA-020 / Oracle Primavera 제품군 / P6, OC-S / 일정 엔진과 편집. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 프로젝트 간 외부 관계
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '프로젝트 간 외부 관계' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 프로젝트 간 외부 관계의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: multi-project 계산 범위. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 외부 프로젝트 열림/닫힘 및 포함/무시 결과 비교. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-03 / J-03 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S12](https://docs.oracle.com/cd/G18296_01/English/User_Guides/p6_pro_user/relationships.htm), [OR-S16](https://docs.oracle.com/cd/E80480_01/English/user_guides/schedule_management_user_guide/88257.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U05, oracle:U10. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-021 · 관계별 lag 달력과 SS lag 기준
- 원본: ORA-021 / Oracle Primavera 제품군 / P6, OC-S / 일정 엔진과 편집. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 관계별 lag 달력과 SS lag 기준
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '관계별 lag 달력과 SS lag 기준' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 관계별 lag 달력과 SS lag 기준의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 실적 역순 진행 포함. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: Early Start/Actual Start 기반 SS lag 차이를 재현. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-03 / J-03 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S02](https://docs.oracle.com/cd/E80480_01/English/admin/p6_eppm_migration_guide/246421.htm), [OR-S16](https://docs.oracle.com/cd/E80480_01/English/user_guides/schedule_management_user_guide/88257.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U05, oracle:U10. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-022 · 1차·2차 제약과 강제 제약
- 원본: ORA-022 / Oracle Primavera 제품군 / P6, OC-S / 일정 엔진과 편집. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 1차·2차 제약과 강제 제약
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '1차·2차 제약과 강제 제약' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 1차·2차 제약과 강제 제약의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: OC 특정 주제약은 보조제약 불가. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 허용 조합과 early/late date 영향 검증. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-03 / J-03 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S17](https://docs.oracle.com/cd/E80480_01/English/user_guides/schedule_management_user_guide/91074.htm), [OR-S02](https://docs.oracle.com/cd/E80480_01/English/admin/p6_eppm_migration_guide/246421.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U05, oracle:U10. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-023 · CPM 전진·후진 계산
- 원본: ORA-023 / Oracle Primavera 제품군 / P6, OC-S / 일정 엔진과 편집. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: CPM 전진·후진 계산
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'CPM 전진·후진 계산' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: CPM 전진·후진 계산의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 달력·기간·관계·data date. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 작은 네트워크의 ES/EF/LS/LF와 수작업 정답 일치. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-03 / J-03 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S13](https://docs.oracle.com/cd/G18296_01/English/User_Guides/p6_pro_user/scheduling_projects.htm), [OR-S15](https://docs.oracle.com/cd/E80480_01/English/user_guides/schedule_management_user_guide/88251.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U05, oracle:U10. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-024 · Total Float·Free Float
- 원본: ORA-024 / Oracle Primavera 제품군 / P6, OC-S / 일정 엔진과 편집. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: Total Float·Free Float
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'Total Float·Free Float' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: Total Float·Free Float의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: float 산식 옵션. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 다중 달력·강제 완료일의 음수 float 사례 검증. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-03 / J-03 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S14](https://docs.oracle.com/cd/G18296_01/English/User_Guides/p6_pro_user/calculate_multiple_float_paths.htm), [OR-S15](https://docs.oracle.com/cd/E80480_01/English/user_guides/schedule_management_user_guide/88251.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U05, oracle:U10. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-025 · 임계활동 기준과 Longest Path
- 원본: ORA-025 / Oracle Primavera 제품군 / P6, OC-S / 일정 엔진과 편집. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 임계활동 기준과 Longest Path
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '임계활동 기준과 Longest Path' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 임계활동 기준과 Longest Path의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 두 기준은 항상 동일하지 않음. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 총여유 기준과 longest path 결과를 분리 표시. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-03 / J-03 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S14](https://docs.oracle.com/cd/G18296_01/English/User_Guides/p6_pro_user/calculate_multiple_float_paths.htm), [OR-S15](https://docs.oracle.com/cd/E80480_01/English/user_guides/schedule_management_user_guide/88251.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U05, oracle:U10. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-026 · Multiple Float Paths
- 원본: ORA-026 / Oracle Primavera 제품군 / P6, OC-S / 일정 엔진과 편집. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: Multiple Float Paths
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'Multiple Float Paths' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: Multiple Float Paths의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 동률 tie-break 포함. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 종료 milestone·경로 수·TF/FF별 경로 순서 검증. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-03 / J-03 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S14](https://docs.oracle.com/cd/G18296_01/English/User_Guides/p6_pro_user/calculate_multiple_float_paths.htm), [OR-S15](https://docs.oracle.com/cd/E80480_01/English/user_guides/schedule_management_user_guide/88251.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U05, oracle:U10. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-027 · Data Date 기반 실적/잔여 일정
- 원본: ORA-027 / Oracle Primavera 제품군 / P6, OC-S / 일정 엔진과 편집. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: Data Date 기반 실적/잔여 일정
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'Data Date 기반 실적/잔여 일정' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: Data Date 기반 실적/잔여 일정의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 프로젝트별 또는 공통 data date. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 기준일 전 실제와 이후 remaining을 구분. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-05 / J-08 / 단계 C; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S16](https://docs.oracle.com/cd/E80480_01/English/user_guides/schedule_management_user_guide/88257.htm), [OR-S03](https://docs.oracle.com/cd/G18296_01/English/User_Guides/p6_pro_user/toc.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U05, oracle:U10. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-028 · 역순 진행 처리 세 가지
- 원본: ORA-028 / Oracle Primavera 제품군 / P6, OC-S / 일정 엔진과 편집. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 역순 진행 처리 세 가지
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '역순 진행 처리 세 가지' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 역순 진행 처리 세 가지의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 실적 데이터 필요. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: Retained Logic/Progress Override/Actual Dates 결과 차이. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-03 / J-03 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S16](https://docs.oracle.com/cd/E80480_01/English/user_guides/schedule_management_user_guide/88257.htm), [OR-S02](https://docs.oracle.com/cd/E80480_01/English/admin/p6_eppm_migration_guide/246421.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U05, oracle:U10. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-029 · Expected Finish와 Must Finish By
- 원본: ORA-029 / Oracle Primavera 제품군 / P6, OC-S / 일정 엔진과 편집. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: Expected Finish와 Must Finish By
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'Expected Finish와 Must Finish By' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: Expected Finish와 Must Finish By의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 날짜의 우선순위. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 예상 완료·필수 완료 제한 영향 별도 확인. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-03 / J-03 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S16](https://docs.oracle.com/cd/E80480_01/English/user_guides/schedule_management_user_guide/88257.htm), [OR-S17](https://docs.oracle.com/cd/E80480_01/English/user_guides/schedule_management_user_guide/91074.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U05, oracle:U10. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-030 · 자동 재계산과 명시적 스케줄 실행
- 원본: ORA-030 / Oracle Primavera 제품군 / P6; OC 실행 방식 별도 / 일정 엔진과 편집. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 자동 재계산과 명시적 스케줄 실행
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '자동 재계산과 명시적 스케줄 실행' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 자동 재계산과 명시적 스케줄 실행의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: OC의 서비스 예약과 P6 자동 계산 혼동 금지. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 자동/수동 상태에서 stale 결과 표시. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-03 / J-03 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S13](https://docs.oracle.com/cd/G18296_01/English/User_Guides/p6_pro_user/scheduling_projects.htm), [OR-S02](https://docs.oracle.com/cd/E80480_01/English/admin/p6_eppm_migration_guide/246421.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U05, oracle:U10. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-031 · Multi-project/program 통합 일정
- 원본: ORA-031 / Oracle Primavera 제품군 / P6, OC-S / 일정 엔진과 편집. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: Multi-project/program 통합 일정
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'Multi-project/program 통합 일정' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: Multi-project/program 통합 일정의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 적용 프로젝트 옵션 선택. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 공유 관계·자원과 개별 data date로 일괄 계산. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-15 / 단계 F; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S16](https://docs.oracle.com/cd/E80480_01/English/user_guides/schedule_management_user_guide/88257.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U05, oracle:U10, oracle:U11. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-032 · 일정 실행 로그·관계 루프 진단
- 원본: ORA-032 / Oracle Primavera 제품군 / OC-S, P6 확인 범위별 / 일정 엔진과 편집. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 일정 실행 로그·관계 루프 진단
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '일정 실행 로그·관계 루프 진단' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 일정 실행 로그·관계 루프 진단의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: OC 문서에서 루프 시 실패 명시. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 순환 입력은 실패 원인과 활동을 명확히 보고. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-03 / J-03 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S16](https://docs.oracle.com/cd/E80480_01/English/user_guides/schedule_management_user_guide/88257.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U05, oracle:U10. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-033 · Suspend/Resume 작업 중단
- 원본: ORA-033 / Oracle Primavera 제품군 / P6, OC-S / 일정 엔진과 편집. 근거 상태: partial; 공식 목차 또는 개요 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: Suspend/Resume 작업 중단
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'Suspend/Resume 작업 중단' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: Suspend/Resume 작업 중단의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 임의의 여러 분할 지원으로 확대 해석 금지. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 실제 작업 중단 기간의 remaining 날짜 검증. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-03 / J-03 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S03](https://docs.oracle.com/cd/G18296_01/English/User_Guides/p6_pro_user/toc.htm), [OR-S08](https://docs.oracle.com/cd/E80480_01/English/user_guides/schedule_management_user_guide/toc.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U05, oracle:U10. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-034 · 활동 복사·번호 재부여·dissolve
- 원본: ORA-034 / Oracle Primavera 제품군 / PP, PC, OC-S / 일정 엔진과 편집. 근거 상태: partial; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 활동 복사·번호 재부여·dissolve
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '활동 복사·번호 재부여·dissolve' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 활동 복사·번호 재부여·dissolve의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: ID 자동증가·번호 공백 처리 정책 별도 검증. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 관계 보존·ID 충돌·삭제 영향 검증. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-03 / J-03 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S92](https://docs.oracle.com/cd/G18296_01/English/User_Guides/p6_pro_user/renumber_activity_ids_in_the_activity_table.htm), [OR-S08](https://docs.oracle.com/cd/E80480_01/English/user_guides/schedule_management_user_guide/toc.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U05, oracle:U10, oracle:U12. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-035 · Global Change 일괄 수정
- 원본: ORA-035 / Oracle Primavera 제품군 / P6, OC-S / 일정 엔진과 편집. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: Global Change 일괄 수정
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'Global Change 일괄 수정' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: Global Change 일괄 수정의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: P6 assignment/expense와 OC activity 범위 차이. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 조건·미리보기·변경 범위·복원 테스트. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-03 / J-03 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S91](https://docs.oracle.com/cd/G18296_01/English/User_Guides/p6_pro_user/global_change.htm), [OR-S07](https://docs.oracle.com/cd/E80480_01/English/admin/p6_eppm_migration_guide/246408.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U05, oracle:U10. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-036 · 간트·표·네트워크·Trace Logic
- 원본: ORA-036 / Oracle Primavera 제품군 / P6, OC-S / 일정 엔진과 편집. 근거 상태: partial; 공식 비교표 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 간트·표·네트워크·Trace Logic
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '간트·표·네트워크·Trace Logic' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 간트·표·네트워크·Trace Logic의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: OC network/trace logic 문서상 6,000 미만 제약. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 선택 활동의 전후행과 막대 결과 동기화. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-03 / J-03 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S02](https://docs.oracle.com/cd/E80480_01/English/admin/p6_eppm_migration_guide/246421.htm), [OR-S08](https://docs.oracle.com/cd/E80480_01/English/user_guides/schedule_management_user_guide/toc.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U05, oracle:U10. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-037 · 공유 레이아웃·그룹·필터·다중 정렬
- 원본: ORA-037 / Oracle Primavera 제품군 / P6, OC / 일정 엔진과 편집. 근거 상태: partial; 공식 목차 또는 개요 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 공유 레이아웃·그룹·필터·다중 정렬
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '공유 레이아웃·그룹·필터·다중 정렬' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 공유 레이아웃·그룹·필터·다중 정렬의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: UI 동작·필터 의미 대조 필요. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 저장/공유/개인 덮어쓰기와 권한 검증. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-03 / J-03 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S03](https://docs.oracle.com/cd/G18296_01/English/User_Guides/p6_pro_user/toc.htm), [OR-S08](https://docs.oracle.com/cd/E80480_01/English/user_guides/schedule_management_user_guide/toc.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U05, oracle:U10. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-038 · 일정 품질 검사와 Health Score
- 원본: ORA-038 / Oracle Primavera 제품군 / P6, OC-S / 일정 엔진과 편집. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 일정 품질 검사와 Health Score
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '일정 품질 검사와 Health Score' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 일정 품질 검사와 Health Score의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 체크 항목·허용치 전체 대조는 후속. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 미연결·과도한 lag·제약 등 위반에 drill-down. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-03 / J-03 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S86](https://docs.oracle.com/cd/E80480_01/English/user_guides/schedule_management_user_guide/149130.htm), [OR-S03](https://docs.oracle.com/cd/G18296_01/English/User_Guides/p6_pro_user/toc.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U05, oracle:U10. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-039 · 계층형 자원과 역할
- 원본: ORA-039 / Oracle Primavera 제품군 / P6, OC / 자원과 역할 계획. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 계층형 자원과 역할
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '계층형 자원과 역할' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 계층형 자원과 역할의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 업무역할과 보안역할 별개. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 역할→실자원 배치 후 실적 보존. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-06 / J-05 / 단계 C; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S75](https://docs.oracle.com/cd/G18296_01/English/User_Guides/p6_pro_user/18823.htm), [OR-S76](https://docs.oracle.com/cd/E80480_01/help/en/user/90967.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U12. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-040 · 인력·비인력·자재 자원
- 원본: ORA-040 / Oracle Primavera 제품군 / P6, OC-S / 자원과 역할 계획. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 인력·비인력·자재 자원
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '인력·비인력·자재 자원' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 인력·비인력·자재 자원의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 자재 UOM 필요. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 시간형과 물량형 단위를 다르게 집계. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-06 / J-05 / 단계 C; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S75](https://docs.oracle.com/cd/G18296_01/English/User_Guides/p6_pro_user/18823.htm), [OR-S76](https://docs.oracle.com/cd/E80480_01/help/en/user/90967.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-041 · 기간별 가용량·단가
- 원본: ORA-041 / Oracle Primavera 제품군 / P6, OC / 자원과 역할 계획. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 기간별 가용량·단가
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '기간별 가용량·단가' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 기간별 가용량·단가의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 환율·근무 달력. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 단가 변경일을 넘는 assignment 비용 분할 검증. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-06 / J-05 / 단계 C; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S84](https://docs.oracle.com/cd/G18296_01/English/User_Guides/p6_pro_user/calculating_cost_when_using_multiple_resource_rates.htm), [OR-S08](https://docs.oracle.com/cd/E80480_01/English/user_guides/schedule_management_user_guide/toc.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U05. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-042 · 자원 교대조
- 원본: ORA-042 / Oracle Primavera 제품군 / PP, PC; PW 설정 범위 확인 / 자원과 역할 계획. 근거 상태: partial; 공식 비교표 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 자원 교대조
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '자원 교대조' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 자원 교대조의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: OC에는 직접 대응 기능 없음으로 비교표 표시. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: shift별 가용 시간과 표시 검증. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-06 / J-05 / 단계 C; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S04](https://docs.oracle.com/cd/E80480_01/English/admin/p6_eppm_migration_guide/246423.htm), [OR-S07](https://docs.oracle.com/cd/E80480_01/English/admin/p6_eppm_migration_guide/246408.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-043 · 자원/역할을 여러 활동에 배정
- 원본: ORA-043 / Oracle Primavera 제품군 / P6, OC-S / 자원과 역할 계획. 근거 상태: partial; 공식 비교표 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 자원/역할을 여러 활동에 배정
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '자원/역할을 여러 활동에 배정' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 자원/역할을 여러 활동에 배정의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: assignment 안정 식별자와 매칭 규칙 필요. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 일괄 배정·대체 시 planned/actual 보존. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-06 / J-05 / 단계 C; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S04](https://docs.oracle.com/cd/E80480_01/English/admin/p6_eppm_migration_guide/246423.htm), [OR-S08](https://docs.oracle.com/cd/E80480_01/English/user_guides/schedule_management_user_guide/toc.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-044 · 자원 배정 곡선과 lag
- 원본: ORA-044 / Oracle Primavera 제품군 / P6, OC-S / 자원과 역할 계획. 근거 상태: partial; 공식 비교표 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 자원 배정 곡선과 lag
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '자원 배정 곡선과 lag' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 자원 배정 곡선과 lag의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: P6 curve와 OC Curve Profile. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: front/back loaded 분포의 합계 보존. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-03 / J-03 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S07](https://docs.oracle.com/cd/E80480_01/English/admin/p6_eppm_migration_guide/246408.htm), [OR-S03](https://docs.oracle.com/cd/G18296_01/English/User_Guides/p6_pro_user/toc.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U06. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-045 · 미래 기간 버킷 수동 계획
- 원본: ORA-045 / Oracle Primavera 제품군 / P6, OC-S / 자원과 역할 계획. 근거 상태: partial; 공식 비교표 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 미래 기간 버킷 수동 계획
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '미래 기간 버킷 수동 계획' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 미래 기간 버킷 수동 계획의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 자동 분포와 수동 분포 우선순위. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 기간별 값을 편집해 전체 units와 일치. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-06 / J-05 / 단계 C; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S04](https://docs.oracle.com/cd/E80480_01/English/admin/p6_eppm_migration_guide/246423.htm), [OR-S08](https://docs.oracle.com/cd/E80480_01/English/user_guides/schedule_management_user_guide/toc.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U06, oracle:U09. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-046 · 자원·역할 histogram·spreadsheet·S-curve
- 원본: ORA-046 / Oracle Primavera 제품군 / P6, OC-S / 자원과 역할 계획. 근거 상태: partial; 공식 비교표 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 자원·역할 histogram·spreadsheet·S-curve
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '자원·역할 histogram·spreadsheet·S-curve' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 자원·역할 histogram·spreadsheet·S-curve의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 평준화 전후 비교. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 일/주/월 집계와 누적 곡선 일치. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-06 / J-05 / 단계 C; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S08](https://docs.oracle.com/cd/E80480_01/English/user_guides/schedule_management_user_guide/toc.htm), [OR-S04](https://docs.oracle.com/cd/E80480_01/English/admin/p6_eppm_migration_guide/246423.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U05, oracle:U06. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-047 · 과부하 분석
- 원본: ORA-047 / Oracle Primavera 제품군 / P6, OC / 자원과 역할 계획. 근거 상태: partial; 공식 비교표 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 과부하 분석
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '과부하 분석' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 과부하 분석의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 조회 범위·요약 서비스. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 여러 프로젝트의 수요와 가용량 차이 표시. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-06 / J-05 / 단계 C; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S04](https://docs.oracle.com/cd/E80480_01/English/admin/p6_eppm_migration_guide/246423.htm), [OR-S08](https://docs.oracle.com/cd/E80480_01/English/user_guides/schedule_management_user_guide/toc.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-048 · 자원 평준화
- 원본: ORA-048 / Oracle Primavera 제품군 / P6, OC-S / 자원과 역할 계획. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 자원 평준화
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '자원 평준화' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 자원 평준화의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: P6 곡선 미사용·수동 미래 버킷 고려. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 동일 자원 동시 수요를 날짜 지연으로 해결. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-06 / J-06 / 단계 D; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S18](https://docs.oracle.com/cd/G18296_01/English/User_Guides/p6_pro_user/leveling_resources.htm), [OR-S19](https://docs.oracle.com/cd/E80480_01/English/user_guides/schedule_management_user_guide/91056.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U05, oracle:U06. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-049 · 역할 평준화와 program 일괄 평준화
- 원본: ORA-049 / Oracle Primavera 제품군 / OC-S / 자원과 역할 계획. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 역할 평준화와 program 일괄 평준화
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '역할 평준화와 program 일괄 평준화' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 역할 평준화와 program 일괄 평준화의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: Level 권한 필요. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 실명 자원 없는 역할 수요도 제한 처리. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-06 / J-06 / 단계 D; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S87](https://docs.oracle.com/cd/E80480_01/English/user_guides/schedule_management_user_guide/91057.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U05, oracle:U06, oracle:U12. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-050 · 우선순위 기반 평준화
- 원본: ORA-050 / Oracle Primavera 제품군 / P6, OC-S / 자원과 역할 계획. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 우선순위 기반 평준화
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '우선순위 기반 평준화' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 우선순위 기반 평준화의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: P6 프로젝트 우선순위 포함. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 동일 입력 재실행 결과와 우선순위 영향 확인. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-06 / J-06 / 단계 D; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S03](https://docs.oracle.com/cd/G18296_01/English/User_Guides/p6_pro_user/toc.htm), [OR-S87](https://docs.oracle.com/cd/E80480_01/English/user_guides/schedule_management_user_guide/91057.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U05, oracle:U06, oracle:U10. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-051 · 평준화 후 자원 유발 임계경로
- 원본: ORA-051 / Oracle Primavera 제품군 / PP, PC / 자원과 역할 계획. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 평준화 후 자원 유발 임계경로
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '평준화 후 자원 유발 임계경로' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 평준화 후 자원 유발 임계경로의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 일반 CPM 임계경로와 구분. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 지연을 일으킨 assignment 추적. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-03 / J-03 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S18](https://docs.oracle.com/cd/G18296_01/English/User_Guides/p6_pro_user/leveling_resources.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U05, oracle:U06. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-052 · 자원/역할 팀
- 원본: ORA-052 / Oracle Primavera 제품군 / PW / 자원과 역할 계획. 근거 상태: partial; 공식 비교표 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 자원/역할 팀
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '자원/역할 팀' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 자원/역할 팀의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: OC 동일 객체는 공식 비교표에서 미지원. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 팀 필터와 담당 자원 범위 검증. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-06 / J-05 / 단계 C; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S04](https://docs.oracle.com/cd/E80480_01/English/admin/p6_eppm_migration_guide/246423.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-053 · 상위 자원 계획·수요·capacity
- 원본: ORA-053 / Oracle Primavera 제품군 / PW, OC-P / 자원과 역할 계획. 근거 상태: partial; 공식 비교표 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 상위 자원 계획·수요·capacity
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '상위 자원 계획·수요·capacity' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 상위 자원 계획·수요·capacity의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 하위 assignment와 중복 집계 방지. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 프로젝트별 월별 역할 수요와 용량 gap 확인. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-06 / J-05 / 단계 C; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S04](https://docs.oracle.com/cd/E80480_01/English/admin/p6_eppm_migration_guide/246423.htm), [OR-S05](https://docs.oracle.com/cd/E80480_01/English/admin/p6_eppm_migration_guide/246420.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-054 · 자원 요청·배치 협업
- 원본: ORA-054 / Oracle Primavera 제품군 / PW / 자원과 역할 계획. 근거 상태: partial; 공식 비교표 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 자원 요청·배치 협업
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '자원 요청·배치 협업' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 자원 요청·배치 협업의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: OC P6 resource request 직접 대응 없음. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 요청→검토→배정 상태와 미해결 수요 검증. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-06 / J-05 / 단계 C; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S56](https://docs.oracle.com/cd/E80480_01/English/admin/p6_eppm_migration_guide/246426.htm), [OR-S02](https://docs.oracle.com/cd/E80480_01/English/admin/p6_eppm_migration_guide/246421.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-055 · P6 baseline snapshot와 복원/갱신
- 원본: ORA-055 / Oracle Primavera 제품군 / P6 / Baseline과 진척 및 Earned Value. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: P6 baseline snapshot와 복원/갱신
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'P6 baseline snapshot와 복원/갱신' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: P6 baseline snapshot와 복원/갱신의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 저장 상한은 admin 설정; 비교용 3개. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 원본과 baseline의 변경 격리. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-04 / J-07 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S20](https://docs.oracle.com/cd/G18296_01/English/User_Guides/p6_pro_user/baselines.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U03, oracle:U04, oracle:U07, oracle:U08, oracle:U13. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-056 · primary·secondary·tertiary·project baseline
- 원본: ORA-056 / Oracle Primavera 제품군 / PP, PC / Baseline과 진척 및 Earned Value. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: primary·secondary·tertiary·project baseline
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'primary·secondary·tertiary·project baseline' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: primary·secondary·tertiary·project baseline의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: PW 표시 옵션 다름. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 표시용과 EVM용 baseline 분리 선택. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-04 / J-07 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S20](https://docs.oracle.com/cd/G18296_01/English/User_Guides/p6_pro_user/baselines.htm), [OR-S02](https://docs.oracle.com/cd/E80480_01/English/admin/p6_eppm_migration_guide/246421.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U03, oracle:U04, oracle:U07, oracle:U08, oracle:U13. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-057 · OC 공식/비공식 baseline·사용자 선택
- 원본: ORA-057 / Oracle Primavera 제품군 / OC-S / Baseline과 진척 및 Earned Value. 근거 상태: partial; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: OC 공식/비공식 baseline·사용자 선택
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'OC 공식/비공식 baseline·사용자 선택' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: OC 공식/비공식 baseline·사용자 선택의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 문서 내부 개수 모순은 U03 참고. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: Original/Current/Supplementary와 user 3개 비교. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-04 / J-07 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S21](https://docs.oracle.com/cd/E80480_01/English/user_guides/schedule_management_user_guide/89344.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U03, oracle:U04, oracle:U07, oracle:U08, oracle:U10, oracle:U13. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-058 · 과거 시점 baseline와 직접 편집
- 원본: ORA-058 / Oracle Primavera 제품군 / OC-S / Baseline과 진척 및 Earned Value. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 과거 시점 baseline와 직접 편집
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '과거 시점 baseline와 직접 편집' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 과거 시점 baseline와 직접 편집의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: P6 XML 유입 baseline의 revert 제약. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 과거 스냅샷 재생과 현재 일정 격리. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-04 / J-07 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S21](https://docs.oracle.com/cd/E80480_01/English/user_guides/schedule_management_user_guide/89344.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U03, oracle:U04, oracle:U05, oracle:U07, oracle:U08, oracle:U13. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-059 · P6 Reflection what-if와 선택 병합
- 원본: ORA-059 / Oracle Primavera 제품군 / PP, PC 기능별 / Baseline과 진척 및 Earned Value. 근거 상태: partial; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: P6 Reflection what-if와 선택 병합
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'P6 Reflection what-if와 선택 병합' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: P6 Reflection what-if와 선택 병합의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: PP/PC 병합 지원 범위 재확인. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 선택 변경만 원본에 반영하고 비선택 데이터 보존. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-04 / J-07 / 단계 D; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S24](https://docs.oracle.com/cd/G18296_01/English/User_Guides/p6_pro_user/what_is_a_reflection.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U03, oracle:U04, oracle:U07, oracle:U08, oracle:U13. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-060 · OC 시나리오와 현재 일정 전환
- 원본: ORA-060 / Oracle Primavera 제품군 / OC-S / Baseline과 진척 및 Earned Value. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: OC 시나리오와 현재 일정 전환
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'OC 시나리오와 현재 일정 전환' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: OC 시나리오와 현재 일정 전환의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: baseline와 scenario 구분. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 여러 대안 비교 후 승인된 안만 활성화. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-04 / J-07 / 단계 D; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S25](https://docs.oracle.com/cd/E80480_01/English/user_guides/schedule_management_user_guide/107323.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U03, oracle:U04, oracle:U05, oracle:U07, oracle:U08, oracle:U10, oracle:U13. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-061 · Schedule Comparison와 차이·병합
- 원본: ORA-061 / Oracle Primavera 제품군 / PP, PC, OC-S / Baseline과 진척 및 Earned Value. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: Schedule Comparison와 차이·병합
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'Schedule Comparison와 차이·병합' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: Schedule Comparison와 차이·병합의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: OC 비교의 날짜 차이는 프로젝트 달력 사용; 활동 화면과 다를 수 있음. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 추가/삭제/수정 활동·날짜·비용 차이 검출. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-04 / J-07 / 단계 D; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S24](https://docs.oracle.com/cd/G18296_01/English/User_Guides/p6_pro_user/what_is_a_reflection.htm), [OR-S85](https://docs.oracle.com/cd/E80480_01/English/user_guides/schedule_management_user_guide/191095.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U03, oracle:U04, oracle:U05, oracle:U07, oracle:U08, oracle:U10, oracle:U13. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-062 · Duration·Units·Physical 완료율
- 원본: ORA-062 / Oracle Primavera 제품군 / P6, OC-S / Baseline과 진척 및 Earned Value. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: Duration·Units·Physical 완료율
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'Duration·Units·Physical 완료율' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: Duration·Units·Physical 완료율의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: OC Scope 추가; Duration 산식의 original/actual 기준 차이 검증. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 서로 다른 완료율을 독립적으로 계산. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-05 / J-08 / 단계 C; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S78](https://docs.oracle.com/cd/G18296_01/English/User_Guides/p6_pro_user/define_activity_percent_complete_types.htm), [OR-S88](https://docs.oracle.com/cd/E80480_01/English/user_guides/schedule_management_user_guide/101995.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U03, oracle:U04, oracle:U07, oracle:U08, oracle:U13. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-063 · 활동 step과 가중 진척
- 원본: ORA-063 / Oracle Primavera 제품군 / P6 / Baseline과 진척 및 Earned Value. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 활동 step과 가중 진척
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '활동 step과 가중 진척' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 활동 step과 가중 진척의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: OC Task로 자동 동일 변환 아님. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: step weight 완료 합이 Physical 값과 연결. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-05 / J-08 / 단계 C; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S79](https://docs.oracle.com/cd/G18296_01/English/User_Guides/p6_pro_user/activity_steps.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U03, oracle:U04, oracle:U07, oracle:U08, oracle:U13. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-064 · 실제 시작/종료·잔여 기간·실제/잔여 units
- 원본: ORA-064 / Oracle Primavera 제품군 / P6, OC-S / Baseline과 진척 및 Earned Value. 근거 상태: partial; 공식 목차 또는 개요 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 실제 시작/종료·잔여 기간·실제/잔여 units
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '실제 시작/종료·잔여 기간·실제/잔여 units' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 실제 시작/종료·잔여 기간·실제/잔여 units의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: Data Date와 자동 실제 적용 정책. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 계획→진행→완료 전환의 불변조건 검증. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-04 / J-07 / 단계 D; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S03](https://docs.oracle.com/cd/G18296_01/English/User_Guides/p6_pro_user/toc.htm), [OR-S08](https://docs.oracle.com/cd/E80480_01/English/user_guides/schedule_management_user_guide/toc.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U03, oracle:U04, oracle:U07, oracle:U08, oracle:U13. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-065 · Progress Spotlight·실적 적용
- 원본: ORA-065 / Oracle Primavera 제품군 / PP, PC; OC 방식 별도 / Baseline과 진척 및 Earned Value. 근거 상태: partial; 공식 목차 또는 개요 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: Progress Spotlight·실적 적용
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'Progress Spotlight·실적 적용' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: Progress Spotlight·실적 적용의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 자동 추정과 실제 보고 구분. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 선택 update window 외 활동은 변경하지 않음. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-05 / J-08 / 단계 C; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S03](https://docs.oracle.com/cd/G18296_01/English/User_Guides/p6_pro_user/toc.htm), [OR-S08](https://docs.oracle.com/cd/E80480_01/English/user_guides/schedule_management_user_guide/toc.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U03, oracle:U04, oracle:U07, oracle:U08, oracle:U10, oracle:U13. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-066 · 기간 실적 저장
- 원본: ORA-066 / Oracle Primavera 제품군 / P6, OC-S / Baseline과 진척 및 Earned Value. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 기간 실적 저장
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '기간 실적 저장' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 기간 실적 저장의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 회계기간 사전 필요. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 당기/누계 실적·EV/PV 저장 후 다음 기간 초기화. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-05 / J-08 / 단계 C; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S23](https://docs.oracle.com/cd/G18296_01/English/User_Guides/p6_pro_user/store_period_performance.htm), [OR-S08](https://docs.oracle.com/cd/E80480_01/English/user_guides/schedule_management_user_guide/toc.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U03, oracle:U04, oracle:U07, oracle:U08, oracle:U13. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-067 · EVM 기본값 PV·EV·AC·BAC
- 원본: ORA-067 / Oracle Primavera 제품군 / P6, OC-S / Baseline과 진척 및 Earned Value. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: EVM 기본값 PV·EV·AC·BAC
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'EVM 기본값 PV·EV·AC·BAC' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: EVM 기본값 PV·EV·AC·BAC의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: baseline·cost recalculation 필요. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 고정된 수치 fixture에서 합계·baseline 참조 검증. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-10 / 단계 D; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S22](https://docs.oracle.com/cd/G18296_01/English/User_Guides/p6_pro_user/earned_value.htm), [OR-S26](https://docs.oracle.com/cd/E80480_01/English/user_guides/schedule_management_user_guide/233373.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U03, oracle:U04, oracle:U07, oracle:U08, oracle:U13. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-068 · CV·SV·CPI·SPI·ETC·EAC 분석
- 원본: ORA-068 / Oracle Primavera 제품군 / P6, OC-S / Baseline과 진척 및 Earned Value. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: CV·SV·CPI·SPI·ETC·EAC 분석
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'CV·SV·CPI·SPI·ETC·EAC 분석' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: CV·SV·CPI·SPI·ETC·EAC 분석의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 공식별 정의/단위 상세 비교 후 확정. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 0 분모·초과 진척·완료 프로젝트 예외 검증. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-10 / 단계 D; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S22](https://docs.oracle.com/cd/G18296_01/English/User_Guides/p6_pro_user/earned_value.htm), [OR-S26](https://docs.oracle.com/cd/E80480_01/English/user_guides/schedule_management_user_guide/233373.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U03, oracle:U04, oracle:U07, oracle:U08, oracle:U13. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-069 · WBS별 performance/ETC 규칙
- 원본: ORA-069 / Oracle Primavera 제품군 / P6 / Baseline과 진척 및 Earned Value. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: WBS별 performance/ETC 규칙
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'WBS별 performance/ETC 규칙' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: WBS별 performance/ETC 규칙의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: OC에 P6 WBS 규칙 직접 동등 기능 없음. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: WBS마다 다른 산식 선택과 상위 rollup 검증. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-04 / J-07 / 단계 D; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S27](https://docs.oracle.com/cd/G18296_01/English/User_Guides/p6_pro_user/define_earned_value_settings_for_a_specific_work_breakdown_structure_element.htm), [OR-S02](https://docs.oracle.com/cd/E80480_01/English/admin/p6_eppm_migration_guide/246421.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U03, oracle:U04, oracle:U07, oracle:U08, oracle:U13. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-070 · Assignment→활동→WBS→프로젝트 EVM 집계
- 원본: ORA-070 / Oracle Primavera 제품군 / OC-S / Baseline과 진척 및 Earned Value. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: Assignment→활동→WBS→프로젝트 EVM 집계
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'Assignment→활동→WBS→프로젝트 EVM 집계' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: Assignment→활동→WBS→프로젝트 EVM 집계의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 공식 baseline 선택·비용 재계산 순서. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 배정 없는 활동과 있는 활동 모두 합계 검증. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-10 / 단계 D; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S26](https://docs.oracle.com/cd/E80480_01/English/user_guides/schedule_management_user_guide/233373.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U03, oracle:U04, oracle:U07, oracle:U08, oracle:U10, oracle:U13. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-071 · 자원단가×수량 비용과 배정 rate type
- 원본: ORA-071 / Oracle Primavera 제품군 / P6, OC-S / 비용과 예산 및 자금. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 자원단가×수량 비용과 배정 rate type
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '자원단가×수량 비용과 배정 rate type' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 자원단가×수량 비용과 배정 rate type의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 원통화·환율·시간 단위. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 단가 출처·변경일·수량에 따른 비용 재현. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-09 / 단계 D; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S84](https://docs.oracle.com/cd/G18296_01/English/User_Guides/p6_pro_user/calculating_cost_when_using_multiple_resource_rates.htm), [OR-S89](https://docs.oracle.com/cd/E80480_01/English/user_guides/schedule_management_user_guide/221658.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-072 · 활동 비용 직접 입력
- 원본: ORA-072 / Oracle Primavera 제품군 / OC-S / 비용과 예산 및 자금. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 활동 비용 직접 입력
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '활동 비용 직접 입력' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 활동 비용 직접 입력의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: P6 expense와 동일 객체 아님. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 배정 없이도 비용과 EVM 연결. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-09 / 단계 D; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S89](https://docs.oracle.com/cd/E80480_01/English/user_guides/schedule_management_user_guide/221658.htm), [OR-S26](https://docs.oracle.com/cd/E80480_01/English/user_guides/schedule_management_user_guide/233373.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-073 · Expense·발생 시점·비용 계정
- 원본: ORA-073 / Oracle Primavera 제품군 / P6 / 비용과 예산 및 자금. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: Expense·발생 시점·비용 계정
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'Expense·발생 시점·비용 계정' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: Expense·발생 시점·비용 계정의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: OC CBS와 구조 차이. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 시작/종료/균등 발생 비용 분포 검증. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-09 / 단계 D; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S74](https://docs.oracle.com/cd/G18296_01/client_help/en_US/specify_an_accrual_type_for_an_expense.htm), [OR-S07](https://docs.oracle.com/cd/E80480_01/English/admin/p6_eppm_migration_guide/246408.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-074 · P6 EPS·프로젝트·WBS 예산
- 원본: ORA-074 / Oracle Primavera 제품군 / P6 / 비용과 예산 및 자금. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: P6 EPS·프로젝트·WBS 예산
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'P6 EPS·프로젝트·WBS 예산' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: P6 EPS·프로젝트·WBS 예산의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: Cloud 예산 이관 보장 불가. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: top-down 배분과 bottom-up 합계 차이 표시. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-09 / 단계 D; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S90](https://docs.oracle.com/cd/G18296_01/English/User_Guides/p6_pro_user/using_a_wbs_for_top_down_planning.htm), [OR-S93](https://docs.oracle.com/cd/G18296_01/English/User_Guides/p6_pro_user/establishing_budgets.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U10. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-075 · top-down 공수 추정
- 원본: ORA-075 / Oracle Primavera 제품군 / PP 기능 기준 / 비용과 예산 및 자금. 근거 상태: partial; 공식 목차 또는 개요 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: top-down 공수 추정
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'top-down 공수 추정' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: top-down 공수 추정의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 일반 예산 입력과 구분. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 경험/기능점수 추정 입력과 배분 재현. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-09 / 단계 D; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S03](https://docs.oracle.com/cd/G18296_01/English/User_Guides/p6_pro_user/toc.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-076 · 월별 지출·benefit 계획과 budget change
- 원본: ORA-076 / Oracle Primavera 제품군 / P6, OC / 비용과 예산 및 자금. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 월별 지출·benefit 계획과 budget change
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '월별 지출·benefit 계획과 budget change' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 월별 지출·benefit 계획과 budget change의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 변경 승인 여부 제품별 다름. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 원예산·변경·현재 예산과 실적 구분. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-09 / 단계 D; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S80](https://docs.oracle.com/cd/G18296_01/English/User_Guides/p6_pro_user/62483.htm), [OR-S28](https://docs.oracle.com/cd/E80480_01/help/en/user/142082.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-077 · CBS 비용 구조와 Cost Sheet
- 원본: ORA-077 / Oracle Primavera 제품군 / OC-S, OC-P / 비용과 예산 및 자금. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: CBS 비용 구조와 Cost Sheet
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'CBS 비용 구조와 Cost Sheet' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: CBS 비용 구조와 Cost Sheet의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: P6 cost account와 일대일 아님. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 코드/카테고리별 비용 drill-down과 rollup 검증. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-09 / 단계 D; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S28](https://docs.oracle.com/cd/E80480_01/help/en/user/142082.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-078 · 예산 line item·변경·전용 승인
- 원본: ORA-078 / Oracle Primavera 제품군 / OC-S, OC-P / 비용과 예산 및 자금. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 예산 line item·변경·전용 승인
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '예산 line item·변경·전용 승인' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 예산 line item·변경·전용 승인의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: workflow·funding·CBS. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 미승인 전용은 현재 승인 예산에 반영 금지. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-09 / 단계 D; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S02](https://docs.oracle.com/cd/E80480_01/English/admin/p6_eppm_migration_guide/246421.htm), [OR-S28](https://docs.oracle.com/cd/E80480_01/help/en/user/142082.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-079 · actuals 수기·물량 기반 입력
- 원본: ORA-079 / Oracle Primavera 제품군 / OC-S, OC-P / 비용과 예산 및 자금. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: actuals 수기·물량 기반 입력
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'actuals 수기·물량 기반 입력' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: actuals 수기·물량 기반 입력의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: scope 실적 중복 반영 방지. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: pending/confirmed 지출과 원장 합계 대조. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-09 / 단계 D; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S28](https://docs.oracle.com/cd/E80480_01/help/en/user/142082.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-080 · 자금원 배정·사용약정·소진
- 원본: ORA-080 / Oracle Primavera 제품군 / OC-S, OC-P / 비용과 예산 및 자금. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 자금원 배정·사용약정·소진
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '자금원 배정·사용약정·소진' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 자금원 배정·사용약정·소진의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 세 상태는 구분 필요. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: allocation≥appropriation/consumption 정책과 잔액 검증. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-09 / 단계 D; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S29](https://docs.oracle.com/cd/E80480_01/English/user_guides/portfolio_management_user_guide/221619.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-081 · 자금 사용처 제한과 단계별 배분
- 원본: ORA-081 / Oracle Primavera 제품군 / OC-P / 비용과 예산 및 자금. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 자금 사용처 제한과 단계별 배분
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '자금 사용처 제한과 단계별 배분' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 자금 사용처 제한과 단계별 배분의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: workspace→portfolio/project. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 자금별 허용 프로젝트·연도 한도 위반 차단. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-09 / 단계 D; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S29](https://docs.oracle.com/cd/E80480_01/English/user_guides/portfolio_management_user_guide/221619.htm), [OR-S43](https://docs.oracle.com/cd/E80480_01/English/user_guides/portfolio_management_user_guide/113057.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-082 · cash flow·forecast·snapshot
- 원본: ORA-082 / Oracle Primavera 제품군 / OC-S, OC-P / 비용과 예산 및 자금. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: cash flow·forecast·snapshot
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'cash flow·forecast·snapshot' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: cash flow·forecast·snapshot의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: schedule·budget와 연결 선택. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 시간 분포 합계와 과거 snapshot 보존. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-09 / 단계 D; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S28](https://docs.oracle.com/cd/E80480_01/help/en/user/142082.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-083 · NPV·ROI 등 재무 평가
- 원본: ORA-083 / Oracle Primavera 제품군 / OC-S, OC-P; PW ROI 범위 / 비용과 예산 및 자금. 근거 상태: partial; 공식 비교표 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: NPV·ROI 등 재무 평가
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'NPV·ROI 등 재무 평가' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: NPV·ROI 등 재무 평가의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 회계/투자 의사결정 도구로 사용 전 별도 검수. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 할인율/시점 fixture로 현금흐름 검증. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-09 / 단계 D; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S02](https://docs.oracle.com/cd/E80480_01/English/admin/p6_eppm_migration_guide/246421.htm), [OR-S05](https://docs.oracle.com/cd/E80480_01/English/admin/p6_eppm_migration_guide/246420.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-084 · CAPEX/OPEX 비용 분류와 곡선
- 원본: ORA-084 / Oracle Primavera 제품군 / OC-S, OC-P / 비용과 예산 및 자금. 근거 상태: partial; 공식 비교표 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: CAPEX/OPEX 비용 분류와 곡선
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'CAPEX/OPEX 비용 분류와 곡선' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: CAPEX/OPEX 비용 분류와 곡선의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 계정 분류 정책 필요. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 자본/비용 항목별 합계와 기간 배분 일치. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-09 / 단계 D; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S02](https://docs.oracle.com/cd/E80480_01/English/admin/p6_eppm_migration_guide/246421.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U06. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-085 · 자금·예산 데이터 교환 경계
- 원본: ORA-085 / Oracle Primavera 제품군 / P6↔OC / 비용과 예산 및 자금. 근거 상태: partial; 공식 비교표 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 자금·예산 데이터 교환 경계
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '자금·예산 데이터 교환 경계' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 자금·예산 데이터 교환 경계의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 공식 문서상 직접 전송 불가 영역. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 불가 데이터의 누락 경고·별도 매핑 보고. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-15 / 단계 F; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S02](https://docs.oracle.com/cd/E80480_01/English/admin/p6_eppm_migration_guide/246421.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U08. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-086 · 리스크 등록·원인·영향·담당·상태
- 원본: ORA-086 / Oracle Primavera 제품군 / PP, PW, OC-S / 리스크와 정량 분석. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 리스크 등록·원인·영향·담당·상태
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '리스크 등록·원인·영향·담당·상태' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 리스크 등록·원인·영향·담당·상태의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: PC와 PP의 리스크 UI 범위 차이. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 위협·기회와 소유자/상태 변경 이력 검증. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-11 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S83](https://docs.oracle.com/cd/G18296_01/English/User_Guides/p6_pro_user/risks.htm), [OR-S30](https://docs.oracle.com/cd/E80480_01/English/user_guides/risk_management_user_guide/90778.htm), [OR-S02](https://docs.oracle.com/cd/E80480_01/English/admin/p6_eppm_migration_guide/246421.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U09. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-087 · 확률·영향 매트릭스·위험 점수
- 원본: ORA-087 / Oracle Primavera 제품군 / PP, PW, OC-S / 리스크와 정량 분석. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 확률·영향 매트릭스·위험 점수
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '확률·영향 매트릭스·위험 점수' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 확률·영향 매트릭스·위험 점수의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: PP 고정 평가표와 PW/OC 매트릭스 구성 범위 구분. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: threshold 경계값별 점수와 색상 검증. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-11 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S83](https://docs.oracle.com/cd/G18296_01/English/User_Guides/p6_pro_user/risks.htm), [OR-S30](https://docs.oracle.com/cd/E80480_01/English/user_guides/risk_management_user_guide/90778.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U09. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-088 · 대응 전/후 평가와 대응 action
- 원본: ORA-088 / Oracle Primavera 제품군 / PW, OC-S; PP 범위별 / 리스크와 정량 분석. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 대응 전/후 평가와 대응 action
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '대응 전/후 평가와 대응 action' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 대응 전/후 평가와 대응 action의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 일정/비용 반영 정책. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 대응 계획 변경 후 pre/post 값을 별도 보존. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-11 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S83](https://docs.oracle.com/cd/G18296_01/English/User_Guides/p6_pro_user/risks.htm), [OR-S30](https://docs.oracle.com/cd/E80480_01/English/user_guides/risk_management_user_guide/90778.htm), [OR-S33](https://docs.oracle.com/cd/E80480_01/help/en/user/88293.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U05, oracle:U09, oracle:U10. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-089 · 리스크 라이브러리와 제안/승인
- 원본: ORA-089 / Oracle Primavera 제품군 / OC-S / 리스크와 정량 분석. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 리스크 라이브러리와 제안/승인
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '리스크 라이브러리와 제안/승인' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 리스크 라이브러리와 제안/승인의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: workflow·권한. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: workspace 재사용 리스크와 프로젝트별 사본 분리. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-11 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S33](https://docs.oracle.com/cd/E80480_01/help/en/user/88293.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U09, oracle:U12. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-090 · 활동과 리스크 연결
- 원본: ORA-090 / Oracle Primavera 제품군 / PP, PW, OC-S / 리스크와 정량 분석. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 활동과 리스크 연결
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '활동과 리스크 연결' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 활동과 리스크 연결의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 미배정 리스크의 영향 처리 확인. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 하나의 리스크가 여러 활동에 미치는 영향 표현. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-11 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S83](https://docs.oracle.com/cd/G18296_01/English/User_Guides/p6_pro_user/risks.htm), [OR-S33](https://docs.oracle.com/cd/E80480_01/help/en/user/88293.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U09. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-091 · 활동 기간 불확실성
- 원본: ORA-091 / Oracle Primavera 제품군 / OC-S / 리스크와 정량 분석. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 활동 기간 불확실성
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '활동 기간 불확실성' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 활동 기간 불확실성의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 분포 종류·절단·상관 상세 후속 검증. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 낙관/최빈/비관 분포와 deterministic 값 구분. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-11 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S33](https://docs.oracle.com/cd/E80480_01/help/en/user/88293.htm), [OR-S32](https://docs.oracle.com/cd/E80480_01/English/user_guides/risk_management_user_guide/88298.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U09. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-092 · Monte Carlo 일정·비용 시뮬레이션
- 원본: ORA-092 / Oracle Primavera 제품군 / OC-S / 리스크와 정량 분석. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: Monte Carlo 일정·비용 시뮬레이션
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'Monte Carlo 일정·비용 시뮬레이션' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: Monte Carlo 일정·비용 시뮬레이션의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: P6 기본 기능 아님; 별도 PRA와도 구분. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 고정 fixture에서 통계 허용오차·재현성 검증. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-11 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S31](https://docs.oracle.com/cd/E80480_01/English/user_guides/risk_management_user_guide/88432.htm), [OR-S32](https://docs.oracle.com/cd/E80480_01/English/user_guides/risk_management_user_guide/88298.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U05, oracle:U09, oracle:U10. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-093 · 반복 수·수렴 조건·난수 옵션
- 원본: ORA-093 / Oracle Primavera 제품군 / OC-S / 리스크와 정량 분석. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 반복 수·수렴 조건·난수 옵션
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '반복 수·수렴 조건·난수 옵션' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 반복 수·수렴 조건·난수 옵션의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 수렴 사용 시 난수 옵션 제약. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 최대 반복과 수렴 종료 로그를 기록. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-11 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S32](https://docs.oracle.com/cd/E80480_01/English/user_guides/risk_management_user_guide/88298.htm), [OR-S63](https://docs.oracle.com/cd/E80480_01/help/en/user/97638.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U05, oracle:U09. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-094 · 목표 비용·완료일 달성 확률
- 원본: ORA-094 / Oracle Primavera 제품군 / OC-S / 리스크와 정량 분석. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 목표 비용·완료일 달성 확률
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '목표 비용·완료일 달성 확률' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 목표 비용·완료일 달성 확률의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 프로젝트/활동별 가용 결과 차이. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: P50/P80 등 분위수와 목표점 확률을 교차검증. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-11 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S34](https://docs.oracle.com/cd/E80480_01/help/en/user/162022.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U09, oracle:U10. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-095 · 일정·비용 공동 신뢰도와 scatter plot
- 원본: ORA-095 / Oracle Primavera 제품군 / OC-S / 리스크와 정량 분석. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 일정·비용 공동 신뢰도와 scatter plot
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '일정·비용 공동 신뢰도와 scatter plot' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 일정·비용 공동 신뢰도와 scatter plot의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: joint probability 모델. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 각각 80%와 동시 만족 80%를 혼동하지 않음. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-11 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S33](https://docs.oracle.com/cd/E80480_01/help/en/user/88293.htm), [OR-S34](https://docs.oracle.com/cd/E80480_01/help/en/user/162022.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U05, oracle:U09, oracle:U10. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-096 · Mean Impact·Risk Removal tornado
- 원본: ORA-096 / Oracle Primavera 제품군 / OC-S / 리스크와 정량 분석. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: Mean Impact·Risk Removal tornado
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'Mean Impact·Risk Removal tornado' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: Mean Impact·Risk Removal tornado의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 서로 다른 분석 결과. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 평균 기여와 위험 제거 민감도를 구분. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-11 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S34](https://docs.oracle.com/cd/E80480_01/help/en/user/162022.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U09. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-097 · 반복별 일정·간트·임계경로 탐색
- 원본: ORA-097 / Oracle Primavera 제품군 / OC-S / 리스크와 정량 분석. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 반복별 일정·간트·임계경로 탐색
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '반복별 일정·간트·임계경로 탐색' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 반복별 일정·간트·임계경로 탐색의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 문서상 첫 50 iterations 표시. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 특정 iteration의 원인·기간·비용 추적. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-03 / J-03 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S35](https://docs.oracle.com/cd/E80480_01/English/user_guides/risk_management_user_guide/240574.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U05, oracle:U09, oracle:U10. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-098 · 날씨 위험과 확률적 비근무 달력
- 원본: ORA-098 / Oracle Primavera 제품군 / OC-S / 리스크와 정량 분석. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 날씨 위험과 확률적 비근무 달력
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '날씨 위험과 확률적 비근무 달력' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 날씨 위험과 확률적 비근무 달력의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 기간 자체와 완료일 영향 구분. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 날씨로 인한 비근무와 기본 휴일 중복 처리 검증. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-03 + PRD-07 / J-03 + J-11 / 단계 B(결정론 달력 기반), D-E(확률 날씨 검증); Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S36](https://docs.oracle.com/cd/E80480_01/help/en/user/233361.htm), [OR-S34](https://docs.oracle.com/cd/E80480_01/help/en/user/162022.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U05, oracle:U09. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-099 · Risk Factor 기간/비용 배율과 상관
- 원본: ORA-099 / Oracle Primavera 제품군 / OC-S / 리스크와 정량 분석. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: Risk Factor 기간/비용 배율과 상관
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'Risk Factor 기간/비용 배율과 상관' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: Risk Factor 기간/비용 배율과 상관의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 100%=영향 없음; 정성분석에는 미지원. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 여러 factor 곱셈·공동/독립 sampling 검증. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-11 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S37](https://docs.oracle.com/cd/E80480_01/help/en/user/281075.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U09. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-100 · 프로그램 리스크 통합·정량분석 경계
- 원본: ORA-100 / Oracle Primavera 제품군 / OC-S / 리스크와 정량 분석. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 프로그램 리스크 통합·정량분석 경계
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '프로그램 리스크 통합·정량분석 경계' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 프로그램 리스크 통합·정량분석 경계의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: **program 수준 Monte Carlo 실행은 미지원**. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 프로그램 register와 개별 프로젝트 분석을 분리. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-15 / 단계 F; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S30](https://docs.oracle.com/cd/E80480_01/English/user_guides/risk_management_user_guide/90778.htm), [OR-S31](https://docs.oracle.com/cd/E80480_01/English/user_guides/risk_management_user_guide/88432.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U09, oracle:U11. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-101 · 수동/조건 기반 portfolio 구성
- 원본: ORA-101 / Oracle Primavera 제품군 / PW, OC-P; PP/PC 제한 / 포트폴리오와 프로그램 및 자본 계획. 근거 상태: partial; 공식 비교표 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 수동/조건 기반 portfolio 구성
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '수동/조건 기반 portfolio 구성' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 수동/조건 기반 portfolio 구성의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: PP portfolio는 프로젝트 열기 용도 중심. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 조건 변경 후 포함 프로젝트와 합계 검증. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-12 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S05](https://docs.oracle.com/cd/E80480_01/English/admin/p6_eppm_migration_guide/246420.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U10. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-102 · 다층 portfolio와 program 포함
- 원본: ORA-102 / Oracle Primavera 제품군 / OC-P / 포트폴리오와 프로그램 및 자본 계획. 근거 상태: partial; 공식 비교표 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 다층 portfolio와 program 포함
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '다층 portfolio와 program 포함' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 다층 portfolio와 program 포함의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 순환 참조 방지. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 중첩 portfolio 중복 집계 정책 확인. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-12 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S05](https://docs.oracle.com/cd/E80480_01/English/admin/p6_eppm_migration_guide/246420.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-103 · Idea→proposal→project 접수
- 원본: ORA-103 / Oracle Primavera 제품군 / OC-P / 포트폴리오와 프로그램 및 자본 계획. 근거 상태: partial; 공식 비교표 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: Idea→proposal→project 접수
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'Idea→proposal→project 접수' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: Idea→proposal→project 접수의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: P6에 동일 Ideas 기능 없음. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 승인 전/후 객체 생성과 연결 추적. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-12 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S65](https://docs.oracle.com/cd/E80480_01/English/admin/p6_eppm_migration_guide/246834.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-104 · 전략 계층·측정지표·프로젝트 기여
- 원본: ORA-104 / Oracle Primavera 제품군 / OC-P / 포트폴리오와 프로그램 및 자본 계획. 근거 상태: partial; 공식 비교표 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 전략 계층·측정지표·프로젝트 기여
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '전략 계층·측정지표·프로젝트 기여' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 전략 계층·측정지표·프로젝트 기여의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: measures·평가 사전. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 가중 기여점수와 전략별 달성률 검증. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-12 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S06](https://docs.oracle.com/cd/E80480_01/English/admin/p6_eppm_migration_guide/246409.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U10. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-105 · 평가 matrix·우선순위·scorecard
- 원본: ORA-105 / Oracle Primavera 제품군 / PW 일부, OC-P / 포트폴리오와 프로그램 및 자본 계획. 근거 상태: partial; 공식 비교표 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 평가 matrix·우선순위·scorecard
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '평가 matrix·우선순위·scorecard' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 평가 matrix·우선순위·scorecard의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: P6와 OC 범위 다름. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 동일 점수/가중치 입력의 순위 재현. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-12 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S05](https://docs.oracle.com/cd/E80480_01/English/admin/p6_eppm_migration_guide/246420.htm), [OR-S44](https://docs.oracle.com/cd/E80480_01/English/user_guides/portfolio_management_user_guide/toc.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-106 · Investor Map·bubble·trend chart
- 원본: ORA-106 / Oracle Primavera 제품군 / PW, OC-P 범위별 / 포트폴리오와 프로그램 및 자본 계획. 근거 상태: partial; 공식 비교표 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: Investor Map·bubble·trend chart
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'Investor Map·bubble·trend chart' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: Investor Map·bubble·trend chart의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 계산 시점·snapshot 기준. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 차트 점을 원본 프로젝트 값까지 추적. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-12 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S05](https://docs.oracle.com/cd/E80480_01/English/admin/p6_eppm_migration_guide/246420.htm), [OR-S44](https://docs.oracle.com/cd/E80480_01/English/user_guides/portfolio_management_user_guide/toc.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-107 · 예산 planning 다중 시나리오
- 원본: ORA-107 / Oracle Primavera 제품군 / OC-P / 포트폴리오와 프로그램 및 자본 계획. 근거 상태: partial; 공식 목차 또는 개요 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 예산 planning 다중 시나리오
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '예산 planning 다중 시나리오' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 예산 planning 다중 시나리오의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 프로젝트 예측 연결/해제. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 시나리오별 선택·예산·기간 독립성 검증. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-12 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S44](https://docs.oracle.com/cd/E80480_01/English/user_guides/portfolio_management_user_guide/toc.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U10. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-108 · Waterline 기반 선택
- 원본: ORA-108 / Oracle Primavera 제품군 / PW, OC-P / 포트폴리오와 프로그램 및 자본 계획. 근거 상태: partial; 공식 비교표 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: Waterline 기반 선택
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'Waterline 기반 선택' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: Waterline 기반 선택의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 단순 정렬과 최적화 구분. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 한도에서 포함/제외 경계와 dependency 경고. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-12 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S05](https://docs.oracle.com/cd/E80480_01/English/admin/p6_eppm_migration_guide/246420.htm), [OR-S44](https://docs.oracle.com/cd/E80480_01/English/user_guides/portfolio_management_user_guide/toc.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-109 · Efficient Frontier 최적 조합
- 원본: ORA-109 / Oracle Primavera 제품군 / OC-P / 포트폴리오와 프로그램 및 자본 계획. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: Efficient Frontier 최적 조합
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'Efficient Frontier 최적 조합' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: Efficient Frontier 최적 조합의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: total/time-phased 비용과 주/보조 목적. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 작은 문제를 전수조합으로 최적값 대조. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-12 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S43](https://docs.oracle.com/cd/E80480_01/English/user_guides/portfolio_management_user_guide/113057.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-110 · 자원 planning 시나리오·FTE
- 원본: ORA-110 / Oracle Primavera 제품군 / OC-P / 포트폴리오와 프로그램 및 자본 계획. 근거 상태: partial; 공식 비교표 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 자원 planning 시나리오·FTE
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '자원 planning 시나리오·FTE' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 자원 planning 시나리오·FTE의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 자원 capacity·계획 단위. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 역할 수요·기간별 가용량·선택 프로젝트 검증. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-12 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S05](https://docs.oracle.com/cd/E80480_01/English/admin/p6_eppm_migration_guide/246420.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-111 · 계획 검토·제안·승인 workflow
- 원본: ORA-111 / Oracle Primavera 제품군 / OC-P / 포트폴리오와 프로그램 및 자본 계획. 근거 상태: partial; 공식 목차 또는 개요 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 계획 검토·제안·승인 workflow
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '계획 검토·제안·승인 workflow' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 계획 검토·제안·승인 workflow의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: workflow·사용자 역할. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 승인된 시나리오와 working copy 구분. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-12 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S44](https://docs.oracle.com/cd/E80480_01/English/user_guides/portfolio_management_user_guide/toc.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U10. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-112 · 프로그램 일정·마일스톤·비용·위험 통합
- 원본: ORA-112 / Oracle Primavera 제품군 / P6 일부, OC / 포트폴리오와 프로그램 및 자본 계획. 근거 상태: partial; 공식 비교표 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 프로그램 일정·마일스톤·비용·위험 통합
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '프로그램 일정·마일스톤·비용·위험 통합' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 프로그램 일정·마일스톤·비용·위험 통합의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: OC 프로젝트 간 관계는 같은 program 제약. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 프로젝트별 데이터 drill-down과 총계 검증. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-15 / 단계 F; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S64](https://docs.oracle.com/cd/E80480_01/English/admin/p6_eppm_migration_guide/246422.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U05, oracle:U09, oracle:U10, oracle:U11. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-113 · PBS 제품 분해 구조
- 원본: ORA-113 / Oracle Primavera 제품군 / OC-S / Scope와 설치 물량 및 작업 패키지. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: PBS 제품 분해 구조
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'PBS 제품 분해 구조' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: PBS 제품 분해 구조의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: PBS≠WBS. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 제품/시설 구성과 WBS 작업 구조를 독립 관리. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-02 / J-01 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S41](https://docs.oracle.com/cd/E80480_01/English/user_guides/scope_management_user_guide/88366.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-114 · scope item·자재·납품물 목록
- 원본: ORA-114 / Oracle Primavera 제품군 / OC-S / Scope와 설치 물량 및 작업 패키지. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: scope item·자재·납품물 목록
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'scope item·자재·납품물 목록' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: scope item·자재·납품물 목록의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 상세 일정 없이도 추적 가능. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: item별 수량·단위·비용/분류 보존. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-02 / J-01 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S41](https://docs.oracle.com/cd/E80480_01/English/user_guides/scope_management_user_guide/88366.htm), [OR-S66](https://docs.oracle.com/cd/E80480_01/English/user_guides/scope_management_user_guide/toc.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U05, oracle:U10. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-115 · work package와 scope assignment
- 원본: ORA-115 / Oracle Primavera 제품군 / OC-S / Scope와 설치 물량 및 작업 패키지. 근거 상태: partial; 공식 목차 또는 개요 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: work package와 scope assignment
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'work package와 scope assignment' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: work package와 scope assignment의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 수량 단위 일치. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 한 item의 여러 패키지 배정 합계 검증. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-02 / J-01 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S66](https://docs.oracle.com/cd/E80480_01/English/user_guides/scope_management_user_guide/toc.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-116 · 물량·공수·비용의 time-phasing
- 원본: ORA-116 / Oracle Primavera 제품군 / OC-S / Scope와 설치 물량 및 작업 패키지. 근거 상태: partial; 공식 목차 또는 개요 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 물량·공수·비용의 time-phasing
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '물량·공수·비용의 time-phasing' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 물량·공수·비용의 time-phasing의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: calendar·reporting period. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: curve/수동 분포의 전체 합계 보존. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-02 / J-01 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S66](https://docs.oracle.com/cd/E80480_01/English/user_guides/scope_management_user_guide/toc.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-117 · 설치 수량·실제 공수·비용 진척
- 원본: ORA-117 / Oracle Primavera 제품군 / OC-S / Scope와 설치 물량 및 작업 패키지. 근거 상태: partial; 공식 목차 또는 개요 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 설치 수량·실제 공수·비용 진척
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '설치 수량·실제 공수·비용 진척' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 설치 수량·실제 공수·비용 진척의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 시공 수량 기반 실적. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 누적·당기·잔여 물량을 대조. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-05 / J-08 / 단계 C; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S66](https://docs.oracle.com/cd/E80480_01/English/user_guides/scope_management_user_guide/toc.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U07, oracle:U13. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-118 · Rules of Credit 마일스톤 가중 진척
- 원본: ORA-118 / Oracle Primavera 제품군 / OC-S / Scope와 설치 물량 및 작업 패키지. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: Rules of Credit 마일스톤 가중 진척
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'Rules of Credit 마일스톤 가중 진척' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: Rules of Credit 마일스톤 가중 진척의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: work package의 진척 측정 방법. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 조건별 부분/완료 인정과 가중 합 검증. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-05 / J-08 / 단계 C; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S40](https://docs.oracle.com/cd/E80480_01/English/user_guides/scope_management_user_guide/89290.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U07, oracle:U13. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-119 · scope와 일정 연결
- 원본: ORA-119 / Oracle Primavera 제품군 / OC-S / Scope와 설치 물량 및 작업 패키지. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: scope와 일정 연결
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'scope와 일정 연결' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: scope와 일정 연결의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 양방향 계산 루프 방지. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 활동 날짜 상속 및 scope→활동 진척 반영 검증. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-02 / J-01 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S41](https://docs.oracle.com/cd/E80480_01/English/user_guides/scope_management_user_guide/88366.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U05, oracle:U10. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-120 · scope original/current baseline
- 원본: ORA-120 / Oracle Primavera 제품군 / OC-S / Scope와 설치 물량 및 작업 패키지. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: scope original/current baseline
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'scope original/current baseline' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: scope original/current baseline의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: schedule baseline와 관련되나 다른 필드. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 원본 승인 물량과 승인 변경량 분리. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-04 / J-07 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S21](https://docs.oracle.com/cd/E80480_01/English/user_guides/schedule_management_user_guide/89344.htm), [OR-S42](https://docs.oracle.com/cd/E80480_01/English/user_guides/scope_management_user_guide/89825.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U03, oracle:U04, oracle:U08. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-121 · scope 변경요청·승인·전용
- 원본: ORA-121 / Oracle Primavera 제품군 / OC-S / Scope와 설치 물량 및 작업 패키지. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: scope 변경요청·승인·전용
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'scope 변경요청·승인·전용' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: scope 변경요청·승인·전용의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 새 current baseline 전 원본 유지. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 미승인 변경은 실행값에 미반영. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-02 / J-01 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S42](https://docs.oracle.com/cd/E80480_01/English/user_guides/scope_management_user_guide/89825.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U03, oracle:U04, oracle:U08. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-122 · 변경 철회·재개와 증빙
- 원본: ORA-122 / Oracle Primavera 제품군 / OC-S / Scope와 설치 물량 및 작업 패키지. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 변경 철회·재개와 증빙
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '변경 철회·재개와 증빙' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 변경 철회·재개와 증빙의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 실제 전환 권한과 감사 필요. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 승인 철회/거절 재개 후 재검토 이력 보존. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-02 / J-01 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S42](https://docs.oracle.com/cd/E80480_01/English/user_guides/scope_management_user_guide/89825.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U12. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-123 · scope 보고기간 마감·P6 연결
- 원본: ORA-123 / Oracle Primavera 제품군 / OC-S + integration / Scope와 설치 물량 및 작업 패키지. 근거 상태: partial; 공식 목차 또는 개요 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: scope 보고기간 마감·P6 연결
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'scope 보고기간 마감·P6 연결' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: scope 보고기간 마감·P6 연결의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: Primavera Gateway 등 별도 연결. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 기간 마감 후 실적 정합성·동기화 중복 검증. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-02 / J-01 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S66](https://docs.oracle.com/cd/E80480_01/English/user_guides/scope_management_user_guide/toc.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U10, oracle:U11, oracle:U15. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-124 · 활동과 별도인 task·task milestone
- 원본: ORA-124 / Oracle Primavera 제품군 / OC-T / 현장 단기 계획과 Lean Tasks. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 활동과 별도인 task·task milestone
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '활동과 별도인 task·task milestone' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 활동과 별도인 task·task milestone의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: P6 activity와 1:1 강제 금지. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: task를 활동에 연결하거나 독립 운영. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-13 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S38](https://docs.oracle.com/cd/E80480_01/English/user_guides/task_management_user_guide/207799.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-125 · 주간 계획 board·swimlane
- 원본: ORA-125 / Oracle Primavera 제품군 / OC-T / 현장 단기 계획과 Lean Tasks. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 주간 계획 board·swimlane
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '주간 계획 board·swimlane' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 주간 계획 board·swimlane의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: task plan 저장·권한. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 회사/담당/WBS별 같은 task의 일관된 표시. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-13 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S38](https://docs.oracle.com/cd/E80480_01/English/user_guides/task_management_user_guide/207799.htm), [OR-S67](https://docs.oracle.com/cd/E80480_01/English/user_guides/task_management_user_guide/toc.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U12. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-126 · push/pull 계획과 hand-off
- 원본: ORA-126 / Oracle Primavera 제품군 / OC-T / 현장 단기 계획과 Lean Tasks. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: push/pull 계획과 hand-off
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'push/pull 계획과 hand-off' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: push/pull 계획과 hand-off의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: CPM SF 4종과 달리 task hand-off 3종. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 역방향 마일스톤 계획과 전후 task 영향 검증. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-13 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S38](https://docs.oracle.com/cd/E80480_01/English/user_guides/task_management_user_guide/207799.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U05. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-127 · hand-off slack·lag·keep/break
- 원본: ORA-127 / Oracle Primavera 제품군 / OC-T / 현장 단기 계획과 Lean Tasks. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: hand-off slack·lag·keep/break
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'hand-off slack·lag·keep/break' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: hand-off slack·lag·keep/break의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 활동 float와 별도 계산. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 날짜 변경 시 관계 유지/해제 결과 검증. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-03 / J-03 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S38](https://docs.oracle.com/cd/E80480_01/English/user_guides/task_management_user_guide/207799.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-128 · task longest path
- 원본: ORA-128 / Oracle Primavera 제품군 / OC-T / 현장 단기 계획과 Lean Tasks. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: task longest path
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'task longest path' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: task longest path의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: CPM critical path와 구분. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 종료 task milestone까지 최장 경로 확인. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-13 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S38](https://docs.oracle.com/cd/E80480_01/English/user_guides/task_management_user_guide/207799.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U05. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-129 · 완료 약속·재약속·미준수 사유
- 원본: ORA-129 / Oracle Primavera 제품군 / OC-T / 현장 단기 계획과 Lean Tasks. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 완료 약속·재약속·미준수 사유
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '완료 약속·재약속·미준수 사유' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 완료 약속·재약속·미준수 사유의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 완료율과 commitment performance 구분. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 원약속·수정 약속·사유 이력 보존. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-13 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S38](https://docs.oracle.com/cd/E80480_01/English/user_guides/task_management_user_guide/207799.htm), [OR-S39](https://docs.oracle.com/cd/E80480_01/English/user_guides/task_management_user_guide/105747.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U07. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-130 · 예정일 제안과 수락
- 원본: ORA-130 / Oracle Primavera 제품군 / OC-T / 현장 단기 계획과 Lean Tasks. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 예정일 제안과 수락
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '예정일 제안과 수락' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 예정일 제안과 수락의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 사용자·회사 책임. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 협력사 간 날짜 영향 검토 후 반영. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-13 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S38](https://docs.oracle.com/cd/E80480_01/English/user_guides/task_management_user_guide/207799.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U10. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-131 · task constraint와 해소 관리
- 원본: ORA-131 / Oracle Primavera 제품군 / OC-T / 현장 단기 계획과 Lean Tasks. 근거 상태: partial; 공식 목차 또는 개요 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: task constraint와 해소 관리
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'task constraint와 해소 관리' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: task constraint와 해소 관리의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: schedule date constraint와 다른 객체. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 자재/도면/검사 등의 준비 조건별 담당·해소일. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-13 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S67](https://docs.oracle.com/cd/E80480_01/English/user_guides/task_management_user_guide/toc.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U16. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-132 · crew size 추정
- 원본: ORA-132 / Oracle Primavera 제품군 / OC-T / 현장 단기 계획과 Lean Tasks. 근거 상태: partial; 공식 목차 또는 개요 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: crew size 추정
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'crew size 추정' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: crew size 추정의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 실제 자원 배정과 분리. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 작업별 인원 추정과 resource allocation 중복 방지. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-13 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S67](https://docs.oracle.com/cd/E80480_01/English/user_guides/task_management_user_guide/toc.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-133 · 공개/개인 task와 회사 책임
- 원본: ORA-133 / Oracle Primavera 제품군 / OC-T / 현장 단기 계획과 Lean Tasks. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 공개/개인 task와 회사 책임
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '공개/개인 task와 회사 책임' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 공개/개인 task와 회사 책임의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 공유된 commitment/hand-off 전환 제약. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 개인 task가 타사 사용자에게 보이지 않음. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-13 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S38](https://docs.oracle.com/cd/E80480_01/English/user_guides/task_management_user_guide/207799.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U05. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-134 · PPC·회사별 성과·지연사유 분석
- 원본: ORA-134 / Oracle Primavera 제품군 / OC-T / 현장 단기 계획과 Lean Tasks. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: PPC·회사별 성과·지연사유 분석
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'PPC·회사별 성과·지연사유 분석' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: PPC·회사별 성과·지연사유 분석의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 현재 일/주는 pending 처리 등 확인. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 완료 task/commitment 분모와 기준기간 재현. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-13 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S39](https://docs.oracle.com/cd/E80480_01/English/user_guides/task_management_user_guide/105747.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U07. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-135 · Team Member·My Activities 진척 보고
- 원본: ORA-135 / Oracle Primavera 제품군 / P6 Team Member, OC-G/OC-S / 협업과 운영 거버넌스 및 보고. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: Team Member·My Activities 진척 보고
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'Team Member·My Activities 진척 보고' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: Team Member·My Activities 진척 보고의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: P6 Team Member는 별도 구성. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 맡은 활동만 보고하고 변경 권한 검증. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-05 / J-08 / 단계 C; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S46](https://docs.oracle.com/cd/G18296_01/client_help/en_US/102615.htm), [OR-S61](https://docs.oracle.com/cd/E80480_01/English/admin/p6_eppm_migration_guide/246428.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U07, oracle:U10, oracle:U12, oracle:U13. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-136 · 타임시트 작성·대리 입력·승인/반려
- 원본: ORA-136 / Oracle Primavera 제품군 / P6 Team Member, OC-G/OC-S / 협업과 운영 거버넌스 및 보고. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 타임시트 작성·대리 입력·승인/반려
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '타임시트 작성·대리 입력·승인/반려' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 타임시트 작성·대리 입력·승인/반려의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: OC 승인자는 Schedule 라이선스 필요. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 승인 전/후 actual units 반영과 재제출 검증. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-05 / J-08 / 단계 C; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S46](https://docs.oracle.com/cd/G18296_01/client_help/en_US/102615.htm), [OR-S47](https://docs.oracle.com/cd/E80480_01/English/user_guides/progress_user_guide/216127.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U10, oracle:U12, oracle:U13. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-137 · 타임시트 단계·기간·hour type
- 원본: ORA-137 / Oracle Primavera 제품군 / P6 Team Member, OC-G/OC-S / 협업과 운영 거버넌스 및 보고. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 타임시트 단계·기간·hour type
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '타임시트 단계·기간·hour type' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 타임시트 단계·기간·hour type의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: OC는 1단계 승인; P6 옵션 다름. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 0/1/2 단계와 주간/월간 정책을 제품별 분리. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-05 / J-08 / 단계 C; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S46](https://docs.oracle.com/cd/G18296_01/client_help/en_US/102615.htm), [OR-S61](https://docs.oracle.com/cd/E80480_01/English/admin/p6_eppm_migration_guide/246428.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U10, oracle:U12, oracle:U13. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-138 · 활동/문서 discussion와 알림
- 원본: ORA-138 / Oracle Primavera 제품군 / PW, OC / 협업과 운영 거버넌스 및 보고. 근거 상태: partial; 공식 비교표 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 활동/문서 discussion와 알림
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '활동/문서 discussion와 알림' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 활동/문서 discussion와 알림의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 문서와 활동의 권한 분리. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 댓글 작성·수신자 접근·읽기/처리 상태 검증. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-13 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S02](https://docs.oracle.com/cd/E80480_01/English/admin/p6_eppm_migration_guide/246421.htm), [OR-S61](https://docs.oracle.com/cd/E80480_01/English/admin/p6_eppm_migration_guide/246428.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U10, oracle:U12. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-139 · issues·threshold 자동 탐지
- 원본: ORA-139 / Oracle Primavera 제품군 / P6 / 협업과 운영 거버넌스 및 보고. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: issues·threshold 자동 탐지
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'issues·threshold 자동 탐지' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: issues·threshold 자동 탐지의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: OC는 custom logs/alerts 대체 구조. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 임계치 초과 시 이슈 생성과 책임자 추적. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-13 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S81](https://docs.oracle.com/cd/G18296_01/English/User_Guides/p6_pro_user/issues.htm), [OR-S94](https://docs.oracle.com/cd/E80480_01/help/en/user/142244.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U10, oracle:U12. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-140 · Custom Logs와 template register
- 원본: ORA-140 / Oracle Primavera 제품군 / OC / 협업과 운영 거버넌스 및 보고. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: Custom Logs와 template register
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'Custom Logs와 template register' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: Custom Logs와 template register의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: P6 native issue 데이터와 직접 동등 아님. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: lessons learned/품질/이슈별 필드·첨부·뷰. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-13 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S94](https://docs.oracle.com/cd/E80480_01/help/en/user/142244.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U10, oracle:U12. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-141 · Work Product·문서/파일 연결
- 원본: ORA-141 / Oracle Primavera 제품군 / P6, OC / 협업과 운영 거버넌스 및 보고. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: Work Product·문서/파일 연결
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'Work Product·문서/파일 연결' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: Work Product·문서/파일 연결의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: P6 reference와 OC repository 차이. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 활동/WBS 문서 링크의 무결성과 접근 검증. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-13 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S82](https://docs.oracle.com/cd/G18296_01/English/User_Guides/p6_pro_user/work_products_and_documents.htm), [OR-S08](https://docs.oracle.com/cd/E80480_01/English/user_guides/schedule_management_user_guide/toc.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U10, oracle:U12. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-142 · 파일 check-out/in·preview·권한
- 원본: ORA-142 / Oracle Primavera 제품군 / OC / 협업과 운영 거버넌스 및 보고. 근거 상태: partial; 공식 목차 또는 개요 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 파일 check-out/in·preview·권한
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '파일 check-out/in·preview·권한' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 파일 check-out/in·preview·권한의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 저장소/파일 형식별 기능 확인. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 잠금 중 편집·버전 충돌·권한 우회 방지. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-16 / 단계 C; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S08](https://docs.oracle.com/cd/E80480_01/English/user_guides/schedule_management_user_guide/toc.htm), [OR-S95](https://docs.oracle.com/cd/E80480_01/help/en/user/toc.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U10, oracle:U12. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-143 · Workflow 설계·분기·하위 workflow
- 원본: ORA-143 / Oracle Primavera 제품군 / OC-S, OC-P / 협업과 운영 거버넌스 및 보고. 근거 상태: partial; 공식 목차 또는 개요 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: Workflow 설계·분기·하위 workflow
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'Workflow 설계·분기·하위 workflow' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: Workflow 설계·분기·하위 workflow의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: workflow 모델·폼·권한. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 수동/자동 결정과 중첩 실행 결과 검증. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-13 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S45](https://docs.oracle.com/cd/E80480_01/English/user_guides/workflow_design_guide/toc.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U10, oracle:U12. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-144 · Form 설계·검증·publish
- 원본: ORA-144 / Oracle Primavera 제품군 / OC-S, OC-P / 협업과 운영 거버넌스 및 보고. 근거 상태: partial; 공식 목차 또는 개요 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: Form 설계·검증·publish
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'Form 설계·검증·publish' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: Form 설계·검증·publish의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 미배포 변경의 실행 인스턴스 영향. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 필수값·table/file field·배포 버전 검증. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-13 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S45](https://docs.oracle.com/cd/E80480_01/English/user_guides/workflow_design_guide/toc.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U10, oracle:U12. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-145 · Workflow 모니터·중단·로그
- 원본: ORA-145 / Oracle Primavera 제품군 / OC-S, OC-P / 협업과 운영 거버넌스 및 보고. 근거 상태: partial; 공식 목차 또는 개요 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: Workflow 모니터·중단·로그
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'Workflow 모니터·중단·로그' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: Workflow 모니터·중단·로그의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 실행 상태·서비스 연동. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 실패/중단/재배정 로그와 담당자 추적. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-13 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S45](https://docs.oracle.com/cd/E80480_01/English/user_guides/workflow_design_guide/toc.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U10, oracle:U11, oracle:U12. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-146 · 사용자·보안 profile/permission set
- 원본: ORA-146 / Oracle Primavera 제품군 / P6, OC / 협업과 운영 거버넌스 및 보고. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 사용자·보안 profile/permission set
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '사용자·보안 profile/permission set' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 사용자·보안 profile/permission set의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: OBS와 OC 사용자 그룹은 별개. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 읽기·편집·비용 조회·관리 권한 경계 테스트. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-16 / 단계 C; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S07](https://docs.oracle.com/cd/E80480_01/English/admin/p6_eppm_migration_guide/246408.htm), [OR-S50](https://docs.oracle.com/cd/E80480_01/help/en/user/154358.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U10, oracle:U12. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-147 · 백그라운드·예약 서비스
- 원본: ORA-147 / Oracle Primavera 제품군 / P6 구성별, OC / 협업과 운영 거버넌스 및 보고. 근거 상태: partial; 공식 목차 또는 개요 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 백그라운드·예약 서비스
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '백그라운드·예약 서비스' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 백그라운드·예약 서비스의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 스케줄·요약·보고·알림 대상 구분. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 실행 중/실패/완료 상태와 재시도 중복 검증. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-13 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S03](https://docs.oracle.com/cd/G18296_01/English/User_Guides/p6_pro_user/toc.htm), [OR-S08](https://docs.oracle.com/cd/E80480_01/English/user_guides/schedule_management_user_guide/toc.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U10, oracle:U12. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-148 · Dashboard·scorecard·차트
- 원본: ORA-148 / Oracle Primavera 제품군 / PW, OC / 협업과 운영 거버넌스 및 보고. 근거 상태: partial; 공식 비교표 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: Dashboard·scorecard·차트
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'Dashboard·scorecard·차트' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: Dashboard·scorecard·차트의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: PP dashboard 지원 표는 별도 UI 확인 필요. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 카드 수치→원본 객체 drill-down 검증. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-13 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S56](https://docs.oracle.com/cd/E80480_01/English/admin/p6_eppm_migration_guide/246426.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U10, oracle:U12. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-149 · 사용자 보고서·BI Publisher·템플릿
- 원본: ORA-149 / Oracle Primavera 제품군 / P6 구성별, OC / 협업과 운영 거버넌스 및 보고. 근거 상태: partial; 공식 비교표 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 사용자 보고서·BI Publisher·템플릿
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '사용자 보고서·BI Publisher·템플릿' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 사용자 보고서·BI Publisher·템플릿의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: BI Publisher 배포/권한 별도. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 파라미터·필터·집계·권한별 보고 결과 검증. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-13 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S55](https://docs.oracle.com/cd/E80480_01/English/admin/p6_eppm_migration_guide/246424.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U10, oracle:U12. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-150 · Visualizer 간트·Timescaled Logic Diagram
- 원본: ORA-150 / Oracle Primavera 제품군 / PP/PC + Visualizer / 협업과 운영 거버넌스 및 보고. 근거 상태: partial; 공식 비교표 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: Visualizer 간트·Timescaled Logic Diagram
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'Visualizer 간트·Timescaled Logic Diagram' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: Visualizer 간트·Timescaled Logic Diagram의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: OC 동일 Visualizer 제품 기능 아님. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 대형 일정 인쇄·논리도·페이지 경계 확인. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-13 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S55](https://docs.oracle.com/cd/E80480_01/English/admin/p6_eppm_migration_guide/246424.htm), [OR-S68](https://docs.oracle.com/en/industries/construction-engineering/primavera-p6-professional/26/user.html)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U10, oracle:U12. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-151 · 모바일·오프라인 진행 보고
- 원본: ORA-151 / Oracle Primavera 제품군 / P6 mobile, OC mobile / 협업과 운영 거버넌스 및 보고. 근거 상태: partial; 공식 비교표 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 모바일·오프라인 진행 보고
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '모바일·오프라인 진행 보고' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 모바일·오프라인 진행 보고의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: OS별 첨부/마크업 차이, OC mobile timesheet 제약. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: offline 수정→sync 충돌·첨부·권한 검증. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-13 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S57](https://docs.oracle.com/cd/E80480_01/English/admin/p6_eppm_migration_guide/246429.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U05, oracle:U10, oracle:U12, oracle:U13. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-152 · 사용자 검색·Inbox·proxy·preferences
- 원본: ORA-152 / Oracle Primavera 제품군 / OC; P6 일부 / 협업과 운영 거버넌스 및 보고. 근거 상태: partial; 공식 비교표 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 사용자 검색·Inbox·proxy·preferences
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '사용자 검색·Inbox·proxy·preferences' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 사용자 검색·Inbox·proxy·preferences의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: proxy 권한 범위·비밀 정보 노출 점검. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 대리 사용자 행위와 본인 행위를 구분 기록. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-13 / 단계 E; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S61](https://docs.oracle.com/cd/E80480_01/English/admin/p6_eppm_migration_guide/246428.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U10, oracle:U12. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-153 · XER 프로젝트 import/export
- 원본: ORA-153 / Oracle Primavera 제품군 / P6, OC-S / 데이터 교환과 통합. 근거 상태: partial; 공식 비교표 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: XER 프로젝트 import/export
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'XER 프로젝트 import/export' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: XER 프로젝트 import/export의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: XER baseline 미지원. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: ID·WBS·활동·관계·달력·자원 round-trip 비교. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-15 / 단계 F; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S49](https://docs.oracle.com/cd/G18296_01/English/admin/p6_pro_importing_exporting/toc.htm), [OR-S02](https://docs.oracle.com/cd/E80480_01/English/admin/p6_eppm_migration_guide/246421.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U03, oracle:U04, oracle:U08, oracle:U10, oracle:U11. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-154 · XER 자원/역할 교환
- 원본: ORA-154 / Oracle Primavera 제품군 / PP, PC; 기타 제품 세부 확인 / 데이터 교환과 통합. 근거 상태: partial; 공식 목차 또는 개요 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: XER 자원/역할 교환
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'XER 자원/역할 교환' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: XER 자원/역할 교환의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 이름 아닌 안정 ID 매칭. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 프로젝트와 별도로 사전 중복·충돌 처리. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-15 / 단계 F; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S49](https://docs.oracle.com/cd/G18296_01/English/admin/p6_pro_importing_exporting/toc.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U08, oracle:U11. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-155 · P6 XML과 baseline 교환
- 원본: ORA-155 / Oracle Primavera 제품군 / P6, OC-S / 데이터 교환과 통합. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: P6 XML과 baseline 교환
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'P6 XML과 baseline 교환' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: P6 XML과 baseline 교환의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: OC import baseline 수·시나리오 의미 제약. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: current/baseline 각 데이터 손실 보고. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-04 / J-07 / 단계 B; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S06](https://docs.oracle.com/cd/E80480_01/English/admin/p6_eppm_migration_guide/246409.htm), [OR-S48](https://docs.oracle.com/cd/E80480_01/English/admin/p6_import_guide/index.html)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U03, oracle:U04, oracle:U05, oracle:U08, oracle:U11. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-156 · Microsoft Project XML 교환
- 원본: ORA-156 / Oracle Primavera 제품군 / P6, OC / 데이터 교환과 통합. 근거 상태: partial; 공식 비교표 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: Microsoft Project XML 교환
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'Microsoft Project XML 교환' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: Microsoft Project XML 교환의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: MPP 직접 완전 호환으로 확대 해석 금지. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 일정·달력·자원·타입 매핑 차이 보고. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-15 / 단계 F; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S49](https://docs.oracle.com/cd/G18296_01/English/admin/p6_pro_importing_exporting/toc.htm), [OR-S02](https://docs.oracle.com/cd/E80480_01/English/admin/p6_eppm_migration_guide/246421.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U08, oracle:U11. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-157 · Microsoft Project MPX legacy 교환
- 원본: ORA-157 / Oracle Primavera 제품군 / PP, PC / 데이터 교환과 통합. 근거 상태: partial; 공식 비교표 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: Microsoft Project MPX legacy 교환
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'Microsoft Project MPX legacy 교환' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: Microsoft Project MPX legacy 교환의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: OC에는 같은 MPX 경로 없음. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 지원 버전과 손실 필드를 명시. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-15 / 단계 F; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S49](https://docs.oracle.com/cd/G18296_01/English/admin/p6_pro_importing_exporting/toc.htm), [OR-S02](https://docs.oracle.com/cd/E80480_01/English/admin/p6_eppm_migration_guide/246421.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U08, oracle:U11, oracle:U16. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-158 · Excel/XLSX 행 기반 import/export
- 원본: ORA-158 / Oracle Primavera 제품군 / P6, OC / 데이터 교환과 통합. 근거 상태: partial; 공식 비교표 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: Excel/XLSX 행 기반 import/export
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'Excel/XLSX 행 기반 import/export' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: Excel/XLSX 행 기반 import/export의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 데이터 영역별 지원 범위 다름. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: template·고유필드·수정/삭제 의미 검증. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-15 / 단계 F; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S49](https://docs.oracle.com/cd/G18296_01/English/admin/p6_pro_importing_exporting/toc.htm), [OR-S06](https://docs.oracle.com/cd/E80480_01/English/admin/p6_eppm_migration_guide/246409.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U08, oracle:U11. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-159 · SDEF 전환
- 원본: ORA-159 / Oracle Primavera 제품군 / PP, PC, OC 도구별 / 데이터 교환과 통합. 근거 상태: partial; 공식 비교표 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: SDEF 전환
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'SDEF 전환' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: SDEF 전환의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: converter 경유 여부 확인. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 표준 필드·날짜·code mapping round-trip. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-15 / 단계 F; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S49](https://docs.oracle.com/cd/G18296_01/English/admin/p6_pro_importing_exporting/toc.htm), [OR-S02](https://docs.oracle.com/cd/E80480_01/English/admin/p6_eppm_migration_guide/246421.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U08, oracle:U11. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-160 · UN/CEFACT·IPMDAR·CPP export
- 원본: ORA-160 / Oracle Primavera 제품군 / P6 구성별 / 데이터 교환과 통합. 근거 상태: partial; 공식 목차 또는 개요 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: UN/CEFACT·IPMDAR·CPP export
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'UN/CEFACT·IPMDAR·CPP export' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: UN/CEFACT·IPMDAR·CPP export의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 형식별 edition·버전 매핑 확인. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 계약 제출 형식별 validation와 누락 경고. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-15 / 단계 F; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S49](https://docs.oracle.com/cd/G18296_01/English/admin/p6_pro_importing_exporting/toc.htm), [OR-S69](https://docs.oracle.com/en/industries/construction-engineering/primavera-p6-project/26/p6-import-export/index.html)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U08, oracle:U11. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-161 · import 선택 업데이트·설정·검증
- 원본: ORA-161 / Oracle Primavera 제품군 / P6, OC / 데이터 교환과 통합. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: import 선택 업데이트·설정·검증
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'import 선택 업데이트·설정·검증' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: import 선택 업데이트·설정·검증의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 동일 이름의 다른 ID 관리. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: create/update·global data 충돌·거부 목록. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-15 / 단계 F; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S48](https://docs.oracle.com/cd/E80480_01/English/admin/p6_import_guide/index.html), [OR-S06](https://docs.oracle.com/cd/E80480_01/English/admin/p6_eppm_migration_guide/246409.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U08, oracle:U11. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-162 · 프로젝트 check-out/in·원격 편집
- 원본: ORA-162 / Oracle Primavera 제품군 / PP, PC / 데이터 교환과 통합. 근거 상태: partial; 공식 비교표 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 프로젝트 check-out/in·원격 편집
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '프로젝트 check-out/in·원격 편집' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 프로젝트 check-out/in·원격 편집의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: OC 파일 checkout과 다른 기능. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 체크아웃 잠금과 선택 check-in 충돌 검증. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-15 / 단계 F; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S03](https://docs.oracle.com/cd/G18296_01/English/User_Guides/p6_pro_user/toc.htm), [OR-S06](https://docs.oracle.com/cd/E80480_01/English/admin/p6_eppm_migration_guide/246409.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U08, oracle:U10, oracle:U11. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-163 · 명령행 XER import/export
- 원본: ORA-163 / Oracle Primavera 제품군 / PP/PC 배포별 / 데이터 교환과 통합. 근거 상태: partial; 공식 목차 또는 개요 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 명령행 XER import/export
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '명령행 XER import/export' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 명령행 XER import/export의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 데스크톱 배포·자격 증명·로그. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 동일 batch 재실행 시 중복 방지. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-15 / 단계 F; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S49](https://docs.oracle.com/cd/G18296_01/English/admin/p6_pro_importing_exporting/toc.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U08, oracle:U11. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-164 · P6 REST JSON API
- 원본: ORA-164 / Oracle Primavera 제품군 / P6 EPPM / 데이터 교환과 통합. 근거 상태: partial; 공식 목차 또는 개요 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: P6 REST JSON API
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'P6 REST JSON API' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: P6 REST JSON API의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: v26 endpoint별 coverage 후속. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 객체 CRUD·필터·pagination·권한 테스트. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-15 / 단계 F; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S51](https://docs.oracle.com/en/industries/construction-engineering/primavera-p6-project/26/integration.html)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U08, oracle:U11. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-165 · P6 SOAP Web Services
- 원본: ORA-165 / Oracle Primavera 제품군 / P6 EPPM / 데이터 교환과 통합. 근거 상태: partial; 공식 목차 또는 개요 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: P6 SOAP Web Services
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'P6 SOAP Web Services' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: P6 SOAP Web Services의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: REST와 같은 객체 범위인지 별도 확인. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: operation별 오류·트랜잭션·재시도 검증. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-15 / 단계 F; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S51](https://docs.oracle.com/en/industries/construction-engineering/primavera-p6-project/26/integration.html)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U08, oracle:U11. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-166 · P6 Java API·Business Object Events
- 원본: ORA-166 / Oracle Primavera 제품군 / P6 v25 on-premises 기준 / 데이터 교환과 통합. 근거 상태: partial; 공식 목차 또는 개요 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: P6 Java API·Business Object Events
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'P6 Java API·Business Object Events' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: P6 Java API·Business Object Events의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: v26 통합 포털에서 현황 별도 확인 필요. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 이벤트 중복·순서·클라이언트 호환 검증. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-15 / 단계 F; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S60](https://docs.oracle.com/cd/G18294_01/102564.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U08, oracle:U11. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-167 · Primavera Cloud REST API
- 원본: ORA-167 / Oracle Primavera 제품군 / OC / 데이터 교환과 통합. 근거 상태: partial; 공식 목차 또는 개요 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: Primavera Cloud REST API
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'Primavera Cloud REST API' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: Primavera Cloud REST API의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 모든 UI 기능이 API에 있다는 보장 없음. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: UI에서 가능한 핵심 동작의 API coverage 매핑. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-15 / 단계 F; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S62](https://docs.oracle.com/en/industries/construction-engineering/primavera-cloud/opcap/)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U08, oracle:U11. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-168 · Data Service 데이터 조회 API
- 원본: ORA-168 / Oracle Primavera 제품군 / P6 cloud, OC 각각 / 데이터 교환과 통합. 근거 상태: partial; 공식 목차 또는 개요 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: Data Service 데이터 조회 API
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'Data Service 데이터 조회 API' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: Data Service 데이터 조회 API의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: P6 cloud 서비스는 read-only; OC endpoint별 조회 범위 추가 확인. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 쓰기 API와 분리·추출 갱신지연 검증. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-15 / 단계 F; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S51](https://docs.oracle.com/en/industries/construction-engineering/primavera-p6-project/26/integration.html), [OR-S70](https://docs.oracle.com/en/industries/construction-engineering/primavera-cloud/data-service/rest-endpoints.html)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U08, oracle:U11. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-169 · Gateway·Oracle Integration 동기화
- 원본: ORA-169 / Oracle Primavera 제품군 / P6, OC, U 연결 구성 / 데이터 교환과 통합. 근거 상태: confirmed; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: Gateway·Oracle Integration 동기화
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'Gateway·Oracle Integration 동기화' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: Gateway·Oracle Integration 동기화의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: import와 integration은 다른 방식. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 양방향 매핑·충돌 소유권·재처리 검증. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-15 / 단계 F; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S48](https://docs.oracle.com/cd/E80480_01/English/admin/p6_import_guide/index.html), [OR-S51](https://docs.oracle.com/en/industries/construction-engineering/primavera-p6-project/26/integration.html)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U08, oracle:U11, oracle:U15. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-170 · 데이터 사전·schema·형식별 mapping
- 원본: ORA-170 / Oracle Primavera 제품군 / P6, OC / 데이터 교환과 통합. 근거 상태: partial; 공식 목차 또는 개요 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 데이터 사전·schema·형식별 mapping
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '데이터 사전·schema·형식별 mapping' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 데이터 사전·schema·형식별 mapping의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 확보한 목록만으로 전필드 parity 주장 금지. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 필드별 타입·null·enum·단위·precision 계약. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-15 / 단계 F; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S49](https://docs.oracle.com/cd/G18296_01/English/admin/p6_pro_importing_exporting/toc.htm), [OR-S71](https://docs.oracle.com/cd/E80480_01/toc.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U08, oracle:U11. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-171 · capital planning→운영/유지보수 lifecycle
- 원본: ORA-171 / Oracle Primavera 제품군 / U / 인접 제품 경계와 별도 확장 후보. 근거 상태: partial; 공식 목차 또는 개요 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: capital planning→운영/유지보수 lifecycle
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'capital planning→운영/유지보수 lifecycle' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: capital planning→운영/유지보수 lifecycle의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: P6/OC 핵심 일정 범위와 분리. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 프로젝트 인수 후 자산/시설 객체로 연결 검증. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-15 / 단계 F; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S58](https://docs.oracle.com/cd/G18097_01/English/User_Guides/user_general/index.html)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U05, oracle:U10, oracle:U11, oracle:U15. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-172 · 구성 가능한 business process·shell·권한
- 원본: ORA-172 / Oracle Primavera 제품군 / U / 인접 제품 경계와 별도 확장 후보. 근거 상태: partial; 공식 목차 또는 개요 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 구성 가능한 business process·shell·권한
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '구성 가능한 business process·shell·권한' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 구성 가능한 business process·shell·권한의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 고객별 customization이 큼. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 조직별 승인 흐름과 form 변경 이력 검증. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-16 / 단계 C; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S58](https://docs.oracle.com/cd/G18097_01/English/User_Guides/user_general/index.html)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U12, oracle:U15. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-173 · 비용·문서·portfolio·planning managers
- 원본: ORA-173 / Oracle Primavera 제품군 / U / 인접 제품 경계와 별도 확장 후보. 근거 상태: partial; 공식 목차 또는 개요 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 비용·문서·portfolio·planning managers
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '비용·문서·portfolio·planning managers' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 비용·문서·portfolio·planning managers의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: Unifier 기능군 별도 전체 조사 필요. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: manager별 데이터 원장과 승인 객체 정합성. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-15 / 단계 F; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S59](https://docs.oracle.com/cd/G18097_01/English/User_Guides/user_managers/index.html)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U14, oracle:U15. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-174 · Activity Manager·Space Manager
- 원본: ORA-174 / Oracle Primavera 제품군 / U / 인접 제품 경계와 별도 확장 후보. 근거 상태: partial; 공식 목차 또는 개요 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: Activity Manager·Space Manager
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'Activity Manager·Space Manager' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: Activity Manager·Space Manager의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 기존 Schedule/Resource Manager deprecated 주의. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 일정 통합과 공간/시설 관리 도메인 구분. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-15 / 단계 F; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S59](https://docs.oracle.com/cd/G18097_01/English/User_Guides/user_managers/index.html)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U15, oracle:U16. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-175 · Primavera Analytics·Risk Analysis 등 인접 제품
- 원본: ORA-175 / Oracle Primavera 제품군 / 별도 제품 / 인접 제품 경계와 별도 확장 후보. 근거 상태: confirmed; 인접 제품; 상세 범위 보류; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: Primavera Analytics·Risk Analysis 등 인접 제품
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'Primavera Analytics·Risk Analysis 등 인접 제품' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: Primavera Analytics·Risk Analysis 등 인접 제품의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 본 인벤토리는 독립 제품의 전 기능을 포함하지 않음. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 포함할 경우 별도 제품 버전과 데이터 계약 확정. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-15 / 단계 F; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S72](https://docs.oracle.com/en/industries/construction-engineering/)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U15. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-176 · 사용법 질의용 Help Assistant
- 원본: ORA-176 / Oracle Primavera 제품군 / OC 조건부 / 조건부 AI와 Legacy 기능 경계. 근거 상태: partial; 공식 상세문서 확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: 사용법 질의용 Help Assistant
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 '사용법 질의용 Help Assistant' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: 사용법 질의용 Help Assistant의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 지역 지원과 관리자 enable 필요; 자동 PM 의사결정 기능으로 확대 금지. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 근거 있는 도움말 답변·비활성화·권한 노출 검증. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-15 / 단계 F; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S73](https://docs.oracle.com/cd/E80480_01/293955.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U16. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-177 · Submittals 검토·상태·reviewer 협업
- 원본: ORA-177 / Oracle Primavera 제품군 / OC legacy 문서 / 조건부 AI와 Legacy 기능 경계. 근거 상태: unknown; 레거시 문서 존재; 현행 가용성 미확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: Submittals 검토·상태·reviewer 협업
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'Submittals 검토·상태·reviewer 협업' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: Submittals 검토·상태·reviewer 협업의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 현재 도움말에 Deprecated 관련 표시 존재; 현행 구매/사용 가능성 미확인. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 도면/자재 승인 이력·재제출·첨부 무결성 검증. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-15 / 단계 F; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S96](https://docs.oracle.com/cd/E80480_01/help/en/user/113053.htm), [OR-S95](https://docs.oracle.com/cd/E80480_01/help/en/user/toc.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U16. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.

## REQ-ORA-178 · Contract·Payment Application 등 legacy 항목
- 원본: ORA-178 / Oracle Primavera 제품군 / OC deprecated 문서 / 조건부 AI와 Legacy 기능 경계. 근거 상태: unknown; 레거시 문서 존재; 현행 가용성 미확인; 제품의 세부 수치 결과와 JOBS 동등성은 실행 미검증.
- 동작: Contract·Payment Application 등 legacy 항목
- 제안 입력: 해당 객체의 ID·revision·권한·profile과 'Contract·Payment Application 등 legacy 항목' 실행에 필요한 필드/옵션. 구체 필드·기본값은 원본 의미와 fixture 검토 후 확정한다.
- 제안 출력: Contract·Payment Application 등 legacy 항목의 결과와 변경 대상 ID·전후 값·진단. 조회 동작은 조회 범위·기준시각·출처를 포함한다.
- 예외/의존: 목차에 Deprecated 표시; 현행 기본 기능으로 표기하지 않음. 누락 입력은 미완성, 미지원 의미는 원본 보존과 경고, 충돌은 정상 결과와 분리한다. 권한/파일 충돌 시 무음 덮어쓰기하지 않는다.
- 수용조건 초안: 과거 데이터 이관 요구와 신규 업무 범위를 분리. 대표 정상·누락·충돌 fixture를 정하고 실제 결과·원본 의미·손실 여부를 대조해야 한다. 현재 미실행.
- 추적 제안: PRD-07 / J-15 / 단계 F; Parity 미검증, Superiority 별도. 매핑은 초안이며 의존성 확인 후 조정한다.
- 근거: [OR-S95](https://docs.oracle.com/cd/E80480_01/help/en/user/toc.htm)
- 확인과제 연결: oracle:U01, oracle:U02, oracle:U04, oracle:U16. 주제 매핑 초안이며 해결증거 NOT_PROVIDED. 관련 제품별 동등성 판정은 과제 해결 전 보류한다.
