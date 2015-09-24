#include <znc/main.h>
#include <znc/User.h>
#include <znc/IRCNetwork.h>
#include <znc/Modules.h>
#include <znc/Nick.h>
#include <znc/Chan.h>
#include <znc/IRCSock.h>

#include <limits>

#include "twitchtmi.h"
#include "jload.h"

TwitchTMI::~TwitchTMI()
{
	debug = false;
}

bool TwitchTMI::OnLoad(const CString& sArgsi, CString& sMessage)
{
	OnBoot();

	if(GetNetwork())
	{
		for(CChan *ch: GetNetwork()->GetChans())
		{
			ch->SetTopic(CString());

			CString chname = ch->GetName().substr(1);
			CThreadPool::Get().addJob(new TwitchTMIJob(this, chname));
		}
	}

	PutIRC("CAP REQ :twitch.tv/membership");
	PutIRC("CAP REQ :twitch.tv/tags");
	PutIRC("CAP REQ :twitch.tv/commands");

	return true;
}

bool TwitchTMI::OnBoot()
{
	initCurl();

	timer = new TwitchTMIUpdateTimer(this);
	AddTimer(timer);

	return true;
}

void TwitchTMI::OnIRCConnected()
{
	PutIRC("CAP REQ :twitch.tv/membership");
	PutIRC("CAP REQ :twitch.tv/tags");
	PutIRC("CAP REQ :twitch.tv/commands");
}

CModule::EModRet TwitchTMI::OnRaw(CString &sLine)
{
	if(debug)
		PutUser(":twitch PRIVMSG twitch:" + sLine);
	return CModule::CONTINUE;
}

CModule::EModRet TwitchTMI::OnUserRaw(CString &sLine)
{
	if(sLine.Left(5).Equals("WHO #"))
		return CModule::HALT;

	if(sLine.Left(5).Equals("AWAY "))
		return CModule::HALT;

	if(sLine.Left(12).Equals("TWITCHCLIENT"))
		return CModule::HALT;

	if(sLine.Left(9).Equals("JTVCLIENT"))
		return CModule::HALT;

	return CModule::CONTINUE;
}

CModule::EModRet TwitchTMI::OnUserJoin(CString& sChannel, CString& sKey)
{
	CString chname = sChannel.substr(1);
	CThreadPool::Get().addJob(new TwitchTMIJob(this, chname));

	return CModule::CONTINUE;
}

CModule::EModRet TwitchTMI::OnPrivMsg(CNick& nick, CString& sMessage)
{
	if(nick.GetNick().Equals("jtv", true))
		return CModule::HALT;

	return CModule::CONTINUE;
}

CModule::EModRet TwitchTMI::OnChanMsg(CNick& nick, CChan& channel, CString& sMessage)
{
	if(nick.GetNick().Equals("jtv", true))
		return CModule::HALT;

	return CModule::CONTINUE;
}

CModule::EModRet TwitchTMI::OnChanAction (CNick &Nick, CChan &Channel, CString &sMessage)
{
	if(Nick.GetNick().Equals("jtv", true))
		return CModule::HALT;

	return CModule::CONTINUE;
}

bool TwitchTMI::OnServerCapAvailable(const CString &sCap)
{
	if(sCap == "twitch.tv/membership")
	{
		CUtils::PrintMessage("TwitchTMI: Requesting twitch.tv/membership cap");
		return true;
	}
	else if(sCap == "twitch.tv/tags")
	{
		CUtils::PrintMessage("TwitchTMI: Requesting twitch.tv/tags cap");
		return true;
	}
	else if(sCap == "twitch.tv/commands")
	{
		CUtils::PrintMessage("TwitchTMI: Requesting twitch.tv/commands cap");
		return true;
	}

	CUtils::PrintMessage(CString("TwitchTMI: Not requesting ") + sCap + " cap");

	return false;
}



TwitchTMIUpdateTimer::TwitchTMIUpdateTimer(TwitchTMI *tmod)
	:CTimer(tmod, 30, 0, "TwitchTMIUpdateTimer", "Downloads Twitch information")
	,mod(tmod)
{
}

void TwitchTMIUpdateTimer::RunJob()
{
	if(!mod->GetNetwork())
		return;

	for(CChan *chan: mod->GetNetwork()->GetChans())
	{
		CString chname = chan->GetName().substr(1);
		CThreadPool::Get().addJob(new TwitchTMIJob(mod, chname));
	}
}


void TwitchTMIJob::runThread()
{
	std::stringstream ss;
	ss << "https://api.twitch.tv/kraken/channels/" << channel;

	CString url = ss.str();

	Json::Value root = getJsonFromUrl(url.c_str(), "Accept: application/vnd.twitchtv.v3+json");

	if(root.isNull())
	{
		return;
	}

	Json::Value &titleVal = root["status"];
	title = CString();

	if(!titleVal.isString())
		titleVal = root["title"];

	if(!titleVal.isString())
		return;

	title = titleVal.asString();
	title.Trim();
}

void TwitchTMIJob::runMain()
{
	if(title.empty())
		return;

	CChan *chan = mod->GetNetwork()->FindChan(CString("#") + channel);

	if(!chan)
		return;

	if(chan->GetTopic() != title)
	{
		chan->SetTopic(title);
		chan->SetTopicOwner("jtv");
		chan->SetTopicDate((unsigned long)time(nullptr));

		std::stringstream ss;
		ss << ":jtv TOPIC #" << channel << " :" << title;

		mod->PutUser(ss.str());
	}
}

void TwitchTMI::DebugCommand(const CString& sLine)
{
	debug = sLine.AsLower() == "on";
}

template<> void TModInfo<TwitchTMI>(CModInfo &info)
{
	info.SetWikiPage("Twitch");
	info.SetHasArgs(false);
}

NETWORKMODULEDEFS(TwitchTMI, "Twitch IRC helper module")
