#pragma once

#include "k1_6.h"
#include "k1_7.h"
#include "k2_1.h"
#include "k2_2.h"
#include "k2_3.h"
#include "k2_4.h"
/// INCLUDE

#include "shame.h"

#define SKETCH(sketch) retval.push_back(std::make_unique<sketch>(parent))
#define SHAME(name, background, label) { \
 string _name = #name; \
_name.replace(2, 1, "."); \
 retval.push_back(std::make_unique<shame>(parent, _name, background, label)); \
}

std::vector<std::unique_ptr<IDailySketch> > getSketches(ofApp* parent) {
	std::vector<std::unique_ptr<IDailySketch> > retval;

	/// SKETCH LIST
	SKETCH(k2_4);
	SKETCH(k2_3);
	SKETCH(k2_2);
	SKETCH(k2_1);
	SKETCH(k1_7);
	SKETCH(k1_6);
	SHAME(k1_5, ofColor::wheat, ofColor::darkOliveGreen);
	SHAME(k1_4, ofColor::lemonChiffon, ofColor::maroon);
	SHAME(k1_3, ofColor::darkBlue, ofColor::goldenRod);
	SHAME(k1_2, ofColor::mediumSeaGreen, ofColor::greenYellow);
	SHAME(k1_1, ofColor::fuchsia, ofColor::darkSalmon);

	return retval;
}