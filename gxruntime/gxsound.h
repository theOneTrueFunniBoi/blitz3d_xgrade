
#ifndef GXSOUND_H
#define GXSOUND_H

#include "gxchannel.h"

class gxAudio;
//struct FSOUND_SAMPLE;

class gxSound{
public:
	gxAudio *audio;

	gxSound(gxAudio* audio, ALuint sample);
	~gxSound();

private:
	float def_gain, def_pitch;
	bool def_loop;
	ALuint sample;
	float pos[3],vel[3];

	//void setDefaults();

	/***** GX INTERFACE *****/
public:
	//actions
	gxChannel *play();
	gxChannel *play3d( const float pos[3],const float vel[3] );

	//modifiers
	void setLoop(bool loop);
	void setPitch(float pitch);
	void setVolume(float volume);
	//void setPan( float pan );
};

#endif
