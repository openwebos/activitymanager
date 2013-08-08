// @@@LICENSE
//
//      Copyright (c) 2009-2013 LG Electronics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// LICENSE@@@

#include "MojoURL.h"

#include <stdexcept>

MojoURL::MojoURL()
	: m_valid(false)
{
}

MojoURL::MojoURL(const MojString& url)
{
	Init(url);
}

MojoURL::MojoURL(const char *data)
{
	MojString url;
	url.assign(data);
	Init(url);
}

MojoURL::MojoURL(const MojString& targetService, const MojString& method)
	: m_targetService(targetService)
	, m_method(method)
	, m_valid(true)
{
}

MojoURL::~MojoURL()
{
}

MojoURL& MojoURL::operator=(const char *data)
{
	MojString url;
	url.assign(data);
	Init(url);
	return *this;
}

MojoURL& MojoURL::operator=(const MojString& url)
{
	Init(url);
	return *this;
}

MojString MojoURL::GetTargetService() const
{
	if (!m_valid)
		throw std::runtime_error("URL not initialized");

	return m_targetService;
}

MojString MojoURL::GetMethod() const
{
	if (!m_valid)
		throw std::runtime_error("URL not initialized");

	return m_method;
}

MojString MojoURL::GetURL() const
{
	if (!m_valid)
		throw std::runtime_error("URL not initialized");

	std::string url = "palm://";
	url += m_targetService.data();
	url += "/";
	url += m_method.data();

	MojString result;
	result.assign(url.c_str());
	return result;
}

std::string MojoURL::GetString() const
{
	if (!m_valid)
		throw std::runtime_error("URL not initialized");

	std::string url = "palm://";
	url += m_targetService.data();
	url += "/";
	url += m_method.data();

	return url;
}

void MojoURL::Init(const MojString& url)
{
	/* There has to be something after "palm://" */
	if (url.length() <= 7)
		throw std::runtime_error("URL string too short to be valid.");

	/* Has to start with "palm://" or "luna://"*/
	if ( (url.compare("palm://", 7)) && (url.compare("luna://", 7)) )
		throw std::runtime_error("URL does not start with \"palm://\" or \"luna://\".");

	MojSize serviceEnd = url.find('/', 7);

	/* Must be a method present */
	if (serviceEnd == MojInvalidIndex)
		throw std::runtime_error("URL does not specify a method.");

	/* The method must be non-zero length */
	if (serviceEnd == url.length())
		throw std::runtime_error("URL specifies a zero length method.");

	url.substring(7, serviceEnd - 7, m_targetService);
	url.substring(serviceEnd + 1, url.length() - serviceEnd - 1, m_method);

	m_valid = true;
}
