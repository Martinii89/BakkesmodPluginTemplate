#include "pch.h"
#include "$projectname$.h"


BAKKESMOD_PLUGIN($projectname$, "write a plugin description here", plugin_version, PLUGINTYPE_FREEPLAY)

std::shared_ptr<CVarManagerWrapper> _globalCvarManager;

void $projectname$::onLoad()
{
	_globalCvarManager = cvarManager;

	Netcode = std::make_shared<NetcodeManager>(cvarManager, gameWrapper, exports,
		[this](const std::string& Message, PriWrapper Sender) { OnMessageReceived(Message, Sender); });

	// Example netcode request
	// cvarManager->registerNotifier("$projectname$_RequestBall", 
	//	[this](std::vector<std::string> params){RequestBall();}, "Give requester the ball", PERMISSION_ALL);

	//cvarManager->log("Plugin loaded!");

	//cvarManager->registerNotifier("my_aweseome_notifier", [&](std::vector<std::string> args) {
	//	cvarManager->log("Hello notifier!");
	//}, "", 0);

	//auto cvar = cvarManager->registerCvar("template_cvar", "hello-cvar", "just a example of a cvar");
	//auto cvar2 = cvarManager->registerCvar("template_cvar2", "0", "just a example of a cvar with more settings", true, true, -10, true, 10 );

	//cvar.addOnValueChanged([this](std::string cvarName, CVarWrapper newCvar) {
	//	cvarManager->log("the cvar with name: " + cvarName + " changed");
	//	cvarManager->log("the new value is:" + newCvar.getStringValue());
	//});

	//cvar2.addOnValueChanged(std::bind(&$projectname$::YourPluginMethod, this, _1, _2));

	// enabled decleared in the header
	//enabled = std::make_shared<bool>(false);
	//cvarManager->registerCvar("TEMPLATE_Enabled", "0", "Enable the TEMPLATE plugin", true, true, 0, true, 1).bindTo(enabled);

	//cvarManager->registerNotifier("NOTIFIER", [this](std::vector<std::string> params){FUNCTION();}, "DESCRIPTION", PERMISSION_ALL);
	//cvarManager->registerCvar("CVAR", "DEFAULTVALUE", "DESCRIPTION", true, true, MINVAL, true, MAXVAL);//.bindTo(CVARVARIABLE);
	//gameWrapper->HookEvent("FUNCTIONNAME", std::bind(&TEMPLATE::FUNCTION, this));
	//gameWrapper->HookEventWithCallerPost<ActorWrapper>("FUNCTIONNAME", std::bind(&$projectname$::FUNCTION, this, _1, _2, _3));
	//gameWrapper->RegisterDrawable(bind(&TEMPLATE::Render, this, std::placeholders::_1));


	//gameWrapper->HookEvent("Function TAGame.Ball_TA.Explode", [this](std::string eventName) {
	//	cvarManager->log("Your hook got called and the ball went POOF");
	//});
	// You could also use std::bind here
	//gameWrapper->HookEvent("Function TAGame.Ball_TA.Explode", std::bind(&$projectname$::YourPluginMethod, this);
}

void $projectname$::onUnload()
{
}

// returns a valid serverwrapper only if the user is in a plugin environment that can use netcode
ServerWrapper $projectname$::GetCurrentGameState() {
	if (gameWrapper->IsInReplay()) {
		return NULL;
	} else if (gameWrapper->IsInOnlineGame()) {
		// client is in an online game
		ServerWrapper sw = gameWrapper->GetOnlineGame();

		if (!sw) {
			return sw;
		}

		auto playlist = sw.GetPlaylist();

		if (!playlist) {
			return NULL;
		}

		// playlist 24 is a LAN match for a client
		if (playlist.GetPlaylistId() != 24) {
			return NULL;
		}

		return sw;
	} else {
		// host is in an offline game
		return gameWrapper->GetGameEventAsServer();
	}
}


// SEND REQUEST //
/*
// Teleport ball example from CinderBlock's example plugin
// Requests ball to be sent to a user
void $projectname$::RequestBall()
{
	ServerWrapper sw = GetCurrentGameState();
	if (!sw) { return; }
    CarWrapper MyCar = gameWrapper->GetLocalCar();
    if(MyCar.IsNull()) { return; }
    PriWrapper MyPRI = MyCar.GetPRI();
    if(MyPRI.IsNull()) { return; }

    Netcode->SendNewMessage("give me the ball");
}
*/

// FULFILL REQUEST //
void $projectname$::OnMessageReceived(const std::string& Message, PriWrapper Sender)
{
    if(Sender.IsNull()) { return; }

	ServerWrapper sw = GetCurrentGameState();

	if (!sw) {
		// exits if not in Netcode-friendly environment
		return;
	}

	// Do your netcode here
	// Teleport ball example from CinderBlock's example plugin
	/*
	if(Message == "give me the ball")
    {
        CarWrapper SenderCar = Sender.GetCar();
        if(SenderCar.IsNull()) { return; }
        BallWrapper Ball = sw.GetBall();
        if(Ball.IsNull()) { return; }

        //All clients should place the ball on the corresponding car
        //This makes replication much smoother and avoids odd interpolation from host
        Ball.SetLocation(SenderCar.GetLocation() + Vector(0, 0, Ball.GetRadius() + 100));
        Ball.SetVelocity(SenderCar.GetVelocity());
    }*/
}