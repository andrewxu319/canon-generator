cmake_minimum_required(VERSION 3.17)
project(my-musicxml-proj)
set(CMAKE_CXX_STANDARD 17)
set(CPP_VERSION 17)


add_executable(canon_generator "canon_generator.cpp" "EXAMPLE.cpp"  "settings.h" "file_reader.h" "file_reader.cpp"  "exception.cpp" "exception.h" "file_writer.cpp" "file_writer.h" "counterpoint_checker.cpp" "counterpoint_checker.h" "sonority.cpp" "sonority.h"    "canon.h" "canon.cpp")
add_subdirectory(lib/mx)
target_link_libraries(canon_generator mx)
target_include_directories(canon_generator PRIVATE lib/m/x/Sourcecode/include)