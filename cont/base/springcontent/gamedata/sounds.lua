--- Valid entries used by engine: IncomingChat, MultiSelect, MapPoint
--- other than that, you can give it any name and access it like before with filenames
local Sounds = {
	SoundItems = {
		IncomingChat = {
			--- always play on the front speaker(s)
			file = "sounds/beep4.wav",
			in3d = "false",
		},
		MultiSelect = {
			--- always play on the front speaker(s)
			file = "sounds/button9.wav",
			in3d = "false",
		},
		MapPoint = {
			--- respect where the point was set, but don't attuenuate in distace
			--- also, when moving the camera, don't pitch it
			file = "sounds/beep6.wav",
			rolloff = 0,
			dopplerscale = 0,
		},
		ExampleSound = {
			--- some things you can do with this file

			--- can be either ogg or wav
			file = "somedir/subdir/soundfile.ogg",

			--- loudness, > 1 is louder, < 1  is more quiet, you will most likely not set it to 0
			gain = 1,

			--- > 1 -> high pitched, < 1 lowered
			pitch = 1,

			--- If > 0.0 then this adds a random amount to gain each time the sound is played.
			--- Clamped between 0.0 and 1.0. The result is in the range [(gain * (1 + gainMod)), (gain * (1 - gainMod))].
			gainmod = 0.0,

			--- If > 0.0 then this adds a random amount to pitch each time the sound is played.
			--- Clamped between 0.0 and 1.0. The result is in the range [(pitch * (1 + pitchMod)), (pitch * (1 - pitchMod))].
			pitchmod = 0.0,

			--- how unit / camera speed affects the sound, to exagerate it, use values > 1
			--- dopplerscale = 0 completely disables the effect
			dopplerscale = 1,

			--- when lots of sounds are played, sounds with lwoer priority are more likely to get cut off
			--- priority > 0 will never be cut of (priorities can be negative)
			priority = 0,

			--- this sound will not be played more than 16 times at a time
			maxconcurrent = 16,

			--- cutoff distance
			maxdist = 20000,

			--- how fast it becomes more quiet in the distance (0 means aleays the same loudness regardless of dist)
			rolloff = 1,

			--- non-3d sounds do always came out of the front-speakers (or the center one)
			--- 3d sounds are, well, in 3d
			in3d = true,

			--- you can loop it for X miliseconds
			looptime = 0,
		},
		FailedCommand = {
			file = "sounds/beep3.wav",
		},

		default = {
			--- new since 89.0
			--- you can overwrite the fallback profile here (used when no corresponding SoundItem is defined for a sound)
			--gainmod = 0.35,
			--pitchmod = 0.3,
			--pitch = 0.7,
			--in3d = true,
		},
	},
}

return Sounds
