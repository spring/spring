
# only one of the two targets needs this
REMOVE_DEFINITIONS(-DNO_SOUND)

# Dummy Sound implementaiton
SET(noSoundSources
		IAudioChannel.cpp
		ISound.cpp
		SoundChannels.cpp
		NullSound.cpp
	)

ADD_LIBRARY(no-sound STATIC EXCLUDE_FROM_ALL ${noSoundSources})
set_target_properties(no-sound PROPERTIES COMPILE_FLAGS "-DNO_SOUND")


# Define default sound implementation
if    (NO_SOUND)
	SetGlobal(sound-impl no-sound)
else  (NO_SOUND)
	SetGlobal(sound-impl sound)
endif (NO_SOUND)


# Real Sound implementaiton
if    (NOT NO_SOUND)
	SET(soundSources
			${noSoundSources}
			ALShared.cpp
			EFX.cpp
			EFXfuncs.cpp
			EFXPresets.cpp
			AudioChannel.cpp
			OggStream.cpp
			Sound.cpp
			SoundBuffer.cpp
			SoundItem.cpp
			SoundSource.cpp
			VorbisShared.cpp
		)

	FIND_PACKAGE_STATIC(OpenAL REQUIRED)
	INCLUDE_DIRECTORIES(${OPENAL_INCLUDE_DIR})
	FIND_PACKAGE_STATIC(OggVorbis REQUIRED)
	INCLUDE_DIRECTORIES(${OGG_INCLUDE_DIR} ${VORBIS_INCLUDE_DIR})

	FIND_PACKAGE(SDL REQUIRED)
	INCLUDE_DIRECTORIES(${SDL_INCLUDE_DIR})

	INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/include/)
	INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/include/AL)

	ADD_LIBRARY(sound STATIC EXCLUDE_FROM_ALL ${soundSources})
	TARGET_LINK_LIBRARIES(sound ${VORBISFILE_LIBRARY} ${VORBIS_LIBRARY} ${OGG_LIBRARY})
	TARGET_LINK_LIBRARIES(sound ${OPENAL_LIBRARY})
	TARGET_LINK_LIBRARIES(sound ${SDL_LIBRARY})
endif (NOT NO_SOUND)

