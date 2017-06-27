/*
	Aseba - an event-based framework for distributed robot control
	Copyright (C) 2007--2016:
		Stephane Magnenat <stephane at magnenat dot net>
		(http://stephane.magnenat.net)
		and other contributors, see authors.txt for details

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU Lesser General Public License as published
	by the Free Software Foundation, version 3 of the License.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public License
	along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include "../utils/utils.h"
#include "../utils/FormatableString.h"
#include "zeroconf-dashelhub.h"

using namespace std;

namespace Aseba
{
	using namespace Dashel;

	//! Constructor, taking a reference to a Dashel::Hub
	DashelhubZeroconf::DashelhubZeroconf(Hub& hub):
		hub(hub)
	{}

	//! Set up function called after a discovery request has been made. The file
	//! descriptor associated with zdr.serviceref must be watched, to know when to
	//! call DNSServiceProcessResult, which in turn calls the callback that was
	//! registered with the discovery request.
	void DashelhubZeroconf::processDiscoveryRequest(DiscoveryRequest & zdr)
	{
		int socket = DNSServiceRefSockFD(zdr.serviceRef);
		if (socket != -1)
		{
			string dashelTarget = FormatableString("tcppoll:sock=%0").arg(socket);
			if (auto stream = hub.connect(dashelTarget))
				zeroconfStreams.emplace(stream, ref(zdr));
		}
	}

	//! Check if data were coming on one of our streams, and if so process
	void DashelhubZeroconf::dashelIncomingData(Dashel::Stream * stream)
	{
		if (zeroconfStreams.find(stream) != zeroconfStreams.end())
		{
			const char status = stream->read<char>();
			auto serviceRef = zeroconfStreams.at(stream).get().serviceRef;
			DNSServiceErrorType err = DNSServiceProcessResult(serviceRef);
			if (err != kDNSServiceErr_NoError)
				throw Zeroconf::Error(FormatableString("DNSServiceProcessResult (service ref %1): error %0").arg(err).arg(serviceRef));
		}
	}

	//! Check if one of our streams was disconnected, and if so take note
	void DashelhubZeroconf::dashelConnectionClosed(Stream * stream)
	{
		zeroconfStreams.erase(stream);
	}

} // namespace Aseba
