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

#include "Subscriber.h"

Subscriber::Subscriber()
{
}

Subscriber::Subscriber(const std::string& id, BusIdType type)
	: m_id(id, type)
{
}

Subscriber::Subscriber(const char *id, BusIdType type)
	: m_id(id, type)
{
}

Subscriber::~Subscriber()
{
}

BusIdType Subscriber::GetType() const
{
	return m_id.GetType();
}

const BusId& Subscriber::GetId() const
{
	return m_id;
}

std::string Subscriber::GetString() const
{
	return m_id.GetString();
}

MojErr Subscriber::ToJson(MojObject& rep) const
{
	return m_id.ToJson(rep);
}

bool Subscriber::operator<(const Subscriber& rhs) const
{
	return m_id < rhs.m_id;
}

bool Subscriber::operator!=(const Subscriber& rhs) const
{
	return m_id != rhs.m_id;
}

bool Subscriber::operator==(const Subscriber& rhs) const
{
	return m_id == rhs.m_id;
}

bool Subscriber::operator!=(const BusId& rhs) const
{
	return m_id != rhs;
}

bool Subscriber::operator==(const BusId& rhs) const
{
	return m_id == rhs;
}

bool Subscriber::operator!=(const std::string& rhs) const
{
	return m_id != rhs;
}

bool Subscriber::operator==(const std::string& rhs) const
{
	return m_id == rhs;
}

