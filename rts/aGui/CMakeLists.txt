add_library(engineaGui STATIC EXCLUDE_FROM_ALL
	Button.cpp
	Gui.cpp
	GuiElement.cpp
	HorizontalLayout.cpp
	LineEdit.cpp
	List.cpp
	Picture.cpp
	TextElement.cpp
	VerticalLayout.cpp
	Window.cpp
)
target_include_directories(engineaGui PRIVATE
		${Spring_SOURCE_DIR}/rts
		${SDL2_INCLUDE_DIR}
		${GLEW_INCLUDE_DIR}
	PUBLIC
		${Spring_SOURCE_DIR}/rts/lib/slimsig/include
)
