# 设置cmake最低版本
cmake_minimum_required(VERSION 3.4.1)

# 设置GCC编译器的编译选项
if (CMAKE_COMPILER_IS_GNUCC)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wmissing-declarations -Wmissing-prototypes")
endif (CMAKE_COMPILER_IS_GNUCC)

# 设置clang编译器的编译选项
if (CMAKE_COMPILER_IS_CLANG)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wmissing-declarations -Wmissing-prototypes -Wdocumentation -Wunreachable-code")
endif (CMAKE_COMPILER_IS_CLANG)

# 为了确保 CMake 可以在编译时定位头文件
# 这样就可以使用 #include <xx.h> 引入
# 否则需要使用 #include "path/xx"
# 添加include目录路径
include_directories(common
        include)

# 添加ffmpeg库，IMPORTED指以导入的形式加入到项目中来，下面的set_target_properties是指定IMPORTED的路径
add_library( ffmpeg SHARED IMPORTED )

#链接库文件 ，${CMAKE_SOURCE_DIR}指cmakeList.txt所在的文件夹路径， ${ANDROID_ABI}指构建的COU，../代表父级目录
set_target_properties( ffmpeg
        PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/../jniLibs/${ANDROID_ABI}/libffmpeg.so )

# Metadata库
add_subdirectory(${CMAKE_SOURCE_DIR}/metadata)

#mediaplayer库
add_subdirectory(${CMAKE_SOURCE_DIR}/mediaplayer)