cmake_minimum_required(VERSION 3.20)
project(tatter-renderer CXX)

# ÉrÉãÉhÇÃê›íË
add_executable(${PROJECT_NAME} "main.cpp" "App.cpp")
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
target_compile_options(${PROJECT_NAME} PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/Zc:__cplusplus /utf-8>)

add_subdirectory(sqrap-vk)

target_link_libraries(
	${PROJECT_NAME} PUBLIC
	sqrap-vk
)

target_include_directories(
	${PROJECT_NAME} PUBLIC
	sqrap-vk/inc
)

set(SHADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/shader/")
add_compile_definitions(SHADER_DIR="${SHADER_DIR}")
set(MODEL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/model/")
add_compile_definitions(MODEL_DIR="${MODEL_DIR}")