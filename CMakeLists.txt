set(JASTERIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}")
message("  Path: ${JASTERIX_PATH}")

cmake_minimum_required(VERSION 3.3)
set ( CMAKE_BUILD_TYPE RelWithDebInfo ) #Debug RelWithDebInfo Release

project( jASTERIX )
SET(CPACK_PACKAGE_VERSION_MAJOR "0")
SET(CPACK_PACKAGE_VERSION_MINOR "1")
SET(CPACK_PACKAGE_VERSION_PATCH "3")

message("  System: ${CMAKE_SYSTEM}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if(EXISTS "/.dockerenv")
message("  Platform: Linux (inside docker)")
add_definitions ( -UNDEBUG -Wall -std=c++14 -fext-numeric-literals) # -DQT_NO_KEYWORDS
ELSE()
message("  Platform: Linux (local)")
add_definitions ( -UNDEBUG -Wall -std=c++17 -fno-omit-frame-pointer) # -DQT_NO_KEYWORDS -fext-numeric-literals
ENDIF()


set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed.
#set(CMAKE_AUTOMOC ON)

add_library(jasterix "")

set (USE_LOG4CPP true)
message("USE_LOG4CPP: ${USE_LOG4CPP}")

set (USE_OPENSSL true)
message("USE_OPENSSL: ${USE_OPENSSL}")

configure_file(include/jasterix/global.h.in include/jasterix/global.h)

find_package ( Boost 1.73.0 COMPONENTS program_options filesystem iostreams regex system stacktrace_backtrace REQUIRED)
message("  Boost_INCLUDE_DIRS: ${Boost_INCLUDE_DIRS}")
message("  Boost_LIBRARY_DIR: ${Boost_LIBRARY_DIR}")
message("  Boost_LIBRARIES: ${Boost_LIBRARIES}")

find_package(Threads REQUIRED)

include_directories (
    ${Boost_INCLUDE_DIRS}
    ${JASTERIX_PATH}/include/
    )

link_directories(
    ${Boost_LIBRARY_DIR}
    )

target_link_libraries ( jasterix
    ${Boost_LIBRARIES}
    Threads::Threads
    )

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH};${JASTERIX_PATH}/cmake_modules)
message("  CMAKE_MODULE_PATH: ${CMAKE_MODULE_PATH}") 


IF (USE_LOG4CPP)
find_package ( LOG4CPP REQUIRED )
message("  LOG4CPP_INCLUDE_DIR: ${LOG4CPP_INCLUDE_DIR}")
message("  LOG4CPP_LIBRARY: ${LOG4CPP_LIBRARIES}") 

target_link_libraries ( jasterix
    ${LOG4CPP_LIBRARIES}
    )
ENDIF()

IF (USE_OPENSSL)
find_package ( OpenSSL REQUIRED )
message("  OPENSSL_INCLUDE_DIR: ${OPENSSL_INCLUDE_DIR}")
message("  OPENSSL_CRYPTO_LIBRARY: ${OPENSSL_CRYPTO_LIBRARY}")

target_link_libraries ( jasterix
    ${OPENSSL_CRYPTO_LIBRARY}
    )
ENDIF()

find_package(TBB REQUIRED)

find_package ( LibArchive REQUIRED )
message("  LibArchive_INCLUDE_DIRS: ${LibArchive_INCLUDE_DIRS}")
message("  LibArchive_LIBRARIES: ${LibArchive_LIBRARIES}")


message("Install path: ${CMAKE_INSTALL_PREFIX}")

include(lib/CMakeLists.txt)
include(include/jasterix/CMakeLists.txt)
include(src/CMakeLists.txt)

include_directories (
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${PROJECT_BINARY_DIR}/include
    ${Boost_INCLUDE_DIRS}
    ${LibArchive_INCLUDE_DIRS}
    )

target_link_libraries ( jasterix
    #${TBB_LIBRARIES}
    TBB::tbb
    ${LibArchive_LIBRARIES}
    ${Boost_LIBRARIES}
    )

target_sources(jasterix
    PUBLIC
        "${LIB_DIR}/json.hpp"
    PRIVATE
)    
    
target_compile_definitions(jasterix PRIVATE
    BOOST_STACKTRACE_USE_BACKTRACE
    #BOOST_ENABLE_ASSERT_HANDLER
    BOOST_STACKTRACE_LINK
)    
    
message("Installing using prefix: ${CMAKE_INSTALL_PREFIX}")
#install(DIRECTORY "specifications" DESTINATION jasterix)

set_target_properties(jasterix PROPERTIES PUBLIC_HEADER
    "include/jasterix/jasterix.h;include/jasterix/category.h;include/jasterix/editionbase.h;include/jasterix/edition.h;include/jasterix/refedition.h;include/jasterix/ref.h;include/jasterix/spfedition.h;include/jasterix/spf.h;include/jasterix/mapping.h;include/jasterix/frameparser.h;include/jasterix/record.h;include/jasterix/iteminfo.h;include/jasterix/itemparserbase.h;build/include/jasterix/global.h")

#install (TARGETS jasterix DESTINATION lib)
install (TARGETS jasterix DESTINATION lib
    PUBLIC_HEADER DESTINATION include/jasterix)

install (TARGETS jasterix_client DESTINATION bin)

#install(FILES src/jasterix.h DESTINATION include)

# build a CPack driven installer package
include (InstallRequiredSystemLibraries)
set (CPACK_RESOURCE_FILE_LICENSE
     "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
include (CPack)


# add a target to generate API documentation with Doxygen
#find_package(Doxygen)
#if(DOXYGEN_FOUND)
#    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/doc/Doxyfile ${CMAKE_CURRENT_SOURCE_DIR}/doc/Doxyfile.in @ONLY)
#    add_custom_target(doc
#        ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/doc/Doxyfile
#        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/doc
#        COMMENT "Generating API documentation with Doxygen" VERBATIM
#        )
#endif(DOXYGEN_FOUND)

#file(GLOB DOT ".*")
#file(GLOB TILD "*~")
#file(GLOB TEMP "*.cmake")





