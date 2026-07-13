# vcpkg release-only triplets (e.g. x64-windows-release) install Release libraries only.
# MSVC Debug builds pull in OpenCV debug ABI symbols that those libraries do not provide.

function(biopic_apply_release_only_triplet_policy reason)
  if(CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_CONFIGURATION_TYPES Release CACHE STRING "bioPic release-only vcpkg triplet" FORCE)
  elseif(NOT CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_BUILD_TYPE Release CACHE STRING "bioPic release-only vcpkg triplet" FORCE)
  endif()
  set(BIOPIC_RELEASE_ONLY_TRIPLET ON CACHE BOOL "Using a release-only vcpkg triplet" FORCE)
  message(STATUS "bioPic: ${reason} — use Release builds only.")
endfunction()

if(NOT DEFINED VCPKG_TARGET_TRIPLET)
  if(WIN32 AND EXISTS "${CMAKE_SOURCE_DIR}/cmake/triplets/x64-windows-release.cmake")
    set(VCPKG_TARGET_TRIPLET "x64-windows-release" CACHE STRING "vcpkg target triplet" FORCE)
  elseif(UNIX AND NOT APPLE AND EXISTS "${CMAKE_SOURCE_DIR}/cmake/triplets/x64-linux-release.cmake")
    set(VCPKG_TARGET_TRIPLET "x64-linux-release" CACHE STRING "vcpkg target triplet" FORCE)
  elseif(APPLE AND EXISTS "${CMAKE_SOURCE_DIR}/cmake/triplets/arm64-osx-release.cmake")
    set(VCPKG_TARGET_TRIPLET "arm64-osx-release" CACHE STRING "vcpkg target triplet" FORCE)
  endif()
endif()

if(DEFINED VCPKG_TARGET_TRIPLET AND VCPKG_TARGET_TRIPLET MATCHES "-release$")
  biopic_apply_release_only_triplet_policy(
    "'${VCPKG_TARGET_TRIPLET}' is a release-only vcpkg triplet")
endif()

macro(biopic_enforce_release_only_dependencies)
  if(DEFINED OpenCV_DIR AND OpenCV_DIR MATCHES "/x64-windows-release/")
    biopic_apply_release_only_triplet_policy(
      "OpenCV is installed from the x64-windows-release vcpkg triplet")
  elseif(DEFINED OpenCV_DIR AND OpenCV_DIR MATCHES "/x64-linux-release/")
    biopic_apply_release_only_triplet_policy(
      "OpenCV is installed from the x64-linux-release vcpkg triplet")
  elseif(DEFINED OpenCV_DIR AND OpenCV_DIR MATCHES "/arm64-osx-release/")
    biopic_apply_release_only_triplet_policy(
      "OpenCV is installed from the arm64-osx-release vcpkg triplet")
  endif()
endmacro()
