/* @@@LICENSE
*
*      Copyright (c) 2009-2012 Hewlett-Packard Development Company, L.P.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
* LICENSE@@@ */

#ifndef __ACTIVITYMANAGER_MOJOURL_H__
#define __ACTIVITYMANAGER_MOJOURL_H__

#include <core/MojString.h>
#include <string>

class MojoURL {
public:
	MojoURL();
	MojoURL(const MojString& url);
	MojoURL(const char *data);
	MojoURL(const MojString& targetService, const MojString& method);

	virtual ~MojoURL();

	MojoURL& operator=(const char *url);
	MojoURL& operator=(const MojString& url);

	MojString	GetTargetService() const;
	MojString	GetMethod() const;
	MojString	GetURL() const;

	std::string	GetString() const;

protected:
	void Init(const MojString& url);

	MojString	m_targetService;
	MojString	m_method;

	bool		m_valid;
};

#endif /* __ACTIVITYMANAGER_MOJOURL_H__ */
