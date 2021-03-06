#include "ofApp.h"
#include "JzAzBzColor.h"

//#define RENDER_FLOWS
#define RENDER_MAGIC
//#define RENDER_DEPTH

//--------------------------------------------------------------
void ofApp::setup() {

	ofSetFrameRate(60.);
	ofEnableAlphaBlending();
	ofEnableAntiAliasing();
	ofDisableArbTex();

	depth_output_width = (float)ofGetWidth() / SCALE;
	depth_output_height = depth_output_width / DEPTH_ASPECT;

	gui.setup();
	gui.add(threshold.set("Depth Threshold", 128, 0, 255));

	gui.setPosition(0, 480);

	dust = make_unique<MagicDust>(ofGetWidth(), ofGetHeight());

	channel0.allocate(ofGetWidth(), ofGetHeight(), GL_RGBA);
	channel1.allocate(ofGetWidth(), ofGetHeight(), GL_RGBA);

	output.allocate(ofGetWidth(), ofGetHeight(), OF_IMAGE_COLOR);
	depth_scaled.allocate(depth_output_width, depth_output_height);

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

	camera.setupDevice();
	// TODO: setupFilter - Research
	camera.enableDepth(DEPTH_WIDTH, DEPTH_HEIGHT, 30, false);  // useArbTex = false
	camera.startStream();

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

	camera.update();
	if (camera.isFrameNew()) {
		camera.getDepthTexture()->readToPixels(pixels);
		c_depth.setFromPixels(pixels);
		c_depth.convertToGrayscalePlanarImage(depth_image, 0);

		depth_image.contrastStretch();
		depth_image.blurHeavily();
		depth_image.dilate();
		depth_image.erode();

		depth_image.threshold(threshold); // ~TODO: Threshold creates a mask so that we have detail in the foreground depth
		depth_image.updateTexture();

		depth_scaled.scaleIntoMe(depth_image);
		update_target_location();
	}

	ofPixels& depth = depth_scaled.getPixels();

	dust->update([&](float x, float y, float speed) {
		glm::ivec2 coords = field_coord(x, y);
		glm::vec2 force = field_force_at(coords.x, coords.y, speed);
		return force;
		}, [&](float x, float y) {
			return field_coord(x, y);
		}, depth);

}

void ofApp::update_target_location() {
	ofPixels& depth = depth_scaled.getPixels();
	glm::vec3 acc = { 0., 0., 0. };
	for (int x = 0; x < depth.getWidth(); x++) {
		for (int y = 0; y < depth.getHeight(); y++) {
			if (depth.getColor(x, y).getBrightness() > 128) {
				acc += { (float)x, (float)y, 1. };
			}
		}
	}

	if (acc.z <= 1) return;

	dust->set_target(acc.x / acc.z * SCALE, acc.y / acc.z * SCALE);
}

glm::ivec2 ofApp::field_coord(float x, float y) {
	return { ofClamp((int)floor(x / SCALE), 0, cols - 1),
			 ofClamp((int)floor(y / SCALE), 0, rows - 1) };

}

glm::vec2 ofApp::field_force_at(int col, int row, float speed) {

	float angle = field[col][row];

	glm::vec2 force = { sin(angle), cos(angle) };

	return glm::normalize(force) * speed;
}

//--------------------------------------------------------------
void ofApp::draw() {

#ifdef RENDER_MAGIC
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

#endif // RENDER_MAGIC

#ifdef RENDER_DEPTH
	ofPushMatrix();
	c_depth.draw(640, 0);
	depth_image.draw(0, 0);
	depth_scaled.draw(640, 480);
	ofPopMatrix();
#endif // RENDER_DEPTH

#ifdef RENDER_FLOWS

	ofSetLineWidth(3.);

	ofPixels& depth = depth_scaled.getPixels();

	for (int col = 0; col < cols; col++) {
		for (int row = 0; row < rows; row++) {
			int x = (col + 0.5) * SCALE;
			int y = (row + 0.5) * SCALE;


			glm::vec2 center = { x, y };
			ofColor c = depth.getColor(col, row);
			ofSetColor(fieldColor, c.getBrightness());
			glm::vec2 bias = c.getBrightness() > 128. ? VERTICAL : HORIZONTAL;
			glm::vec2 start = field_force_at(col, row, 7.) + bias * 7.;
			glm::vec2 end = start * -1.;
			ofDrawLine(start + center, end + center);
			ofDrawCircle(start + center, 3.);
		}
	}
#endif // RENDER_FLOWS

	ofSetColor(neighborColor);
	ofDrawCircle(dust->get_target(), 10.0);

	gui.draw();
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
		{ 15, 15. },
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
