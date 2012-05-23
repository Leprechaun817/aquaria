#ifndef MODDOWNLOADER_H
#define MODDOWNLOADER_H

#include <string>
#include <set>
#include "Network.h"

class ModlistRequest;
class ModRequest;
class IconRequest;

class ModDL
{
public:
	ModDL();
	~ModDL();
	void init();

	void GetModlist(const std::string& url, bool allowChaining);
	void NotifyModlist(ModlistRequest *rq, Network::NetEvent ev, size_t recvd, size_t total);
	bool ParseModXML(const std::string& fn, bool allowChaining);

	void GetMod(const std::string& url, const std::string& localname);
	void NotifyMod(ModRequest *rq, Network::NetEvent ev, size_t recvd, size_t total);

	void GetIcon(const std::string& url, const std::string& localname);
	void NotifyIcon(IconRequest *rq, Network::NetEvent ev, size_t recvd, size_t total);


	std::string remoteToLocalName(const std::string& url);
	bool hasUrlFileCached(const std::string& url);


	std::set<std::string> knownServers; // FIXME: clear on re-init
	std::string tempDir;
};

extern ModDL moddl;


#endif
