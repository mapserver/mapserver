# - Find PHP7
# This module finds if PHP7 is installed and determines where the include files
# and libraries are. It also determines what the name of the library is. This
# code sets the following variables:
#
#  PHP7_INCLUDE_PATH       = path to where php.h can be found
#  PHP7_EXECUTABLE         = full path to the php4 binary
#
#  file is derived from FindPHP5.cmake
#

SET(PHP7_FOUND "NO")

SET(PHP7_POSSIBLE_INCLUDE_PATHS
  /usr/include/php/*
  /usr/local/include/php/*
  /usr/include/php
  /usr/local/include/php
  /usr/local/apache/php
  ${PHP7_INCLUDES}
)

if(WIN32)
  SET(PHP7_POSSIBLE_INCLUDE_PATHS C:/php-sdk/phpmaster/$ENV{PHP_SDK_VC}/$ENV{VSCMD_ARG_TGT_ARCH}/php-src C:/dev/work/php-sdk/phpmaster/$ENV{PHP_SDK_VC}/$ENV{VSCMD_ARG_TGT_ARCH}/php-src)
endif(WIN32)

FIND_PATH(PHP7_FOUND_INCLUDE_PATH main/php.h ${PHP7_POSSIBLE_INCLUDE_PATHS})

find_library(PHP7_LIBRARY NAMES php7ts PATHS /sw /opt/local)
if(WIN32)
  find_library(PHP7_LIBRARY NAMES php7 php7ts PATHS ${PHP7_FOUND_INCLUDE_PATH}/Release ${PHP7_FOUND_INCLUDE_PATH}/Release_TS)
  if(CMAKE_CL_64)
    find_library(PHP7_LIBRARY NAMES php7 php7ts PATHS ${PHP7_FOUND_INCLUDE_PATH}/$ENV{VSCMD_ARG_TGT_ARCH}/Release ${PHP7_FOUND_INCLUDE_PATH}/$ENV{VSCMD_ARG_TGT_ARCH}/Release_TS)
  endif(CMAKE_CL_64)
endif(WIN32)

IF(PHP7_FOUND_INCLUDE_PATH)
  SET(php7_paths "${PHP7_POSSIBLE_INCLUDE_PATHS}")
  FOREACH(php7_path Zend main TSRM)
    SET(php7_paths ${php7_paths} "${PHP7_FOUND_INCLUDE_PATH}/${php7_path}")
  ENDFOREACH(php7_path Zend main TSRM)
  SET(PHP7_INCLUDE_PATH "${php7_paths}" INTERNAL "PHP7 include paths")
ENDIF(PHP7_FOUND_INCLUDE_PATH)

FIND_PROGRAM(PHP7_EXECUTABLE
  NAMES php7 php
  PATHS
  /usr/local/bin
  )

MARK_AS_ADVANCED(
  PHP7_EXECUTABLE
  PHP7_FOUND_INCLUDE_PATH
  )

IF( NOT PHP7_CONFIG_EXECUTABLE )
FIND_PROGRAM(PHP7_CONFIG_EXECUTABLE
  NAMES php7-config php-config
  )
ENDIF( NOT PHP7_CONFIG_EXECUTABLE )

IF(PHP7_CONFIG_EXECUTABLE)
  EXECUTE_PROCESS(COMMAND ${PHP7_CONFIG_EXECUTABLE} --version
    OUTPUT_VARIABLE PHP7_VERSION)
  STRING(REPLACE "\n" "" PHP7_VERSION "${PHP7_VERSION}")

  EXECUTE_PROCESS(COMMAND ${PHP7_CONFIG_EXECUTABLE} --extension-dir
    OUTPUT_VARIABLE PHP7_EXTENSION_DIR)
  STRING(REPLACE "\n" "" PHP7_EXTENSION_DIR "${PHP7_EXTENSION_DIR}")

  EXECUTE_PROCESS(COMMAND ${PHP7_CONFIG_EXECUTABLE} --includes
    OUTPUT_VARIABLE PHP7_INCLUDES)
  STRING(REPLACE "-I" "" PHP7_INCLUDES "${PHP7_INCLUDES}")
  STRING(REPLACE " " ";" PHP7_INCLUDES "${PHP7_INCLUDES}")
  STRING(REPLACE "\n" "" PHP7_INCLUDES "${PHP7_INCLUDES}")
  LIST(GET PHP7_INCLUDES 0 PHP7_INCLUDE_DIR)

  set(PHP7_MAIN_INCLUDE_DIR ${PHP7_INCLUDE_DIR}/main)
  set(PHP7_TSRM_INCLUDE_DIR ${PHP7_INCLUDE_DIR}/TSRM)
  set(PHP7_ZEND_INCLUDE_DIR ${PHP7_INCLUDE_DIR}/Zend)
  set(PHP7_REGEX_INCLUDE_DIR ${PHP7_INCLUDE_DIR}/regex)
  set(PHP7_EXT_INCLUDE_DIR ${PHP7_INCLUDE_DIR}/ext)
  set(PHP7_DATE_INCLUDE_DIR ${PHP7_INCLUDE_DIR}/ext/date/lib)
  set(PHP7_STANDARD_INCLUDE_DIR ${PHP7_INCLUDE_DIR}/ext/standard)

  MESSAGE(STATUS ${PHP7_MAIN_INCLUDE_DIR})

  IF(NOT PHP7_INCLUDE_PATH)
    set(PHP7_INCLUDE_PATH ${PHP7_INCLUDES})
  ENDIF(NOT PHP7_INCLUDE_PATH)

  IF(PHP7_VERSION LESS 5)
    MESSAGE(FATAL_ERROR "PHP version is not 5 or later")
  ENDIF(PHP7_VERSION LESS 5)

  IF(PHP7_EXECUTABLE AND PHP7_INCLUDES)
    set(PHP7_FOUND "yes")
    MESSAGE(STATUS "Found PHP7-Version ${PHP7_VERSION} (using ${PHP7_CONFIG_EXECUTABLE})")
  ENDIF(PHP7_EXECUTABLE AND PHP7_INCLUDES)

  FIND_PROGRAM(PHPUNIT_EXECUTABLE
    NAMES phpunit phpunit2
    PATHS
    /usr/local/bin
  )

  IF(PHPUNIT_EXECUTABLE)
    MESSAGE(STATUS "Found phpunit: ${PHPUNIT_EXECUTABLE}")
  ENDIF(PHPUNIT_EXECUTABLE)

ENDIF(PHP7_CONFIG_EXECUTABLE)
