/* Copyright (C) 2010 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef INCLUDED_NETWORK_NETWORK
#define INCLUDED_NETWORK_NETWORK

#include "ps/Pyrogenesis.h"
#include "NetMessage.h"

class CErrorMessage : public CNetMessage
{
public:
	PS_RESULT m_Error;

	inline CErrorMessage() :
		CNetMessage(NMT_ERROR)
	{
	}

	inline CErrorMessage(PS_RESULT error) :
		CNetMessage(NMT_ERROR), m_Error(error)
	{
	}

	virtual CStr ToString() const;
};

struct CCloseRequestMessage : public CNetMessage
{
	inline CCloseRequestMessage() :
		CNetMessage(NMT_CLOSE_REQUEST)
	{
	}

	virtual CStr ToString() const;
};

struct CConnectCompleteMessage : public CNetMessage
{
	inline CConnectCompleteMessage() :
		CNetMessage(NMT_CONNECT_COMPLETE)
	{
	}

	virtual CStr ToString() const;
};

#endif
