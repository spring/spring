
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

	find_package_static(OpenAL REQUIRED)
	find_package_static(OggVorbis REQUIRED)

	find_package(SDL2 REQUIRED)

	add_library(sound STATIC EXCLUDE_FROM_ALL ${soundSources})
	target_link_libraries(sound ${VORBISFILE_LIBRARY} ${VORBIS_LIBRARY} ${OGG_LIBRARY})
	target_link_libraries(sound ${OPENAL_LIBRARY})
	target_include_directories(sound PRIVATE
		${Spring_SOURCE_DIR}/rts
		${OPENAL_INCLUDE_DIR}
		${OGG_INCLUDE_DIR} ${VORBIS_INCLUDE_DIR}
		${SDL2_INCLUDE_DIR}
	)
endif ()
