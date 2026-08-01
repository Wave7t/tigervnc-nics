if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  message(FATAL_ERROR "cmake must be invoked with the top level directory")
endif()

#
# Windows installer (Inno Setup)
#

if(WIN32)

if(CMAKE_SIZEOF_VOID_P MATCHES 8)
  set(INST_SUFFIX 64)
  set(INST_DEFS -DWIN64)
endif()

configure_file(tigervnc.iss.in tigervnc.iss)

add_custom_command(OUTPUT ${CMAKE_PROJECT_NAME}${INST_SUFFIX}-${VERSION}.exe
  COMMAND iscc -o. ${INST_DEFS} -F${CMAKE_PROJECT_NAME}${INST_SUFFIX}-${VERSION} tigervnc.iss
  DEPENDS vncviewer tigervnc.iss)
add_custom_target(installer DEPENDS ${CMAKE_PROJECT_NAME}${INST_SUFFIX}-${VERSION}.exe)

if(BUILD_WINVNC)
  configure_file(winvnc.iss.in winvnc.iss)

  add_custom_command(OUTPUT ${CMAKE_PROJECT_NAME}${INST_SUFFIX}-winvnc-${VERSION}.exe
    COMMAND iscc -o. ${INST_DEFS} -F${CMAKE_PROJECT_NAME}${INST_SUFFIX}-winvnc-${VERSION} winvnc.iss
    DEPENDS winvnc4 wm_hooks vncconfig winvnc.iss)
  add_custom_target(winvnc_installer DEPENDS ${CMAKE_PROJECT_NAME}${INST_SUFFIX}-winvnc-${VERSION}.exe)
endif()

endif() # WIN32


#
# Mac DMG
#

if(APPLE)

if(ENABLE_NLS)
  set(MACOS_PACKAGE_TRANSLATIONS 1)
  set(MACOS_DMG_DEPENDENCIES translations)
else()
  set(MACOS_PACKAGE_TRANSLATIONS 0)
endif()

configure_file(makemacapp.in makemacapp @ONLY)
configure_file(Info.plist.in Info.plist)
configure_file(BundleMacOS.cmake.in BundleMacOS.cmake @ONLY)

add_custom_command(OUTPUT TigerVNC-${VERSION}.dmg
  COMMAND sh makemacapp
  DEPENDS vncviewer ${MACOS_DMG_DEPENDENCIES}
          makemacapp Info.plist BundleMacOS.cmake)
add_custom_target(dmg DEPENDS TigerVNC-${VERSION}.dmg)

endif() # APPLE


#
# Binary tarball
#

if(UNIX)

configure_file(maketarball.in maketarball)

if(BUILD_JAVA)
  set(TARBALL_JAVA_DEPENDENCY java)
endif()

set(PACKAGE_FILE ${CMAKE_PROJECT_NAME}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}-${VERSION}.tar.gz)
add_custom_command(OUTPUT ${PACKAGE_FILE}
  COMMAND bash maketarball
  DEPENDS maketarball vncviewer vncpasswd vncconfig ${TARBALL_JAVA_DEPENDENCY})

add_custom_target(tarball DEPENDS ${PACKAGE_FILE})

endif() #UNIX

#
# Common
#

install(FILES ${CMAKE_SOURCE_DIR}/LICENCE.TXT DESTINATION ${CMAKE_INSTALL_FULL_DOCDIR})
install(FILES ${CMAKE_SOURCE_DIR}/README.rst DESTINATION ${CMAKE_INSTALL_FULL_DOCDIR})
