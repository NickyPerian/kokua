/** 
* @file llbottomtray.cpp
* @brief LLBottomTray class implementation
*
* $LicenseInfo:firstyear=2009&license=viewergpl$
* 
* Copyright (c) 2009, Linden Research, Inc.
* 
* Second Life Viewer Source Code
* The source code in this file ("Source Code") is provided by Linden Lab
* to you under the terms of the GNU General Public License, version 2.0
* ("GPL"), unless you have obtained a separate licensing agreement
* ("Other License"), formally executed by you and Linden Lab.  Terms of
* the GPL can be found in doc/GPL-license.txt in this distribution, or
* online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
* 
* There are special exceptions to the terms and conditions of the GPL as
* it is applied to this Source Code. View the full text of the exception
* in the file doc/FLOSS-exception.txt in this software distribution, or
* online at
* http://secondlifegrid.net/programs/open_source/licensing/flossexception
* 
* By copying, modifying or distributing this software, you acknowledge
* that you have read and understood your obligations described above,
* and agree to abide by those obligations.
* 
* ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
* WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
* COMPLETENESS OR PERFORMANCE.
* $/LicenseInfo$
*/

#include "llviewerprecompiledheaders.h" // must be first include
#include "llbottomtray.h"
#include "llagent.h"
#include "llchiclet.h"
#include "lllayoutstack.h"
#include "llkeyboard.h"
#include "llgesturemgr.h"
#include "llanimationstates.h"

//FIXME: temporary, for send_chat_from_viewer() proto
#include "llchatbar.h"

LLBottomTray::LLBottomTray()
	:mLastSpecialChatChannel(0)
{
	LLUICtrlFactory::getInstance()->buildPanel(this,"panel_bottomtray.xml");

	mChicletPanel = getChild<LLChicletPanel>("chiclet_list",TRUE,FALSE);

	LLLineEditor* chat_box = getChatBox();
	chat_box->setCommitCallback(boost::bind(&LLBottomTray::onChatBoxCommit, this));
	chat_box->setKeystrokeCallback(&onChatBoxKeystroke, this);
	chat_box->setFocusLostCallback(&onChatBoxFocusLost, this);

	LLIMMgr::getInstance()->addSessionObserver(this);
}

LLBottomTray::~LLBottomTray()
{
	if (!LLSingleton<LLIMMgr>::destroyed())
	{
		LLIMMgr::getInstance()->removeSessionObserver(this);
	}
}

LLLineEditor* LLBottomTray::getChatBox()
{
	return getChild<LLLineEditor>("chat_box",TRUE,FALSE);
}

void LLBottomTray::onChatBoxCommit()
{
	if (getChatBox()->getText().length() > 0)
	{
		sendChat(CHAT_TYPE_NORMAL);
		
		LLLineEditor* chat_box = getChatBox();

		if (chat_box)
		{
			chat_box->setText(LLStringExplicit(""));			
		}

		gAgent.stopTyping();
	}
}

void LLBottomTray::sendChatFromViewer(const std::string &utf8text, EChatType type, BOOL animate)
{
	sendChatFromViewer(utf8str_to_wstring(utf8text), type, animate);
}

void LLBottomTray::sendChatFromViewer(const LLWString &wtext, EChatType type, BOOL animate)
{
	// Look for "/20 foo" channel chats.
	S32 channel = 0;
	LLWString out_text = stripChannelNumber(wtext, &channel);
	std::string utf8_out_text = wstring_to_utf8str(out_text);
	std::string utf8_text = wstring_to_utf8str(wtext);

	utf8_text = utf8str_trim(utf8_text);
	if (!utf8_text.empty())
	{
		utf8_text = utf8str_truncate(utf8_text, MAX_STRING - 1);
	}

	// Don't animate for chats people can't hear (chat to scripts)
	if (animate && (channel == 0))
	{
		if (type == CHAT_TYPE_WHISPER)
		{
			lldebugs << "You whisper " << utf8_text << llendl;
			gAgent.sendAnimationRequest(ANIM_AGENT_WHISPER, ANIM_REQUEST_START);
		}
		else if (type == CHAT_TYPE_NORMAL)
		{
			lldebugs << "You say " << utf8_text << llendl;
			gAgent.sendAnimationRequest(ANIM_AGENT_TALK, ANIM_REQUEST_START);
		}
		else if (type == CHAT_TYPE_SHOUT)
		{
			lldebugs << "You shout " << utf8_text << llendl;
			gAgent.sendAnimationRequest(ANIM_AGENT_SHOUT, ANIM_REQUEST_START);
		}
		else
		{
			llinfos << "send_chat_from_viewer() - invalid volume" << llendl;
			return;
		}
	}
	else
	{
		if (type != CHAT_TYPE_START && type != CHAT_TYPE_STOP)
		{
			lldebugs << "Channel chat: " << utf8_text << llendl;
		}
	}

	send_chat_from_viewer(utf8_out_text, type, channel);
}

// static
void LLBottomTray::onChatBoxKeystroke(LLLineEditor* caller, void* userdata)
{
	LLBottomTray* self = (LLBottomTray *)userdata;

	LLWString raw_text;
	if (self->getChatBox()) raw_text = self->getChatBox()->getWText();

	// Can't trim the end, because that will cause autocompletion
	// to eat trailing spaces that might be part of a gesture.
	LLWStringUtil::trimHead(raw_text);

	S32 length = raw_text.length();

	if( (length > 0) && (raw_text[0] != '/') )  // forward slash is used for escape (eg. emote) sequences
	{
		gAgent.startTyping();
	}
	else
	{
		gAgent.stopTyping();
	}

	/* Doesn't work -- can't tell the difference between a backspace
	   that killed the selection vs. backspace at the end of line.
	if (length > 1 
		&& text[0] == '/'
		&& key == KEY_BACKSPACE)
	{
		// the selection will already be deleted, but we need to trim
		// off the character before
		std::string new_text = raw_text.substr(0, length-1);
		self->mInputEditor->setText( new_text );
		self->mInputEditor->setCursorToEnd();
		length = length - 1;
	}
	*/

	KEY key = gKeyboard->currentKey();

	// Ignore "special" keys, like backspace, arrows, etc.
	if (length > 1 
		&& raw_text[0] == '/'
		&& key < KEY_SPECIAL)
	{
		// we're starting a gesture, attempt to autocomplete

		std::string utf8_trigger = wstring_to_utf8str(raw_text);
		std::string utf8_out_str(utf8_trigger);

		if (gGestureManager.matchPrefix(utf8_trigger, &utf8_out_str))
		{
			if (self->getChatBox())
			{
				std::string rest_of_match = utf8_out_str.substr(utf8_trigger.size());
				self->getChatBox()->setText(utf8_trigger + rest_of_match); // keep original capitalization for user-entered part
				S32 outlength = self->getChatBox()->getLength(); // in characters
			
				// Select to end of line, starting from the character
				// after the last one the user typed.
				self->getChatBox()->setSelection(length, outlength);
			}
		}

		//llinfos << "GESTUREDEBUG " << trigger 
		//	<< " len " << length
		//	<< " outlen " << out_str.getLength()
		//	<< llendl;
	}
}

// static
void LLBottomTray::onChatBoxFocusLost(LLFocusableElement* caller, void* userdata)
{
	// stop typing animation
	gAgent.stopTyping();
}


//virtual
void LLBottomTray::sessionAdded(const LLUUID& session_id, const std::string& name, const LLUUID& other_participant_id)
{
	if(getChicletPanel())
	{
		LLSD sid(session_id);

		if(getChicletPanel()->findIMChiclet(&sid))
		{

		}
		else
		{
			LLIMChiclet* chicklet = (LLIMChiclet *)getChicletPanel()->createChiclet(&sid);
			chicklet->setIMSessionName(name);
			chicklet->setOtherParticipantId(other_participant_id);

			getChicletPanel()->arrange();
		}
	}
}

//virtual
void LLBottomTray::sessionRemoved(const LLUUID& session_id)
{
	if(getChicletPanel())
	{
		LLSD sid(session_id);
		getChicletPanel()->removeIMChiclet(&sid);
		getChicletPanel()->arrange();
	}
}

void LLBottomTray::sendChat( EChatType type )
{
	LLLineEditor* chat_box = getChatBox();

	if (chat_box)
	{
		LLWString text = chat_box->getConvertedText();
		if (!text.empty())
		{
			// store sent line in history, duplicates will get filtered
			chat_box->updateHistory();
			// Check if this is destined for another channel
			S32 channel = 0;
			stripChannelNumber(text, &channel);
			
			std::string utf8text = wstring_to_utf8str(text);
			// Try to trigger a gesture, if not chat to a script.
			std::string utf8_revised_text;
			if (0 == channel)
			{
				// discard returned "found" boolean
				gGestureManager.triggerAndReviseString(utf8text, &utf8_revised_text);
			}
			else
			{
				utf8_revised_text = utf8text;
			}

			utf8_revised_text = utf8str_trim(utf8_revised_text);

			if (!utf8_revised_text.empty())
			{
				// Chat with animation
				sendChatFromViewer(utf8_revised_text, type, TRUE);
			}
		}
	}

	gAgent.stopTyping();
}

// If input of the form "/20foo" or "/20 foo", returns "foo" and channel 20.
// Otherwise returns input and channel 0.
LLWString LLBottomTray::stripChannelNumber(const LLWString &mesg, S32* channel)
{
	if (mesg[0] == '/'
		&& mesg[1] == '/')
	{
		// This is a "repeat channel send"
		*channel = mLastSpecialChatChannel;
		return mesg.substr(2, mesg.length() - 2);
	}
	else if (mesg[0] == '/'
			 && mesg[1]
			 && LLStringOps::isDigit(mesg[1]))
	{
		// This a special "/20" speak on a channel
		S32 pos = 0;

		// Copy the channel number into a string
		LLWString channel_string;
		llwchar c;
		do
		{
			c = mesg[pos+1];
			channel_string.push_back(c);
			pos++;
		}
		while(c && pos < 64 && LLStringOps::isDigit(c));
		
		// Move the pointer forward to the first non-whitespace char
		// Check isspace before looping, so we can handle "/33foo"
		// as well as "/33 foo"
		while(c && iswspace(c))
		{
			c = mesg[pos+1];
			pos++;
		}
		
		mLastSpecialChatChannel = strtol(wstring_to_utf8str(channel_string).c_str(), NULL, 10);
		*channel = mLastSpecialChatChannel;
		return mesg.substr(pos, mesg.length() - pos);
	}
	else
	{
		// This is normal chat.
		*channel = 0;
		return mesg;
	}
}
