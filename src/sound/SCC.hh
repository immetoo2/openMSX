// $Id$

#ifndef SCC_HH
#define SCC_HH

#include "openmsx.hh"
#include "SoundDevice.hh"
#include "Clock.hh"
#include "Debuggable.hh"

namespace openmsx {

class MSXMotherBoard;
class Debugger;

class SCC : private SoundDevice
{
public:
	enum ChipMode {SCC_Real, SCC_Compatible, SCC_plusmode};

	SCC(MSXMotherBoard& motherBoard, const std::string& name,
	    const XMLElement& config, const EmuTime& time,
	    ChipMode mode = SCC_Real);
	virtual ~SCC();

	// interaction with realCartridge
	void reset(const EmuTime& time);
	byte readMemInterface(byte address,const EmuTime& time);
	void writeMemInterface(byte address, byte value, const EmuTime& time);
	void setChipMode(ChipMode newMode);

private:
	// SoundDevice
	virtual const std::string& getName() const;
	virtual const std::string& getDescription() const;
	virtual void setSampleRate(int sampleRate);
	virtual void setVolume(int maxVolume);
	virtual void updateBuffer(unsigned length, int* buffer,
		const EmuTime& time, const EmuDuration& sampDur);

	class SCCDebuggable : public Debuggable {
	public:
		SCCDebuggable(SCC& parent);
		virtual unsigned getSize() const;
		virtual const std::string& getDescription() const;
		virtual byte read(unsigned address);
		virtual void write(unsigned address, byte value);
	private:
		SCC& parent;
	} sccDebuggable;

	inline void checkMute();
	byte readWave(byte channel, byte address, const EmuTime& time);
	void writeWave(byte channel, byte offset, byte value);
	void setDeformReg(byte value, const EmuTime& time);
	void setFreqVol(byte address, byte value);
	byte getFreqVol(byte address);

	int filter(int input);

	static const int GETA_BITS = 22;
	static const int CLOCK_FREQ = 3579545;
	static const unsigned int SCC_STEP =
		(unsigned)(((unsigned)(1 << 31)) / (CLOCK_FREQ / 2));

	ChipMode currentChipMode;
	int masterVolume;
	unsigned baseStep;
	unsigned nbSamples;

	signed char wave[5][32];
	signed char volAdjustedWave[5][32];
	unsigned incr[5];
	unsigned count[5];
	unsigned freq[5];
	byte volume[5];
	byte ch_enable;

	byte deformValue;
	Clock<CLOCK_FREQ> deformTimer;
	bool rotate[5];
	bool readOnly[5];
	byte offset[5];

	Debugger& debugger;
	const std::string name;

	int in[5], inHp[3], outHp[3];
};

} // namespace openmsx

#endif
