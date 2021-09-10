cmake_minimum_required( VERSION 3.18.3 ) # Latest version of CMake when this file was created.

set( HEADER_FILES
    inc/Camera.h
    inc/Light.h
    inc/SceneVisitor.h
    inc/DirectX12HDR.h
)

set( SRC_FILES
    src/Camera.cpp
    src/main.cpp
    src/Light.cpp
    src/SceneVisitor.cpp
    src/DirectX12HDR.cpp
)

set( VERTEX_SHADER_FILES 
    shaders/HDR_VS.hlsl
    shaders/HDRtoSDR_VS.hlsl
    shaders/Skybox_VS.hlsl
)

set( PIXEL_SHADER_FILES
    shaders/HDR_PS.hlsl
    shaders/HDRtoSDR_PS.hlsl
    shaders/Skybox_PS.hlsl
    shaders/Unlit_PS.hlsl
)

set( SHADER_FILES
    ${VERTEX_SHADER_FILES}
    ${PIXEL_SHADER_FILES}
)

source_group( "Resources\\Shaders" FILES ${SHADER_FILES} )

set_source_files_properties( ${SHADER_FILES} 
    PROPERTIES
        VS_SHADER_OBJECT_FILE_NAME "${CMAKE_SOURCE_DIR}/Data/Shaders/DirectX12EngineHDRSample/%(Filename).cso"
        VS_SHADER_MODEL 6.0
        VS_SHADER_DISABLE_OPTIMIZATIONS $<$<CONFIG:Debug>:ON>
        VS_SHADER_ENABLE_DEBUG $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:ON>
        VS_SHADER_FLAGS $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:-Qembed_debug>
)

set_source_files_properties( ${VERTEX_SHADER_FILES}
    PROPERTIES 
        VS_SHADER_TYPE Vertex
)

set_source_files_properties( ${PIXEL_SHADER_FILES}
    PROPERTIES 
        VS_SHADER_TYPE Pixel
)

add_executable( DirectX12EngineHDRSample WIN32
    ${HEADER_FILES} 
    ${SRC_FILES}
    ${SHADER_FILES}
)

target_include_directories( DirectX12EngineHDRSample
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

target_link_libraries( DirectX12EngineHDRSample
    GameFramework
    DX12Lib
    d3dcompiler.lib
    Shlwapi.lib
)

# Set debug database to enable the "Edit and Continue" feature.
target_compile_options( DirectX12EngineHDRSample
    PRIVATE $<$<CONFIG:Debug>:/ZI>
)

# Set Local Debugger Settings (Command Arguments and Environment Variables)
set( COMMAND_ARGUMENTS "-wd \"${CMAKE_SOURCE_DIR}\"" )
configure_file( DirectX12EngineHDRSample.vcxproj.user.in ${CMAKE_CURRENT_BINARY_DIR}/DirectX12EngineHDRSample.vcxproj.user @ONLY )
