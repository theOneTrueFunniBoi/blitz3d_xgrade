#include "bbaudio_soloud.h"

#include "soloud.h"
#include "soloud_wav.h"
#include "soloud_wavstream.h"

SoLoud::Soloud *soloud;

namespace {
	std::set<Sound *> sounds;

	inline bool debugSound(Sound *sound,const char* a) {
		if (::debug)
		{
			if (sounds.find(sound) == sounds.end())
			{
				RTEX("Sound does not exist"); // NOLINT
				return false;
			}
		} else {
			if (sounds.find(sound) == sounds.end())
			{
				errorLog.push_back(std::string(a)+std::string(": Sound does not exist"));
				return false;
			}
		}
		return true;
	}

	inline bool debugChannel(uint32_t channel,const char* a) {
		if (::debug)
		{
			if (!soloud)
			{
				RTEX("Soloud has not been initialized"); // NOLINT
				return false;
			}
		} else {
			if (!soloud)
			{
				errorLog.push_back(std::string(a)+std::string(": Soloud has not been initialized"));
				return false;
			}
		}
		return true;
	}

	SoLoud::WavStream musicStream;
	uint32_t musicChannel;

	float rolloffFactor = 1.0f;
	float dopplerFactor = 1.0f;
	float distanceFactor = 1.0f;
}

struct Sound {
	SoLoud::Wav wav;

	float volume{1};
	float pan{0};
	int pitch{0};

	explicit Sound() {
		if (debug) sounds.insert(this);
	}

	~Sound() {
		if (debug) sounds.erase(this);
	}
};

Sound *bbLoadSound(BBStr *path) {
	if (!soloud) {
		delete path;
		return nullptr;
	}
	auto sound = new Sound();
	auto r = sound->wav.load(path->c_str());
	delete path;
	if (r == SoLoud::SO_NO_ERROR) return sound;
	delete sound;
	return nullptr;
}

void bbFreeSound(Sound *sound) {
	if (!sound) return;
	if (debugSound( sound, "FreeSound" )) {
		delete sound;
	}
}

void bbLoopSound(Sound *sound) {
	if (!sound) return;
	if (debugSound( sound, "LoopSound" )) {
		sound->wav.setLooping(true);
	}
}

void bbSoundVolume(Sound *sound, float volume) {
	if (!sound) return;
	if (debugSound( sound, "SoundVolume" )) {
		sound->volume = volume;
	}
}

void bbSoundPan(Sound *sound, float pan) {
	if (!sound) return;
	if (debugSound( sound, "SoundPan" )) {
		sound->pan = pan;
	}
}

void bbSoundPitch(Sound *sound, int pitch) {
	if (!sound) return;
	if (debugSound( sound, "SoundPitch" )) {
		sound->pitch = pitch;
	}
}

uint32_t bbPlaySound(Sound *sound) {
	if (!sound) return 0;
	if (!debugSound( sound, "PlaySound" )) return 0;
	uint32_t chan;
	if (sound->pitch) {
		chan = soloud->play(sound->wav, sound->volume, sound->pan, true, 0);
		soloud->setSamplerate(chan, (float) sound->pitch);
		soloud->setPause(chan, false);
		return chan;
	}
	return soloud->play(sound->wav, sound->volume, sound->pan, false, 0);
}

uint32_t bbPlay3dSound(Sound *sound, float x, float y, float z, float vx, float vy, float vz) {
	if (!sound) return 0;
	if (!debugSound( sound, "Play3dSound" )) return 0;
	debugSound(sound,"Play3dSound");
	sound->wav.set3dAttenuation(SoLoud::AudioSource::INVERSE_DISTANCE, rolloffFactor);
	sound->wav.set3dDopplerFactor(dopplerFactor);
	if (sound->pitch) {
		auto chan = soloud->play3d(sound->wav, x, y, z, vx, vy, vz, sound->volume, true, 0);
		soloud->setSamplerate(chan, (float) sound->pitch);
		soloud->setPause(chan, false);
		return chan;
	}
	return soloud->play3d(sound->wav, x, y, z, vx, vy, vz, sound->volume, false, 0);
}

uint32_t bbPlayMusic(BBStr *path) {
	if (!soloud) {
		delete path;
		return 0;
	}
	if (musicChannel) {
		soloud->stop(musicChannel);
		musicChannel = 0;
	}
	auto r = musicStream.load(path->c_str());
	delete path;
	if (r != SoLoud::SO_NO_ERROR) return 0;
	return musicChannel = soloud->play(musicStream);
}

void bbStopChannel(uint32_t channel) {
	if (!channel) return;
	if (debugChannel( channel, "StopChannel" )) {
		soloud->stop(channel);
	}
}

void bbPauseChannel(uint32_t channel) {
	if (!channel) return;
	if (debugChannel( channel, "PauseChannel" )) {
		soloud->setPause(channel, true);
	}
}

void bbResumeChannel(uint32_t channel) {
	if (!channel) return;
	if (debugChannel( channel, "ResumeChannel" )) {
		soloud->setPause(channel, false);
	}
}

void bbChannelVolume(uint32_t channel, float volume) {
	if (!channel) return;
	if (debugChannel( channel, "ChannelVolume" )) {
		soloud->setVolume(channel, volume);
	}
}

void bbChannelPan(uint32_t channel, float pan) {
	if (!channel) return;
	if (debugChannel( channel, "ChannelPan" )) {
		soloud->setPan(channel, pan);
	}
}

void bbChannelPitch(uint32_t channel, int pitch) {
	if (!channel) return;
	if (debugChannel( channel, "ChannelPitch" )) {
		soloud->setSamplerate(channel, (float) pitch);
	}
}

int bbChannelPlaying(uint32_t channel) {
	if (!channel) return 0;
	if (debugChannel( channel, "ChannelPlaying" )) {
		return soloud->isValidVoiceHandle(channel);
	}
}

void bbSet3dChannel(uint32_t channel, float x, float y, float z, float vx, float vy, float vz) {
	if (!channel) return;
	if (debugChannel( channel, "Set3dChannel" )) {
		auto sc = distanceFactor;
		soloud->set3dSourceParameters(channel, x * sc, y * sc, z * sc, vx * sc, vy * sc, vz * sc);
	}
}

void bbSet3dListenerConfig(float roll, float dopp, float dist) {
	if (!soloud) return;
	rolloffFactor = roll;
	dopplerFactor = dopp;
	distanceFactor = dist;
}

void bbSet3dListener(float x, float y, float z, float kx, float ky, float kz, float jx, float jy, float jz, float vx,
					 float vy, float vz) {
	if (!soloud) return;
	auto sc = distanceFactor;
	soloud->set3dListenerParameters(x * sc, y * sc, z * sc, kx, ky, kz, jy, jy, jz, vx * sc, vy * sc, vz * sc);
	soloud->update3dAudio();
}

bool audio_create() {
	soloud = new SoLoud::Soloud();
	if (soloud->init(SoLoud::Soloud::LEFT_HANDED_3D) != SoLoud::SO_NO_ERROR) {
		delete soloud;
		soloud = nullptr;
	}
	return true;
}

bool audio_destroy() {
	if (!soloud) return true;
	soloud->deinit();
	delete soloud;
	soloud = nullptr;
	return true;
}

void audio_link(void(*rtSym)(const char *, void *)) {
	rtSym("%LoadSound$filename", bbLoadSound);
	rtSym("%Load3DSound$filename", bbLoadSound);
	rtSym("FreeSound%sound", bbFreeSound);
	rtSym("LoopSound%sound", bbLoopSound);
	rtSym("SoundPitch%sound%pitch", bbSoundPitch);
	rtSym("SoundVolume%sound#volume", bbSoundVolume);
	rtSym("SoundPan%sound#pan", bbSoundPan);
	rtSym("%PlaySound%sound", bbPlaySound);
	rtSym("%PlayMusic$midifile", bbPlayMusic);
	rtSym("StopChannel%channel", bbStopChannel);
	rtSym("PauseChannel%channel", bbPauseChannel);
	rtSym("ResumeChannel%channel", bbResumeChannel);
	rtSym("ChannelPitch%channel%pitch", bbChannelPitch);
	rtSym("ChannelVolume%channel#volume", bbChannelVolume);
	rtSym("ChannelPan%channel#pan", bbChannelPan);
	rtSym("%ChannelPlaying%channel", bbChannelPlaying);
}
