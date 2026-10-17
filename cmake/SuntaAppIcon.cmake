# Adds the default Sunta Engine icon to a program (the editor and the SuntaRuntime template)
# The icon of a game is not set here: the editor puts it into the copy of SuntaRuntime when it builds the game (see GameBuilder)
#
#   Windows: the icon is a resource of the .exe (Sunta/res/App/resource.rc)
#   Linux:   an executable can't hold an icon, so a .desktop file is created next to the program
#   macOS:   the programs are plain executables, a game gets its icon in the .app bundle made by the editor
function(sunta_add_app_icon sources_variable)

	set(icon_dir "${CMAKE_SOURCE_DIR}/Sunta/res/App")
	set(updated_sources ${${sources_variable}})

	if(WIN32)
		if(EXISTS "${icon_dir}/resource.rc")
			list(APPEND updated_sources "${icon_dir}/resource.rc")
		endif()

	elseif(UNIX AND NOT APPLE)
		if(EXISTS "${icon_dir}/app.desktop.in")
			configure_file("${icon_dir}/app.desktop.in" "${SUNTA_BIN_DIR}/${PROJECT_NAME}.desktop" @ONLY)
		endif()
	endif()

	set(${sources_variable} ${updated_sources} PARENT_SCOPE)

endfunction()
