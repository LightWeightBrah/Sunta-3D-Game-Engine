# Adds the platform specific icon files of a program to <sources_variable>
#
# custom_resources_dir: folder that may contain App/ with the program's own icon
#                       if it doesn't, the default Sunta Engine icon is used
function(sunta_add_app_icon sources_variable custom_resources_dir)

	set(default_icon_dir "${CMAKE_SOURCE_DIR}/Sunta/res/App")
	set(custom_icon_dir  "${custom_resources_dir}/App")
	set(updated_sources  ${${sources_variable}})

	# Windows
	if(WIN32)
		if(EXISTS "${custom_icon_dir}/resource.rc")
			message(STATUS "${PROJECT_NAME}: Using custom executable icon")
			list(APPEND updated_sources "${custom_icon_dir}/resource.rc")
		elseif(EXISTS "${default_icon_dir}/resource.rc")
			message(STATUS "${PROJECT_NAME}: Using default Sunta Engine executable icon")
			list(APPEND updated_sources "${default_icon_dir}/resource.rc")
		endif()

	# Linux
	elseif(UNIX AND NOT APPLE)
		message(STATUS "${PROJECT_NAME}: Linux platform detected (Icons handled via desktop files and runtime GLFW)")

		if(EXISTS "${custom_icon_dir}/app.desktop.in")
			set(desktop_template "${custom_icon_dir}/app.desktop.in")
		elseif(EXISTS "${default_icon_dir}/app.desktop.in")
			set(desktop_template "${default_icon_dir}/app.desktop.in")
		endif()

		if(desktop_template)
			configure_file("${desktop_template}" "${SUNTA_BIN_DIR}/${PROJECT_NAME}.desktop" @ONLY)
		endif()

	# Mac
	elseif(APPLE)
		if(EXISTS "${custom_icon_dir}/window_icon.icns")
			set(mac_icon "${custom_icon_dir}/window_icon.icns")
		elseif(EXISTS "${default_icon_dir}/engine_logo.icns")
			set(mac_icon "${default_icon_dir}/engine_logo.icns")
		endif()

		if(mac_icon)
			message(STATUS "${PROJECT_NAME}: Using App Icon for MacOS Bundle")
			get_filename_component(mac_icon_name "${mac_icon}" NAME)
			set(MACOSX_BUNDLE_ICON_FILE ${mac_icon_name} PARENT_SCOPE)
			list(APPEND updated_sources "${mac_icon}")
			set_source_files_properties("${mac_icon}" PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
		endif()
	endif()

	set(${sources_variable} ${updated_sources} PARENT_SCOPE)

endfunction()
