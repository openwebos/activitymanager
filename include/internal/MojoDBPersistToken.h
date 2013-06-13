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

#ifndef __ACTIVITYMANAGER_MOJODBPERSISTTOKEN_H__
#define __ACTIVITYMANAGER_MOJODBPERSISTTOKEN_H__

#include "Base.h"
#include "PersistToken.h"

#include <core/MojCoreDefs.h>

class MojoDBPersistToken : public PersistToken
{
public:
	MojoDBPersistToken();
	MojoDBPersistToken(const MojString& id, MojInt64 rev);
	virtual ~MojoDBPersistToken();

	virtual bool IsValid() const;

	void Set(const MojString& id, MojInt64 rev);
	void Update(const MojString& id, MojInt64 rev);
	void Clear();

	MojInt64 GetRev() const;
	const MojString& GetId() const;

	MojErr ToJson(MojObject& rep) const;

	std::string GetString() const;

protected:
	bool		m_valid;
	MojString	m_id;
	MojInt64	m_rev;
};

#endif /* __ACTIVITYMANAGER_MOJODBPERSISTTOKEN_H__ */
