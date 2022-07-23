# datamosh
動画ファイル中のキーフレームを欠落させ、意図的に壊れた動画を生成するプログラムです。
2つ以上の動画1つに結合し、結合部分のキーフレームを欠落させます。

# ビルド手順
## Ubuntu or MacOS
### 必要なツールのインストール

Ubuntuの場合

```sh
sudo apt install -y git cmake libavcodec-dev libavformat-dev libavutil-dev libswscale-dev libswresample-dev
```

MacOSの場合

```sh
brew install git cmake
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
cd ./build/bin
./datamosh <video1 path> <video2 path> ...
```

## Windows
### 必要なソフトのインストール
事前に以下のソフトをインストールして下さい。

- [Visual Studio](https://visualstudio.microsoft.com/ja/downloads/)  (Visual Studio Community 2019で動作確認済み)
- [Git](http://git-scm.com/downloads)
- [CMake](https://cmake.org/download/)

### Visual Studioのソリューションファイルを生成
ファイルエクスプローラーでプロジェクトフォルダを作成したい場所を開き、アドレスバーに `cmd` と入力してコマンドプロンプトを起動します。
次に、コマンドプロンプトに以下のコマンドを入力してVisual Studioのソリューションファイルを生成します。
(1行ずつ実行することをオススメします)

```sh
git clone https://github.com/wakewakame/datamosh
cd datamosh
git submodule update --init --recursive
mkdir build
cd build
cmake ..
```

完了すると `datamosh` フォルダが生成されます。
また、 `datamosh\build\datamosh_project.sln` にVisual Studioのソリューションファイルが生成されます。

### ビルド
`datamosh\build\datamosh_project.sln` を開き、 `datamosh` をスタートアッププロジェクトに設定します。
F5キーを押すとビルドが開始します。

