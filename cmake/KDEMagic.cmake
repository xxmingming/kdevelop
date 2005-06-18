#this file contains the following macros
#KDE_MAGIC_FILE_HANDLING
#KDE_CREATE_FINAL_FILE(filename srcs...)
#KDE_ADD_KPART(target srcs....)
#KDE_ADD_KLM()
#KDE_ADD_EXECUTABLE()
#and the following user adjustable options:
#KDE_ENAABLE_FINAL
#KDE_BUILD_TESTS

OPTION(KDE_ENABLE_FINAL "Enable final all-in-one compilation")
OPTION(KDE_BUILD_TESTS  "Build the tests")

MACRO(KDE_MAGIC_FILE_HANDLING _srcs_NAME)
   FOREACH (_current_FILE ${ARGN})

	  GET_FILENAME_COMPONENT(_ext ${_current_FILE} EXT)

      IF(${_ext} STREQUAL ".ui")
         SET(_magic_UIS ${_magic_UIS} ${_current_FILE})
      ENDIF(${_ext} STREQUAL ".ui")

      IF(${_ext} STREQUAL ".c" OR ${_ext} STREQUAL ".C" OR ${_ext} STREQUAL ".cpp" OR ${_ext} STREQUAL ".cxx" )
         SET(${_srcs_NAME} ${${_srcs_NAME}} ${_current_FILE})
      ENDIF(${_ext} STREQUAL ".c" OR ${_ext} STREQUAL ".C" OR ${_ext} STREQUAL ".cpp" OR ${_ext} STREQUAL ".cxx" )

   ENDFOREACH (_current_FILE)

   IF(_magic_UIS)
      KDE_ADD_UI_FILES(${_srcs_NAME} ${_magic_UIS} )
   ENDIF(_magic_UIS)

   KDE_AUTOMOC(${${_srcs_NAME}})
ENDMACRO(KDE_MAGIC_FILE_HANDLING _target _SRCS)


MACRO(KDE_CREATE_FINAL_FILE _filename)
   FILE(WRITE ${_filename} "//autogenerated file\n")
   FOREACH (_current_FILE ${ARGN})
      FILE(APPEND ${_filename} "#include \"${_current_FILE}\"\n")
   ENDFOREACH (_current_FILE)

ENDMACRO(KDE_CREATE_FINAL_FILE _filename)

MACRO(KDE_ADD_KPART _target_NAME )

   KDE_MAGIC_FILE_HANDLING(_magic_SRCS ${ARGN} )

   IF (KDE_ENABLE_FINAL)
      KDE_CREATE_FINAL_FILE(${_target_NAME}_final.cpp ${_magic_SRCS})
      ADD_LIBRARY(${_target_NAME} MODULE  ${_target_NAME}_final.cpp)
   ELSE (KDE_ENABLE_FINAL)
      ADD_LIBRARY(${_target_NAME} MODULE ${_magic_SRCS} )
   ENDIF (KDE_ENABLE_FINAL)

   KDE_CREATE_LIBTOOL_FILE(${_target_NAME})

ENDMACRO(KDE_ADD_KPART _target_NAME)

MACRO(KDE_ADD_KLM _target_NAME )

   KDE_MAGIC_FILE_HANDLING(_magic_SRCS ${ARGN} )

   IF (KDE_ENABLE_FINAL)
      KDE_CREATE_FINAL_FILE(${_target_NAME}_final.cpp ${_magic_SRCS})
      ADD_LIBRARY(kdeinit_${_target_NAME} SHARED  ${_target_NAME}_final.cpp)
   ELSE (KDE_ENABLE_FINAL)
      ADD_LIBRARY(kdeinit_${_target_NAME} SHARED ${_magic_SRCS} )
   ENDIF (KDE_ENABLE_FINAL)

   CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/cmake&kdeinit_dummy.cpp.in ${CMAKE_CURRENT_BINARY_DIR}/${_target_NAME}_dummy.cpp)

   ADD_EXECUTABLE( ${_target_NAME} ${CMAKE_CURRENT_BINARY_DIR}/${_target_NAME}_dummy.cpp )
   TARGET_LINK_LIBRARIES( ${_target_NAME} kdeinit_${_target_NAME} )

ENDMACRO(KDE_ADD_KLM _target_NAME)

MACRO(KDE_ADD_EXECUTABLE _target_NAME )

   KDE_MAGIC_FILE_HANDLING(_magic_SRCS ${ARGN} )

   IF (KDE_ENABLE_FINAL)
      KDE_CREATE_FINAL_FILE(${_target_NAME}_final.cpp ${_magic_SRCS})
      ADD_EXECUTABLE(${_target_NAME} ${_target_NAME}_final.cpp)
   ELSE (KDE_ENABLE_FINAL)
      ADD_EXECUTABLE(${_target_NAME} ${_magic_SRCS} )
   ENDIF (KDE_ENABLE_FINAL)

ENDMACRO(KDE_ADD_EXECUTABLE _target_NAME)


#MACRO(KDE_ADD_CONV_LIB _target_NAME)

#ENDMACRO(KDE_ADD_CONV_LIB _target_NAME)

