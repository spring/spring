
# only one of the two targets needs this
REMOVE_DEFINITIONS(-DNO_SOUND)

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

	ADD_LIBRARY(sound STATIC EXCLUDE_FROM_ALL ${soundSources})
	TARGET_LINK_LIBRARIES(sound ${OPENAL_LIBRARY} ${OGG_LIBRARY} ${VORBISFILE_LIBRARY} ${VORBIS_LIBRARY})
endif (NOT NO_SOUND)

