
# only one of the two targets needs this
REMOVE_DEFINITIONS(-DNO_SOUND)

# Dummy Sound implementaiton
SET(noSoundSources
		IAudioChannel
		IEffectChannel
		IMusicChannel
		ISound
		NullSound
		SoundLog
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
			ALShared
			EffectChannel
			MusicChannel
			OggStream
			Sound
			SoundBuffer
			SoundItem
			SoundSource
			VorbisShared
		)

	FIND_PACKAGE(OpenAL REQUIRED)
	INCLUDE_DIRECTORIES(${OPENAL_INCLUDE_DIR})
	FIND_PACKAGE(OggVorbis REQUIRED)

	FIND_PACKAGE(SDL REQUIRED)
	INCLUDE_DIRECTORIES(${SDL_INCLUDE_DIR})

	ADD_LIBRARY(sound STATIC EXCLUDE_FROM_ALL ${soundSources})
	TARGET_LINK_LIBRARIES(sound ${OPENAL_LIBRARY} ${OGG_LIBRARY} ${VORBISFILE_LIBRARY} ${VORBIS_LIBRARY} ${SDL_LIBRARY})
endif (NOT NO_SOUND)

