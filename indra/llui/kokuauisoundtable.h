/** 
 * @file kokuauisoundtable.h
 * @brief brief KOKUAUISoundTable class header file
 *
 * $LicenseInfo:firstyear=2011&license=viewerlgpl$
 * Kokua Viewer Source Code
 * Copyright (C) 2011, Armin.Weatherwax (at) googlemail.com
 * for the Kokua Viewer Team.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * $/LicenseInfo$
 */

#ifndef LL_KOKUAUISOUNDTABLE_H_
#define LL_KOKUAUISOUNDTABLE_H_

#include <map>

#include "llinitparam.h"
#include "llsingleton.h"
#include "kokuauisound.h"

class KOKUAUISound;

class KOKUAUISoundTable : public LLSingleton<KOKUAUISoundTable>
{
LOG_CLASS(KOKUAUISoundTable);

	// consider using sorted vector, can be much faster
	typedef std::map<std::string, KOKUAUISound>  string_sound_map_t;

public:
	struct SoundParams : LLInitParam::Choice<SoundParams>
	{
		Alternative<LLUUID>    sound_id;
// 		Alternative<std::string> file_name; //not supported yet
		Alternative<std::string> reference;

		SoundParams();
	};

	struct SoundEntryParams : LLInitParam::Block<SoundEntryParams>
	{
		Mandatory<std::string> name;
		Mandatory<std::string> description;
		Mandatory<SoundParams> sound;

		SoundEntryParams();
	};

	struct Params : LLInitParam::Block<Params>
	{
		Multiple<SoundEntryParams> sound_entries;

		Params();
	};

	// define sounds by passing in a param block that can be generated via XUI file or manually
	void insertFromParams(const Params& p);

	// reset all sounds to default magenta sound
	void clear();

	// sound lookup
	KOKUAUISound getUISound(const std::string& name, const LLUUID& default_sound = LLUUID::null) const;
	LLUUID getSoundID(const std::string& name, const LLUUID& default_sound = LLUUID::null) const;
	// if the sound is in the table, it's sound id is changed, otherwise it is added
	void setSound(const std::string& name, const LLUUID& sound);

	// returns true if sound_name exists in the table
	bool soundExists(const std::string& sound_name) const;

	// loads sounds from settings files
	bool loadFromSettings();

	// saves sounds specified by the user to the users skin directory
	void saveUserSettings() const;

private:
	bool loadFromFilename(const std::string& filename, string_sound_map_t& table);

	void insertFromParams(const Params& p, string_sound_map_t& table);
	
	void clearTable(string_sound_map_t& table);
	void setSound(const std::string& name, const LLUUID& sound, string_sound_map_t& table);

	string_sound_map_t mLoadedSounds;
	string_sound_map_t mUserSetSounds;
};

#endif // KOKUAUISOUNDTABLE_H
