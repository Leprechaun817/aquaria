#ifndef AQ_NETWORK_H
#define AQ_NETWORK_H

namespace Network
{
	class RequestData
	{
	public:
		RequestData() : fp(0), fail(false) {}
		std::string url;
		unsigned int port;
		std::string tempFilename; // file name to write to while downloading
		std::string finalFilename; // name under which the file should be stored when finished
		FILE *fp;
		bool fail;

		virtual void notify() = 0;
	};

	// Download a file described by rq.
	// Once the download is finished, rq's notify() method  will be called in the next update.
	void download(RequestData *rq);
	void update();
	void shutdown();
};



#endif
