#include "ReymentaClientApp.h"

// -------- SPOUT -------------
void ReymentaClientApp::setup()
{
	// parameters
	mParameterBag = ParameterBag::create();
	mParameterBag->mLiveCode = true;
	mParameterBag->mRenderThumbs = false;
	// instanciate the logger class
	mLog = Logan::create();
	CI_LOG_V("reymenta setup");

	fs::path passthruVertFile;
	passthruVertFile = getAssetPath("") / "default.fs";
	passthruvert = loadString(loadFile(passthruVertFile));
	loadShader(passthruVertFile);
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
		wsPing();
	});
	// set shader resolution uniform on resize (if shader has that uniform)
	getWindow()->getSignalResize().connect([this]()
	{
		mProg->uniform("iResolution", vec3(getWindowWidth(), getWindowHeight(), 0.0f));
	});
	// ws
	clientConnected = false;
	if (mParameterBag->mAreWebSocketsEnabledAtStartup) {
		wsConnect();
	}
	mPingTime = getElapsedSeconds();
	// remoteImGui
	ClientActive = false;
	Frame = 0;
	FrameReceived = 0;
	IsKeyFrame = false;
	PrevPacketSize = 0;
}
void ReymentaClientApp::loadShaderString(const string fs)
{
	try
	{	// load and compile our shader
		mProg = gl::GlslProg::create(passthruvert.c_str(), fs.c_str());
		mProg->uniform("iResolution", vec3(getWindowWidth(), getWindowHeight(), 0.0f));
	}
	catch (ci::gl::GlslProgCompileExc &exc)
	{
		CI_LOG_E( exc.what());
		console() << "Error compiling shader: " << exc.what() << endl;
	}
	catch (ci::Exception &exc)
	{
		CI_LOG_E( exc.what());
		console() << "Error loading shader: " << exc.what() << endl;
	}
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
		CI_LOG_E( exc.what());
		console() << "Error compiling shader: " << exc.what() << endl;
	}
	catch (ci::Exception &exc)
	{
		CI_LOG_E( exc.what());
		console() << "Error loading shader: " << exc.what() << endl;
	}
}
void ReymentaClientApp::update()
{
	mProg->uniform("iGlobalTime", static_cast<float>(getElapsedSeconds()));
	mProg->uniform("iMouse", mMouseCoord);
	mProg->uniform("iChannel0", 0);

	// imgui
	Frame++;
	// websockets
	if (mParameterBag->mAreWebSocketsEnabledAtStartup)
	{
		if (mParameterBag->mIsWebSocketsServer)
		{
			mServer.poll();
		}
		else
		{
			if (clientConnected)
			{
				mClient.poll();
			}
		}
	}

	if (mParameterBag->newMsg) {
		mParameterBag->newMsg = false;
		CI_LOG_V(mParameterBag->mMsg);
	}

}

void ReymentaClientApp::shutdown()
{
  
}

void ReymentaClientApp::mouseDown(MouseEvent event)
{
    
}
// --webSockets---

void ReymentaClientApp::wsPing()
{
	if (clientConnected)
	{
		if (!mParameterBag->mIsWebSocketsServer)
		{
			mClient.ping();
		}
	}
}

void ReymentaClientApp::wsConnect()
{
	// either a client or a server
	if (mParameterBag->mIsWebSocketsServer)
	{
		mServer.connectOpenEventHandler([&]()
		{
			clientConnected = true;
			mParameterBag->mMsg = "Connected";
			mParameterBag->newMsg = true;
		});
		mServer.connectCloseEventHandler([&]()
		{
			clientConnected = false;
			mParameterBag->mMsg = "Disconnected";
			mParameterBag->newMsg = true;
		});
		mServer.connectFailEventHandler([&](string err)
		{
			mParameterBag->mMsg = "WS Error";
			mParameterBag->newMsg = true;
			if (!err.empty()) {
				mParameterBag->mMsg += ": " + err;
			}

		});
		mServer.connectInterruptEventHandler([&]()
		{
			mParameterBag->mMsg = "WS Interrupted";
			mParameterBag->newMsg = true;
		});
		mServer.connectPingEventHandler([&](string msg)
		{
			mParameterBag->mMsg = "WS Ponged";
			mParameterBag->newMsg = true;
			if (!msg.empty())
			{
				mParameterBag->mMsg += ": " + msg;
			}
		});
		mServer.connectMessageEventHandler([&](string msg)
		{
			int left;
			int index;
			mParameterBag->mMsg = "WS onRead";
			mParameterBag->newMsg = true;
			if (!msg.empty())
			{
				mParameterBag->mMsg += ": " + msg;
				string first = msg.substr(0, 1);
				if (first == "{")
				{
					// json
					JsonTree json;
					try
					{
						json = JsonTree(msg);
						JsonTree jsonParams = json.getChild("params");
						for (JsonTree::ConstIter jsonElement = jsonParams.begin(); jsonElement != jsonParams.end(); ++jsonElement)
						{
							int name = jsonElement->getChild("name").getValue<int>();
							float value = jsonElement->getChild("value").getValue<float>();
							if (name > mParameterBag->controlValues.size()) {
								switch (name)
								{
								case 300:
									//selectShader
									left = jsonElement->getChild("left").getValue<int>();
									index = jsonElement->getChild("index").getValue<int>();
									//selectShader(left, index);
									break;
								default:
									break;
								}

							}
							else {
								// basic name value 
								mParameterBag->controlValues[name] = value;
							}
						}
						JsonTree jsonSelectShader = json.getChild("selectShader");
						for (JsonTree::ConstIter jsonElement = jsonSelectShader.begin(); jsonElement != jsonSelectShader.end(); ++jsonElement)
						{
						}
					}
					catch (cinder::JsonTree::Exception exception)
					{
						mParameterBag->mMsg += " error jsonparser exception: ";
						mParameterBag->mMsg += exception.what();
						mParameterBag->mMsg += "  ";
					}
				}
			}
		});
		mServer.listen(mParameterBag->mWebSocketsPort);
	}
	else
	{
		mClient.connectOpenEventHandler([&]()
		{
			clientConnected = true;
			mParameterBag->mMsg = "Connected";
			mParameterBag->newMsg = true;
		});
		mClient.connectCloseEventHandler([&]()
		{
			clientConnected = false;
			mParameterBag->mMsg = "Disconnected";
			mParameterBag->newMsg = true;
		});
		mClient.connectFailEventHandler([&](string err)
		{
			mParameterBag->mMsg = "WS Error";
			mParameterBag->newMsg = true;
			if (!err.empty()) {
				mParameterBag->mMsg += ": " + err;
			}

		});
		mClient.connectInterruptEventHandler([&]()
		{
			mParameterBag->mMsg = "WS Interrupted";
			mParameterBag->newMsg = true;
		});
		mClient.connectPingEventHandler([&](string msg)
		{
			mParameterBag->mMsg = "WS Ponged";
			mParameterBag->newMsg = true;
			if (!msg.empty())
			{
				mParameterBag->mMsg += ": " + msg;
			}
		});
		mClient.connectMessageEventHandler([&](string msg)
		{
			int left;
			int index;
			mParameterBag->mMsg = "WS onRead";
			mParameterBag->newMsg = true;
			if (!msg.empty())
			{
				mParameterBag->mMsg += ": " + msg;
				string first = msg.substr(0, 1);
				if (first == "{")
				{
					// json
					JsonTree json;
					try
					{
						json = JsonTree(msg);
						JsonTree jsonParams = json.getChild("params");
						for (JsonTree::ConstIter jsonElement = jsonParams.begin(); jsonElement != jsonParams.end(); ++jsonElement)
						{
							int name = jsonElement->getChild("name").getValue<int>();
							float value = jsonElement->getChild("value").getValue<float>();
							if (name > mParameterBag->controlValues.size()) {
								switch (name)
								{
								case 300:
									//selectShader
									left = jsonElement->getChild("left").getValue<int>();
									index = jsonElement->getChild("index").getValue<int>();
									//selectShader(left, index);
									break;
								default:
									break;
								}

							}
							else {
								// basic name value 
								mParameterBag->controlValues[name] = value;
							}
						}
						JsonTree jsonSelectShader = json.getChild("selectShader");
						for (JsonTree::ConstIter jsonElement = jsonSelectShader.begin(); jsonElement != jsonSelectShader.end(); ++jsonElement)
						{
						}
					}
					catch (cinder::JsonTree::Exception exception)
					{
						mParameterBag->mMsg += " error jsonparser exception: ";
						mParameterBag->mMsg += exception.what();
						mParameterBag->mMsg += "  ";
					}
				}
				else if (msg.substr(0, 7) == "uniform")
				{
					// fragment shader from live coding
					loadShaderString(msg);

				}
				else if (msg.substr(0, 7) == "#version")
				{
					// fragment shader from live coding
					loadShaderString(msg);

				}
				else if (first == "I")
				{

					if (msg == "ImInit") {
						// send ImInit OK
						if (!ClientActive)
						{
							ClientActive = true;
							ForceKeyFrame = true;
							// Send confirmation
							mServer.write("ImInit");
							// Send font texture
							/*unsigned char* pixels;
							int width, height;
							ImGui::GetIO().Fonts->GetTexDataAsAlpha8(&pixels, &width, &height);

							PreparePacketTexFont(pixels, width, height);
							SendPacket();*/
						}
					}
					else if (msg.substr(0, 11) == "ImMouseMove") {
						string trail = msg.substr(12);
						unsigned commaPosition = trail.find(",");
						if (commaPosition > 0) {
							mMouseCoord.x = atoi(trail.substr(0, commaPosition).c_str());
							mMouseCoord.y = atoi(trail.substr(commaPosition + 1).c_str());
							ImGuiIO& io = ImGui::GetIO();
							io.MousePos = toPixels(vec2(mMouseCoord.x, mMouseCoord.y));

						}

					}
					else if (msg.substr(0, 12) == "ImMousePress") {
						ImGuiIO& io = ImGui::GetIO(); // 1,0 left click 1,1 right click
						io.MouseDown[0] = false;
						io.MouseDown[1] = false;
						int rightClick = atoi(msg.substr(15).c_str());
						if (rightClick == 1) {

							io.MouseDown[0] = false;
							io.MouseDown[1] = true;
						}
						else {
							io.MouseDown[0] = true;
							io.MouseDown[1] = false;
						}
					}
				}
			}
		});
		wsClientConnect();
	}
	mParameterBag->mAreWebSocketsEnabledAtStartup = true;
	clientConnected = true;
}
void ReymentaClientApp::wsClientConnect()
{
	stringstream s;
	s << "ws://" << mParameterBag->mWebSocketsHost << ":" << mParameterBag->mWebSocketsPort;
	mClient.connect(s.str());
}
void ReymentaClientApp::wsClientDisconnect()
{
	if (clientConnected)
	{
		mClient.disconnect();
	}
}

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
