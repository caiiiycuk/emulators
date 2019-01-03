//
// Created by caiiiycuk on 28.02.2020.
// originally was mixer.cpp
//
#include <string.h>
#include <sys/types.h>
#include <math.h>
#include <protocol.h>

#ifdef EMSCRIPTEN
#include <emscripten.h>
#endif
#if defined (WIN32)
//Midi listing
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <mmsystem.h>
#endif

#include "mem.h"
#include "pic.h"
#include "dosbox.h"
#include "mixer.h"
#include "timer.h"
#include "setup.h"
#include "cross.h"
#include "support.h"
#include "mapper.h"
#include "hardware.h"
#include "programs.h"
#include "midi.h"

#define MIXER_SSIZE 4

//#define MIXER_SHIFT 14
//#define MIXER_REMAIN ((1<<MIXER_SHIFT)-1)

constexpr int BLOCK_SIZE = 2048;
constexpr int PUSH_SIZE = 512;
float blockBuffer[BLOCK_SIZE];
static void MIXER_CallBack(float *stream, int len);

#define MIXER_VOLSHIFT 13

#define FREQ_SHIFT 14
#define FREQ_NEXT ( 1 << FREQ_SHIFT)
#define FREQ_MASK ( FREQ_NEXT -1 )

#define TICK_SHIFT 14
#define TICK_NEXT ( 1 << TICK_SHIFT)
#define TICK_MASK (TICK_NEXT -1)

typedef float mixer_t;
static INLINE float MIXER_CLIP(Bits SAMP) {
	Bit16s samp16s;
	if (SAMP < MAX_AUDIO) {
		if (SAMP > MIN_AUDIO)
			samp16s = SAMP;
		else samp16s = MIN_AUDIO;
	} else samp16s = MAX_AUDIO;
	return samp16s / 32768.0f;
}

static struct {
	Bit32s work[MIXER_BUFSIZE][2];
	//Write/Read pointers for the buffer
	Bitu pos,done;
	Bitu needed, min_needed, max_needed;
	//For every millisecond tick how many samples need to be generated
	Bit32u tick_add;
	Bit32u tick_counter;
	float mastervol[2];
	MixerChannel * channels;
	bool nosound;
	Bit32u freq;
	Bit32u blocksize;
} mixer;

Bit8u MixTemp[MIXER_BUFSIZE];

MixerChannel * MIXER_AddChannel(MIXER_Handler handler,Bitu freq,const char * name) {
	MixerChannel * chan=new MixerChannel();
	chan->scale = 1.0;
	chan->handler=handler;
	chan->name=name;
	chan->next=mixer.channels;
	chan->SetVolume(1,1);
	chan->enabled=false;
	chan->interpolate = false;
	chan->SetFreq(freq); //Sets interpolate as well.
	chan->last_samples_were_silence = true;
	chan->last_samples_were_stereo = false;
	chan->offset[0] = 0;
	chan->offset[1] = 0;
	mixer.channels = chan;
	return chan;
}

MixerChannel * MIXER_FindChannel(const char * name) {
	MixerChannel * chan=mixer.channels;
	while (chan) {
		if (!strcasecmp(chan->name,name)) break;
		chan=chan->next;
	}
	return chan;
}

void MIXER_DelChannel(MixerChannel* delchan) {
	MixerChannel * chan=mixer.channels;
	MixerChannel * * where=&mixer.channels;
	while (chan) {
		if (chan==delchan) {
			*where=chan->next;
			delete delchan;
			return;
		}
		where=&chan->next;
		chan=chan->next;
	}
}

void MixerChannel::UpdateVolume(void) {
	volmul[0]=(Bits)((1 << MIXER_VOLSHIFT)*scale*volmain[0]*mixer.mastervol[0]);
	volmul[1]=(Bits)((1 << MIXER_VOLSHIFT)*scale*volmain[1]*mixer.mastervol[1]);
}

void MixerChannel::SetVolume(float _left,float _right) {
	volmain[0]=_left;
	volmain[1]=_right;
	UpdateVolume();
}

void MixerChannel::SetScale( float f ) {
	scale = f;
	UpdateVolume();
}

void MixerChannel::Enable(bool _yesno) {
	if (_yesno==enabled) return;
	enabled=_yesno;
	if (enabled) {
		freq_counter = 0;
		if (done<mixer.done) done=mixer.done;
	}
}

void MixerChannel::SetFreq(Bitu freq) {
	freq_add=(freq<<FREQ_SHIFT)/mixer.freq;

	if (freq != mixer.freq) {
		interpolate = true;
	} else {
		interpolate = false;
	}
}

void MixerChannel::Mix(Bitu _needed) {
	needed=_needed;
	while (enabled && needed>done) {
		Bitu left = (needed - done);
		left *= freq_add;
		left  = (left >> FREQ_SHIFT) + ((left & FREQ_MASK)!=0);
		handler(left);
	}
}

void MixerChannel::AddSilence(void) {
	if (done < needed) {
		if(prevSample[0] == 0 && prevSample[1] == 0) {
			done = needed;
			//Make sure the next samples are zero when they get switched to prev
			nextSample[0] = 0;
			nextSample[1] = 0;
			//This should trigger an instant request for new samples
			freq_counter = FREQ_NEXT;
		} else {
			bool stereo = last_samples_were_stereo;
			//Position where to write the data
			Bitu mixpos = mixer.pos + done;
			while (done < needed) {
				// Maybe depend on sample rate. (the 4)
				if (prevSample[0] > 4)       nextSample[0] = prevSample[0] - 4;
				else if (prevSample[0] < -4) nextSample[0] = prevSample[0] + 4;
				else nextSample[0] = 0;
				if (prevSample[1] > 4)       nextSample[1] = prevSample[1] - 4;
				else if (prevSample[1] < -4) nextSample[1] = prevSample[1] + 4;
				else nextSample[1] = 0;

				mixpos &= MIXER_BUFMASK;
				Bit32s* write = mixer.work[mixpos];

				write[0] += prevSample[0] * volmul[0];
				write[1] += (stereo ? prevSample[1] : prevSample[0]) * volmul[1];

				prevSample[0] = nextSample[0];
				prevSample[1] = nextSample[1];
				mixpos++;
				done++;
				freq_counter = FREQ_NEXT;
			} 
		}
	}
	last_samples_were_silence = true;
	offset[0] = offset[1] = 0;
}

//4 seems to work . Disabled for now
#define MIXER_UPRAMP_STEPS 0
#define MIXER_UPRAMP_SAVE 512

template<class Type,bool stereo,bool signeddata,bool nativeorder>
inline void MixerChannel::AddSamples(Bitu len, const Type* data) {
	last_samples_were_stereo = stereo;
	
	//Position where to write the data
	Bitu mixpos = mixer.pos + done;
	//Position in the incoming data
	Bitu pos = 0;
	//Mix and data for the full length
	while (1) {
		//Does new data need to get read?
		while (freq_counter >= FREQ_NEXT) {
			//Would this overflow the source data, then it's time to leave
			if (pos >= len) {
				last_samples_were_silence = false;
#if MIXER_UPRAMP_STEPS > 0
				if (offset[0] || offset[1]) {
					//Should be safe to do, as the value inside offset is 16 bit while offset itself is at least 32 bit
					offset[0] = (offset[0]*(MIXER_UPRAMP_STEPS-1))/MIXER_UPRAMP_STEPS;
					offset[1] = (offset[1]*(MIXER_UPRAMP_STEPS-1))/MIXER_UPRAMP_STEPS;
					if (offset[0] < MIXER_UPRAMP_SAVE && offset[0] > -MIXER_UPRAMP_SAVE) offset[0] = 0;
					if (offset[1] < MIXER_UPRAMP_SAVE && offset[1] > -MIXER_UPRAMP_SAVE) offset[1] = 0;
				}
#endif
				return;
			}
			freq_counter -= FREQ_NEXT;
			prevSample[0] = nextSample[0];
			if (stereo) {
				prevSample[1] = nextSample[1];
			}
			if ( sizeof( Type) == 1) {
				if (!signeddata) {
					if (stereo) {
						nextSample[0]=(((Bit8s)(data[pos*2+0] ^ 0x80)) << 8);
						nextSample[1]=(((Bit8s)(data[pos*2+1] ^ 0x80)) << 8);
					} else {
						nextSample[0]=(((Bit8s)(data[pos] ^ 0x80)) << 8);
					}
				} else {
					if (stereo) {
						nextSample[0]=(data[pos*2+0] << 8);
						nextSample[1]=(data[pos*2+1] << 8);
					} else {
						nextSample[0]=(data[pos] << 8);
					}
				}
			//16bit and 32bit both contain 16bit data internally
			} else  {
				if (signeddata) {
					if (stereo) {
						if (nativeorder) {
							nextSample[0]=data[pos*2+0];
							nextSample[1]=data[pos*2+1];
						} else {
							if ( sizeof( Type) == 2) {
								nextSample[0]=(Bit16s)host_readw((HostPt)&data[pos*2+0]);
								nextSample[1]=(Bit16s)host_readw((HostPt)&data[pos*2+1]);
							} else {
								nextSample[0]=(Bit32s)host_readd((HostPt)&data[pos*2+0]);
								nextSample[1]=(Bit32s)host_readd((HostPt)&data[pos*2+1]);
							}
						}
					} else {
						if (nativeorder) {
							nextSample[0] = data[pos];
						} else {
							if ( sizeof( Type) == 2) {
								nextSample[0]=(Bit16s)host_readw((HostPt)&data[pos]);
							} else {
								nextSample[0]=(Bit32s)host_readd((HostPt)&data[pos]);
							}
						}
					}
				} else {
					if (stereo) {
						if (nativeorder) {
							nextSample[0]=(Bits)data[pos*2+0]-32768;
							nextSample[1]=(Bits)data[pos*2+1]-32768;
						} else {
							if ( sizeof( Type) == 2) {
								nextSample[0]=(Bits)host_readw((HostPt)&data[pos*2+0])-32768;
								nextSample[1]=(Bits)host_readw((HostPt)&data[pos*2+1])-32768;
							} else {
								nextSample[0]=(Bits)host_readd((HostPt)&data[pos*2+0])-32768;
								nextSample[1]=(Bits)host_readd((HostPt)&data[pos*2+1])-32768;
							}
						}
					} else {
						if (nativeorder) {
							nextSample[0]=(Bits)data[pos]-32768;
						} else {
							if ( sizeof( Type) == 2) {
								nextSample[0]=(Bits)host_readw((HostPt)&data[pos])-32768;
							} else {
								nextSample[0]=(Bits)host_readd((HostPt)&data[pos])-32768;
							}
						}
					}
				}
			}
			//This sample has been handled now, increase position
			pos++;
#if MIXER_UPRAMP_STEPS > 0
			if (last_samples_were_silence && pos == 1) {
				offset[0] = nextSample[0] - prevSample[0];
				if (stereo) offset[1] = nextSample[1] - prevSample[1];
				//Don't bother with small steps.
				if (offset[0] < (MIXER_UPRAMP_SAVE*4) && offset[0] > (-MIXER_UPRAMP_SAVE*4)) offset[0] = 0;
				if (offset[1] < (MIXER_UPRAMP_SAVE*4) && offset[1] > (-MIXER_UPRAMP_SAVE*4)) offset[1] = 0;
			}

			if (offset[0] || offset[1]) {
				nextSample[0] = nextSample[0] - (offset[0]*(MIXER_UPRAMP_STEPS*static_cast<Bits>(len)-static_cast<Bits>(pos))) /( MIXER_UPRAMP_STEPS*static_cast<Bits>(len) );
				nextSample[1] = nextSample[1] - (offset[1]*(MIXER_UPRAMP_STEPS*static_cast<Bits>(len)-static_cast<Bits>(pos))) /( MIXER_UPRAMP_STEPS*static_cast<Bits>(len) );
			}
#endif
		}
		//Where to write
		mixpos &= MIXER_BUFMASK;
		Bit32s* write = mixer.work[mixpos];
		if (!interpolate) {
			write[0] += prevSample[0] * volmul[0];
			write[1] += (stereo ? prevSample[1] : prevSample[0]) * volmul[1];
		}
		else {
			Bits diff_mul = freq_counter & FREQ_MASK;
			Bits sample = prevSample[0] + (((nextSample[0] - prevSample[0]) * diff_mul) >> FREQ_SHIFT);
			write[0] += sample*volmul[0];
			if (stereo) {
				sample = prevSample[1] + (((nextSample[1] - prevSample[1]) * diff_mul) >> FREQ_SHIFT);
			}
			write[1] += sample*volmul[1];
		}
		//Prepare for next sample
		freq_counter += freq_add;
		mixpos++;
		done++;
	}
}

void MixerChannel::AddStretched(Bitu len,Bit16s * data) {
	if (done >= needed) {
		LOG_MSG("Can't add, buffer full");
		return;
	}
	//Target samples this inputs gets stretched into
	Bitu outlen = needed - done;
	Bitu index = 0;
	Bitu index_add = (len << FREQ_SHIFT)/outlen;
	Bitu mixpos = mixer.pos + done;
	done = needed;
	Bitu pos = 0;

	while (outlen--) {
		Bitu new_pos = index >> FREQ_SHIFT;
		if (pos != new_pos) {
			pos = new_pos;
			//Forward the previous sample
			prevSample[0] = data[0];
			data++;
		}
		Bits diff = data[0] - prevSample[0];
		Bits diff_mul = index & FREQ_MASK;
		index += index_add;
		mixpos &= MIXER_BUFMASK;
		Bits sample = prevSample[0] + ((diff * diff_mul) >> FREQ_SHIFT);
		mixer.work[mixpos][0] += sample * volmul[0];
		mixer.work[mixpos][1] += sample * volmul[1];
		mixpos++;
	}
}

void MixerChannel::AddSamples_m8(Bitu len, const Bit8u * data) {
	AddSamples<Bit8u,false,false,true>(len,data);
}
void MixerChannel::AddSamples_s8(Bitu len,const Bit8u * data) {
	AddSamples<Bit8u,true,false,true>(len,data);
}
void MixerChannel::AddSamples_m8s(Bitu len,const Bit8s * data) {
	AddSamples<Bit8s,false,true,true>(len,data);
}
void MixerChannel::AddSamples_s8s(Bitu len,const Bit8s * data) {
	AddSamples<Bit8s,true,true,true>(len,data);
}
void MixerChannel::AddSamples_m16(Bitu len,const Bit16s * data) {
	AddSamples<Bit16s,false,true,true>(len,data);
}
void MixerChannel::AddSamples_s16(Bitu len,const Bit16s * data) {
	AddSamples<Bit16s,true,true,true>(len,data);
}
void MixerChannel::AddSamples_m16u(Bitu len,const Bit16u * data) {
	AddSamples<Bit16u,false,false,true>(len,data);
}
void MixerChannel::AddSamples_s16u(Bitu len,const Bit16u * data) {
	AddSamples<Bit16u,true,false,true>(len,data);
}
void MixerChannel::AddSamples_m32(Bitu len,const Bit32s * data) {
	AddSamples<Bit32s,false,true,true>(len,data);
}
void MixerChannel::AddSamples_s32(Bitu len,const Bit32s * data) {
	AddSamples<Bit32s,true,true,true>(len,data);
}
void MixerChannel::AddSamples_m16_nonnative(Bitu len,const Bit16s * data) {
	AddSamples<Bit16s,false,true,false>(len,data);
}
void MixerChannel::AddSamples_s16_nonnative(Bitu len,const Bit16s * data) {
	AddSamples<Bit16s,true,true,false>(len,data);
}
void MixerChannel::AddSamples_m16u_nonnative(Bitu len,const Bit16u * data) {
	AddSamples<Bit16u,false,false,false>(len,data);
}
void MixerChannel::AddSamples_s16u_nonnative(Bitu len,const Bit16u * data) {
	AddSamples<Bit16u,true,false,false>(len,data);
}
void MixerChannel::AddSamples_m32_nonnative(Bitu len,const Bit32s * data) {
	AddSamples<Bit32s,false,true,false>(len,data);
}
void MixerChannel::AddSamples_s32_nonnative(Bitu len,const Bit32s * data) {
	AddSamples<Bit32s,true,true,false>(len,data);
}

void MixerChannel::FillUp(void) {
	if (!enabled) return;

	if (done < mixer.done) {
		return;
	}
	float index = PIC_TickIndex();
	Mix((Bitu)(index * mixer.needed));
}

extern bool ticksLocked;
static inline bool Mixer_irq_important(void) {
	/* In some states correct timing of the irqs is more important then
	 * non stuttering audo */
	return (ticksLocked || (CaptureState & (CAPTURE_WAVE|CAPTURE_VIDEO)));
}

static Bit32u calc_tickadd(Bit32u freq) {
#if TICK_SHIFT > 16
	Bit64u freq64 = static_cast<Bit64u>(freq);
	freq64 = (freq64<<TICK_SHIFT)/1000;
	Bit32u r = static_cast<Bit32u>(freq64);
	return r;
#else
	return (freq<<TICK_SHIFT)/1000;
#endif
}

/* Mix a certain amount of new samples */
static void MIXER_MixData(Bitu needed) {
	MixerChannel * chan=mixer.channels;
	while (chan) {
		chan->Mix(needed);
		chan=chan->next;
	}
	if (CaptureState & (CAPTURE_WAVE|CAPTURE_VIDEO)) {
		Bit16s convert[1024][2];
		Bitu added=needed-mixer.done;
		if (added>1024)
			added=1024;
		Bitu readpos=(mixer.pos+mixer.done)&MIXER_BUFMASK;
		for (Bitu i=0;i<added;i++) {
			Bits sample=mixer.work[readpos][0] >> MIXER_VOLSHIFT;
			convert[i][0]=MIXER_CLIP(sample);
			sample=mixer.work[readpos][1] >> MIXER_VOLSHIFT;
			convert[i][1]=MIXER_CLIP(sample);
			readpos=(readpos+1)&MIXER_BUFMASK;
		}
		CAPTURE_AddWave( mixer.freq, added, (Bit16s*)convert );
	}
	//Reset the the tick_add for constant speed
	if( Mixer_irq_important() )
		mixer.tick_add = calc_tickadd(mixer.freq);
	mixer.done = needed;
}

static void MIXER_Mix(void) {
	MIXER_MixData(mixer.needed);
	mixer.tick_counter += mixer.tick_add;
	mixer.needed+=(mixer.tick_counter >> TICK_SHIFT);
	mixer.tick_counter &= TICK_MASK;


  static auto pushedAt = GetMsPassedFromStart();
  static double restSamplesCount = 0;
  auto now = GetMsPassedFromStart();
  auto dt = now - pushedAt;

  auto exactSamplesCount = dt * mixer.freq / 1000 + restSamplesCount;
  int samplesCount = exactSamplesCount;
  if (samplesCount >= PUSH_SIZE) {
    restSamplesCount = exactSamplesCount - samplesCount;
    if (samplesCount > BLOCK_SIZE) {
      samplesCount = BLOCK_SIZE;
    }
    MIXER_CallBack(blockBuffer, samplesCount);
    client_sound_push(blockBuffer, samplesCount);
    pushedAt = now;
  }
}

static void MIXER_Mix_NoSound(void) {
	MIXER_MixData(mixer.needed);
	/* Clear piece we've just generated */
	for (Bitu i=0;i<mixer.needed;i++) {
		mixer.work[mixer.pos][0]=0;
		mixer.work[mixer.pos][1]=0;
		mixer.pos=(mixer.pos+1)&MIXER_BUFMASK;
	}
	/* Reduce count in channels */
	for (MixerChannel * chan=mixer.channels;chan;chan=chan->next) {
		if (chan->done>mixer.needed) chan->done-=mixer.needed;
		else chan->done=0;
	}
	/* Set values for next tick */
	mixer.tick_counter += mixer.tick_add;
	mixer.needed = (mixer.tick_counter >> TICK_SHIFT);
	mixer.tick_counter &= TICK_MASK;
	mixer.done=0;
}

static void MIXER_CallBack(float *output, int frames) {
        int need = frames;
	Bitu reduce;
	Bitu pos;
	//Local resampling counter to manipulate the data when sending it off to the callback
	Bitu index, index_add;
	/* Enough room in the buffer ? */
	if (mixer.done < need) {
//		LOG_MSG("Full underrun need %d, have %d, min %d", need, mixer.done, mixer.min_needed);
		if((need - mixer.done) > (need >>7) ) //Max 1 procent stretch.
			return;
		reduce = mixer.done;
		index_add = (reduce << TICK_SHIFT) / need;
		mixer.tick_add = calc_tickadd(mixer.freq+mixer.min_needed);
	} else if (mixer.done < mixer.max_needed) {
		Bitu left = mixer.done - need;
		if (left < mixer.min_needed) {
			if( !Mixer_irq_important() ) {
				Bitu needed = mixer.needed - need;
				Bitu diff = (mixer.min_needed>needed?mixer.min_needed:needed) - left;
				mixer.tick_add = calc_tickadd(mixer.freq+(diff*3));
				left = 0; //No stretching as we compensate with the tick_add value
			} else {
				left = (mixer.min_needed - left);
				left = 1 + (2*left) / mixer.min_needed; //left=1,2,3
			}
//			LOG_MSG("needed underrun need %d, have %d, min %d, left %d", need, mixer.done, mixer.min_needed, left);
			reduce = need - left;
			index_add = (reduce << TICK_SHIFT) / need;
		} else {
			reduce = need;
			index_add = (1 << TICK_SHIFT);
//			LOG_MSG("regular run need %d, have %d, min %d, left %d", need, mixer.done, mixer.min_needed, left);

			/* Mixer tick value being updated:
			 * 3 cases:
			 * 1) A lot too high. >division by 5. but maxed by 2* min to prevent too fast drops.
			 * 2) A little too high > division by 8
			 * 3) A little to nothing above the min_needed buffer > go to default value
			 */
			Bitu diff = left - mixer.min_needed;
			if(diff > (mixer.min_needed<<1)) diff = mixer.min_needed<<1;
			if(diff > (mixer.min_needed>>1))
				mixer.tick_add = calc_tickadd(mixer.freq-(diff/5));
			else if (diff > (mixer.min_needed>>2))
				mixer.tick_add = calc_tickadd(mixer.freq-(diff>>3));
			else
				mixer.tick_add = calc_tickadd(mixer.freq);
		}
	} else {
		/* There is way too much data in the buffer */
//		LOG_MSG("overflow run need %d, have %d, min %d", need, mixer.done, mixer.min_needed);
		if (mixer.done > MIXER_BUFSIZE)
			index_add = MIXER_BUFSIZE - 2*mixer.min_needed;
		else
			index_add = mixer.done - 2*mixer.min_needed;
		index_add = (index_add << TICK_SHIFT) / need;
		reduce = mixer.done - 2* mixer.min_needed;
		mixer.tick_add = calc_tickadd(mixer.freq-(mixer.min_needed/5));
	}
	/* Reduce done count in all channels */
	for (MixerChannel * chan=mixer.channels;chan;chan=chan->next) {
		if (chan->done>reduce) chan->done-=reduce;
		else chan->done=0;
	}

	// Reset mixer.tick_add when irqs are important
	if( Mixer_irq_important() )
		mixer.tick_add = calc_tickadd(mixer.freq);

	mixer.done -= reduce;
	mixer.needed -= reduce;
	pos = mixer.pos;
	mixer.pos = (mixer.pos + reduce) & MIXER_BUFMASK;
	index = 0;
	if(need != reduce) {
		while (need--) {
			Bitu i = (pos + (index >> TICK_SHIFT )) & MIXER_BUFMASK;
			index += index_add;
			float left = MIXER_CLIP(mixer.work[i][0]>>MIXER_VOLSHIFT);
                        float right = MIXER_CLIP(mixer.work[i][1]>>MIXER_VOLSHIFT);
			*output++= (left + right) / 2;
		}
		/* Clean the used buffer */
		while (reduce--) {
			pos &= MIXER_BUFMASK;
			mixer.work[pos][0]=0;
			mixer.work[pos][1]=0;
			pos++;
		}
	} else {
		while (reduce--) {
			pos &= MIXER_BUFMASK;
                        float left = MIXER_CLIP(mixer.work[pos][0]>>MIXER_VOLSHIFT);
                        float right = MIXER_CLIP(mixer.work[pos][1]>>MIXER_VOLSHIFT);
                        *output++= (left + right) / 2;
			mixer.work[pos][0]=0;
			mixer.work[pos][1]=0;
			pos++;
		}
	}
}

static void MIXER_Stop(Section* sec) {
}

class MIXER : public Program {
public:
	void MakeVolume(char * scan,float & vol0,float & vol1) {
		Bitu w=0;
		bool db=(toupper(*scan)=='D');
		if (db) scan++;
		while (*scan) {
			if (*scan==':') {
				++scan;w=1;
			}
			char * before=scan;
			float val=(float)strtod(scan,&scan);
			if (before==scan) {
				++scan;continue;
			}
			if (!db) val/=100;
			else val=powf(10.0f,(float)val/20.0f);
			if (val<0) val=1.0f;
			if (!w) {
				vol0=val;
			} else {
				vol1=val;
			}
		}
		if (!w) vol1=vol0;
	}

	void Run(void) {
		if(cmd->FindExist("/LISTMIDI")) {
			ListMidi();
			return;
		}
		if (cmd->FindString("MASTER",temp_line,false)) {
			MakeVolume((char *)temp_line.c_str(),mixer.mastervol[0],mixer.mastervol[1]);
		}
		MixerChannel * chan = mixer.channels;
		while (chan) {
			if (cmd->FindString(chan->name,temp_line,false)) {
				MakeVolume((char *)temp_line.c_str(),chan->volmain[0],chan->volmain[1]);
			}
			chan->UpdateVolume();
			chan = chan->next;
		}
		if (cmd->FindExist("/NOSHOW")) return;
		WriteOut("Channel  Main    Main(dB)\n");
		ShowVolume("MASTER",mixer.mastervol[0],mixer.mastervol[1]);
		for (chan = mixer.channels;chan;chan = chan->next)
			ShowVolume(chan->name,chan->volmain[0],chan->volmain[1]);
	}
private:
	void ShowVolume(const char * name,float vol0,float vol1) {
		WriteOut("%-8s %3.0f:%-3.0f  %+3.2f:%-+3.2f \n",name,
			vol0*100,vol1*100,
			20*log(vol0)/log(10.0f),20*log(vol1)/log(10.0f)
		);
	}

	void ListMidi(){
		if(midi.handler) midi.handler->ListAll(this);
	};

};

static void MIXER_ProgramStart(Program * * make) {
	*make=new MIXER;
}

MixerChannel* MixerObject::Install(MIXER_Handler handler,Bitu freq,const char * name){
	if(!installed) {
		if(strlen(name) > 31) E_Exit("Too long mixer channel name");
		safe_strncpy(m_name,name,32);
		installed = true;
		return MIXER_AddChannel(handler,freq,name);
	} else {
		E_Exit("already added mixer channel.");
		return 0; //Compiler happy
	}
}

MixerObject::~MixerObject(){
	if(!installed) return;
	MIXER_DelChannel(MIXER_FindChannel(m_name));
}


void MIXER_Init(Section* sec) {
	sec->AddDestroyFunction(&MIXER_Stop);

	Section_prop * section=static_cast<Section_prop *>(sec);
	/* Read out config section */
	mixer.freq=section->Get_int("rate");
	mixer.nosound=section->Get_bool("nosound");
	mixer.blocksize= ::BLOCK_SIZE; // section->Get_int("blocksize");

        client_sound_init(mixer.freq);

	mixer.channels=0;
	mixer.pos=0;
	mixer.done=0;
	memset(mixer.work,0,sizeof(mixer.work));
	mixer.mastervol[0]=1.0f;
	mixer.mastervol[1]=1.0f;

	mixer.tick_counter=0;
	if (mixer.nosound) {
		LOG_MSG("MIXER: No Sound Mode Selected.");
		mixer.tick_add=calc_tickadd(mixer.freq);
		TIMER_AddTickHandler(MIXER_Mix_NoSound);
	} else {
		mixer.tick_add=calc_tickadd(mixer.freq);
		TIMER_AddTickHandler(MIXER_Mix);
	}
	mixer.min_needed=section->Get_int("prebuffer");
	if (mixer.min_needed>100) mixer.min_needed=100;
	mixer.min_needed=(mixer.freq*mixer.min_needed)/1000;
	mixer.max_needed=mixer.blocksize * 2 + 2*mixer.min_needed;
	mixer.needed=mixer.min_needed+1;
	PROGRAMS_MakeFile("MIXER.COM",MIXER_ProgramStart);
}
