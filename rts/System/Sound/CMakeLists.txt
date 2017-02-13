
# only one of the two targets needs this
REMOVE_DEFINITIONS(-DNO_SOUND)

# Dummy Sound implementaiton
SET(noSoundSources
		IAudioChannel.cpp
		ISound.cpp
		Null/SoundChannels.cpp
		Null/NullSound.cpp
	)

ADD_LIBRARY(no-sound STATIC EXCLUDE_FROM_ALL ${noSoundSources})
set_target_properties(no-sound PROPERTIES COMPILE_FLAGS "-DNO_SOUND")


# Define default sound implementation
if    (NO_SOUND)
	set(sound-impl no-sound PARENT_SCOPE)
else  (NO_SOUND)
	set(sound-impl sound PARENT_SCOPE)
endif (NO_SOUND)


# Real Sound implementaiton
if    (NOT NO_SOUND)
	SET(soundSources
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

	FIND_PACKAGE_STATIC(OpenAL REQUIRED)
	INCLUDE_DIRECTORIES(${OPENAL_INCLUDE_DIR})
	FIND_PACKAGE_STATIC(OggVorbis REQUIRED)
	INCLUDE_DIRECTORIES(${OGG_INCLUDE_DIR} ${VORBIS_INCLUDE_DIR})

	INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/include/)
	INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/include/AL)

	ADD_LIBRARY(sound STATIC EXCLUDE_FROM_ALL ${soundSources})
	TARGET_LINK_LIBRARIES(sound ${VORBISFILE_LIBRARY} ${VORBIS_LIBRARY} ${OGG_LIBRARY})
	TARGET_LINK_LIBRARIES(sound ${OPENAL_LIBRARY})
endif (NOT NO_SOUND)

