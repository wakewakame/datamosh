# datamosh
動画ファイル中のキーフレームを欠落させ、意図的に壊れた動画を生成するプログラムです。
2つ以上の動画を1つに結合し、結合部分のキーフレームを欠落させます。

# ビルド手順
## Ubuntu
### 必要なツールのインストール

```sh
sudo apt install -y git cmake libavcodec-dev libavformat-dev libavutil-dev libswscale-dev libswresample-dev
```

### リポジトリのクローン

```sh
git clone https://github.com/wakewakame/datamosh
cd datamosh
git submodule update --init --recursive
```

### ビルド

```sh
mkdir build
cd build
cmake ..
make -j
```

### 実行

```sh
./bin/datamosh <video1 path> <video2 path> ...
```

