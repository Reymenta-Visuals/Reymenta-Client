#include "ReymentaClientApp.h"

// -------- SPOUT -------------
void ReymentaClientApp::setup()
{
	loadShader(getAssetPath("default.fs"));
	string pathToStartupFile = (getAssetPath("") / "reymenta.jpg").string();
	if (fs::exists(pathToStartupFile))
	{
		texture = gl::Texture::create(loadImage(loadAsset("reymenta.jpg")));
	}
	// create a rectangle to be drawn with our shader program
	// default is from -0.5 to 0.5, so we scale by 2 to get -1.0 to 1.0
	mMesh = gl::VboMesh::create(geom::Rect());// .scale(vec2(2.0f, 2.0f)));

	// load a new shader on file drop
	getWindow()->getSignalFileDrop().connect([this](FileDropEvent &event)
	{
		auto file = event.getFile(0);
		if (fs::is_regular_file(file));
		{ loadShader(file); }
	});
	// set iMouse XY on mouse move
	getWindow()->getSignalMouseMove().connect([this](MouseEvent &event)
	{
		mMouseCoord.x = event.getX();
		mMouseCoord.y = getWindowHeight() - event.getY();
	});
	// set iMouse ZW on mouse down
	getWindow()->getSignalMouseDown().connect([this](MouseEvent &event)
	{
		mMouseCoord.z = event.getX();
		mMouseCoord.w = getWindowHeight() - event.getY();
	});
	// set shader resolution uniform on resize (if shader has that uniform)
	getWindow()->getSignalResize().connect([this]()
	{
		mProg->uniform("iResolution", vec3(getWindowWidth(), getWindowHeight(), 0.0f));
	});
}
void ReymentaClientApp::loadShader(const fs::path &fragment_path)
{
	try
	{	// load and compile our shader
		mProg = gl::GlslProg::create(gl::GlslProg::Format().vertex(loadAsset("default.vs"))
			.fragment(loadFile(fragment_path)));
		// no exceptions occurred, so store the shader's path for reloading on keypress
		//mCurrentShaderPath = fragment_path;
		mProg->uniform("iResolution", vec3(getWindowWidth(), getWindowHeight(), 0.0f));

	}
	catch (ci::gl::GlslProgCompileExc &exc)
	{
		console() << "Error compiling shader: " << exc.what() << endl;
	}
	catch (ci::Exception &exc)
	{
		console() << "Error loading shader: " << exc.what() << endl;
	}
}
void ReymentaClientApp::update()
{
	mProg->uniform("iGlobalTime", static_cast<float>(getElapsedSeconds()));
	mProg->uniform("iMouse", mMouseCoord);
	mProg->uniform("iChannel0", 0);
}
// -------- SPOUT -------------
void ReymentaClientApp::shutdown()
{
  
}

void ReymentaClientApp::mouseDown(MouseEvent event)
{
    
}
// ----------------------------


void ReymentaClientApp::draw()
{
	// clear out the window with black
	gl::clear(Color(0.2, 0, 0));
	// use our shader for this draw loop
	//gl::GlslProgScope prog(mProg);
	gl::ScopedGlslProg shader(mProg);
	texture->bind(0);
	// draw our screen rectangle
	gl::draw(mMesh);

}


CINDER_APP(ReymentaClientApp, RendererGl)
