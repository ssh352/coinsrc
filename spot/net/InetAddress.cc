// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/ftmd/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include <spot/net/InetAddress.h>
#include <spot/utility/Compatible.h>
#include <spot/utility/Logging.h>
#include <spot/net/Endian.h>
#include <spot/net/SocketsOps.h>
#include <assert.h>
#ifdef __LINUX__
// INADDR_ANY use (type)value casting.
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic error "-Wold-style-cast"
#endif
//     /* Structure describing an Internet socket address.  */
//     struct sockaddr_in {
//         sa_family_t    sin_family; /* address family: AF_INET */
//         uint16_t       sin_port;   /* port in network byte order */
//         struct in_addr sin_addr;   /* internet address */
//     };

//     /* Internet address. */
//     typedef uint32_t in_addr_t;
//     struct in_addr {
//         in_addr_t       s_addr;     /* address in network byte order */
//     };

//     struct sockaddr_in6 {
//         sa_family_t     sin6_family;   /* address family: AF_INET6 */
//         uint16_t        sin6_port;     /* port in network byte order */
//         uint32_t        sin6_flowinfo; /* IPv6 flow information */
//         struct in6_addr sin6_addr;     /* IPv6 address */
//         uint32_t        sin6_scope_id; /* IPv6 scope-id */
//     };

using namespace spot;
using namespace spot::net;

#ifndef __LINUX__
static_assert(sizeof(InetAddress) == sizeof(struct sockaddr_in), "InetAddress != sockaddr_in6");
static_assert(offsetof(sockaddr_in, sin_family) == 0, "sockaddr_in != sin_family");
static_assert(offsetof(sockaddr_in6, sin6_family) == 0, "sockaddr_in6 != sin6_family");
static_assert(offsetof(sockaddr_in, sin_port) == 2, "sockaddr_in sin_port !=2");
static_assert(offsetof(sockaddr_in6, sin6_port) == 2, "sockaddr_in6 sin6_port != 2");
//static_assert(offsetof(InetAddress, addr_) == 0, "InetAddress != addr_");
#endif

InetAddress::InetAddress(uint16_t port, bool loopbackOnly)
{
	//static_assert(offsetof(InetAddress, addr_) == 0, "InetAddress != addr_");
	memset(&addr_, 0, sizeof addr_);
	addr_.sin_family = AF_INET;
	addr_.sin_addr.s_addr = sockets::hostToNetwork32(INADDR_ANY);
	addr_.sin_port = sockets::hostToNetwork16(port);
}

InetAddress::InetAddress(StringArg ip, uint16_t port, bool ipv6)
{
	memset(&addr_, 0, sizeof addr_);
	sockets::fromIpPort(ip.c_str(), port, &addr_);
}

string InetAddress::toIpPort() const
{
	char buf[64] = "";
	sockets::toIpPort(buf, sizeof buf, getSockAddr());
	return buf;
}

string InetAddress::toIp() const
{
	char buf[64] = "";
	sockets::toIp(buf, sizeof buf, getSockAddr());
	return buf;
}

uint32_t InetAddress::ipNetEndian() const
{
	assert(family() == AF_INET);
	return addr_.sin_addr.s_addr;
}

uint16_t InetAddress::toPort() const
{
	return sockets::networkToHost16(portNetEndian());
}

static thread_local char t_resolveBuffer[64 * 1024];

bool InetAddress::resolve(StringArg hostname, InetAddress* out)
{
	assert(out != NULL);
	struct hostent hent;
	struct hostent* he = NULL;
	int herrno = 0;
	memset(&hent, 0, sizeof(hent));
	int ret = 0;
#ifdef EVENT__HAVE_GETHOSTBYNAME_R_6_ARG
	//thread safe
	ret = gethostbyname_r(hostname.c_str(), &hent, t_resolveBuffer, sizeof t_resolveBuffer, &he, &herrno);
#else
	he = gethostbyname(hostname.c_str());
	herrno = WSAGetLastError();
#endif
	if (ret == 0 && he != NULL)
	{
		assert(he->h_addrtype == AF_INET && he->h_length == sizeof(uint32_t));
		out->addr_.sin_addr = *reinterpret_cast<struct in_addr*>(he->h_addr);
		return true;
	}
	else
	{
		if (ret)
		{
			LOG_SYSERR << "InetAddress::resolve errno " << herrno;
			switch (herrno) {
			case TRY_AGAIN:
				LOG_SYSERR << "InetAddress::TRY_AGAIN ";
				break;
			case NO_RECOVERY:
				LOG_SYSERR << "InetAddress::NO_RECOVERY ";
				break;
			case HOST_NOT_FOUND:
				LOG_SYSERR << "InetAddress::HOST_NOT_FOUND ";
				break;
			case NO_DATA:
				LOG_SYSERR << "InetAddress::NO_DATA ";
				break;
			default:
				break;
			}
		}
		return false;
	}
}
