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

#include "trust-table.h"
#include "ns3/simulator.h"

namespace ns3 {

TrustTable::TrustTable ()
{
}

void TrustTable::AddRecord (TrustEntry entry)
{
//  m_tableRecords.push_back (entry);
}

void TrustTable::RemoveRecord (TrustEntry entry)
{
//  m_tableRecords.pop_back (); //need to change this
}

void TrustTable::UpdateRecord (TrustEntry entry)
{
  // TODO write update trust record logic
}

bool TrustTable::LookupTrustEntry (Address dst,
                                       TrustEntry & tt)
{
  if (m_tableRecords.empty ())
    {
      std::cout << "Trust entry to " << dst << " not found; trust table is empty";
      return false;
    }
  std::map<Address, TrustEntry>::const_iterator i = m_tableRecords.find (dst);
  if (i == m_tableRecords.end ())
    {
      std::cout << "Trust entry to " << dst << " not found";
      return false;
    }
  tt = i->second;
  std::cout << "Trust entry to " << dst << " found";
  return true;
}

void TrustTable::AddOrUpdateTrustTableEntry (Address dst,
                                                 double trustValue)
{
  std::map<Address, TrustEntry>::const_iterator i = m_tableRecords.find (dst);

  if (i == m_tableRecords.end ())
    {
      TrustEntry newTrustEntry;
      newTrustEntry.SetNeighbourAddress (dst);
      newTrustEntry.SetTrustValue (trustValue);
      newTrustEntry.SetTimestamp (Simulator::Now ());
      m_tableRecords[dst] = newTrustEntry;
    }
  else
    {
      TrustEntry trustEntry = i->second;
      trustEntry.SetTrustValue (trustValue);
      trustEntry.SetTimestamp (Simulator::Now ());
      m_tableRecords[dst] = trustEntry;
    }
}

TrustTable::~TrustTable ()
{
}

} // namespace ns3
