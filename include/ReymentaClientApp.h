#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Batch.h"
#include "cinder/gl/Context.h"
#include "cinder/gl/VboMesh.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class ReymentaClientApp : public App {

public:

	void						setup();
	void						draw();
	void						update();
	void						mouseDown(MouseEvent event);
	void						shutdown();

private:
	void						loadShader(const fs::path &fragment_path);
	gl::GlslProgRef				mProg;
	gl::VboMeshRef				mMesh;
	ci::vec4					mMouseCoord;
	ci::gl::TextureRef			texture;
};