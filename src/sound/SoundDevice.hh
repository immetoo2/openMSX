#ifndef SOUNDDEVICE_HH
#define SOUNDDEVICE_HH

#include "EmuTime.hh"
#include "WavWriter.hh"
#include "static_string_view.hh"
#include <optional>
#include <string>
#include <string_view>

namespace openmsx {

class DeviceConfig;
class DynamicClock;
class Filename;
class MSXMixer;

class SoundDevice
{
public:
	static constexpr unsigned MAX_CHANNELS = 24;

	/** Get the unique name that identifies this sound device.
	  * Used to create setting names.
	  */
	[[nodiscard]] const std::string& getName() const { return name; }

	/** Gets a description of this sound device,
	  * to be presented to the user.
	  */
	[[nodiscard]] std::string_view getDescription() const { return description; }

	/** Is this a stereo device?
	  * This is set in the constructor and cannot be changed anymore
	  */
	[[nodiscard]] bool isStereo() const;

	/** Gets this device its 'amplification factor'.
	  *
	  * Each sample generated by the 'updateBuffer' method will get
	  * multiplied by this factor during mixing (see MSXMixer).
	  *
	  * This factor is the combination of:
	  *  - a fixed per device factor (see getAmplificationFactorImpl()).
	  *  - a MSX software controlled volume setting (e.g. used in
	  *    Carnivore2 emulation).
	  * The influence of the different volume settings is not part of this
	  * factor.
	  */
	struct AmplificationFactors {
		float left;
		float right;
	};
	[[nodiscard]] AmplificationFactors getAmplificationFactor() const {
		auto f = getAmplificationFactorImpl();
		return {f * softwareVolumeLeft, f * softwareVolumeRight};
	}

	/** Change the 'software volume' of this sound device.
	  *
	  * Some devices like 'Carnivore2' include sound chips (e.g. SCC and
	  * fm-pac) and allow the MSX software to configure the volume of
	  * those chips. To be clear: these are not the regular per channel
	  * volume registers of the SCC/YM2413 sound chips, but another
	  * register that controls the total output of those chips.
	  *
	  * This method allows to change that per-chip volume.
	  */
	void setSoftwareVolume(float volume, EmuTime::param time);
	void setSoftwareVolume(float left, float right, EmuTime::param time);

	void recordChannel(unsigned channel, const Filename& filename);
	void muteChannel  (unsigned channel, bool muted);

protected:
	/** Constructor.
	  * @param mixer The Mixer object
	  * @param name Name for this device, will be made unique
	  * @param description Description for this sound device
	  * @param numChannels The number of channels for this device
	  * @param inputRate The sample rate of this sound device
	  * @param stereo Is this a stereo device
	  */
	SoundDevice(MSXMixer& mixer, std::string_view name, static_string_view description,
	            unsigned numChannels, unsigned inputRate, bool stereo);
	~SoundDevice();

	/** Get amplification/attenuation factor for this device.
	  * Normally the outputBuffer() method should scale the output to
	  * the range [-1.0..+1.0]. But sometimes it's more convenient to
	  * generate another output range. In later stages the output
	  * is anyway still multiplied by some factor. This method tells which
	  * factor should be used to scale the output to the correct range.
	  * The default implementation returns '1.0 / 32768.0'.
	  */
	[[nodiscard]] virtual float getAmplificationFactorImpl() const;

	/**
	 * Registers this sound device with the Mixer.
	 * Call this method when the sound device is ready to start receiving
	 * calls to updateBuffer, so after all initialisation is done.
	 * @param config Configuration data for this sound device.
	 */
	void registerSound(const DeviceConfig& config);

	/**
	 * Unregisters this sound device with the Mixer.
	 * Call this method before any deallocation is done.
	 */
	void unregisterSound();

	/** @see Mixer::updateStream */
	void updateStream(EmuTime::param time);

	void setInputRate(unsigned sampleRate) { inputSampleRate = sampleRate; }
	[[nodiscard]] unsigned getInputRate() const { return inputSampleRate; }

public: // Will be called by Mixer:
	/**
	 * When a SoundDevice registers itself with the Mixer, the Mixer sets
	 * the required sampleRate through this method. All sound devices share
	 * a common sampleRate.
	 */
	virtual void setOutputRate(unsigned sampleRate) = 0;

	/** Generate sample data
	  * @param length The number of required samples
	  * @param buffer This buffer should be filled
	  * @param time current time
	  * @result false iff the output is empty. IOW filling the buffer with
	  *         zeros or returning false has the same effect, but the latter
	  *         can be more efficient
	  *
	  * This method is regularly called from the Mixer, it should return a
	  * pointer to a buffer filled with the required number of samples.
	  * Samples are always floats, later they are converted to the systems
	  * native format (e.g. 16-bit signed).
	  *
	  * Note: To enable various optimizations (like SSE), this method can
	  * fill the output buffer with up to 3 extra samples. Those extra
	  * samples should be ignored, though the caller must make sure the
	  * buffer has enough space to hold them.
	  */
	[[nodiscard]] virtual bool updateBuffer(unsigned length, float* buffer,
	                                        EmuTime::param time) = 0;

protected:
	/** Adds a number of samples that all have the same value.
	  * Can be used to synthesize segments of a square wave.
	  * @param buffer Pointer to the position in a sample buffer where the
	  *               samples should be added. This pointer is updated to
	  *               the position right after the written samples.
	  * @param value Sample value (amplitude).
	  * @param num The number of samples.
	  */
	static void addFill(float*& buffer, float value, unsigned num);

	/** Abstract method to generate the actual sound data.
	  * @param buffers An array of pointer to buffers. Each buffer must
	  *                be big enough to hold 'num' samples.
	  * @param num The number of samples.
	  *
	  * This method should fill each buffer with sound data that
	  * corresponds to one channel of the sound device. The same channel
	  * should each time be written to the same buffer (needed for record).
	  *
	  * If a certain channel is muted it is allowed to set the buffer
	  * pointer to nullptr. This has exactly the same effect as filling the
	  * buffer completely with zeros, but it can be more efficient.
	  */
	virtual void generateChannels(float** buffers, unsigned num) = 0;

	/** Calls generateChannels() and combines the output to a single
	  * channel.
	  * @param dataOut Output buffer, must be big enough to hold
	  *                'samples' number of samples
	  * @param samples The number of samples
	  * @result true iff at least one channel was unmuted
	  *
	  * Note: To enable various optimizations (like SSE), this method can
	  * fill the output buffer with up to 3 extra samples. Those extra
	  * samples should be ignored, though the caller must make sure the
	  * buffer has enough space to hold them.
	  */
	[[nodiscard]] bool mixChannels(float* dataOut, unsigned samples);

	/** See MSXMixer::getHostSampleClock(). */
	[[nodiscard]] const DynamicClock& getHostSampleClock() const;
	[[nodiscard]] double getEffectiveSpeed() const;

private:
	MSXMixer& mixer;
	const std::string name;
	const static_string_view description;

	std::optional<Wav16Writer> writer[MAX_CHANNELS];

	float softwareVolumeLeft = 1.0f;
	float softwareVolumeRight = 1.0f;
	unsigned inputSampleRate;
	const unsigned numChannels;
	const unsigned stereo;
	unsigned numRecordChannels;
	int channelBalance[MAX_CHANNELS];
	bool channelMuted[MAX_CHANNELS];
	bool balanceCenter;
};

} // namespace openmsx

#endif
