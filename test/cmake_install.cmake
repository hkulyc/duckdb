# Install script for directory: /Users/lyc/Downloads/duckdb/test

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/Library/Developer/CommandLineTools/usr/bin/objdump")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/Users/lyc/Downloads/duckdb/test/api/cmake_install.cmake")
  include("/Users/lyc/Downloads/duckdb/test/appender/cmake_install.cmake")
  include("/Users/lyc/Downloads/duckdb/test/arrow_roundtrip/cmake_install.cmake")
  include("/Users/lyc/Downloads/duckdb/test/common/cmake_install.cmake")
  include("/Users/lyc/Downloads/duckdb/test/extension/cmake_install.cmake")
  include("/Users/lyc/Downloads/duckdb/test/helpers/cmake_install.cmake")
  include("/Users/lyc/Downloads/duckdb/test/memoryleak/cmake_install.cmake")
  include("/Users/lyc/Downloads/duckdb/test/sql/cmake_install.cmake")
  include("/Users/lyc/Downloads/duckdb/test/sqlite/cmake_install.cmake")
  include("/Users/lyc/Downloads/duckdb/test/ossfuzz/cmake_install.cmake")
  include("/Users/lyc/Downloads/duckdb/test/mbedtls/cmake_install.cmake")
  include("/Users/lyc/Downloads/duckdb/test/persistence/cmake_install.cmake")

endif()

