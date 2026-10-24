# 네이티브 코드 편집기 지침

앱 소스는 App/, 출력은 build/에 둔다. iiSDKCore 진입점과 LVUI AOT·iiGUI 자체 위젯을 사용한다.
파일 생성·쓰기는 iiFileProvider를 통한다. 테스트·self-test·앱은 전체 목표 마지막까지 실행하지 않는다.
CodeEditor는 PlainText/NoWrap이며 RichText TextEditor와 구분한다. 서식과 전체 원본 API 동등성은 아직 완료되지 않았다.
