/** 
 * @file kokuauisound.cpp
 * @brief brief KOKUAUISound class implementation file
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

#include "kokuauisound.h"

KOKUAUISound::KOKUAUISound()
	:mSoundPtr(NULL)
{
}


KOKUAUISound::KOKUAUISound(const LLUUID& sound)
:	mSound(sound), 
	mSoundPtr(NULL)
{
}

KOKUAUISound::KOKUAUISound(const KOKUAUISound* sound)
:	mSoundPtr(sound)
{
}

void KOKUAUISound::set(const LLUUID& sound)
{
	mSound = sound;
	mSoundPtr = NULL;
}

void KOKUAUISound::set(const KOKUAUISound* sound)
{
	mSoundPtr = sound;
}

const LLUUID& KOKUAUISound::get() const
{
	return (mSoundPtr == NULL ? mSound : mSoundPtr->get());
}

KOKUAUISound::operator const LLUUID& () const
{
	return get();
}

const LLUUID& KOKUAUISound::operator()() const
{
	return get();
}

bool KOKUAUISound::isReference() const
{
	return mSoundPtr != NULL;
}

namespace LLInitParam
{
	// used to detect equivalence with default values on export
	bool ParamCompare<KOKUAUISound, false>::equals(const KOKUAUISound &a, const KOKUAUISound &b)
	{
		// do not detect value equivalence, treat pointers to sounds as distinct from sound values
		return (a.mSoundPtr == NULL && b.mSoundPtr == NULL ? a.mSound == b.mSound : a.mSoundPtr == b.mSoundPtr);
	}
}
