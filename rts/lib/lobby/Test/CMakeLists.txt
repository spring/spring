
set(lobbyTestAppSources
		"TestConnection"
		"main"
	)

add_executable(lobby-test EXCLUDE_FROM_ALL ${lobbyTestAppSources})
target_link_libraries(lobby-test lobby)

