# Microsoft Store 제출 준비와 남은 절차

2026-10-29. **현재 상태: 개인 계정 등록/앱 이름 예약 전. 실제 제출·심사·승인은 아직 진행하지 않았다.** 개발용 MSIX는 심사용 파일이 아니다.

## 1. 지금 준비한 파일

- `dist/msix/JOBS-development-1.0.2.0-x64-<revision>.msix`: Windows 11 이상 관리자 테스트 전용. 개발 OID와 분리된 identity를 사용한다.
- `scripts/package-msix.py`: 실제 Partner Center identity를 받아 unsigned Store MSIX를 생성한다. Store에서는 심사 후 Microsoft가 다시 서명하므로 별도 공인 인증서 구매를 요구하지 않는다.
- `packaging/msix/store-identity.template.json`: 실제 ID 입력용 서식. 빈 값으로는 Store 패키지를 만들 수 없다.
- `docs/store/listing.ko.md`, `privacy-policy.ko.md`, 벡터 아이콘과 PNG 자산, 실제 앱 화면, 검증 결과.

Microsoft Store는 단일 아키텍처 `.msix` 제출을 지원한다. 이 파이프라인은 x64 패키지이며 UWP 전용 업로드/심볼 묶음으로 가장하는 빈 `.msixupload`를 만들지 않는다. 앱 표시 버전 0.2.0과 Windows 패키지 버전 1.0.2.0은 별개다.

## 2. 계정 소유자가 확정할 항목

- [ ] Microsoft Partner Center 개인 개발자 계정과 신원 확인을 완료한다.
- [ ] 새 Windows 앱을 만들고 사용 가능한 앱 이름을 예약한다.
- [ ] 제품 관리 → 제품 ID/앱 ID 화면에서 `Package/Identity/Name`, `Package/Identity/Publisher`, `Package/Properties/PublisherDisplayName`을 정확하게 복사한다. 대소문자·공백·구두점을 유지한다.
- [ ] `DisplayName`은 예약한 앱 표시명으로 입력한다. JSON 서식의 네 필드를 모두 채운 별도 파일을 준비한다.
- [ ] 게시자 표시명/문의, 가격(무료/유료), 출시 국가, 연령 등급, 분류, 출시 시기와 공개 범위를 결정한다. 현재 앱에는 앱 내 구매나 광고 기능이 없다.

## 3. 제출용 파일 생성

Windows x64에서 Qt 6.8.3 MinGW와 CMake/Ninja Release 빌드를 준비한다. `README.md`의 빌드 절차를 따른다. 저장소 변경 사항은 커밋한다.

```powershell
python scripts/setup-msix-tools.py
# 템플릿을 build/store-identity.json으로 복사하고 실제 네 값을 입력한다.
python scripts/package-msix.py --mode store --identity build/store-identity.json --version 1.0.2.0
```

출력의 `.json`에서 `mode=store`, `store_identity_supplied=true`, `test_probe_included=false`, 원본 커밋과 SHA-256을 확인한다. 생성된 manifest의 identity가 Partner Center와 정확히 일치하는지도 확인한다. SDK/ZIP 검증 성공은 Store 심사 통과와 별개다. 다음 업데이트에는 앞선 버전보다 큰 패키지 버전을 사용하고 네 번째 숫자는 0으로 둔다. 동일한 Name/Publisher를 유지해야 같은 앱으로 업데이트된다.

## 4. 제출 전 확인

- [ ] 최종 Store identity의 패키지를 Windows App Certification Kit(WACK)로 검사하고 원본 결과를 보관한다. WACK는 이 저장소가 번들하는 MakeAppx와 다른 도구다. 설치된 Windows SDK의 WACK GUI 또는 `appcert.exe` 도움말에 따라 실행한다. 미실행은 통과로 표시하지 않는다.
- [ ] 실제 대상 Windows PC에서 설치/실행/파일 저장·열기/자동 복구/업데이트/제거를 확인한다. 서명 없는 개발 패키지 테스트는 Windows 11 이상용이며, Store 배포 최소 OS 19041에서의 인증을 대신하지 않는다.
- [ ] 일반 제거가 패키지 설정/복구를 지우고 별도 저장 문서는 보존하는지 확인한다. 제거 후 데이터 보존 옵션을 선택한 Windows 동작은 예외다.
- [ ] MSIX와 EXE판은 앱 상태를 공유하지 않는다. 함께 열려 있는 같은 프로젝트는 파일 잠금과 외부 변경 감지로 보호된다. EXE판 제거기는 MSIX를 제거하지 않는다.
- [ ] `listing.ko.md`의 문구, 공개된 개인정보/지원 URL, 최소 1366×768 실제 앱 화면과 300×300 Store 아이콘을 확인한다.
- [ ] 초기 공개 버전의 형식 변환 제한과 기능 상태를 확인한다. 실제 업무 파일의 사본으로 검증한다.
- [ ] LGPL 등 동봉 런타임 라이선스 고지와 소스/재링크 안내가 최종 배포 방식에 적합한지 게시자가 확인한다. Store 심사가 라이선스 검토를 대신하지 않는다.

## 5. Partner Center 제출

새 제출에서 가격/국가/등급/속성/한국어 소개를 채우고, **Store 모드로 생성한 `.msix`**를 패키지에 올린다. `runFullTrust` 권한 설명은 `listing.ko.md`의 심사 메모를 사용한다. 심사 미리보기와 패키지 경고를 확인한 뒤 계정 소유자가 제출한다. 검토 요청이나 거절이 오면 해당 사유를 반영한다. 이 저장소의 CI는 계정 로그인·업로드·공개 게시를 수행하지 않는다.

## 개발용 설치와 검증

Windows 11 이상 관리자 PowerShell에서만 다음 경로를 사용한다. 개발 OID는 별도 인증서/신뢰 저장소/개발자 모드 변경 없이 unsigned 테스트를 허용한다. 일반 사용자 배포용으로 안내하지 않는다.

```powershell
Add-AppxPackage -Path "C:\path\JOBS-development-1.0.2.0-x64-REV.msix" -AllowUnsigned
# 시작 메뉴: JOBS MSIX Development
Get-AppxPackage -Name ToolMoa.JOBS.MsixDevelopment | Remove-AppxPackage
```

CI의 `check-msix-lifecycle.ps1`은 probe 포함 개발 패키지 두 버전만 허용하며, 기존 개발 패키지가 설치되어 있으면 중단한다. 실제 package identity에서 생성/저장/열기/일정/undo/작업 영역/복구를 검사하고 버전 업데이트 후 상태를 다시 읽고 제거한다. 외부 문서 및 EXE판 상태의 보존도 검사한다. 실제 앱 UI 캡처와 결과 JSON은 CI artifact로 남긴다. 이 검사는 Store 승인을 뜻하지 않는다.

근거: [공식 Store 패키지·서명·버전 요건](https://learn.microsoft.com/en-us/windows/apps/publish/publish-your-app/msix/app-package-requirements), [unsigned 개발 패키지](https://learn.microsoft.com/en-us/windows/msix/package/unsigned-package), [패키지 ID 확인](https://learn.microsoft.com/en-us/windows/apps/publish/view-app-identity-details), [Store 스크린샷](https://learn.microsoft.com/en-us/windows/apps/publish/publish-your-app/screenshots-and-images).
