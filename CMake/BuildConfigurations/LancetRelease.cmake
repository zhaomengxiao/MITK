message(STATUS "Configuring Lancet Build")

include(${CMAKE_CURRENT_LIST_DIR}/Default.cmake)

set(MITK_CONFIG_PACKAGES ${MITK_CONFIG_PACKAGES}
  MatchPoint
)

set(MITK_USE_OpenCV ON CACHE BOOL "MITK Use OpenCV Library" FORCE)
set(MITK_USE_OpenIGTLink ON CACHE BOOL "MITK Use OpenIGTLink Library" FORCE)

set(MITK_CONFIG_PLUGINS ${MITK_CONFIG_PLUGINS}
  org.mitk.gui.qt.multilabelsegmentation
  org.mitk.matchpoint.core.helper
  org.mitk.gui.qt.matchpoint.algorithm.browser
  org.mitk.gui.qt.matchpoint.algorithm.control
  org.mitk.gui.qt.matchpoint.mapper
  org.mitk.gui.qt.matchpoint.framereg
  org.mitk.gui.qt.matchpoint.visualizer
  org.mitk.gui.qt.matchpoint.evaluator
  org.mitk.gui.qt.matchpoint.manipulator
  org.mitk.gui.qt.igttracking
  org.mitk.gui.qt.openigtlink
  org.mitk.gui.qt.moviemaker
  org.mitk.planarfigure
)

if(NOT MITK_USE_SUPERBUILD)
  set(BUILD_CoreCmdApps ON CACHE BOOL "" FORCE)
endif()

set(MITK_VTK_DEBUG_LEAKS OFF CACHE BOOL "Enable VTK Debug Leaks" FORCE)

find_package(Doxygen REQUIRED)

# Ensure that the in-application help can be build
set(BLUEBERRY_QT_HELP_REQUIRED ON CACHE BOOL "Required Qt help documentation in plug-ins" FORCE)
