/** 
* @file kokuauisoundtable.cpp
 * @brief brief KOKUAUISoundTable class implementation file
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

#include "linden_common.h"

#include <queue>

#include "lldir.h"
#include "llui.h"
#include "kokuauisoundtable.h"
#include "lluictrlfactory.h"

KOKUAUISoundTable::SoundParams::SoundParams()
:	sound_id("sound_id"),
// 	file_name("file_name"), //not supported yet
	reference("reference")
{
}

KOKUAUISoundTable::SoundEntryParams::SoundEntryParams()
:	name("name"),
	description("description"),
	sound("")
{
}

KOKUAUISoundTable::Params::Params()
:	sound_entries("sound")
{
}

void KOKUAUISoundTable::insertFromParams(const Params& p, string_sound_map_t& table)
{
	// this map will contain all sound references after the following loop
	typedef std::map<std::string, std::string> string_string_map_t;
	string_string_map_t unresolved_refs;

	for(LLInitParam::ParamIterator<SoundEntryParams>::const_iterator it = p.sound_entries.begin();
		it != p.sound_entries.end();
		++it)
	{
		SoundEntryParams sound_entry = *it;
		if(sound_entry.sound.sound_id.isChosen())
		{
			setSound(sound_entry.name, sound_entry.sound.sound_id, table);
		}
		else
		{
			unresolved_refs.insert(string_string_map_t::value_type(sound_entry.name, sound_entry.sound.reference));
		}
	}

	// maintain an in order queue of visited references for better debugging of cycles
	typedef std::queue<std::string> string_queue_t;
	string_queue_t ref_chain;

	// maintain a map of the previously visited references in the reference chain for detecting cycles
	typedef std::map<std::string, string_string_map_t::iterator> string_sound_ref_iter_map_t;
	string_sound_ref_iter_map_t visited_refs;

	// loop through the unresolved sound references until there are none left
	while(!unresolved_refs.empty())
	{
		// we haven't visited any references yet
		visited_refs.clear();

		string_string_map_t::iterator current = unresolved_refs.begin();
		string_string_map_t::iterator previous;

		while(true)
		{
			if(current != unresolved_refs.end())
			{
				// locate the current reference in the previously visited references...
				string_sound_ref_iter_map_t::iterator visited = visited_refs.lower_bound(current->first);
				if(visited != visited_refs.end()
			    && !(visited_refs.key_comp()(current->first, visited->first)))
				{
					// ...if we find the current reference in the previously visited references
					// we know that there is a cycle
					std::string ending_ref = current->first;
					std::string warning("The following sounds form a cycle: ");

					// warn about the references in the chain and remove them from
					// the unresolved references map because they cannot be resolved
					for(string_sound_ref_iter_map_t::iterator iter = visited_refs.begin();
						iter != visited_refs.end();
						++iter)
					{
						if(!ref_chain.empty())
						{
							warning += ref_chain.front() + "->";
							ref_chain.pop();
						}
						unresolved_refs.erase(iter->second);
					}

					llwarns << warning + ending_ref << llendl;

					break;
				}
				else
				{
					// ...continue along the reference chain
					ref_chain.push(current->first);
					visited_refs.insert(visited, string_sound_ref_iter_map_t::value_type(current->first, current));
				}
			}
			else
			{
				// since this reference does not refer to another reference it must refer to an
				// actual sound, lets find it...
				string_sound_map_t::iterator sound_sound_id = mLoadedSounds.find(previous->second);

				if(sound_sound_id != mLoadedSounds.end())
				{
					// ...we found the sound, and we now add every reference in the reference chain
					// to the sound map
					for(string_sound_ref_iter_map_t::iterator iter = visited_refs.begin();
						iter != visited_refs.end();
						++iter)
					{
						setSound(iter->first, sound_sound_id->second, mLoadedSounds);
						unresolved_refs.erase(iter->second);
					}

					break;
				}
				else
				{
					// ... we did not find the sound which imples that the current reference
					// references a non-existant sound
					for(string_sound_ref_iter_map_t::iterator iter = visited_refs.begin();
						iter != visited_refs.end();
						++iter)
					{
						llwarns << iter->first << " references a non-existent sound" << llendl;
						unresolved_refs.erase(iter->second);
					}

					break;
				}
			}

			// find the next sound reference in the reference chain
			previous = current;
			current = unresolved_refs.find(current->second);
		}
	}
}

void KOKUAUISoundTable::clear()
{
	clearTable(mLoadedSounds);
	clearTable(mUserSetSounds);
}

KOKUAUISound KOKUAUISoundTable::getUISound(const std::string& name, const LLUUID& default_sound) const
{
	string_sound_map_t::const_iterator iter = mUserSetSounds.find(name);
	
	if(iter != mUserSetSounds.end())
	{
		return KOKUAUISound(&iter->second);
	}

	iter = mLoadedSounds.find(name);
	
	if(iter != mLoadedSounds.end())
	{
		return KOKUAUISound(&iter->second);
	}
	
	return  KOKUAUISound(default_sound);
}

LLUUID KOKUAUISoundTable::getSoundID(const std::string& name, const LLUUID& default_sound) const
{
	KOKUAUISound ui_sound = getUISound(name, default_sound);
	LLUUID sound_id = ui_sound.get();
	return sound_id;
}

// update user sound, loaded sounds are parsed on initialization
void KOKUAUISoundTable::setSound(const std::string& name, const LLUUID& sound)
{
	setSound(name, sound, mUserSetSounds);
	setSound(name, sound, mLoadedSounds);
}

bool KOKUAUISoundTable::loadFromSettings()
{
	bool result = false;

	std::string default_filename = gDirUtilp->getExpandedFilename(LL_PATH_DEFAULT_SKIN, "sounds.xml");
	result |= loadFromFilename(default_filename, mLoadedSounds);

	std::string current_filename = gDirUtilp->getExpandedFilename(LL_PATH_TOP_SKIN, "sounds.xml");
	if(current_filename != default_filename)
	{
		result |= loadFromFilename(current_filename, mLoadedSounds);
	}

	std::string user_filename = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, "sounds.xml");
	loadFromFilename(user_filename, mUserSetSounds);

	return result;
}

void KOKUAUISoundTable::saveUserSettings() const
{
	Params params;

	for(string_sound_map_t::const_iterator it = mUserSetSounds.begin();
		it != mUserSetSounds.end();
		++it)
	{
		SoundEntryParams sound_entry;
		sound_entry.name = it->first;
		sound_entry.sound.sound_id = it->second;

		params.sound_entries.add(sound_entry);
	}

	LLXMLNodePtr output_node = new LLXMLNode("sounds", false);
	LLXUIParser parser;
	parser.writeXUI(output_node, params);

	if(!output_node->isNull())
	{
		const std::string& filename = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, "sounds.xml");
		LLFILE *fp = LLFile::fopen(filename, "w");

		if(fp != NULL)
		{
			LLXMLNode::writeHeaderToFile(fp);
			output_node->writeToFile(fp);

			fclose(fp);
		}
	}
}

bool KOKUAUISoundTable::soundExists(const std::string& sound_name) const
{
	return ((mLoadedSounds.find(sound_name) != mLoadedSounds.end())
		 || (mUserSetSounds.find(sound_name) != mUserSetSounds.end()));
}

void KOKUAUISoundTable::clearTable(string_sound_map_t& table)
{
	for(string_sound_map_t::iterator it = table.begin();
		it != table.end();
		++it)
	{
		it->second = LLUUID::null;
	}
}

// this method inserts a sound into the table if it does not exist
// if the sound already exists it changes the sound
void KOKUAUISoundTable::setSound(const std::string& name, const LLUUID& sound, string_sound_map_t& table)
{
	string_sound_map_t::iterator it = table.lower_bound(name);
	if(it != table.end()
	&& !(table.key_comp()(name, it->first)))
	{
		it->second = sound;
	}
	else
	{
		table.insert(it, string_sound_map_t::value_type(name, sound));
	}
}

bool KOKUAUISoundTable::loadFromFilename(const std::string& filename, string_sound_map_t& table)
{
	LLXMLNodePtr root;

	if(!LLXMLNode::parseFile(filename, root, NULL))
	{
		llwarns << "Unable to parse sound file " << filename << llendl;
		return false;
	}

	if(!root->hasName("sounds"))
	{
		llwarns << filename << " is not a valid sound definition file" << llendl;
		return false;
	}

	Params params;
	LLXUIParser parser;
	parser.readXUI(root, params, filename);

	if(params.validateBlock())
	{
		insertFromParams(params, table);
	}
	else
	{
		llwarns << filename << " failed to load" << llendl;
		return false;
	}

	return true;
}

void KOKUAUISoundTable::insertFromParams(const Params& p)
{
	insertFromParams(p, mUserSetSounds);
}

// EOF

