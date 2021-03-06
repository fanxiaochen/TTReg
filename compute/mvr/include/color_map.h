#pragma once
#ifndef ColorMap_H
#define ColorMap_H

#include <osg/Vec4>
#include <vector>

class ColorMap {
public:
	static ColorMap& Instance() {
		static ColorMap theSingleton;
		return theSingleton;
	}

	typedef enum {JET} Map;
	typedef enum {LIGHT_BLUE} NamedColor;
	typedef enum {DISCRETE_TOTAL, DISCRETE_KEY} DiscreteColor;

	/* more (non-static) functions here */
	const osg::Vec4& getColor(NamedColor nameColor);
	const osg::Vec4& getColor(Map map, float value, float low=0.0f, float high=1.0f);
	const osg::Vec4& getColor(DiscreteColor map, int idx);

private:
	ColorMap();                            // ctor hidden
	ColorMap(ColorMap const&){}            // copy ctor hidden
	ColorMap& operator=(ColorMap const&){} // assign op. hidden
	virtual ~ColorMap(){}                          // dtor hidden

	std::vector<osg::Vec4> _jet;
	osg::Vec4              _lightBlue;
	std::vector<osg::Vec4> _discrete_total;
	std::vector<osg::Vec4> _discrete_key;

	const osg::Vec4& getColor(const std::vector<osg::Vec4>& map, float value, float low, float high);
	const osg::Vec4& getColor(const std::vector<osg::Vec4>& map, int idx);
};

#endif // ColorMap_H