# -*- cmake -*-
include(Prebuilt)

if (STANDALONE)
  include(FindPkgConfig)

  pkg_check_modules(GSTREAMER010 REQUIRED gstreamer-0.10)
  pkg_check_modules(GSTREAMER010_PLUGINS_BASE REQUIRED gstreamer-plugins-base-0.10)
elseif (LINUX)
  use_prebuilt_binary(gstreamer)
  # possible libxml should have its own .cmake file instead
  use_prebuilt_binary(libxml)
  set(GSTREAMER010_FOUND ON FORCE BOOL)
  set(GSTREAMER010_PLUGINS_BASE_FOUND ON FORCE BOOL)
  set(GSTREAMER010_INCLUDE_DIRS
      ${LIBS_PREBUILT_DIR}/${LL_ARCH_DIR}/include/gstreamer-0.10
      ${LIBS_PREBUILT_DIR}/${LL_ARCH_DIR}/include/glib-2.0
      ${LIBS_PREBUILT_DIR}/${LL_ARCH_DIR}/include/libxml2
      )
  set(GSTREAMER010_LIBRARIES
      gstvideo-0.10
      gstaudio-0.10
      gstbase-0.10
      gstreamer-0.10
      gobject-2.0
      gmodule-2.0
      dl
      gthread-2.0
      glib-2.0
      )
endif (STANDALONE)

if (GSTREAMER010_FOUND AND GSTREAMER010_PLUGINS_BASE_FOUND)
  set(GSTREAMER010 ON CACHE BOOL "Build with GStreamer-0.10 streaming media support.")
endif (GSTREAMER010_FOUND AND GSTREAMER010_PLUGINS_BASE_FOUND)

if (GSTREAMER010)
  add_definitions(-DLL_GSTREAMER010_ENABLED=1)
endif (GSTREAMER010)

