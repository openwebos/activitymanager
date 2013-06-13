/* @@@LICENSE
*
*      Copyright (c) 2009-2013 LG Electronics, Inc.
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

#ifndef __ACTIVITYMANAGER_MOJOOBJECTWRAPPER_H__
#define __ACTIVITYMANAGER_MOJOOBJECTWRAPPER_H__

#include <string>

#include <core/MojObject.h>

class MojoObjectString {
public:
	MojoObjectString(const MojObject& obj);
	MojoObjectString(const MojString& str);
	MojoObjectString(const MojObject& obj, const MojChar *key);

	virtual ~MojoObjectString();

	std::string	str() const;
	const char *c_str() const;

protected:
	MojString	m_str;
};

class MojoObjectJson {
public:
	MojoObjectJson(const MojObject& obj);
	virtual ~MojoObjectJson();

	std::string	str() const;
	const char *c_str() const;

protected:
	MojString	m_str;
};

#endif /* __ACTIVITYMANAGER_MOJOBJECTWRAPPER_H__ */
