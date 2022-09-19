set(WebP_VERSION 1.2.4)
set(WEBP_VERSION ${WebP_VERSION})


####### Expanded from @PACKAGE_INIT@ by configure_package_config_file() #######
####### Any changes to this file will be overwritten by the next CMake run ####
####### The input file was WebPConfig.cmake.in                            ########

get_filename_component(PACKAGE_PREFIX_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../" ABSOLUTE)

macro(set_and_check _var _file)
  set(${_var} "${_file}")
  if(NOT EXISTS "${_file}")
    message(FATAL_ERROR "File or directory ${_file} referenced by variable ${_var} does not exist !")
  endif()
endmacro()

macro(check_required_components _NAME)
  foreach(comp ${${_NAME}_FIND_COMPONENTS})
    if(NOT ${_NAME}_${comp}_FOUND)
      if(${_NAME}_FIND_REQUIRED_${comp})
        set(${_NAME}_FOUND FALSE)
      endif()
    endif()
  endforeach()
endmacro()

####################################################################################

include ("${CMAKE_CURRENT_LIST_DIR}/WebPTargets.cmake")

set(WebP_INCLUDE_DIRS "/usr/local/include")
set(WEBP_INCLUDE_DIRS ${WebP_INCLUDE_DIRS})
set(WebP_LIBRARIES "webpdecoder;webp;webpdemux")
set(WEBP_LIBRARIES "${WebP_LIBRARIES}")
