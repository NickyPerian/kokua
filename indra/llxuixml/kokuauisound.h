/** 
 * @file kokuauisound.h
 * @brief brief KOKUAUISound class header file
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

#ifndef LL_KOKUAUISOUND_H
#define LL_KOKUAUISOUND_H

namespace LLInitParam
{
	template<typename T, bool>
	struct ParamCompare;
}

class KOKUAUISound
{
public:
	KOKUAUISound();
	KOKUAUISound(const LLUUID& sound);
	KOKUAUISound(const KOKUAUISound* sound);

	void set(const LLUUID& sound);
	void set(const KOKUAUISound* sound);

	const LLUUID& get() const;

	operator const LLUUID& () const;
	const LLUUID& operator()() const;

	bool isReference() const;

private:
	friend struct LLInitParam::ParamCompare<KOKUAUISound, false>;

	const KOKUAUISound* mSoundPtr;
	LLUUID mSound;
};

namespace LLInitParam
{
	template<>
	struct ParamCompare<KOKUAUISound, false>
	{
		static bool equals(const KOKUAUISound& a, const KOKUAUISound& b);
	};
}

#endif
