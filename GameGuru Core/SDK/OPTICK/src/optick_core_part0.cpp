// The MIT License(MIT)
//
// Copyright(c) 2019 Vadim Slyusarev
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "optick_core.h"

#if USE_OPTICK

#include "optick.h"
#include "optick_server.h"

#include <algorithm>
#include <fstream>
#include <iomanip>

//////////////////////////////////////////////////////////////////////////
// Start of the Platform-specific stuff
//////////////////////////////////////////////////////////////////////////
#if defined(OPTICK_MSVC)
#include "optick_core.win.h"
#elif defined(OPTICK_LINUX)
#include "optick_core.linux.h"
#elif defined(OPTICK_OSX)
#include "optick_core.macos.h"
#elif defined(OPTICK_PS4)
#include "optick_core.ps4.h"
#elif defined(OPTICK_FREEBSD)
#include "optick_core.freebsd.h"
#endif
//////////////////////////////////////////////////////////////////////////
// End of the Platform-specific stuff
//////////////////////////////////////////////////////////////////////////

extern "C" Optick::EventData* NextEvent()
{
	if (Optick::EventStorage* storage = Optick::Core::storage)
	{
		return &storage->NextEvent();
	}

	return nullptr;
}

