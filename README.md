# DriveTree

DriveTree는 **C++로 작성된 CLI 디스크 분석기**입니다. 지정한 경로를 재귀적으로 스캔해서 폴더/파일 크기를 집계하고, WizTree처럼 가장 큰 항목을 빠르게 확인할 수 있게 요약을 출력합니다.

## 빌드

```bash
cmake -S . -B build
cmake --build build
```

## 실행

샘플 데이터로 실행:

```bash
./build/drivetree --sample
```

실제 경로 스캔:

```bash
./build/drivetree /path/to/folder --limit 10 --depth 2
```

## 테스트

```bash
ctest --test-dir build --output-on-failure
```
