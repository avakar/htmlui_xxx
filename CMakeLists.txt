cmake_minimum_required(VERSION 3.1)
project(avakar_tiny_html)

include(deps.cmake OPTIONAL)

add_executable(avakar_tiny_html
    main.cpp
    dom.cpp
    dom.h
    dom_impl.h
    css.h
	html_parser.cpp
	html_parser.h
    )
target_link_libraries(avakar_tiny_html avakar_di)
target_compile_options(avakar_tiny_html PUBLIC "/std:c++17")
