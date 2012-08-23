// @@@LICENSE
//
//      Copyright (c) 2009-2012 Hewlett-Packard Development Company, L.P.
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

#include "MojoObjectWrapper.h"

MojoObjectString::MojoObjectString(const MojObject& obj)
{
	MojErr err = obj.stringValue(m_str);

	/* Intentially ignore err here.  This is for debug output, let's not
	 * make the problem worse... */
	(void)err;
}

MojoObjectString::MojoObjectString(const MojString& str)
	: m_str(str)
{
}

MojoObjectString::MojoObjectString(const MojObject& obj, const MojChar *key)
{
	MojErr err;

	MojObject::ConstIterator found = obj.find(key);

	if (found != obj.end()) {
		err = found.value().stringValue(m_str);
		if (err) {
			m_str.assign("(error retreiving value)");
		}
	} else {
		m_str.assign("(not found)");
	}
}

MojoObjectString::~MojoObjectString()
{
}

std::string MojoObjectString::str() const
{
	return std::string(m_str.data());
}

const char *MojoObjectString::c_str() const
{
	return m_str.data();
}

MojoObjectJson::MojoObjectJson(const MojObject& obj)
{
	MojErr err = obj.toJson(m_str);

	/* Intentially ignore err here.  This is for debug output, let's not
	 * make the problem worse... */
	(void)err;
}

MojoObjectJson::~MojoObjectJson()
{
}

std::string MojoObjectJson::str() const
{
	return std::string(m_str.data());
}

const char *MojoObjectJson::c_str() const
{
	return m_str.data();
}

