// $Id$

#include "IDEHD.hh"


byte IDEHD::identifyBlock[512] = {
	0x5a,0x0c,0xba,0x09,0x00,0x00,0x10,0x00,0x00,0x00,0x00,0x00,0x3f,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
	0x20,0x20,0x20,0x20,0x20,0x20,0x31,0x20,0x00,0x00,0x00,0x01,0x04,0x00,0x31,0x56,
	0x30,0x2e,0x20,0x20,0x20,0x20,0x50,0x4f,0x4e,0x45,0x4d,0x20,0x58,0x53,0x48,0x20,
	0x52,0x41,0x20,0x44,0x49,0x44,0x4b,0x53,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
	0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x10,0x80,
	0x00,0x00,0x00,0x0b,0x00,0x00,0x00,0x02,0x00,0x02,0x03,0x00,0xba,0x09,0x10,0x00,
	0x3f,0x00,0x60,0x4c,0x26,0x00,0x00,0x00,0xe0,0x53,0x26,0x00,0x07,0x00,0x07,0x04,
	0x03,0x00,0x78,0x00,0x78,0x00,0xf0,0x00,0x78,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};


IDEHD::IDEHD()
{
	buffer = new byte[512 * 256];

	file = FileOpener::openFilePreferRW("hd.dsk");	// TODO 

	file->seekg(0, std::ios::end);
	totalSectors = file->tellg() / 512;
}

IDEHD::~IDEHD()
{
	delete file;
	delete[] buffer;
}


void IDEHD::reset(const EmuTime &time)
{
	errorReg = 0x01;
	sectorCountReg = 0x01;
	sectorNumReg = 0x01;
	cylinderLowReg = 0x00;
	cylinderHighReg = 0x00;
	reg6 = 0x00;
	statusReg = 0x50;	// DRDY DSC
	featureReg = 0x00;
	transferRead = false;
	transferWrite = false;
}

byte IDEHD::readReg(nibble reg, const EmuTime &time)
{
	switch (reg) {
	case 1:	// error register
		return errorReg;
	
	case 2:	// sector count register
		return sectorCountReg;
		
	case 3:	// sector number register
		return sectorNumReg;
	
	case 4:	// cyclinder low register
		return cylinderLowReg;
		
	case 5:	// cyclinder high register
		return cylinderHighReg;
		
	case 6:	// TODO name
		// DEV bit is handled by IDE interface
		return reg6;
		
	case 7:	// status register
		return statusReg;

	case 8:
	case 9:
	case 10:
	case 11:
	case 12:
	case 13:
	case 15:// not used
		return 0x7F;
		
	case 0:	// data register, converted to readData by IDE interface
	case 14:// alternate status reg, converted to read from normal
		// status register by IDE interface
	default:
		assert(false);
		return 0x7F;	// avoid warning
	}
}

void IDEHD::writeReg(nibble reg, byte value, const EmuTime &time)
{
	switch (reg) {
	case 1:	// feature register
		featureReg = value;
		break;
	
	case 2:	// sector count register
		sectorCountReg = value;
		break;
		
	case 3:	// sector number register
		sectorNumReg = value;
		break;
	
	case 4:	// cyclinder low register
		cylinderLowReg = value;
		break;
		
	case 5:	// cyclinder high register
		cylinderHighReg = value;
		break;
		
	case 6:	// TODO name
		// DEV bit is handled by IDE interface
		reg6 = value;
		break;
		
	case 7:	// command register
		executeCommand(value);
		break;

	case 8:
	case 9:
	case 10:
	case 11:
	case 12:
	case 13:
	case 15:// not used
	case 14:// device control register, handled by IDE interface
		// do nothing
		break;
		
	case 0:	// data register, converted to readData by IDE interface
	default:
		assert(false);
		break;
	}
}


word IDEHD::readData(const EmuTime &time)
{
	if (!transferRead) {
		// no read in progress
		return 0x7F7F;
	}
	if (--transferCount == 0) {
		// everything read
		transferRead = false;
		statusReg &= ~0x08;	// DRQ
	}
	return *(transferPntr++);
}

void IDEHD::writeData(word value, const EmuTime &time)
{
	if (!transferWrite) {
		// no write in progress
		return;
	}
	*(transferPntr++) = value;
	if (--transferCount == 0) {
		// everything written
		transferWrite = false;
		statusReg &= ~0x08;	// DRQ
		int numSectors = getNumSectors();
		file->write(buffer, numSectors * 512);
		if (file->fail()) {
			setError(0x44);
		}
	}
}


void IDEHD::setError(byte error)
{
	errorReg = error;
	statusReg |= 0x01;	// ERR
}

int IDEHD::getSectorNumber()
{
	return sectorNumReg | (cylinderLowReg << 8) |
		(cylinderHighReg << 16) | ((reg6 & 0x0F) << 24);
}

int IDEHD::getNumSectors()
{
	return (sectorNumReg == 0) ? 256 : sectorNumReg;
}

void IDEHD::executeCommand(byte cmd)
{
	statusReg &= ~0x09;	// reset DRQ ERR
	transferRead = transferWrite = false;
	switch (cmd) {
	case 0xEF: // Set Feature
		if (featureReg != 0x03) {
			// not "Set Transfer Mode"
			setError(0x04);
		}
		break;

	case 0xEC: // ATA Identify Device
		transferCount = 256;
		transferPntr = (word*)(&identifyBlock);
		transferRead = true;
		statusReg |= 0x08;	// DRQ
		break;

	case 0x91: // Initialize Device Parameters
		// ignore command
		break;

	case 0x30: { // Write Sector
		int sectorNumber = getSectorNumber();
		int numSectors = getNumSectors();
		if ((sectorNumber + numSectors) > totalSectors) {
			setError(0x14);
			break;
		}
		file->seekg(512 * sectorNumber, std::ios::beg);
		transferCount = 512/2 * numSectors;
		transferPntr = (word*)buffer;
		transferWrite = true;
		statusReg |= 0x08;	// DRQ
		break;
	}
	case 0x20: { // Read Sector
		int sectorNumber = getSectorNumber();
		int numSectors = getNumSectors();
		if ((sectorNumber + numSectors) > totalSectors) {
			setError(0x14);
			break;
		}
		file->seekg(512 * sectorNumber, std::ios::beg);
		file->read(buffer, 512 * numSectors);
		if (file->fail()) {
			setError(0x44);
			break;
		}
		transferCount = 512/2 * numSectors;
		transferPntr = (word*)buffer;
		transferRead = true;
		statusReg |= 0x08;	// DRQ
		break;
	}
	default: // all others
		setError(0x04);
	}
}
