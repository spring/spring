
# only one of the two targets needs this
remove_definitions(-DNO_SOUND)

# Dummy Sound implementaiton
set(noSoundSources
		IAudioChannel.cpp
		ISound.cpp
		Null/SoundChannels.cpp
		Null/NullSound.cpp
	)

add_library(no-sound STATIC EXCLUDE_FROM_ALL ${noSoundSources})
set_target_properties(no-sound PROPERTIES COMPILE_FLAGS "-DNO_SOUND")
target_include_directories(no-sound PRIVATE
		${Spring_SOURCE_DIR}/rts
)

# Define default sound implementation
if    (NO_SOUND)
	set(sound-impl no-sound PARENT_SCOPE)
else ()
	set(sound-impl sound PARENT_SCOPE)
endif ()


# Real Sound implementaiton
if    (NOT NO_SOUND)
	set(soundSources
			ISound.cpp
			IAudioChannel.cpp
			Null/NullSound.cpp
			OpenAL/ALShared.cpp
			OpenAL/EFX.cpp
			OpenAL/EFXfuncs.cpp
			OpenAL/EFXPresets.cpp
			OpenAL/AudioChannel.cpp
			OpenAL/OggStream.cpp
			OpenAL/Sound.cpp
			OpenAL/SoundChannels.cpp
			OpenAL/SoundBuffer.cpp
			OpenAL/SoundItem.cpp
			OpenAL/SoundSource.cpp
			OpenAL/VorbisShared.cpp
		)

	pkg_check_modules(OPENAL REQUIRED openal)
	pkg_check_modules(OGG REQUIRED ogg)
	pkg_check_modules(VORBISFILE REQUIRED vorbisfile)

	add_library(sound STATIC EXCLUDE_FROM_ALL ${soundSources})
	target_link_libraries(sound
		PRIVATE
			${OPENAL_LIBRARIES}
			${VORBISFILE_LIBRARIES}
			${VORBIS_LIBRARIES}
			${OGG_LIBRARIES}
	)
	target_include_directories(sound PRIVATE
		${Spring_SOURCE_DIR}/rts
		${OPENAL_INCLUDE_DIRS}
		${OGG_INCLUDE_DIR} ${VORBIS_INCLUDE_DIR}
		${SDL2_INCLUDE_DIRS}
		${GLEW_INCLUDE_DIR}
	)
endif ()
