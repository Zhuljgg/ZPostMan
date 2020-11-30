#pragma once

#include<sstream>
#include<functional>
#include"curl/curl.h"


#define	 USER_AGENT "Mozilla/5.0"

#define DEFAULT_HTTP_HEADERS \
	ZPostMan::header_pair{"Accept","*/*"}\
	<<ZPostMan::header_pair{"Accept-Language","zh-cn"}\
	<<ZPostMan::header_pair{"User-Agent",USER_AGENT}



class ZPostMan
{
protected:
	curl_slist* _header{ nullptr };
	CURL* _handle{ nullptr };
	std::stringstream _resp_header, _resp_data;
protected:
	static size_t __wcallback(char* ptr, size_t size, size_t num, void* userdata);
	void __set_opts(const std::string& url);
public:
	ZPostMan();
	~ZPostMan();
	using header_pair = std::pair<std::string, std::string>;
	using on_response = std::function<void(const std::string&, const std::string&, void*)>;
	void set_proxy(const std::string& ip, unsigned short port);
	static size_t get_headcode(const std::string& headcontent);

	ZPostMan& operator <<(const header_pair& head);
	ZPostMan& operator >>(const std::string& head);

	bool POST(const std::string& url, const std::string& args, on_response cb, void* userdata);
	bool GET(const std::string& urlandargs, on_response cb, void* userdata);
};

