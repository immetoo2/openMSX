// $Id$

#include <map>
#include <string>
#include "RomInfo.hh"
#include "Rom.hh"
#include "sha1.hh"
#include "md5.hh"
#include "libxmlx/xmlx.hh"
#include "FileContext.hh"
#include "File.hh"
#include "Device.hh"
#include "CliCommOutput.hh"

namespace openmsx {

struct caseltstr {
	bool operator()(const string s1, const string s2) const {
		return strcasecmp(s1.c_str(), s2.c_str()) < 0;
	}
};

RomInfo::RomInfo(const string& ntitle, const string& nyear,
                 const string& ncompany, const string& nremark,
                 const MapperType& nmapperType)
{
	title = ntitle;
	year = nyear;
	company = ncompany;
	remark = nremark;
	mapperType = nmapperType;
}

// TODO: Turn MapperType into a class and move naming there.
MapperType RomInfo::nameToMapperType(const string& name)
{
	static map<string, MapperType, caseltstr> mappertype;
	static bool init = false;

	if (!init) {
		init = true;
		
		mappertype["0"]           = GENERIC_8KB;
		mappertype["8kB"]         = GENERIC_8KB;

		mappertype["1"]           = GENERIC_16KB;
		mappertype["16kB"]        = GENERIC_16KB;
		mappertype["MSXDOS2"]     = GENERIC_16KB; /* For now...*/

		mappertype["2"]           = KONAMI5;
		mappertype["KONAMI5"]     = KONAMI5;
		mappertype["SCC"]         = KONAMI5;

		mappertype["3"]           = KONAMI4;
		mappertype["KONAMI4"]     = KONAMI4;

		mappertype["4"]           = ASCII_8KB;
		mappertype["ASCII8"]      = ASCII_8KB;
 
		mappertype["5"]           = ASCII_16KB;
		mappertype["ASCII16"]     = ASCII_16KB;

		mappertype["RTYPE"]       = R_TYPE;

		mappertype["CROSSBLAIM"]  = CROSS_BLAIM;
		
		mappertype["PANASONIC"]   = PANASONIC;
		
		mappertype["NATIONAL"]    = NATIONAL;
	
		mappertype["MSX-AUDIO"]   = MSX_AUDIO;
		
		mappertype["HARRYFOX"]    = HARRY_FOX;
		
		mappertype["HALNOTE"]     = HALNOTE;
		
		mappertype["KOREAN80IN1"] = KOREAN80IN1;
		
		mappertype["KOREAN90IN1"] = KOREAN90IN1;
		
		mappertype["KOREAN126IN1"]= KOREAN126IN1;
		
		mappertype["HOLYQURAN"]   = HOLY_QURAN;
	
		mappertype["FSA1FM1"]     = FSA1FM1;
		mappertype["FSA1FM2"]     = FSA1FM2;
		
		mappertype["64kB"]        = PLAIN;
		mappertype["PLAIN"]       = PLAIN;
		
		// SRAM
		mappertype["HYDLIDE2"]    = HYDLIDE2;
		mappertype["ASCII16-2"]   = HYDLIDE2;

		// TODO: proper support for ASCII 8kB with a certain
		// amount of SRAM (8 - 32kB)
		mappertype["ASCII8-8"]    = ASCII8_8; 

		mappertype["GAMEMASTER2"] = GAME_MASTER2;
		mappertype["RC755"]       = GAME_MASTER2;

		// DAC
		mappertype["MAJUTSUSHI"]  = MAJUTSUSHI;
		
		mappertype["SYNTHESIZER"] = SYNTHESIZER;
		
		mappertype["PAGE0"]       = PAGE0;
		mappertype["PAGE01"]      = PAGE01;
		mappertype["PAGE012"]     = PAGE012;
		mappertype["PAGE0123"]    = PAGE0123;
		mappertype["PAGE1"]       = PAGE1;
		mappertype["PAGE12"]      = PAGE12;
		mappertype["PAGE123"]     = PAGE123;
		mappertype["PAGE2"]       = PAGE2;
		mappertype["ROMBAS"]      = PAGE2;
		mappertype["PAGE23"]      = PAGE23;
		mappertype["PAGE3"]       = PAGE3;
	}

	map<string, MapperType, caseltstr>::const_iterator
		it = mappertype.find(name);
	if (it == mappertype.end()) {
		return UNKNOWN;
	}
	return it->second;
}

MapperType RomInfo::guessMapperType(const Rom* rom)
{
	int size = rom->getSize();
	if (size == 0) {
		return PLAIN;
	}
	const byte* data = rom->getBlock();
	
	if (size <= 0x10000) {
		if (size == 0x10000) {
			// There are some programs convert from tape to
			// 64kB rom cartridge these 'fake'roms are from
			// the ASCII16 type
			return ASCII_16KB;
		} else if ((size <= 0x4000) &&
		           (data[0] == 'A') && (data[1] == 'B')) {
			word initAddr = data[2] + 256 * data[3];
			word textAddr = data[8] + 256 * data[9];
			if ((initAddr == 0) && ((textAddr & 0xC000) == 0x8000)) {
				return PAGE2;
			}
		}
		// not correct for Konami-DAC, but does this really need
		// to be correct for _every_ rom?
		return PLAIN;
	} else {
		//  GameCartridges do their bankswitching by using the Z80
		//  instruction ld(nn),a in the middle of program code. The
		//  adress nn depends upon the GameCartridge mappertype used.
		//  To guess which mapper it is, we will look how much writes
		//  with this instruction to the mapper-registers-addresses
		//  occur.

		unsigned int typeGuess[] = {0,0,0,0,0,0};
		for (int i=0; i<size-3; i++) {
			if (data[i] == 0x32) {
				word value = data[i+1]+(data[i+2]<<8);
				switch (value) {
					case 0x5000:
					case 0x9000:
					case 0xb000:
						typeGuess[KONAMI5]++;
						break;
					case 0x4000:
						typeGuess[KONAMI4]++;
						break;
					case 0x8000:
					case 0xa000:
						typeGuess[KONAMI4]++;
						break;
					case 0x6800:
					case 0x7800:
						typeGuess[ASCII_8KB]++;
						break;
					case 0x6000:
						typeGuess[KONAMI4]++;
						typeGuess[ASCII_8KB]++;
						typeGuess[ASCII_16KB]++;
						break;
					case 0x7000:
						typeGuess[KONAMI5]++;
						typeGuess[ASCII_8KB]++;
						typeGuess[ASCII_16KB]++;
						break;
					case 0x77ff:
						typeGuess[ASCII_16KB]++;
						break;
				}
			}
		}
		if (typeGuess[ASCII_8KB]) typeGuess[ASCII_8KB]--; // -1 -> max_int
		MapperType type = GENERIC_8KB; // 0
		for (int i=GENERIC_8KB; i <= ASCII_16KB; i++) {
			if ((typeGuess[i]) && (typeGuess[i]>=typeGuess[type])) {
				type = (MapperType)i;
			}
		}
		// in case of doubt we go for type 0
		// in case of even type 5 and 4 we would prefer 5
		// but we would still prefer 0 above 4 or 5
		if ((type == ASCII_16KB) &&
		    (typeGuess[GENERIC_8KB] == typeGuess[ASCII_16KB])) {
			type = GENERIC_8KB;
		}
		return type;
	}
}

RomInfo *RomInfo::searchRomDB(const Rom* rom)
{
	// TODO: Turn ROM DB into a separate class.
	static map<string, RomInfo*> romDBSHA1;
	static map<string, RomInfo*> romDBMD5;
	static bool init = false;

	if (!init) {
		init = true;
		try {
			SystemFileContext context;
			File file(context.resolve("share/romdb.xml"));
			XML::Document doc(file.getLocalName().c_str());
			list<XML::Element*>::iterator it1 = doc.root->children.begin();
			for ( ; it1 != doc.root->children.end(); it1++) {
				// TODO there can be multiple title tags
				string title   = (*it1)->getElementPcdata("title");
				string year    = (*it1)->getElementPcdata("year");
				string company = (*it1)->getElementPcdata("company");
				string remark  = (*it1)->getElementPcdata("remark");
				string romtype = (*it1)->getElementPcdata("romtype");
				
				RomInfo* romInfo = new RomInfo(title, year,
				   company, remark, nameToMapperType(romtype));
				for (list<XML::Element*>::iterator it2 = (*it1)->children.begin();
				     it2 != (*it1)->children.end(); ++it2) {
					if ((*it2)->name == "md5") {
						string md5 = (*it2)->pcdata;
						if (romDBMD5.find(md5) ==
						    romDBMD5.end()) {
							romDBMD5[md5] = romInfo;
						} else {
							CliCommOutput::instance().printWarning(
								"duplicate romdb entry MD5: " + md5);
						}
					} else if ((*it2)->name == "sha1") {
						string sha1 = (*it2)->pcdata;
						if (romDBSHA1.find(sha1) ==
						    romDBSHA1.end()) {
							romDBSHA1[sha1] = romInfo;
						} else {
							CliCommOutput::instance().printWarning(
								"duplicate romdb entry SHA1: " + sha1);
						}
					}
				}
			}
		} catch (FileException& e) {
			CliCommOutput::instance().printWarning(
				"Warning: couldn't open romdb.xml.\n"
				"Romtype detection might fail because of this.");
		}
	}
	
	int size = rom->getSize();
	if (size == 0) {
		return new RomInfo("", "", "", "Empty ROM", UNKNOWN);
	}

	// first try SHA1
	SHA1 sha1;
	sha1.update(rom->getBlock(), size);
	sha1.finalize();
	string digestSHA1(sha1.hex_digest());
	if (romDBSHA1.find(digestSHA1) != romDBSHA1.end()) {
		romDBSHA1[digestSHA1]->print();
		// Return a copy of the DB entry.
		return new RomInfo(*romDBSHA1[digestSHA1]);
	}

	// then try MD5 (obsolete)
	MD5 md5;
	md5.update(rom->getBlock(), size);
	md5.finalize();
	string digestMD5(md5.hex_digest());
	if (romDBMD5.find(digestMD5) != romDBMD5.end()) {
		CliCommOutput::instance().printWarning(
			"Warning: You're using an old romdb.xml file.\n"
			"         Please replace it with a recent version.");
		romDBMD5[digestMD5]->print();
		// Return a copy of the DB entry.
		return new RomInfo(*romDBMD5[digestMD5]);
	}

	// no match found
	return NULL;
}

RomInfo *RomInfo::fetchRomInfo(const Rom* rom, const Device& deviceConfig)
{
	// Look for the ROM in the ROM DB.
	RomInfo* info = searchRomDB(rom);
	if (!info) {
		info = new RomInfo(deviceConfig.getId(), "", "", "", UNKNOWN);
	}
	
	// Get specified mapper type from the config.
	// Note: config may specify "auto" as well.
	string typestr = deviceConfig.getParameter("mappertype", "auto");
	if (typestr == "auto") {
		// Guess mapper type, if it was not in DB.
		if (info->mapperType == UNKNOWN) {
			info->mapperType = guessMapperType(rom);
		}
	} else {
		// Use mapper type from config, even if this overrides DB.
		info->mapperType = nameToMapperType(typestr);
	}
	PRT_DEBUG("MapperType: " << info->mapperType);

	return info;
}

void RomInfo::print()
{
	string year(getYear());
	if (year.empty()) {
		year = "(info not available)";
	}
	string company(getCompany());
	if (company.empty()) {
		company = "(info not available)";
	}
	string info = "Found this ROM in the database:\n"
	              "  Title:    " + getTitle() + "\n"
	              "  Year:     " + year + "\n"
	              "  Company:  " + company;
	if (!getRemark().empty()) {
		info += "\n  Remark:   " + getRemark();
	}
	CliCommOutput::instance().printInfo(info);
}

} // namespace openmsx

