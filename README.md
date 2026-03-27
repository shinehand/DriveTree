# DriveTree

DriveTree는 **C++로 작성된 GUI 디스크 분석기**입니다. 폴더를 선택하면 경로를 재귀적으로 스캔해서 폴더/파일 크기를 집계하고, WizTree처럼 큰 항목과 디렉터리 구조를 한 화면에서 확인할 수 있습니다.

## 빌드

```bash
sudo apt-get install -y libfltk1.3-dev
cmake -S . -B build
cmake --build build
```

## 실행

GUI 실행:

```bash
./build/drivetree
```

실행 후:
- **Load Sample** 버튼으로 샘플 데이터 확인
- **Scan Folder** 버튼으로 실제 폴더 선택 및 분석

## 테스트

```bash
ctest --test-dir build --output-on-failure
```
