// $Id$

#ifndef __FLOATSETTING_HH__
#define __FLOATSETTING_HH__

#include "Setting.hh"
#include "NonInheritable.hh"

namespace openmsx {

/** A Setting with a floating point value.
  */
class FloatSetting: public Setting<float>, NON_INHERITABLE(FloatSetting)
{
public:
	FloatSetting(const string& name, const string& description,
	             float initialValue, float minValue, float maxValue);
	virtual ~FloatSetting();

	/** Change the allowed range.
	  * @param minValue New minimal value (inclusive).
	  * @param maxValue New maximal value (inclusive).
	  */
	void setRange(float minValue, float maxValue);

	// Implementation of Setting interface:
	virtual string getValueString() const;
	virtual void setValueString(const string& valueString)
		throw(CommandException);
	virtual void setValue(const float& newValue);

protected:
	float minValue;
	float maxValue;
};

} // namespace openmsx

#endif //__FLOATSETTING_HH__
