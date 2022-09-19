# Generates a macro to auto-configure everything

####### Expanded from @PACKAGE_INIT@ by configure_package_config_file() #######
####### Any changes to this file will be overwritten by the next CMake run ####
####### The input file was httplibConfig.cmake.in                            ########

get_filename_component(PACKAGE_PREFIX_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../" ABSOLUTE)

macro(set_and_check _var _file)
  set(${_var} "${_file}")
  if(NOT EXISTS "${_file}")
    message(FATAL_ERROR "File or directory ${_file} referenced by variable ${_var} does not exist !")
  endif()
endmacro()

####################################################################################

# Setting these here so they're accessible after install.
# Might be useful for some users to check which settings were used.
set(HTTPLIB_IS_USING_OPENSSL )
set(HTTPLIB_IS_USING_ZLIB )
set(HTTPLIB_IS_COMPILED OFF)
set(HTTPLIB_IS_USING_BROTLI )
set(HTTPLIB_VERSION 0.10.3)

include(CMakeFindDependencyMacro)

# We add find_dependency calls here to not make the end-user have to call them.
find_dependency(Threads REQUIRED)
if()
	# OpenSSL COMPONENTS were added in Cmake v3.11
	if(CMAKE_VERSION VERSION_LESS "3.11")
		find_dependency(OpenSSL 1.1.1 REQUIRED)
	else()
		# Once the COMPONENTS were added, they were made optional when not specified.
		# Since we use both, we need to search for both.
		find_dependency(OpenSSL 1.1.1 COMPONENTS Crypto SSL REQUIRED)
	endif()
endif()
if()
	find_dependency(ZLIB REQUIRED)
endif()

if()
	# Needed so we can use our own FindBrotli.cmake in this file.
	# Note that the FindBrotli.cmake file is installed in the same dir as this file.
	list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}")
	set(BROTLI_USE_STATIC_LIBS )
	find_dependency(Brotli COMPONENTS common encoder decoder REQUIRED)
endif()

# Mildly useful for end-users
# Not really recommended to be used though
set_and_check(HTTPLIB_INCLUDE_DIR "${PACKAGE_PREFIX_DIR}/include")
# Lets the end-user find the header path with the header appended
# This is helpful if you're using Cmake's pre-compiled header feature
set_and_check(HTTPLIB_HEADER_PATH "${PACKAGE_PREFIX_DIR}/include/httplib.h")

# Brings in the target library
include("${CMAKE_CURRENT_LIST_DIR}/httplibTargets.cmake")

# Ouputs a "found httplib /usr/include/httplib.h" message when using find_package(httplib)
include(FindPackageMessage)
if(TARGET httplib::httplib)
	set(HTTPLIB_FOUND TRUE)

	# Since the compiled version has a lib, show that in the message
	if(OFF)
		# The list of configurations is most likely just 1 unless they installed a debug & release
		get_target_property(_httplib_configs httplib::httplib "IMPORTED_CONFIGURATIONS")
		# Need to loop since the "IMPORTED_LOCATION" property isn't want we want.
		# Instead, we need to find the IMPORTED_LOCATION_RELEASE or IMPORTED_LOCATION_DEBUG which has the lib path.
		foreach(_httplib_conf "${_httplib_configs}")
			# Grab the path to the lib and sets it to HTTPLIB_LIBRARY
			get_target_property(HTTPLIB_LIBRARY httplib::httplib "IMPORTED_LOCATION_${_httplib_conf}")
			# Check if we found it
			if(HTTPLIB_LIBRARY)
				break()
			endif()
		endforeach()

		unset(_httplib_configs)
		unset(_httplib_conf)

		find_package_message(httplib "Found httplib: ${HTTPLIB_LIBRARY} (found version \"${HTTPLIB_VERSION}\")" "[${HTTPLIB_LIBRARY}][${HTTPLIB_HEADER_PATH}]")
	else()
		find_package_message(httplib "Found httplib: ${HTTPLIB_HEADER_PATH} (found version \"${HTTPLIB_VERSION}\")" "[${HTTPLIB_HEADER_PATH}]")
	endif()
endif()
