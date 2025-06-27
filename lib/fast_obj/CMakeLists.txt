project(fast_obj)
add_library(fast_obj STATIC fast_obj.c)
target_include_directories(fast_obj PUBLIC .)