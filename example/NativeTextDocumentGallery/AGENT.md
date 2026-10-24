# 네이티브 문서 갤러리 지침

앱 소스는 App/, 출력은 build/에 둔다. Core 진입점과 LVUI AOT·iiGUI 자체 위젯을 사용한다.
파일 생성·쓰기는 iiFileProvider를 통한다. 앱·테스트·self-test는 전체 목표의 마지막까지 실행하지 않는다.
이 예제는 문서 모델 시연이다. 별도의 PlainText CodeEditor 갤러리와 아직 구현하지 않은 RichText TextEditor 화면을 구분한다.
