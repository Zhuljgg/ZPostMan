#include<iostream>
#include "ZPostMan.h"


ZPostMan::ZPostMan()
{
	auto code = curl_global_init(CURL_GLOBAL_ALL);
	if (code) throw std::runtime_error("curl init failed,code:" + std::to_string(code));
	if (_handle = curl_easy_init())
	{
		curl_easy_setopt(_handle, CURLOPT_SSL_VERIFYPEER, false);
		curl_easy_setopt(_handle, CURLOPT_SSL_VERIFYHOST, false);
	}
}

ZPostMan::~ZPostMan()
{
	if (_handle)
	{
		curl_easy_cleanup(_handle);
		_handle = nullptr;
	}

	if (_header)
	{
		curl_slist_free_all(_header);
		_header = nullptr;
	}
	
	curl_global_cleanup();
}

void ZPostMan::set_proxy(const std::string& ip, unsigned short port)
{
	if (ip.empty()) throw std::runtime_error("bad proxy ip");
	if (_handle)
	{
		curl_easy_setopt(_handle, CURLOPT_PROXY, ip.c_str());
		curl_easy_setopt(_handle, CURLOPT_PROXYPORT, port);
		curl_easy_setopt(_handle, CURLOPT_PROXYTYPE, CURLPROXY_SOCKS5_HOSTNAME);
		curl_easy_setopt(_handle, CURLOPT_HTTPPROXYTUNNEL, 1l);
	}
}

size_t ZPostMan::get_headcode(const std::string& headcontent)
{
	/*HTTP/1.1 200*/
	constexpr size_t offset = sizeof("HTTP/1.1");
	if (headcontent.length() < offset) return 0;
	auto code = headcontent.substr(offset, headcontent.find(' ', offset) - offset);
	return std::atoi(code.c_str());
}

ZPostMan& ZPostMan::operator<<(const header_pair& head)
{
	_header = curl_slist_append(
		_header,
		std::string(head.first + ":" + head.second).c_str()
	);
	return *this;
}

ZPostMan& ZPostMan::operator>>(const std::string& head)
{
	if (_header)
	{
		if (head.empty())
		{
			curl_slist_free_all(_header);
			_header = nullptr;
			return *this;
		}
		auto cur = _header->next;
		auto prev = _header;
		while (cur)
		{
			if (strstr(cur->data, std::string(head + ":").c_str()))
			{
				prev->next = cur->next;
				break;
			}
			prev = cur, cur = cur->next;
		}
	}
	return *this;
}

bool ZPostMan::POST(const std::string& url, const std::string& args, on_response cb, void* userdata)
{
	CURLcode code;
	if (!_handle) throw std::runtime_error("bad handle");
	_resp_header.str("");//donot use .clear, it would not really free the buffer
	_resp_data.str("");
	__set_opts(url);
	curl_easy_setopt(_handle, CURLOPT_POSTFIELDSIZE, args.length());//long or size_t
	curl_easy_setopt(_handle, CURLOPT_POSTFIELDS, args.c_str());
	code = curl_easy_perform(_handle);
	if (CURLE_OK == code)
	{
		if (cb)
			cb(_resp_header.str(), _resp_data.str(), userdata);
		return true;
	}
	std::cout << "..." << code;
	return false;
}

bool ZPostMan::GET(const std::string& url_and_args, on_response cb, void* userdata)
{
	CURLcode code;
	if (!_handle) throw std::runtime_error("bad handle");
	_resp_header.str("");//donot use .clear, it would not really free the buffer
	_resp_data.str("");
	__set_opts(url_and_args);
	curl_easy_setopt(_handle, CURLOPT_TIMEOUT, 300);
	curl_easy_setopt(_handle, CURLOPT_CONNECTTIMEOUT, 120);
	code = curl_easy_perform(_handle);
	if (CURLE_OK == code)
	{
		if (cb)
			cb(_resp_header.str(), _resp_data.str(), userdata);
		return true;
	}
	return false;
}


size_t ZPostMan::__wcallback(char* ptr, size_t size, size_t num, void* userdata)
{
	size_t lsize = size * num;
	auto response = static_cast<std::stringstream*>(userdata);
	(*response).write(ptr, num);
	return lsize;
}

void ZPostMan::__set_opts(const std::string& url)
{
	if (_handle)
	{
		curl_easy_setopt(_handle, CURLOPT_URL, url.c_str());
		curl_easy_setopt(_handle, CURLOPT_HTTPHEADER, _header);
		curl_easy_setopt(_handle, CURLOPT_HEADERFUNCTION, __wcallback);
		curl_easy_setopt(_handle, CURLOPT_HEADERDATA, &_resp_header);
		curl_easy_setopt(_handle, CURLOPT_WRITEFUNCTION, __wcallback);
		curl_easy_setopt(_handle, CURLOPT_WRITEDATA, &_resp_data);
		curl_easy_setopt(_handle, CURLOPT_NOBODY, 0);
	}
}
