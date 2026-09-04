cmake_minimum_required( VERSION 3.18 )

project( wasm-tic-tac-toe CXX )

set( CMAKE_CXX_STANDARD 17 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )
set( CMAKE_CXX_EXTENSIONS OFF )

# Warnings shared by every target.
add_library( warnings INTERFACE )
target_compile_options( warnings INTERFACE
    -Wall
    -Wextra
    -Wpedantic
    -Wshadow
    -Wconversion
)

# ---- embedded resources -----------------------------------------------------
# Each file listed here becomes <build>/generated/resources/<identifier>.h with
# its bytes as an unsigned char array (RedX.png -> RedX_png), written by
# scripts/convert.py. They are regenerated whenever the file or the script
# changes; the `includes` target (`make includes`) builds just these.
find_package( Python3 REQUIRED COMPONENTS Interpreter )

set( RESOURCES
    resources/RedX.png
    resources/RedO.png
    resources/RobotoMono-Regular.ttf
)
set( GENERATED_DIR ${CMAKE_BINARY_DIR}/generated )
set( GENERATED_HEADERS )
foreach( resource IN LISTS RESOURCES )
    get_filename_component( basename ${resource} NAME )
    string( MAKE_C_IDENTIFIER ${basename} identifier )
    set( header ${GENERATED_DIR}/resources/${identifier}.h )
    add_custom_command(
        OUTPUT ${header}
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/scripts/convert.py
                ${CMAKE_SOURCE_DIR}/${resource} -o ${header}
        DEPENDS ${resource} scripts/convert.py
        COMMENT "Embedding ${resource}"
        VERBATIM
    )
    list( APPEND GENERATED_HEADERS ${header} )
endforeach()
add_custom_target( includes DEPENDS ${GENERATED_HEADERS} )

# ---- game logic (no SDL, unit tested) ---------------------------------------
add_library( board STATIC
    src/board.cpp
    src/ai.cpp
    src/game.cpp
)
target_include_directories( board PUBLIC src )
target_link_libraries( board PRIVATE warnings )

# ---- SDL front end ----------------------------------------------------------
add_executable( wasm-tic-tac-toe
    src/main.cpp
    ${GENERATED_HEADERS}
)
target_include_directories( wasm-tic-tac-toe PRIVATE ${GENERATED_DIR} )

find_package( SDL2 REQUIRED )
target_link_libraries( wasm-tic-tac-toe PRIVATE board warnings SDL2::SDL2 )

if( EMSCRIPTEN )
    # SDL2_ttf and SDL2_image (PNG only) are Emscripten ports, prebuilt into
    # the devcontainer image by scripts/install-emscripten.sh. The same flags
    # must be given when compiling (headers) and linking (libraries).
    set( EMSCRIPTEN_PORTS
        --use-port=sdl2_ttf
        --use-port=sdl2_image:formats=png
    )
    target_compile_options( wasm-tic-tac-toe PRIVATE ${EMSCRIPTEN_PORTS} )
    target_link_options( wasm-tic-tac-toe PRIVATE
        ${EMSCRIPTEN_PORTS}
        -sALLOW_MEMORY_GROWTH=1
        --shell-file ${CMAKE_SOURCE_DIR}/web/shell.html
        # Debug only: keep DWARF in the .wasm so breakpoints in the .cpp work
        # from VS Code (WebAssembly DWARF Debugging extension) or Chrome
        # DevTools (C/C++ DevTools Support), and turn on SAFE_HEAP, which
        # bounds-checks every memory access at a large size/speed cost.
        $<$<CONFIG:Debug>:-g>
        $<$<CONFIG:Debug>:-sSAFE_HEAP=2>
        $<$<CONFIG:Debug>:-sASSERTIONS=2>
    )
    set_property( TARGET wasm-tic-tac-toe APPEND PROPERTY
        LINK_DEPENDS ${CMAKE_SOURCE_DIR}/web/shell.html
    )
    # An .html output makes emcc emit the whole site:
    # dist/index.html, dist/index.js, dist/index.wasm
    set_target_properties( wasm-tic-tac-toe PROPERTIES
        SUFFIX ".html"
        OUTPUT_NAME ${CMAKE_SOURCE_DIR}/dist/index
    )
else()
    find_package( SDL2_ttf REQUIRED )
    find_package( SDL2_image REQUIRED )
    target_link_libraries( wasm-tic-tac-toe PRIVATE
        SDL2_ttf::SDL2_ttf
        SDL2_image::SDL2_image
    )

    # ---- tests (native only) --------------------------------------------
    enable_testing()
    foreach( test board_test game_test )
        add_executable( ${test} tests/${test}.cpp )
        target_link_libraries( ${test} PRIVATE board warnings )
        add_test( NAME ${test} COMMAND ${test} )
    endforeach()
endif()
