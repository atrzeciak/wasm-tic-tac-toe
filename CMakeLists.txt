cmake_minimum_required( VERSION 3.18 )

project( wasm-tic-tac-toe )

add_executable( wasm-tic-tac-toe
    wasm-tic-tac-toe.cpp
)

find_package( SDL2 REQUIRED )
target_link_libraries( wasm-tic-tac-toe
    PRIVATE SDL2::SDL2
)

if( EMSCRIPTEN )
find_package( Freetype REQUIRED )
target_link_libraries( wasm-tic-tac-toe
    PRIVATE SDL2_ttf ${FREETYPE_LIBRARIES} harfbuzz z
)
else()
find_package( SDL2_ttf REQUIRED )
target_link_libraries( wasm-tic-tac-toe
    PRIVATE SDL2_ttf::SDL2_ttf
)
endif()

if( EMSCRIPTEN )
    target_link_options( wasm-tic-tac-toe PUBLIC
        -sALLOW_MEMORY_GROWTH=1
        -sSAFE_HEAP=2
    )
    # special way to tell emscripten to build full website
    # /dist/
    # |-- index.html
    # |-- index.js
    # `-- index.wasm
    set( CMAKE_EXECUTABLE_SUFFIX ".html" ) 
    set_target_properties( wasm-tic-tac-toe PROPERTIES OUTPUT_NAME ${CMAKE_SOURCE_DIR}/dist/index ) 
endif ()
