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

SC_PRAGMA_DISABLE_DEPRECATION_WARNINGS_BEGIN

TEST_F(ScMemoryTest, CreateNode_Deprecated)
{
  ScAddr const & nodeAddr = m_ctx->CreateNode(ScType::NodeConst);
  EXPECT_TRUE(nodeAddr.IsValid());
  EXPECT_TRUE(m_ctx->IsElement(nodeAddr));

  EXPECT_TRUE(m_ctx->EraseElement(nodeAddr));
}

TEST_F(ScMemoryTest, CreateLink_Deprecated)
{
  ScAddr const & linkAddr = m_ctx->CreateLink(ScType::LinkConst);
  EXPECT_TRUE(linkAddr.IsValid());
  EXPECT_TRUE(m_ctx->IsElement(linkAddr));

  EXPECT_TRUE(m_ctx->EraseElement(linkAddr));
}

TEST_F(ScMemoryTest, CreateEdge_Deprecated)
{
  ScAddr const & sourceNodeAddr = m_ctx->GenerateNode(ScType::NodeConst);
  ScAddr const & targetNodeAddr = m_ctx->GenerateNode(ScType::NodeConst);

  ScAddr const & edgeAddr = m_ctx->CreateEdge(ScType::EdgeAccessConstPosPerm, sourceNodeAddr, targetNodeAddr);
  EXPECT_TRUE(edgeAddr.IsValid());
  EXPECT_TRUE(m_ctx->IsElement(edgeAddr));

  EXPECT_TRUE(m_ctx->EraseElement(edgeAddr));
}

TEST_F(ScMemoryTest, GetElementOutputArcsCount_Deprecated)
{
  ScAddr const & elementAddr = m_ctx->GenerateNode(ScType::NodeConst);
  EXPECT_TRUE(elementAddr.IsValid());

  size_t outputArcsCount = m_ctx->GetElementOutputArcsCount(elementAddr);
  EXPECT_EQ(outputArcsCount, 0u);
}

TEST_F(ScMemoryTest, GetElementInputArcsCount_Deprecated)
{
  ScAddr const & elementAddr = m_ctx->GenerateNode(ScType::NodeConst);
  EXPECT_TRUE(elementAddr.IsValid());

  size_t inputArcsCount = m_ctx->GetElementInputArcsCount(elementAddr);
  EXPECT_EQ(inputArcsCount, 0u);
}

TEST_F(ScMemoryTest, GetEdgeSource_Deprecated)
{
  ScAddr const & sourceNodeAddr = m_ctx->GenerateNode(ScType::NodeConst);
  ScAddr const & targetNodeAddr = m_ctx->GenerateNode(ScType::NodeConst);

  ScAddr const & connectorAddr =
      m_ctx->GenerateConnector(ScType::EdgeAccessConstPosPerm, sourceNodeAddr, targetNodeAddr);

  ScAddr const & retrievedSourceAddr = m_ctx->GetEdgeSource(connectorAddr);
  EXPECT_EQ(retrievedSourceAddr, sourceNodeAddr);
}

TEST_F(ScMemoryTest, GetEdgeTarget_Deprecated)
{
  ScAddr const & sourceNodeAddr = m_ctx->GenerateNode(ScType::NodeConst);
  ScAddr const & targetNodeAddr = m_ctx->GenerateNode(ScType::NodeConst);

  ScAddr const & connectorAddr =
      m_ctx->GenerateConnector(ScType::EdgeAccessConstPosPerm, sourceNodeAddr, targetNodeAddr);

  ScAddr const & retrievedTargetAddr = m_ctx->GetEdgeTarget(connectorAddr);
  EXPECT_EQ(retrievedTargetAddr, targetNodeAddr);
}

TEST_F(ScMemoryTest, GetEdgeInfo_Deprecated)
{
  ScAddr const & sourceNodeAddr = m_ctx->GenerateNode(ScType::NodeConst);
  ScAddr const & targetNodeAddr = m_ctx->GenerateNode(ScType::NodeConst);

  ScAddr const & edgeAddr = m_ctx->CreateEdge(ScType::EdgeAccessConstPosPerm, sourceNodeAddr, targetNodeAddr);
  EXPECT_TRUE(edgeAddr.IsValid());
  EXPECT_TRUE(m_ctx->IsElement(edgeAddr));

  ScAddr retrievedSourceAddr;
  ScAddr retrievedTargetAddr;
  m_ctx->GetEdgeInfo(edgeAddr, retrievedSourceAddr, retrievedTargetAddr);
  EXPECT_EQ(retrievedSourceAddr, sourceNodeAddr);
  EXPECT_EQ(retrievedTargetAddr, targetNodeAddr);
}

TEST_F(ScMemoryTest, FindLinksByContent_Deprecated)
{
  ScAddr const & linkAddr1 = m_ctx->GenerateLink(ScType::LinkConst);
  m_ctx->SetLinkContent(linkAddr1, "content");
  ScAddr const & linkAddr2 = m_ctx->GenerateLink(ScType::LinkConst);
  m_ctx->SetLinkContent(linkAddr2, "content");
  ScAddr const & linkAddr3 = m_ctx->GenerateLink(ScType::LinkConst);
  m_ctx->SetLinkContent(linkAddr3, "content");

  ScAddrVector const & foundLinks = m_ctx->FindLinksByContent("content");

  EXPECT_TRUE(std::find(foundLinks.cbegin(), foundLinks.cend(), linkAddr1) != foundLinks.cend());
  EXPECT_TRUE(std::find(foundLinks.cbegin(), foundLinks.cend(), linkAddr2) != foundLinks.cend());
  EXPECT_TRUE(std::find(foundLinks.cbegin(), foundLinks.cend(), linkAddr3) != foundLinks.cend());
}

TEST_F(ScMemoryTest, FindLinksByContentSubstring_Deprecated)
{
  ScAddr const & linkAddr1 = m_ctx->GenerateLink(ScType::LinkConst);
  m_ctx->SetLinkContent(linkAddr1, "Hello, world!");

  ScAddr const & linkAddr2 = m_ctx->GenerateLink(ScType::LinkConst);
  m_ctx->SetLinkContent(linkAddr2, "This is a test link.");

  ScAddr const & linkAddr3 = m_ctx->GenerateLink(ScType::LinkConst);
  m_ctx->SetLinkContent(linkAddr3, "Another link with different content.");

  ScAddrVector const & foundLinks = m_ctx->FindLinksByContentSubstring("link");

  EXPECT_TRUE(std::find(foundLinks.cbegin(), foundLinks.cend(), linkAddr1) == foundLinks.cend());
  EXPECT_TRUE(std::find(foundLinks.cbegin(), foundLinks.cend(), linkAddr2) != foundLinks.cend());
  EXPECT_TRUE(std::find(foundLinks.cbegin(), foundLinks.cend(), linkAddr3) != foundLinks.cend());
}

TEST_F(ScMemoryTest, FindLinksContentsByContentSubstring_Deprecated)
{
  ScAddr const & linkAddr1 = m_ctx->GenerateLink(ScType::LinkConst);
  m_ctx->SetLinkContent(linkAddr1, "Hello, world!");

  ScAddr const & linkAddr2 = m_ctx->GenerateLink(ScType::LinkConst);
  m_ctx->SetLinkContent(linkAddr2, "This is a test link.");

  ScAddr const & linkAddr3 = m_ctx->GenerateLink(ScType::LinkConst);
  m_ctx->SetLinkContent(linkAddr3, "Another link with different content.");

  std::vector<std::string> const & foundLinkContents = m_ctx->FindLinksContentsByContentSubstring("link");

  EXPECT_TRUE(
      std::find(foundLinkContents.cbegin(), foundLinkContents.cend(), "Hello, world!") == foundLinkContents.cend());
  EXPECT_TRUE(
      std::find(foundLinkContents.cbegin(), foundLinkContents.cend(), "This is a test link.")
      != foundLinkContents.cend());
  EXPECT_TRUE(
      std::find(foundLinkContents.cbegin(), foundLinkContents.cend(), "Another link with different content.")
      != foundLinkContents.cend());
}

TEST_F(ScMemoryTest, HelperCheckEdge_Deprecated)
{
  ScAddr const & nodeAddr1 = m_ctx->GenerateNode(ScType::NodeConst);
  ScAddr const & nodeAddr2 = m_ctx->GenerateNode(ScType::NodeConst);
  ScType const & arcType = ScType::EdgeDCommonConst;

  m_ctx->GenerateConnector(arcType, nodeAddr1, nodeAddr2);

  EXPECT_TRUE(m_ctx->HelperCheckEdge(nodeAddr1, nodeAddr2, arcType));
}

TEST_F(ScMemoryTest, HelperResolveSystemIdtf_Deprecated)
{
  std::string systemIdentifier = "example_identifier";
  ScAddr const & nodeAddr = m_ctx->GenerateNode(ScType::NodeConst);
  m_ctx->SetElementSystemIdentifier(systemIdentifier, nodeAddr);

  ScAddr resolvedAddr = m_ctx->HelperResolveSystemIdtf(systemIdentifier, ScType::NodeConstClass);
  EXPECT_EQ(resolvedAddr, nodeAddr);
}

TEST_F(ScMemoryTest, HelperFindBySystemIdtf_Deprecated)
{
  std::string systemIdentifier = "example_identifier";
  ScAddr const & nodeAddr = m_ctx->GenerateNode(ScType::NodeConst);
  m_ctx->SetElementSystemIdentifier(systemIdentifier, nodeAddr);

  ScAddr foundAddr;
  EXPECT_TRUE(m_ctx->HelperFindBySystemIdtf(systemIdentifier, foundAddr));
  EXPECT_EQ(foundAddr, nodeAddr);
}

TEST_F(ScMemoryTest, HelperGetSystemIdtf_Deprecated)
{
  ScAddr const & nodeAddr = m_ctx->GenerateNode(ScType::NodeConst);
  std::string systemIdentifier = "example_identifier";
  m_ctx->SetElementSystemIdentifier(systemIdentifier, nodeAddr);

  EXPECT_EQ(m_ctx->HelperGetSystemIdtf(nodeAddr), systemIdentifier);
}

TEST_F(ScMemoryTest, HelperGenTemplate_Deprecated)
{
  ScAddr const & classAddr = m_ctx->ResolveElementSystemIdentifier("my_class", ScType::NodeConst);

  ScTemplate templateToGenerate;
  templateToGenerate.Triple(classAddr, ScType::EdgeAccessVarPosPerm >> "_edge", ScType::NodeVar >> "_addr2");

  ScTemplateResultItem result;
  EXPECT_TRUE(m_ctx->HelperGenTemplate(templateToGenerate, result));
  EXPECT_TRUE(m_ctx->IsElement(result["_addr2"]));

  EXPECT_TRUE(m_ctx->CheckConnector(classAddr, result["_addr2"], ScType::EdgeAccessConstPosPerm));
}

TEST_F(ScMemoryTest, HelperSearchTemplate_Deprecated)
{
  ScAddr const & classAddr = m_ctx->ResolveElementSystemIdentifier("my_class", ScType::NodeConst);
  ScAddr const & setAddr = m_ctx->GenerateNode(ScType::NodeConst);

  ScTemplate templateToFind;
  templateToFind.Triple(classAddr, ScType::EdgeAccessVarPosPerm >> "_edge", ScType::NodeVar >> "_addr2");

  ScTemplateGenResult result;
  EXPECT_TRUE(m_ctx->HelperGenTemplate(templateToFind, result));

  m_ctx->HelperSearchTemplate(
      templateToFind,
      [&](ScTemplateSearchResultItem const & item)
      {
        m_ctx->GenerateConnector(ScType::EdgeAccessConstPosTemp, setAddr, item["_addr2"]);
      },
      [&](ScTemplateSearchResultItem const & item) -> bool
      {
        return true;
      },
      [&](ScAddr const & elementAddr) -> bool
      {
        return true;
      });

  EXPECT_TRUE(m_ctx->HelperCheckEdge(setAddr, result["_addr2"], ScType::EdgeAccessConstPosTemp));
}

TEST_F(ScMemoryTest, HelperSmartSearchTemplate_Deprecated)
{
  ScAddr const & classAddr = m_ctx->ResolveElementSystemIdentifier("my_class", ScType::NodeConst);
  ScAddr const & setAddr = m_ctx->GenerateNode(ScType::NodeConst);

  ScTemplate templateToFind;
  templateToFind.Triple(classAddr, ScType::EdgeAccessVarPosPerm >> "_edge", ScType::NodeVar >> "_addr2");

  ScTemplateGenResult result;
  EXPECT_TRUE(m_ctx->HelperGenTemplate(templateToFind, result));

  m_ctx->HelperSmartSearchTemplate(
      templateToFind,
      [&](ScTemplateSearchResultItem const & item) -> ScTemplateSearchRequest
      {
        m_ctx->GenerateConnector(ScType::EdgeAccessConstPosTemp, setAddr, item["_addr2"]);
        return ScTemplateSearchRequest::CONTINUE;
      },
      [&](ScTemplateSearchResultItem const & item) -> bool
      {
        return true;
      },
      [&](ScAddr const & elementAddr) -> bool
      {
        return true;
      });

  EXPECT_TRUE(m_ctx->HelperCheckEdge(setAddr, result["_addr2"], ScType::EdgeAccessConstPosTemp));
}

TEST_F(ScMemoryTest, HelperBuildTemplate_Deprecated)
{
  ScAddr const addr = m_ctx->GenerateNode(ScType::NodeConst);
  EXPECT_TRUE(addr.IsValid());
  EXPECT_TRUE(m_ctx->SetElementSystemIdentifier("d", addr));

  ScTemplate templ;
  sc_char const * data = "_a _-> d;; _a <- sc_node_class;;";
  m_ctx->BuildTemplate(templ, data);
}

class ScTemplateLoadContext : public ScMemoryContext
{
public:
  void LoadTemplate(
      ScTemplate & translatableTemplate,
      ScAddr & resultTemplateAddr,
      ScTemplateParams const & params = ScTemplateParams())
  {
    ScMemoryContext::LoadTemplate(translatableTemplate, resultTemplateAddr, params);
  }
};

TEST_F(ScMemoryTest, GenerateSearchLoadCheckBuildSearchTemplate_Deprecated)
{
  ScAddr const & testClassAddr = m_ctx->GenerateNode(ScType::NodeConstClass);
  ScAddr const & testRelationAddr = m_ctx->GenerateNode(ScType::NodeConstNoRole);

  ScTemplate templ;
  templ.Triple(testClassAddr, ScType::EdgeAccessVarPosPerm >> "_arc_to_test_object", ScType::LinkVar >> "_test_object");
  templ.Quintuple(
      "_test_object",
      ScType::EdgeDCommonVar,
      ScType::NodeVar >> "_test_set",
      ScType::EdgeAccessVarPosPerm,
      testRelationAddr);
  templ.Triple("_test_set", ScType::EdgeAccessVarPosPerm, "_arc_to_test_object");
  EXPECT_EQ(templ.Size(), 4u);

  ScTemplateGenResult genResult;
  m_ctx->HelperGenTemplate(templ, genResult);

  ScTemplateSearchResult searchResult;
  EXPECT_TRUE(m_ctx->HelperSearchTemplate(templ, searchResult));
  EXPECT_EQ(searchResult.Size(), 1u);

  ScAddr templAddr;
  ScTemplateLoadContext ctx;
  ctx.LoadTemplate(templ, templAddr);

  ScTemplate builtTemplate;
  m_ctx->HelperBuildTemplate(builtTemplate, templAddr);

  EXPECT_EQ(builtTemplate.Size(), 4u);

  EXPECT_TRUE(m_ctx->HelperSearchTemplate(builtTemplate, searchResult));
  EXPECT_EQ(searchResult.Size(), 1u);
}

TEST_F(ScMemoryTest, CalculateStat_Deprecated)
{
  EXPECT_NO_THROW(m_ctx->CalculateStat());
}

SC_PRAGMA_DISABLE_DEPRECATION_WARNINGS_END
