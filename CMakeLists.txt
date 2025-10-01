cmake_minimum_required(VERSION 3.20)
project(tatter-renderer CXX)

# ÉrÉãÉhÇÃê›íË
add_executable(${PROJECT_NAME} "main.cpp")
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
