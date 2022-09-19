
####### Expanded from @PACKAGE_INIT@ by configure_package_config_file() #######
####### Any changes to this file will be overwritten by the next CMake run ####
####### The input file was draco-config.cmake.template                            ########

get_filename_component(PACKAGE_PREFIX_DIR "${CMAKE_CURRENT_LIST_DIR}/../../" ABSOLUTE)

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
set_and_check(DRACO_INCLUDE_DIR "/usr/local/include")
set_and_check(DRACO_LIBRARY_DIR "/usr/local/lib")
set(DRACO_NAMES
    draco.dll libdraco.dylib libdraco.so draco.lib libdraco.dll libdraco.a)
find_library(_DRACO_LIBRARY PATHS ${DRACO_LIBRARY_DIR} NAMES ${DRACO_NAMES})
set_and_check(DRACO_LIBRARY ${_DRACO_LIBRARY})
find_file(DRACO_LIBRARY_DLL
          PATHS ${DRACO_LIBRARY_DIR}
          NAMES draco.dll libdraco.dll)
set(DRACO_FOUND YES)
