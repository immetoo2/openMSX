// $Id$

#include <cassert>
#include <iostream>
#include "KeyJoystick.hh"
#include "EventDistributor.hh"
#include "Keys.hh"
#include "MSXConfig.hh"
#include "Config.hh"
#include "CliCommOutput.hh"

namespace openmsx {

KeyJoystick::KeyJoystick()
{
	EventDistributor::instance()->registerEventListener(SDL_KEYDOWN, this, 1);
	EventDistributor::instance()->registerEventListener(SDL_KEYUP  , this, 1);

	status = JOY_UP | JOY_DOWN | JOY_LEFT | JOY_RIGHT |
	         JOY_BUTTONA | JOY_BUTTONB;

	// built in defaults: no working key joystick
	upKey      = Keys::K_NONE;
	rightKey   = Keys::K_NONE;
	downKey    = Keys::K_NONE;
	leftKey    = Keys::K_NONE;
	buttonAKey = Keys::K_NONE;
	buttonBKey = Keys::K_NONE;

	try {
		Config* config = MSXConfig::instance()->getConfigById("KeyJoystick");
		upKey      = getConfigKeyCode("upkey",      config);
		rightKey   = getConfigKeyCode("rightkey",   config);
		downKey    = getConfigKeyCode("downkey",    config);
		leftKey    = getConfigKeyCode("leftkey",    config);
		buttonAKey = getConfigKeyCode("buttonakey", config);
		buttonBKey = getConfigKeyCode("buttonbkey", config);
	} catch (ConfigException& e) {
		CliCommOutput::instance().printWarning(
			"KeyJoystick not configured, so it won't be usable...");
		return;
	}
}

KeyJoystick::~KeyJoystick()
{
	EventDistributor::instance()->unregisterEventListener(SDL_KEYDOWN, this, 1);
	EventDistributor::instance()->unregisterEventListener(SDL_KEYUP  , this, 1);
}

// auxilliary function for constructor

Keys::KeyCode KeyJoystick::getConfigKeyCode(const string& keyname,
                                            const Config* config)
{
	Keys::KeyCode testKey = Keys::K_NONE;
	if (config->hasParameter(keyname)) {
		testKey = Keys::getCode(config->getParameter(keyname));
		if (testKey == Keys::K_NONE) {
			CliCommOutput::instance().printWarning(
				"unknown keycode \"" +
				config->getParameter(keyname) + "\" for key \"" +
				keyname + "\" in KeyJoystick configuration");
		}
	}
	return testKey;
}

// Pluggable
const string& KeyJoystick::getName() const
{
	static const string name("keyjoystick");
	return name;
}

const string& KeyJoystick::getDescription() const
{
	static const string desc(
		"Key-Joystick, use your keyboard to emulate an MSX joystick. "
		"See manual for information on how to configure this.");
	return desc;
}

void KeyJoystick::plug(Connector* connector, const EmuTime& time)
	throw()
{
}

void KeyJoystick::unplug(const EmuTime& time)
{
}


// KeyJoystickDevice
byte KeyJoystick::read(const EmuTime& time)
{
	return status;
}

void KeyJoystick::write(byte value, const EmuTime& time)
{
	//do nothing
}


// EventListener
bool KeyJoystick::signalEvent(SDL_Event& event) throw()
{
	Keys::KeyCode theKey = Keys::getCode(event.key.keysym.sym);
	switch (event.type) {
	case SDL_KEYDOWN:
		if      (theKey == upKey)      status &= ~JOY_UP;
		else if (theKey == downKey)    status &= ~JOY_DOWN;
		else if (theKey == leftKey)    status &= ~JOY_LEFT;
		else if (theKey == rightKey)   status &= ~JOY_RIGHT;
		else if (theKey == buttonAKey) status &= ~JOY_BUTTONA;
		else if (theKey == buttonBKey) status &= ~JOY_BUTTONB;
		break;
	case SDL_KEYUP:
		if      (theKey == upKey)      status |= JOY_UP;
		else if (theKey == downKey)    status |= JOY_DOWN;
		else if (theKey == leftKey)    status |= JOY_LEFT;
		else if (theKey == rightKey)   status |= JOY_RIGHT;
		else if (theKey == buttonAKey) status |= JOY_BUTTONA;
		else if (theKey == buttonBKey) status |= JOY_BUTTONB;
		break;
	default:
		assert(false);
	}
	return true;
}

} // namespace openmsx
