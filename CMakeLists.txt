cmake_minimum_required(VERSION 3.5...4.0)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "Minimum policy version for all subprojects")

project(Abyssal-ROV)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

include(FetchContent)


# Library dependencies #


FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 3.0.2
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL
    SYSTEM)
FetchContent_MakeAvailable(SFML)

FetchContent_Declare(
    glm
    GIT_REPOSITORY  https://github.com/g-truc/glm.git
    GIT_TAG         1.0.3
)
FetchContent_MakeAvailable(glm)

add_library(common INTERFACE)
target_compile_features(common INTERFACE cxx_std_20)
target_compile_options(common INTERFACE
  $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>: -Wall -DGL_SILENCE_DEPRECATION>
  $<$<CXX_COMPILER_ID:MSVC>: /W4>
)

find_package(OpenGL REQUIRED)


# Sources and executables #


include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR})

# ==========================================
# TAPPA 01
# ==========================================
set(TAPPA01_SOURCES Cartella-Tappa01/main.cpp Cartella-Tappa01/glad.c)
add_executable(Tappa01 ${TAPPA01_SOURCES})
target_link_libraries(Tappa01 PRIVATE common SFML::Graphics SFML::Window SFML::System OpenGL::GL glm::glm-header-only)

# ==========================================
# TAPPA 02: La Telecamera Virtuale
# ==========================================
set(TAPPA02_SOURCES Cartella-Tappa02/main.cpp Cartella-Tappa02/Camera.cpp Cartella-Tappa02/glad.c)
add_executable(Tappa02 ${TAPPA02_SOURCES})
target_link_libraries(Tappa02 PRIVATE common SFML::Graphics SFML::Window SFML::System OpenGL::GL glm::glm-header-only)

# ==========================================
# TAPPA 03: Generazione del Terrain (Heightmap)
# ==========================================
set(TAPPA03_SOURCES Cartella-Tappa03/main.cpp Cartella-Tappa03/Camera.cpp Cartella-Tappa03/glad.c)
add_executable(Tappa03 ${TAPPA03_SOURCES})
target_link_libraries(Tappa03 PRIVATE common SFML::Graphics SFML::Window SFML::System OpenGL::GL glm::glm-header-only)

# ==========================================
# TAPPA 04: Materiali e Texture
# ==========================================
set(TAPPA04_SOURCES Cartella-Tappa04/main.cpp Cartella-Tappa04/Camera.cpp Cartella-Tappa04/glad.c)
add_executable(Tappa04 ${TAPPA04_SOURCES})
target_link_libraries(Tappa04 PRIVATE common SFML::Graphics SFML::Window SFML::System OpenGL::GL glm::glm-header-only)

# ==========================================
# TAPPA 05: Illuminazione Dinamica (Spotlights)
# ==========================================
set(TAPPA05_SOURCES Cartella-Tappa05/main.cpp Cartella-Tappa05/Camera.cpp Cartella-Tappa05/glad.c)
add_executable(Tappa05 ${TAPPA05_SOURCES})
target_link_libraries(Tappa05 PRIVATE common SFML::Graphics SFML::Window SFML::System OpenGL::GL glm::glm-header-only)

# ==========================================
# TAPPA 06: Effetto Nebbia Volumetrica
# ==========================================
set(TAPPA06_SOURCES Cartella-Tappa06/main.cpp Cartella-Tappa06/Camera.cpp Cartella-Tappa06/glad.c)
add_executable(Tappa06 ${TAPPA06_SOURCES})
target_link_libraries(Tappa06 PRIVATE common SFML::Graphics SFML::Window SFML::System OpenGL::GL glm::glm-header-only)

# ==========================================
# TAPPA 07: Modelli 3D e Collisioni
# ==========================================
set(TAPPA07_SOURCES Cartella-Tappa07/main.cpp Cartella-Tappa07/Camera.cpp Cartella-Tappa07/glad.c Cartella-Tappa07/ModelLoader.h)
add_executable(Tappa07 ${TAPPA07_SOURCES})
target_link_libraries(Tappa07 PRIVATE common SFML::Graphics SFML::Window SFML::System OpenGL::GL glm::glm-header-only)

# ==========================================
# TAPPA 08: Comportamento Emergente (CPU)
# ==========================================
set(TAPPA08_SOURCES Cartella-Tappa08/main.cpp Cartella-Tappa08/Camera.cpp Cartella-Tappa08/glad.c Cartella-Tappa08/ModelLoader.h)
add_executable(Tappa08 ${TAPPA08_SOURCES})
target_link_libraries(Tappa08 PRIVATE common SFML::Graphics SFML::Window SFML::System OpenGL::GL glm::glm-header-only)

# ==========================================
# TAPPA 09: Instanced Rendering (GPU)
# ==========================================
set(TAPPA09_SOURCES Cartella-Tappa09/main.cpp Cartella-Tappa09/Camera.cpp Cartella-Tappa09/glad.c Cartella-Tappa09/ModelLoader.h)
add_executable(Tappa09 ${TAPPA09_SOURCES})
target_link_libraries(Tappa09 PRIVATE common SFML::Graphics SFML::Window SFML::System OpenGL::GL glm::glm-header-only)

# ==========================================
# TAPPA 10: UI, Macchina a Stati e Alpha Blending
# ==========================================
set(TAPPA10_SOURCES Cartella-Tappa10/main.cpp Cartella-Tappa10/Camera.cpp Cartella-Tappa10/glad.c Cartella-Tappa10/ModelLoader.h)
add_executable(Tappa10 ${TAPPA10_SOURCES})
target_link_libraries(Tappa10 PRIVATE common SFML::Graphics SFML::Window SFML::System OpenGL::GL glm::glm-header-only)