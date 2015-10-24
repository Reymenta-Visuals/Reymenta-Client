#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Batch.h"
#include "cinder/gl/Context.h"
#include "cinder/gl/VboMesh.h"
// WebSockets
#include "WebSocketClient.h"
#include "WebSocketServer.h"
// lz4 
#include "lz4.h"
// imgui
#include "CinderImGui.h"

#include "ParameterBag.h"
#include "Logan.h"

#define IMGUI_REMOTE_KEY_FRAME    60  // send keyframe every 30 frames
#define IMGUI_REMOTE_INPUT_FRAMES 120 // input valid during 120 frames

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace Reymenta;
// imgui
//------------------
// ImGuiRemoteInput
// - a structure to store input received from remote imgui, so you can use it on your whole app (keys, mouse) or just in imgui engine
// - use GetImGuiRemoteInput to read input data safely (valid for IMGUI_REMOTE_INPUT_FRAMES)
//------------------
struct RemoteInput
{
	ImVec2	MousePos;
	int		MouseButtons;
	int		MouseWheel;
	bool	KeyCtrl;
	bool	KeyShift;
	bool	KeysDown[256];
};
struct Cmd
{
	int   vtx_count;
	float clip_rect[4];
	void Set(const ImDrawCmd &draw_cmd)
	{
		vtx_count = draw_cmd.ElemCount;
		clip_rect[0] = draw_cmd.ClipRect.x;
		clip_rect[1] = draw_cmd.ClipRect.y;
		clip_rect[2] = draw_cmd.ClipRect.z;
		clip_rect[3] = draw_cmd.ClipRect.w;
		//printf("DrawCmd: %d ( %.2f, %.2f, %.2f, %.2f )\n", vtx_count, clip_rect[0], clip_rect[1], clip_rect[2], clip_rect[3]);
	}
};
struct Vtx
{
	short x, y; // 16 short
	short u, v; // 16 fixed point
	unsigned char r, g, b, a; // 8*4
	void Set(const ImDrawVert &vtx)
	{
		x = (short)(vtx.pos.x);
		y = (short)(vtx.pos.y);
		u = (short)(vtx.uv.x * 32767.f);
		v = (short)(vtx.uv.y * 32767.f);
		r = (vtx.col >> 0) & 0xff;
		g = (vtx.col >> 8) & 0xff;
		b = (vtx.col >> 16) & 0xff;
		a = (vtx.col >> 24) & 0xff;
	}
};
class ReymentaClientApp : public App {

public:

	void						setup();
	void						draw();
	void						update();
	void						mouseDown(MouseEvent event);
	void						shutdown();

private:
	// parameters
	ParameterBagRef				mParameterBag;
	// Logger
	LoganRef					mLog;

	void						loadShader(const fs::path &fragment_path);
	gl::GlslProgRef				mProg;
	gl::VboMeshRef				mMesh;
	ci::vec4					mMouseCoord;
	ci::gl::TextureRef			texture;
	// WebSockets
	// Web socket client
	void						wsClientDisconnect();
	WebSocketClient				mClient;
	void						wsClientConnect();
	void						onWsConnect();
	void						onWsDisconnect();
	void						onWsError(std::string err);
	void						onWsInterrupt();
	void						onWsPing(std::string msg);
	void						onWsRead(std::string msg);
	bool						clientConnected;
	// Web socket  server
	WebSocketServer				mServer;
	void						serverConnect();
	void						serverDisconnect();
	double						mPingTime;
	void						wsConnect();
	void						wsPing();
	// remoteImGui
	bool						ClientActive;
	int							Frame;
	int 						FrameReceived;
	int 						PrevPacketSize;
	bool 						IsKeyFrame;
	bool 						ForceKeyFrame;
	std::vector<unsigned char>	Packet;
	std::vector<unsigned char>	PrevPacket;
	RemoteInput					Input;
	void						Write(unsigned char c);
	void						Write(unsigned int i);
	void						Write(const void *data, int size);
	void						WriteDiff(const void *data, int size);

	void						PreparePacket(unsigned char data_type, unsigned int data_size);
	void						PreparePacketTexFont(const void *data, unsigned int w, unsigned int h);


	// packet types
	enum { TEX_FONT = 255, FRAME_KEY = 254, FRAME_DIFF = 253 };

};