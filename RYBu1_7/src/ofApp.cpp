#include "ofApp.h"
#include "JzAzBzColor.h"

//--------------------------------------------------------------
void ofApp::setup() {

	ofSetFrameRate(60.);
	ofEnableAlphaBlending();
	ofEnableAntiAliasing();
	ofDisableArbTex();

	dust = make_unique<MagicDust>(ofGetWidth(), ofGetHeight());

	channel0.allocate(ofGetWidth(), ofGetHeight(), GL_RGBA);
	channel1.allocate(ofGetWidth(), ofGetHeight(), GL_RGBA);

	output.allocate(ofGetWidth(), ofGetHeight(), OF_IMAGE_COLOR);

	char *version = (char*)glGetString(GL_VERSION);
	ofLog() << "GL " << version;

	if (!blur_shade.load("shader")) {
		ofLog() << "Shader failed to load";
		ofExit();
	}

	if (!add_shade.load("shader.vert", "add.frag")) {
		ofLog() << "Shader failed to load";
		ofExit();
	}


	paused = debug = false;

	rows = ofGetHeight() / SCALE;
	cols = ofGetWidth() / SCALE;
	nRows = ofGetHeight() / NEIGHBORHOOD;
	nCols = ofGetWidth() / NEIGHBORHOOD;
	increment = INCREMENT;

	palette.reserve(PALETTE_SIZE);
	for (int i = 0; i < PALETTE_SIZE; i++) {
		palette.push_back(ofColor::chartreuse);
	}

	field = new float*[cols];
	for (int i = 0; i < cols; i++) {
		field[i] = new float[rows];
	}
	dust->boids.reserve(STARTING_BOIDS);

	for (int i = 0; i < STARTING_BOIDS; i++) {
		ofColor c = palette.at((int)floor(ofRandomuf() * palette.size()));
		dust->boids.push_back(new_boid_at(ofRandomWidth(), ofRandomHeight(), c));
	}

	ofSetBackgroundAuto(false);
	initialize();
}

void ofApp::initialize() {
	//channel0.begin();
	//ofBackground(32);
	//channel0.end();

	float j = ofRandomuf();
	float b = ofRandomuf();
	fieldColor = jab_to_of(0.2, j, b);
	boidColor = jab_to_of(0.8, 1 - j, 1 - b);
	neighborColor = jab_to_of(1.0, b, j);

	for (int i = 0; i < PALETTE_SIZE; i++) {
		palette[i] = jab_random(0.8, 1. - j, 1. - b);
	}

	float startY = ofRandomf();

	xoff = ofRandomf();

	for (int i = 0; i < cols; i++) {
		yoff = startY;
		for (int j = 0; j < rows; j++) {
			field[i][j] = ofNoise(xoff, yoff, zoff) * (2 * PI);
			yoff += increment;
		}
		xoff += increment;
	}

	dust->set_colors(palette);
}

ofColor ofApp::jab_random(float j, float a, float b) {
	j = 1;
	a += ofRandomuf();
	b += ofRandomuf();

	return jab_to_of(j, a, b);
}


//--------------------------------------------------------------
void ofApp::update(){
	if (paused) return;

	dust->update([&](float x, float y, float speed) {
		return field_force_at(x, y, speed);
	});

}

glm::vec2 ofApp::field_force_at(float x, float y, float speed) {
	int row = ofClamp((int)floor(y / SCALE), 0, rows - 1);
	int col = ofClamp((int)floor(x / SCALE), 0, cols - 1);
	float angle = field[col][row];

	glm::vec2 force = { sin(angle), cos(angle) };

	return glm::normalize(force) * speed;
}

//--------------------------------------------------------------
void ofApp::draw() {
	channel0.begin();
	ofClear(0, 0, 0, 255);
	dust->draw_boids();
	channel0.end();

	channel1.begin();
	ofClear(0, 0, 0, 255);
	channel1.end();

	
	for (int i = 0; i < ITERATIONS; i++) {
		draw_with(channel0, channel1, blur_shade, HORIZONTAL, i);
		draw_with(channel1, channel0, blur_shade, VERTICAL, i);
	}
	draw_with(channel0, channel1, blur_shade, HORIZONTAL, 1.);
	draw_with(channel1, channel0, blur_shade, VERTICAL, 1.);

	channel1.begin();
	dust->draw_boids();
	channel1.end();

	add_shade.begin();
	add_shade.setUniform3f("resolution", { ofGetWidth(), ofGetHeight(), 0 }); //TODO: optimize
	add_shade.setUniformTexture("channel0", channel0.getTexture(), 0);
	add_shade.setUniformTexture("channel1", channel1.getTexture(), 1);
	ofDrawRectangle(0, 0, ofGetWidth(), ofGetHeight());
	add_shade.end();
}

void ofApp::draw_with(ofFbo& source, ofFbo& target, ofShader& s, const glm::vec2& direction, float scale) {
	target.begin();

	s.begin();
	s.setUniform3f("resolution", { ofGetWidth(), ofGetHeight(), 0 }); //TODO: optimize
	s.setUniformTexture("channel0", source.getTexture(), 0);
	s.setUniform2f("direction", direction * scale);
	ofDrawRectangle(0, 0, ofGetWidth(), ofGetHeight());
	s.end();
	target.end();
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
	switch (key) {
	case 's':
		output.clear();
		channel0.readToPixels(output);
		ofSaveImage(output, "frame.png", OF_IMAGE_QUALITY_BEST);
		break;
	case ' ':
		initialize();
		break;
	case OF_KEY_RETURN:
		paused = !paused;
		break;
	case OF_KEY_TAB:
		debug = !debug;
		break;
	}
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){
	if (button == 0) {
		dust->boids.push_back(new_boid_at(x, y, boidColor));
	}
}

boid ofApp::new_boid_at(float x, float y, ofColor c) {
	return {
		{x, y},
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 8, 6. },
		c
	};
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}
