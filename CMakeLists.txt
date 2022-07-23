# CMakeの最小バージョンの指定
cmake_minimum_required(VERSION 3.10)

# プロジェクトの作成
project(datamosh_project)

# OpenCVのパス
set(OPENCV_DIR "${PROJECT_SOURCE_DIR}/3rdparty/opencv")

# OpenCVで使用するModuleの指定
set(OPENCV_BUILD_LIST
	"core"
	"imgproc"
	"imgcodecs"
	"highgui"
	"videoio"
)
string(REGEX REPLACE ";" "," OPENCV_BUILD_LIST_STRING "${OPENCV_BUILD_LIST}")
string(REGEX REPLACE "([^;]+)" "opencv_\\1" OPENCV_BUILD_LIST_TARGET "${OPENCV_BUILD_LIST}")
string(REGEX REPLACE "([^;]+)" "${OPENCV_DIR}/modules/\\1/include" OPENCV_BUILD_LIST_INCLUDE "${OPENCV_BUILD_LIST}")

# C++17を使用する
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
find_package(Threads REQUIRED)

# compile_commands.jsonを生成するようにする
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "generate compile_commands.json" FORCE)

# OpenCVの中で必要なモジュールのみビルドするように設定
set(BUILD_LIST "${OPENCV_BUILD_LIST_STRING}" CACHE STRING "OpenCV Build List" FORCE)

# OpenCVのCMakeLists.txtを読み込む
add_subdirectory("${OPENCV_DIR}")

# 全てのソースファイルを再帰的に検索する
file(GLOB_RECURSE ALL_CPP_FILES "${PROJECT_SOURCE_DIR}/src/*.cpp")

macro(make_app)
	set(oneValueArgs APP_NAME SOURCE ENABLE_CINDER)
	cmake_parse_arguments(ARG "" "${oneValueArgs}" "" ${ARGN})

	# プログラムの作成
	add_executable("${ARG_APP_NAME}" "${ARG_SOURCE}" "${ALL_CPP_FILES}")
	target_include_directories("${ARG_APP_NAME}" PUBLIC "${PROJECT_SOURCE_DIR}/include")

	# OpenCVをライブラリとして追加
	target_link_libraries("${ARG_APP_NAME}"
		"Threads::Threads"
		"${OPENCV_BUILD_LIST_TARGET}"
		"avcodec"
		"avformat"
		"avutil"
		"swscale"
		"swresample"
		"avutil"
	)
	target_include_directories("${ARG_APP_NAME}" PUBLIC
		"${PROJECT_SOURCE_DIR}/include"
		"${CMAKE_BINARY_DIR}"
		"${OPENCV_DIR}/include"
		"${OPENCV_BUILD_LIST_INCLUDE}"
	)
	target_compile_options("${ARG_APP_NAME}" PUBLIC -pg)
	target_link_options("${ARG_APP_NAME}" PUBLIC -pg)
endmacro()

make_app(APP_NAME datamosh SOURCE "${PROJECT_SOURCE_DIR}/main.cpp")
make_app(APP_NAME read SOURCE "${PROJECT_SOURCE_DIR}/examples/read.cpp")
make_app(APP_NAME write SOURCE "${PROJECT_SOURCE_DIR}/examples/write.cpp")
make_app(APP_NAME read_and_write SOURCE "${PROJECT_SOURCE_DIR}/examples/read_and_write.cpp")

