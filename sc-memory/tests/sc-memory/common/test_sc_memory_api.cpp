#include <gtest/gtest.h>

#include "sc-memory/sc_memory.hpp"
#include <algorithm>

#include "sc_test.hpp"

TEST_F(ScMemoryTest, ScMemory)
{
  EXPECT_TRUE(ScMemory::IsInitialized());
  EXPECT_TRUE(m_ctx->IsValid());
}

TEST_F(ScMemoryTest, MoveContext)
{
  ScMemoryContext context1;
  ScMemoryContext context2 = ScMemoryContext();
  EXPECT_TRUE(context2.IsValid());

  context1 = std::move(context2);

  EXPECT_TRUE(context1.IsValid());
  EXPECT_FALSE(context2.IsValid());

  context1 = std::move(context1);

  EXPECT_TRUE(context1.IsValid());
  EXPECT_FALSE(context2.IsValid());
}

TEST_F(ScMemoryTest, CreateElements)
{
  ScAddr const & nodeAddr = m_ctx->GenerateNode(ScType::NodeConst);
  EXPECT_TRUE(nodeAddr.IsValid());
  EXPECT_TRUE(m_ctx->IsElement(nodeAddr));

  ScAddr const & linkAddr = m_ctx->GenerateLink(ScType::LinkConst);
  EXPECT_TRUE(linkAddr.IsValid());
  EXPECT_TRUE(m_ctx->IsElement(linkAddr));

  ScAddr const & edgeAddr = m_ctx->GenerateConnector(ScType::EdgeAccess, nodeAddr, linkAddr);
  EXPECT_TRUE(edgeAddr.IsValid());
  EXPECT_TRUE(m_ctx->IsElement(edgeAddr));
}

TEST_F(ScMemoryTest, CreateElementsWithInvalidTypes)
{
  EXPECT_THROW(m_ctx->GenerateNode(ScType::EdgeAccess), utils::ExceptionInvalidParams);
  EXPECT_THROW(m_ctx->GenerateLink(ScType::NodeConst), utils::ExceptionInvalidParams);

  ScAddr const & nodeAddr = m_ctx->GenerateNode(ScType::NodeConst);
  EXPECT_TRUE(nodeAddr.IsValid());
  EXPECT_TRUE(m_ctx->IsElement(nodeAddr));

  ScAddr const & linkAddr = m_ctx->GenerateLink(ScType::LinkConst);
  EXPECT_TRUE(linkAddr.IsValid());
  EXPECT_TRUE(m_ctx->IsElement(linkAddr));

  EXPECT_THROW(m_ctx->GenerateConnector(ScType::NodeConst, nodeAddr, linkAddr), utils::ExceptionInvalidParams);
}

TEST_F(ScMemoryTest, SetGetFindSystemIdentifier)
{
  ScAddr const & addr = m_ctx->GenerateNode(ScType::NodeConst);

  EXPECT_TRUE(m_ctx->SetElementSystemIdentifier("test_node", addr));
  EXPECT_EQ(m_ctx->GetElementSystemIdentifier(addr), "test_node");
  EXPECT_EQ(m_ctx->SearchElementBySystemIdentifier("test_node"), addr);
}

TEST_F(ScMemoryTest, SetGetFindSystemIdentifierWithOutFiver)
{
  ScAddr const & addr = m_ctx->GenerateNode(ScType::NodeConst);

  ScSystemIdentifierQuintuple setFiver;
  EXPECT_TRUE(m_ctx->SetElementSystemIdentifier("test_node", addr, setFiver));
  EXPECT_EQ(m_ctx->GetElementSystemIdentifier(addr), "test_node");

  ScSystemIdentifierQuintuple quintuple;
  EXPECT_TRUE(m_ctx->SearchElementBySystemIdentifier("test_node", quintuple));

  EXPECT_EQ(setFiver.addr1, quintuple.addr1);
  EXPECT_EQ(setFiver.addr2, quintuple.addr2);
  EXPECT_EQ(setFiver.addr3, quintuple.addr3);
  EXPECT_EQ(setFiver.addr4, quintuple.addr4);
  EXPECT_EQ(setFiver.addr5, quintuple.addr5);
  EXPECT_TRUE(setFiver.addr1.IsValid());
  EXPECT_TRUE(setFiver.addr2.IsValid());
  EXPECT_TRUE(setFiver.addr3.IsValid());
  EXPECT_TRUE(setFiver.addr4.IsValid());
  EXPECT_TRUE(setFiver.addr5.IsValid());
}

TEST_F(ScMemoryTest, SetGetSystemIdentifierErrorSetTwice)
{
  ScAddr const & addr = m_ctx->GenerateNode(ScType::NodeConst);

  EXPECT_TRUE(m_ctx->SetElementSystemIdentifier("test_node", addr));
  EXPECT_EQ(m_ctx->GetElementSystemIdentifier(addr), "test_node");
  EXPECT_EQ(m_ctx->SearchElementBySystemIdentifier("test_node"), addr);

  ScAddr const & otherAddr = m_ctx->GenerateNode(ScType::NodeConst);
  EXPECT_FALSE(m_ctx->SetElementSystemIdentifier("test_node", otherAddr));
}

TEST_F(ScMemoryTest, ResolveGetSystemIdentifier)
{
  ScAddr const & addr = m_ctx->ResolveElementSystemIdentifier("test_node", ScType::NodeConst);

  EXPECT_TRUE(addr.IsValid());
  EXPECT_EQ(m_ctx->GetElementSystemIdentifier(addr), "test_node");
  EXPECT_EQ(m_ctx->SearchElementBySystemIdentifier("test_node"), addr);
}

TEST_F(ScMemoryTest, ResolveGetSystemIdentifierWithOutFiver)
{
  ScSystemIdentifierQuintuple resolveQuintuple;
  EXPECT_TRUE(m_ctx->ResolveElementSystemIdentifier("test_node", ScType::NodeConst, resolveQuintuple));

  EXPECT_EQ(m_ctx->GetElementSystemIdentifier(resolveQuintuple.addr1), "test_node");
  EXPECT_EQ(m_ctx->SearchElementBySystemIdentifier("test_node"), resolveQuintuple.addr1);

  ScSystemIdentifierQuintuple foundQuintuple;
  EXPECT_TRUE(m_ctx->SearchElementBySystemIdentifier("test_node", foundQuintuple));

  ScAddr addr;
  EXPECT_FALSE(m_ctx->SearchElementBySystemIdentifier("test_node1", addr));

  EXPECT_EQ(resolveQuintuple.addr1, foundQuintuple.addr1);
  EXPECT_EQ(resolveQuintuple.addr2, foundQuintuple.addr2);
  EXPECT_EQ(resolveQuintuple.addr3, foundQuintuple.addr3);
  EXPECT_EQ(resolveQuintuple.addr4, foundQuintuple.addr4);
  EXPECT_EQ(resolveQuintuple.addr5, foundQuintuple.addr5);
  EXPECT_TRUE(resolveQuintuple.addr1.IsValid());
  EXPECT_TRUE(resolveQuintuple.addr2.IsValid());
  EXPECT_TRUE(resolveQuintuple.addr3.IsValid());
  EXPECT_TRUE(resolveQuintuple.addr4.IsValid());
  EXPECT_TRUE(resolveQuintuple.addr5.IsValid());
}
