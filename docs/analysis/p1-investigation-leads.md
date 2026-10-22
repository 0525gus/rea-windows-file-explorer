# P1 보충 조사 후보 — 저장된 Evidence 재사용

2026-10-22 KST. Explorer++ 1.4.0.2293 x64, SHA-256 `22ac20341c5525658ebe5e9c4b9ac9f7c152b18080ff347861fac3f04d56a2d6`. 아래는 기존 REA/Ghidra 12.1.4 출력의 독립 재검토다. 원본 소스·실험 A 자료에 접근하지 않았고 REA session을 열거나 분석을 재실행하지 않았다. **후속 호출은 제안이며 실행 결과가 아니다.** 주소는 선호 VA다.

## 정본과 locator

JSON pointer는 아래 정확한 bundle에 대한 값이다. 같은 Evidence가 여러 export에서 다른 record 위치를 가질 수 있다.

| 약칭 | 파일·SHA-256 | operation / Evidence |
|---|---|---|
| N | [phase2-names-bundle.json](evidence/phase2-names-bundle.json), `38f0b04e9c45f61a46e6a306f7f7f607024dd9352dea69f0ab255a325fd54af4` | `/records/28`, `list_names`, `ev_bcf098d43636ed7566f5a1628c11403e9db45a26d563a614a0d81fea21369d00` |
| I | [phase2-inventory-bundle.json](evidence/phase2-inventory-bundle.json), `57abafb1e0ae4b60c0f6177e3bd12a8630479ef1d26c13a5c5164e6119e8e2fc` | `/records/18`, `list_procedures`, `ev_29f8178882cbca7392ba8a54062330a5988d97b5d82c2e186db38a24864eef76`; `/records/49`, `list_strings`, `ev_9dd2b29a976449277348ba3ac5c1d42604d3f26445a30c880302863eba9a818c` |
| S | [phase2-static-bundle.json](evidence/phase2-static-bundle.json), `0fa335949accc46b85f05360ffe6b3897e6caec8f0d1ec6a26b2e59be521b08a` | 함수별 아래 locator 사용 |

N의 `normalized_result`와 `raw_result`는 실제로 일치하는 101,957개 symbol 목록이다. 이는 Ghidra의 symbol inventory이며 모든 의미·가상 호출을 해결했다는 뜻이 아니다. 아래 `N[n]`은 `/records/28/normalized_result/n`, `I-string[n]`은 `/records/49/normalized_result/n`, `I-proc[n]`은 `/records/18/normalized_result/n`을 뜻한다.

## Rename / mkdir

| 실제 관찰 | 정확한 anchor | 해석·다음 최소 조회 |
|---|---|---|
| 정의 문자열 **`Starting in-place rename of item created via new menu`** | `0x1402de930`, N[80937], I-string[1146] | 새 메뉴로 만든 항목의 rename 시작 후보다. **일반 F2/모든 rename handler의 증거로 확대하지 않는다.** 새 `xrefs(address:0x1402de930)`→각 instruction의 `resolve_containing_procedure`→필요한 한 함수 `analyze_function`. 기존 export에는 이 anchor의 xrefs가 없다. |
| `NewFolder = 45008` 문자열 | `0x1402def78`, N[81003], I-string[1161] | command/config 이름과 숫자의 결합은 관찰이다. 실제 WM_COMMAND dispatch가 45008인지 아직 미확인. API 호출자를 먼저 좁힌다. |
| `KERNEL32.DLL::CreateDirectoryW` import / IAT label | `EXTERNAL:0x118`, N[279]; `0x1402b2700`, N[77540] | 새 `procedure_callers(procedure:EXTERNAL:0x118)` 후 관련 함수 하나의 `analyze_function`. 다른 용도의 폴더 생성일 수 있으므로 입력 경로·UI/새폴더 문자열과 연결해야 한다. |
| `rename` 문자열; MassRename/TabRename 구별 | `0x1402e1d68`, N[81455], I-string[1388]; MassRename vtable `0x1402dfe40` N[81165]; RenameTab vtable `0x1402e1608` N[81379] | `rename`의 xrefs는 Shell verb 후보를 줄 수 있다. 대량 파일 rename/탭 rename/단일 파일 rename을 섞지 않는다. 첫 log/API 조사가 부족할 때만 추가한다. |

101,957개 symbol 이름에서 `SetNameOf` spelling은 **0개**였다. COM slot 호출이 가능하므로 기능/API가 없다는 결론은 아니다. 새 rename 후보의 pseudocode/assembly에서 바인딩한 인터페이스와 slot을 확인한 뒤 SDK 순서로 `IShellFolder::SetNameOf` 여부를 도출해야 한다. 이미 확보된 `PathRenameExtensionW` import는 **경로 문자열의 확장자 조작** 단서이며 실제 파일 rename 실행으로 취급하지 않는다. `boost::filesystem::rename` 문자열(`0x1402b8080`, N[78254])도 실제 호출/대상 파일을 증명하지 않는다.

## Delete / recycle

| 실제 관찰 | 정확한 anchor | 아직 증명하지 못한 점·다음 조회 |
|---|---|---|
| `Delete = 45012`, `DeletePermanently = 45022` | 각각 `0x1402defc0`, N[81007], I-string[1165]; `0x1402df078`, N[81016], I-string[1174] | 별도 명령 이름은 있지만 recycle flag/실제 dispatch는 없다. 새 `xrefs` 두 개로 command/config consumer를 찾고, 코드 참조가 있으면 containing procedure→분석한다. data 참조만이면 dispatch를 확정하지 않는다. |
| `delete` / `SecureDelete` 정의 문자열 | `0x1402e2818`, N[81587], I-string[1406]; `0x1402ed288`, N[82866], I-string[2071] | Shell verb/설정 후보다. 삭제 구현이 어느 것인지 미확인. command anchor 조사 뒤 필요할 때만 xrefs. |
| `KERNEL32.DLL::DeleteFileW` import / IAT label | `EXTERNAL:0xee`, N[237]; `0x1402b25b0`, N[77498] | 새 `procedure_callers(EXTERNAL:0xee)`는 직접 삭제·임시 파일 정리 후보를 찾는다. caller가 사용자 선택 파일 삭제인지 구분해야 한다. 이 API 단독으로 recycle을 증명하지 못한다. |
| RecycleBin 관련 표시 이름·RTTI 기반 `IFileOperationProgressSink` | RecycleBin `0x1402e3080`, N[81675]; sink type descriptor `0x1403643c0`, N[93703] | 휴지통 위치/열 또는 COM callback 구조의 존재 단서다. `IFileOperation::DeleteItem(s)`·`FOF_ALLOWUNDO`·실제 휴지통 이동의 증거는 아니다. |

**이미 분석한 `SHFileOperationW` 경로를 delete 답으로 재사용하지 않는다.** S `/records/50`, `procedure_pseudo_code(0x1401a7630)`, Evidence `ev_a6af60e3bd61d7cca2e8d3255e23dbddbe9bcb8f3147c26c54e7dca7ae1a66e9`는 `wFunc=((int)param_1[8]!=0)+1`, flag `0x28`/`0x20`, API 결과와 abort 검사를 보인다. 현재 복원 표현의 selector는 MOVE/COPY이며 DELETE(3)가 아니다. S `/records/58`, `procedure_callers(EXTERNAL:0x2)`, `ev_b957a5780fddbffbadceb44e7c71fb93ecc8bafa17e4d97b9e57660520e24f6e`도 이미 확보했다. 같은 query 재실행 대신 새 command/verb/직접 삭제 caller를 조사한다. 자세한 상수·한계는 [Shell 분석](shell-integration.md)에 있다.

## Drop delegation

S `/records/20`, `procedure_pseudo_code(0x140179920)`, Evidence `ev_37033ee2017621418f12890bb629601cfcc8fa391268e48485698cbb8c59fd5b`에서 다음을 **실제 의사코드로 관찰**했다.

- `iVar4==2` 분기에서 `FUN_140179ea0(param_1,&local_338)` 호출.
- 다른 경로에서 0x50-byte 객체 할당과 `DropHandler::vftable` 대입, 이어 `FUN_1401a5190(plVar5,param_2,local_338 & 0xffffffff,param_4)` 호출.

대상 `0x140179ea0`은 I-proc[4972], `0x1401a5190`은 I-proc[5465]에서 local/non-thunk recovered procedure로 확인했다. `DropHandler::vftable` 주소는 `0x1402ed908`, N[82913]이다. 이것이 모든 transfer format·copy/move 처리·UI 성공을 입증하지 않는다.

**가장 효율적인 새 질의:** `analyze_function(procedure:0x1401a5190)`로 주 위임 대상의 data format·효과·호출·오류를 한 번에 확보하고, 별도 분기 의미가 필요하면 `analyze_function(procedure:0x140179ea0)`를 추가한다. 두 함수의 본문은 현재 S에 없으며 이미 있는 Drop(0x140179920) 본문을 다시 요청할 필요가 없다. RegisterDragDrop/RevokeDragDrop/DoDragDrop import만으로 등록·해제·실행 생명주기를 추정하지 않는다.

이 후보 목록은 P1 기능 복원/실행 성공 판정이 아니다. 새 Evidence가 확보되면 사용자 명령→대상 항목→작업 요청→OS 위임→충돌/오류/취소/결과를 연결하고, disposable fixture의 실제 원본 동작과 별도 비교해야 한다. 현재 불명확한 P1 구현은 문서화된 일반 API 기반으로 분류하고 나중의 근거를 초기 성공 주장에 소급하지 않는다.
