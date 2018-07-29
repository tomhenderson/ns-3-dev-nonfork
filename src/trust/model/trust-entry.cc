/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2018 Sri Lanka Institute of Information Technology
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Jude Niroshan <jude.niroshan11@gmail.com>
 */

#include "ns3/log.h"
#include "trust-entry.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("TrustEntry");

TrustEntry::TrustEntry ()
{
  NS_LOG_FUNCTION (this);
}

TrustEntry::~TrustEntry ()
{
  NS_LOG_FUNCTION (this);
}

Address TrustEntry::GetAddress (void) const
{
  NS_LOG_FUNCTION (this);
  return m_address;
}

void TrustEntry::SetAddress (Address address)
{
  m_address = address;
}

Time TrustEntry::GetTimestamp (void) const
{
  return m_timestamp;
}

void TrustEntry::SetTimestamp (Time timestamp)
{
  m_timestamp = timestamp;
}

double TrustEntry::GetTrustValue (void) const
{
  return m_trustValue;
}

void TrustEntry::SetTrustValue (double trustValue)
{
  m_trustValue = trustValue;
}

} // namespace ns3

