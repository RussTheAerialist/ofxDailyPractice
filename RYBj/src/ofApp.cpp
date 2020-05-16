#include "ofApp.h"
#include "sketches/sketches.h"

//--------------------------------------------------------------
void ofApp::setup(){
#ifdef NDI_OUTPUT
	ndi = new NdiOut(ofGetWidth(), ofGetHeight());
#endif
	ofEnableSmoothing();

	ofParameterGroup labels;
	labels.setName("Label Controls");
	ofParameterGroup durations;
	durations.setName("Durations");

	gui.setup();
	labels.add(labelBuffer.set("Label Bottom", 30., 0., ofGetHeight()));
	labels.add(lineBuffer.set("Line Spacing", 0., -100., 100.));
	durations.add(sceneDuration.set("Normal Scene", 120., 15., 3600.));
	durations.add(shortSceneDuration.set("Short Scene", 30., 15., 3600.));
	gui.add(durations);
	gui.add(labels);

	sketch.allocate(ofGetWidth(), ofGetHeight(), GL_RGBA);
	light.load("fonts/Montserrat-Light.ttf", 24);
	semibold.load("fonts/Montserrat-SemiBold.ttf", 8);
	selectedScene = -1;
	sketches = getSketches(this);

	for (const auto &sketch : sketches) {
		sketch->setup();
	}

	gui.loadFromFile("settings.xml");
	ofAddListener(sceneChange, this, &ofApp::onSceneChange);
	onSceneChange();
}

//--------------------------------------------------------------
void ofApp::update() {
	float duration = sketches.at(selectedScene)->shortDuration() ? shortSceneDuration : sceneDuration;
	if (ofGetElapsedTimef() - sceneTime > duration) {
		ofNotifyEvent(sceneChange);
	}

	sketches.at(selectedScene)->update();
}

//--------------------------------------------------------------
void ofApp::draw(){
	ofBackground(0);
	ofSetColor(255);
	ofShader* shader = sketches.at(selectedScene)->shader();


	sketch.begin();
		if (shader != nullptr) {
			shader->begin();
			shader->setUniform2f("resolution", ofGetWidth(), ofGetHeight());
		}
		sketches.at(selectedScene)->draw();

		ofSetColor(labelColor);
		ofRectangle fontBox = light.getStringBoundingBox(sketchName, 0, 0);
		fontBox.x = ofGetWidth() / 2. - (fontBox.width / 2.0);
		fontBox.y = ofGetHeight() - fontBox.height - labelBuffer;
		light.drawString(sketchName, fontBox.x, fontBox.y);

		fontBox = semibold.getStringBoundingBox("CYCLE RYB", 0, fontBox.y + fontBox.height + lineBuffer);
		fontBox.x = ofGetWidth() / 2. - (fontBox.width / 2.0);
		semibold.drawString("CYCLE RYB", fontBox.x, fontBox.y);

		if (shader != nullptr) shader->end();
	sketch.end();


#ifdef NDI_OUTPUT
	ndi->getFrame(sketch);
#endif

	sketch.draw(0, 0);



	if (showGui) gui.draw();
}

void ofApp::onSceneChange() {
	sceneTime = ofGetElapsedTimef();
	if (selectedScene >= 0 && selectedScene < sketches.size()) sketches.at(selectedScene)->stop();

	selectedScene = (selectedScene + 1) % sketches.size();

	sketches.at(selectedScene)->reset();
	sketchName = "sk. " + sketches.at(selectedScene)->name();
	labelColor = sketches.at(selectedScene)->labelColor();
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
	switch (key) {
	case ' ':
		ofNotifyEvent(sceneChange);
		break;
	case OF_KEY_TAB:
		showGui = !showGui;
		break;
	case 'r':
		sketches.at(selectedScene)->reset();
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
