/*
 * This source file is part of an OSTIS project. For the latest info, see http://ostis.net
 * Distributed under the MIT License
 * (See accompanying file COPYING.MIT or copy at http://opensource.org/licenses/MIT)
 */

#include "sc_structure.hpp"
#include "sc_memory.hpp"
#include "sc_template.hpp"

ScSet::ScSet(ScMemoryContext * context, ScAddr const & setAddr)
  : ScAddr(setAddr)
  , m_context(context)
{
}

ScSet::~ScSet() = default;

bool ScSet::Append(ScAddr const & elAddr)
{
  if (!HasElement(elAddr))
    return m_context->GenerateConnector(ScType::EdgeAccessConstPosPerm, *this, elAddr).IsValid();

  return false;
}

bool ScSet::Append(ScAddr const & elAddr, ScAddr const & attrAddr)
{
  if (HasElement(elAddr))
    return false;

  ScAddr const arcAddr = m_context->GenerateConnector(ScType::EdgeAccessConstPosPerm, *this, elAddr);
  m_context->GenerateConnector(ScType::EdgeAccessConstPosPerm, attrAddr, arcAddr);

  return true;
}

bool ScSet::Remove(ScAddr const & elAddr)
{
  bool found = false;
  ScIterator3Ptr iter = m_context->CreateIterator3(*this, ScType::EdgeAccessConstPosPerm, elAddr);
  while (iter->Next())
  {
    m_context->EraseElement(iter->Get(1));
    found = true;
  }

  return found;
}

bool ScSet::HasElement(ScAddr const & elAddr) const
{
  return m_context->CheckConnector(*this, elAddr, ScType::EdgeAccessConstPosPerm);
}

ScSet & ScSet::operator<<(ScAddr const & elAddr)
{
  Append(elAddr);
  return *this;
}

ScSet & ScSet::operator<<(ScTemplateResultItem const & res)
{
  size_t const res_num = res.Size();
  for (size_t i = 0; i < res_num; ++i)
    Append(res.m_replacementConstruction[i]);

  return *this;
}

ScSet & ScSet::operator>>(ScAddr const & elAddr)
{
  Remove(elAddr);
  return *this;
}

bool ScSet::IsEmpty() const
{
  ScIterator3Ptr const iter = m_context->CreateIterator3(*this, ScType::EdgeAccessConstPosPerm, ScType::Unknown);
  return !iter->Next();
}
