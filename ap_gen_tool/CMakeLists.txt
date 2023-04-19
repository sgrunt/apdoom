project(ap_gen_tool)
add_executable(${PROJECT_NAME} main.cpp)
target_include_directories(${PROJECT_NAME} PUBLIC ../APCpp/jsoncpp/include)
target_link_libraries(${PROJECT_NAME} jsoncpp_static)
