// $Id$

#ifndef __MSXMEMORYMAPPER_HH__
#define __MSXMEMORYMAPPER_HH__

#include "MSXMemDevice.hh"
#include "MSXIODevice.hh"

// forward declaration;
class MSXMapperIO;
class EmuTime;


class MSXMemoryMapper : public MSXMemDevice, public MSXIODevice
{
	public:
		/**
		 * Constructor
		 */
		MSXMemoryMapper(MSXConfig::Device *config, const EmuTime &time);

		/**
		 * Destructor
		 */
		~MSXMemoryMapper();
		
		byte readMem(word address, const EmuTime &time);
		void writeMem(word address, byte value, const EmuTime &time);
		byte readIO(byte port, const EmuTime &time);
		void writeIO(byte port, byte value, const EmuTime &time);
		byte* getReadCacheLine(word start);
		byte* getWriteCacheLine(word start);
		
		void reset(const EmuTime &time);
		void initIO();
		void resetIO();
	
	private:
		inline word getAdr(word address);

		byte *buffer;
		int blocks;
		bool slowDrainOnReset;

		static MSXMapperIO *device;
		static byte pageNum[4];
};

#endif //__MSXMEMORYMAPPER_HH__
