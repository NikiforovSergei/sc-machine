#include <gtest/gtest.h>

#include "sc-memory/sc_memory.hpp"
#include "sc-memory/sc_structure.hpp"

#include "template_test_utils.hpp"

using ScTemplateGenApiTest = ScTemplateTest;

TEST_F(ScTemplateGenApiTest, GenWithResultNotSafeGet)
{
  ScTemplate templ;
  templ.Triple(ScType::NodeVar >> "_addr1", ScType::EdgeAccessVarPosPerm >> "_arc", ScType::NodeVar >> "_addr2");

  ScTemplateGenResult result;
  m_ctx->GenerateByTemplate(templ, result);
  EXPECT_EQ(result.Size(), 3u);

  EXPECT_TRUE(result[0].IsValid());
  EXPECT_TRUE(result["_addr1"].IsValid());
  EXPECT_TRUE(result[1].IsValid());
  EXPECT_TRUE(result["_arc"].IsValid());
  EXPECT_TRUE(result[2].IsValid());
  EXPECT_TRUE(result["_addr2"].IsValid());

  EXPECT_THROW(result[3], utils::ExceptionInvalidParams);
  EXPECT_THROW(result["_other_arc"], utils::ExceptionInvalidParams);
}

TEST_F(ScTemplateGenApiTest, GenWithResultSafeGet)
{
  ScTemplate templ;
  templ.Triple(ScType::NodeVar >> "_addr1", ScType::EdgeAccessVarPosPerm >> "_arc", ScType::LinkVar >> "_addr2");

  ScTemplateGenResult result;
  m_ctx->GenerateByTemplate(templ, result);
  EXPECT_EQ(result.Size(), 3u);

  ScAddr foundAddr;
  EXPECT_TRUE(result.Get(0, foundAddr));
  EXPECT_TRUE(foundAddr.IsValid());
  EXPECT_TRUE(result.Get("_addr1", foundAddr));
  EXPECT_TRUE(foundAddr.IsValid());
  EXPECT_TRUE(result.Get(1, foundAddr));
  EXPECT_TRUE(foundAddr.IsValid());
  EXPECT_TRUE(result.Get("_arc", foundAddr));
  EXPECT_TRUE(foundAddr.IsValid());
  EXPECT_TRUE(result.Get(2, foundAddr));
  EXPECT_TRUE(foundAddr.IsValid());
  EXPECT_TRUE(result.Get("_addr2", foundAddr));
  EXPECT_TRUE(foundAddr.IsValid());

  EXPECT_FALSE(result.Get(3, foundAddr));
  EXPECT_FALSE(foundAddr.IsValid());
  EXPECT_FALSE(result.Get("_other_arc", foundAddr));
  EXPECT_FALSE(foundAddr.IsValid());
}

TEST_F(ScTemplateGenApiTest, GenTripleWithTargetEdge)
{
  ScTemplate templ;
  templ.Triple(
      ScType::NodeVar >> "_addr1", ScType::EdgeAccessVarPosPerm >> "_arc", ScType::EdgeAccessVarPosTemp >> "_addr2");

  ScTemplateGenResult result;
  EXPECT_THROW(m_ctx->GenerateByTemplate(templ, result), utils::ExceptionInvalidParams);
}

TEST_F(ScTemplateGenApiTest, GenTripleWithUnknownTargetElement)
{
  ScTemplate templ;
  templ.Triple(ScType::NodeVar >> "_addr1", ScType::EdgeAccessVarPosPerm >> "_arc", ScType::Unknown >> "_addr2");

  ScTemplateGenResult result;
  EXPECT_THROW(m_ctx->GenerateByTemplate(templ, result), utils::ExceptionInvalidParams);
}

TEST_F(ScTemplateGenApiTest, GenTripleWithUnknownSourceElement)
{
  ScTemplate templ;
  templ.Triple(ScType::Unknown >> "_addr1", ScType::EdgeAccessVarPosPerm >> "_arc", ScType::NodeVar >> "_addr2");

  ScTemplateGenResult result;
  EXPECT_THROW(m_ctx->GenerateByTemplate(templ, result), utils::ExceptionInvalidParams);
}

TEST_F(ScTemplateGenApiTest, GenTripleWithUnknownSecondElement)
{
  ScTemplate templ;
  templ.Triple(ScType::NodeVar >> "_addr1", ScType::Unknown >> "_arc", ScType::NodeVar >> "_addr2");

  ScTemplateGenResult result;
  EXPECT_THROW(m_ctx->GenerateByTemplate(templ, result), utils::ExceptionInvalidParams);
}

TEST_F(ScTemplateGenApiTest, GenEmptyTemplate)
{
  ScTemplate templ;
  ScTemplateGenResult result;
  m_ctx->GenerateByTemplate(templ, result);
}

TEST_F(ScTemplateGenApiTest, GenTemplateSuccessfully)
{
  ScTemplate templ;
  ScTemplateGenResult result;
  m_ctx->GenerateByTemplate(templ, result, ScTemplateParams::Empty);
}

TEST_F(ScTemplateGenApiTest, GenTripleWithFixedSecondEdge)
{
  ScAddr const & addr1 = m_ctx->GenerateNode(ScType::NodeConst);
  ScAddr const & addr2 = m_ctx->GenerateNode(ScType::NodeConst);
  ScAddr const & arcAddr = m_ctx->GenerateConnector(ScType::EdgeAccessConstPosPerm, addr1, addr2);

  ScTemplate templ;
  templ.Triple(ScType::NodeVar >> "_addr1", arcAddr, ScType::Unknown >> "_addr2");

  ScTemplateGenResult result;
  EXPECT_THROW(m_ctx->GenerateByTemplate(templ, result), utils::ExceptionInvalidParams);
}

TEST_F(ScTemplateGenApiTest, GenTripleWithTypedFirstEdge)
{
  ScTemplate templ;
  templ.Triple(ScType::EdgeAccessVarPosPerm >> "_addr1", ScType::EdgeAccessVarPosPerm, ScType::NodeVar >> "_addr2");

  ScTemplateGenResult result;
  EXPECT_THROW(m_ctx->GenerateByTemplate(templ, result), utils::ExceptionInvalidParams);
}

TEST_F(ScTemplateGenApiTest, GenTripleWithTypedThirdEdge)
{
  ScTemplate templ;
  templ.Triple(ScType::NodeVar >> "_addr1", ScType::EdgeAccessVarPosPerm, ScType::EdgeAccessVarPosPerm >> "_addr2");

  ScTemplateGenResult result;
  EXPECT_THROW(m_ctx->GenerateByTemplate(templ, result), utils::ExceptionInvalidParams);
}

TEST_F(ScTemplateGenApiTest, GenTripleWithInvalidRefSecondElement)
{
  ScTemplate templ;
  templ.Triple(ScType::NodeVar >> "_addr1", "_arc", ScType::NodeVar >> "_addr2");

  ScTemplateGenResult result;
  EXPECT_THROW(m_ctx->GenerateByTemplate(templ, result), utils::ExceptionInvalidParams);
}

TEST_F(ScTemplateGenApiTest, GenTemplateWithFullyReplacedVariableTriple)
{
  ScTemplate templ;
  templ.Triple(ScType::NodeVar >> "_addr1", ScType::EdgeDCommonVar >> "_arc", ScType::NodeVar >> "_addr2");
  templ.Triple("_addr2", ScType::EdgeAccessVarPosTemp, "_arc");

  ScAddr const & nodeAddr1 = m_ctx->GenerateNode(ScType::NodeConst);
  ScAddr const & nodeAddr2 = m_ctx->GenerateNode(ScType::NodeConst);
  ScAddr const & arcAddr = m_ctx->GenerateConnector(ScType::EdgeDCommonConst, nodeAddr1, nodeAddr2);

  ScTemplateParams params;
  params.Add("_addr1", nodeAddr1);
  params.Add("_addr2", nodeAddr2);
  params.Add("_arc", arcAddr);

  ScTemplateGenResult result;
  m_ctx->GenerateByTemplate(templ, result, params);

  EXPECT_EQ(result["_addr1"], nodeAddr1);
  EXPECT_EQ(result["_arc"], arcAddr);
  EXPECT_EQ(result["_addr2"], nodeAddr2);
}

TEST_F(ScTemplateGenApiTest, GenTemplateWithInvalidSourceElementInFullyReplacedVariableTriple)
{
  ScTemplate templ;
  templ.Triple(ScType::NodeVar >> "_addr1", ScType::EdgeDCommonVar >> "_arc", ScType::NodeVar >> "_addr2");
  templ.Triple("_addr2", ScType::EdgeAccessVarPosTemp, "_arc");

  ScAddr const & nodeAddr1 = m_ctx->GenerateNode(ScType::NodeConst);
  ScAddr const & nodeAddr2 = m_ctx->GenerateNode(ScType::NodeConst);
  ScAddr const & arcAddr = m_ctx->GenerateConnector(ScType::EdgeDCommonConst, nodeAddr1, nodeAddr2);

  ScTemplateParams params;
  params.Add("_addr1", nodeAddr2);
  params.Add("_addr2", nodeAddr2);
  params.Add("_arc", arcAddr);

  ScTemplateGenResult result;
  EXPECT_THROW(m_ctx->GenerateByTemplate(templ, result, params), utils::ExceptionInvalidParams);
}

TEST_F(ScTemplateGenApiTest, GenTemplateWithInvalidTargetElementInFullyReplacedVariableTriple)
{
  ScTemplate templ;
  templ.Triple(ScType::NodeVar >> "_addr1", ScType::EdgeDCommonVar >> "_arc", ScType::NodeVar >> "_addr2");
  templ.Triple("_addr2", ScType::EdgeAccessVarPosTemp, "_arc");

  ScAddr const & nodeAddr1 = m_ctx->GenerateNode(ScType::NodeConst);
  ScAddr const & nodeAddr2 = m_ctx->GenerateNode(ScType::NodeConst);
  ScAddr const & arcAddr = m_ctx->GenerateConnector(ScType::EdgeDCommonConst, nodeAddr1, nodeAddr2);

  ScTemplateParams params;
  params.Add("_addr1", nodeAddr1);
  params.Add("_addr2", nodeAddr1);
  params.Add("_arc", arcAddr);

  ScTemplateGenResult result;
  EXPECT_THROW(m_ctx->GenerateByTemplate(templ, result, params), utils::ExceptionInvalidParams);
}

TEST_F(ScTemplateGenApiTest, GenTemplateWithReplacedVariableThirdEdgeInVariableTriple)
{
  ScTemplate templ;
  templ.Triple(ScType::NodeVar >> "_addr2", ScType::EdgeAccessVarPosTemp, ScType::EdgeDCommonVar >> "_arc");

  ScAddr const & nodeAddr1 = m_ctx->GenerateNode(ScType::NodeConst);
  ScAddr const & nodeAddr2 = m_ctx->GenerateNode(ScType::NodeConst);
  ScAddr const & arcAddr = m_ctx->GenerateConnector(ScType::EdgeDCommonConst, nodeAddr1, nodeAddr2);

  ScTemplateParams params;
  params.Add("_arc", arcAddr);
  params.Add("_addr2", nodeAddr2);

  ScTemplateGenResult result;
  m_ctx->GenerateByTemplate(templ, result, params);

  EXPECT_EQ(result["_arc"], arcAddr);
  EXPECT_EQ(result["_addr2"], nodeAddr2);
}

TEST_F(ScTemplateGenApiTest, GenTemplateWithReplacedSourceAndTargetInVariableTriple)
{
  ScAddr const & nodeAddr1 = m_ctx->GenerateNode(ScType::NodeConst);
  ScAddr const & nodeAddr2 = m_ctx->GenerateNode(ScType::NodeConst);
  ScAddr const & arcAddr = m_ctx->GenerateConnector(ScType::EdgeDCommonConst, nodeAddr1, nodeAddr2);

  ScTemplate templ;
  templ.Triple(ScType::NodeVar >> "_addr1", arcAddr, ScType::NodeVar >> "_addr2");
  templ.Triple("_addr2", ScType::EdgeAccessVarPosTemp, arcAddr);

  ScTemplateParams params;
  params.Add("_addr1", nodeAddr1);
  params.Add("_addr2", nodeAddr2);

  ScTemplateGenResult result;
  m_ctx->GenerateByTemplate(templ, result, params);

  EXPECT_EQ(result["_addr1"], nodeAddr1);
  EXPECT_EQ(result[arcAddr], arcAddr);
  EXPECT_EQ(result["_addr2"], nodeAddr2);
}

TEST_F(ScTemplateGenApiTest, GenTemplateWithConstantArc)
{
  ScAddr const & nodeAddr1 = m_ctx->GenerateNode(ScType::NodeConst);
  ScAddr const & nodeAddr2 = m_ctx->GenerateNode(ScType::NodeConst);
  ScAddr const & arcAddr = m_ctx->GenerateConnector(ScType::EdgeDCommonConst, nodeAddr1, nodeAddr2);

  ScTemplate templ;
  templ.Triple(ScType::NodeVar >> "_addr1", arcAddr, ScType::NodeVar >> "_addr2");
  templ.Triple("_addr2", ScType::EdgeAccessVarPosTemp, arcAddr);

  ScTemplateGenResult result;
  m_ctx->GenerateByTemplate(templ, result);

  EXPECT_EQ(result["_addr1"], nodeAddr1);
  EXPECT_EQ(result[arcAddr], arcAddr);
  EXPECT_EQ(result["_addr2"], nodeAddr2);
}

TEST_F(ScTemplateGenApiTest, GenTemplateWithInvalidReplacedSourceInVariableTriple)
{
  ScAddr const & nodeAddr1 = m_ctx->GenerateNode(ScType::NodeConst);
  ScAddr const & nodeAddr2 = m_ctx->GenerateNode(ScType::NodeConst);
  ScAddr const & arcAddr = m_ctx->GenerateConnector(ScType::EdgeDCommonConst, nodeAddr1, nodeAddr2);

  ScTemplate templ;
  templ.Triple(ScType::NodeVar >> "_addr1", arcAddr, ScType::NodeVar >> "_addr2");
  templ.Triple("_addr2", ScType::EdgeAccessVarPosTemp, arcAddr);

  ScTemplateParams params;
  params.Add("_addr1", nodeAddr2);
  params.Add("_addr2", nodeAddr2);

  ScTemplateGenResult result;
  EXPECT_THROW(m_ctx->GenerateByTemplate(templ, result, params), utils::ExceptionInvalidParams);
}

TEST_F(ScTemplateGenApiTest, GenTemplateWithInvalidReplacedTargetInVariableTriple)
{
  ScAddr const & nodeAddr1 = m_ctx->GenerateNode(ScType::NodeConst);
  ScAddr const & nodeAddr2 = m_ctx->GenerateNode(ScType::NodeConst);
  ScAddr const & arcAddr = m_ctx->GenerateConnector(ScType::EdgeDCommonConst, nodeAddr1, nodeAddr2);

  ScTemplate templ;
  templ.Triple(ScType::NodeVar >> "_addr1", arcAddr, ScType::NodeVar >> "_addr2");
  templ.Triple("_addr2", ScType::EdgeAccessVarPosTemp, arcAddr);

  ScTemplateParams params;
  params.Add("_addr1", nodeAddr1);
  params.Add("_addr2", nodeAddr1);

  ScTemplateGenResult result;
  EXPECT_THROW(m_ctx->GenerateByTemplate(templ, result, params), utils::ExceptionInvalidParams);
}

TEST_F(ScTemplateGenApiTest, GenTemplateWithConstantTriple)
{
  ScAddr const & nodeAddr1 = m_ctx->GenerateNode(ScType::NodeConst);
  ScAddr const & nodeAddr2 = m_ctx->GenerateNode(ScType::NodeConst);
  ScAddr const & arcAddr = m_ctx->GenerateConnector(ScType::EdgeDCommonConst, nodeAddr1, nodeAddr2);

  ScTemplate templ;
  templ.Triple(nodeAddr1, arcAddr, nodeAddr2);
  templ.Triple(nodeAddr2, ScType::EdgeAccessVarPosTemp, arcAddr);

  ScTemplateGenResult result;
  m_ctx->GenerateByTemplate(templ, result);

  EXPECT_EQ(result[nodeAddr1], nodeAddr1);
  EXPECT_EQ(result[arcAddr], arcAddr);
  EXPECT_EQ(result[nodeAddr2], nodeAddr2);

  for (ScAddr const & addr : result)
    EXPECT_TRUE(addr.IsValid());
}

TEST_F(ScTemplateGenApiTest, GenTemplateWithInvalidSourceInConstantTriple)
{
  ScAddr const & nodeAddr1 = m_ctx->GenerateNode(ScType::NodeConst);
  ScAddr const & nodeAddr2 = m_ctx->GenerateNode(ScType::NodeConst);
  ScAddr const & arcAddr = m_ctx->GenerateConnector(ScType::EdgeDCommonConst, nodeAddr1, nodeAddr2);

  ScTemplate templ;
  templ.Triple(nodeAddr2, arcAddr, nodeAddr2);
  templ.Triple(nodeAddr2, ScType::EdgeAccessVarPosTemp, arcAddr);

  ScTemplateGenResult result;
  EXPECT_THROW(m_ctx->GenerateByTemplate(templ, result), utils::ExceptionInvalidParams);
}

TEST_F(ScTemplateGenApiTest, GenTemplateWithInvalidTargetInConstantTriple)
{
  ScAddr const & nodeAddr1 = m_ctx->GenerateNode(ScType::NodeConst);
  ScAddr const & nodeAddr2 = m_ctx->GenerateNode(ScType::NodeConst);
  ScAddr const & arcAddr = m_ctx->GenerateConnector(ScType::EdgeDCommonConst, nodeAddr1, nodeAddr2);

  ScTemplate templ;
  templ.Triple(nodeAddr1, arcAddr, nodeAddr1);
  templ.Triple(nodeAddr2, ScType::EdgeAccessVarPosTemp, arcAddr);

  ScTemplateGenResult result;
  EXPECT_THROW(m_ctx->GenerateByTemplate(templ, result), utils::ExceptionInvalidParams);
}

TEST_F(ScTemplateGenApiTest, GenTemplateWithReplacedSecondElementInVariableTriple)
{
  ScTemplate templ;
  templ.Triple(ScType::NodeVar >> "_addr1", ScType::EdgeAccessVarPosTemp, ScType::EdgeDCommonVar >> "_arc");
  templ.Triple("_addr1", "_arc", ScType::NodeVar >> "_addr2");

  ScAddr const & nodeAddr1 = m_ctx->GenerateNode(ScType::NodeConst);
  ScAddr const & nodeAddr2 = m_ctx->GenerateNode(ScType::NodeConst);
  ScAddr const & arcAddr = m_ctx->GenerateConnector(ScType::EdgeDCommonConst, nodeAddr1, nodeAddr2);

  ScTemplateParams params;
  params.Add("_arc", arcAddr);
  params.Add("_addr1", nodeAddr1);

  ScTemplateGenResult result;
  m_ctx->GenerateByTemplate(templ, result, params);

  EXPECT_EQ(result["_arc"], arcAddr);
  EXPECT_EQ(result["_addr1"], nodeAddr1);
  EXPECT_EQ(result["_addr2"], nodeAddr2);
}

TEST_F(ScTemplateGenApiTest, GenTemplateWithInvalidReplacementNameInParams)
{
  ScTemplate templ;
  templ.Triple(ScType::NodeVar >> "_addr1", ScType::EdgeDCommonVar >> "_arc", ScType::NodeVar >> "_addr2");
  templ.Triple("_addr2", ScType::EdgeAccessVarPosTemp, "_arc");

  ScAddr const & nodeAddr1 = m_ctx->GenerateNode(ScType::NodeConst);
  ScAddr const & nodeAddr2 = m_ctx->GenerateNode(ScType::NodeConst);
  ScAddr const & arcAddr = m_ctx->GenerateConnector(ScType::EdgeDCommonConst, nodeAddr1, nodeAddr2);

  ScTemplateParams params;
  params.Add("_invalid_arc_name", arcAddr);

  ScTemplateGenResult result;
  EXPECT_THROW(m_ctx->GenerateByTemplate(templ, result, params), utils::ExceptionInvalidParams);
}

TEST_F(ScTemplateGenApiTest, GenTemplateWithInvalidReplacementVarInParams)
{
  m_ctx->ResolveElementSystemIdentifier("_var", ScType::NodeVar);

  ScTemplate templ;
  templ.Triple(ScType::NodeVar >> "_addr1", ScType::EdgeDCommonVar >> "_arc", ScType::NodeVar >> "_addr2");
  templ.Triple("_addr2", ScType::EdgeAccessVarPosTemp, "_arc");

  ScAddr const & nodeAddr1 = m_ctx->GenerateNode(ScType::NodeConst);
  ScAddr const & nodeAddr2 = m_ctx->GenerateNode(ScType::NodeConst);
  m_ctx->GenerateConnector(ScType::EdgeDCommonConst, nodeAddr1, nodeAddr2);

  ScTemplateParams params;
  params.Add("_var", nodeAddr1);

  ScTemplateGenResult result;
  EXPECT_THROW(m_ctx->GenerateByTemplate(templ, result, params), utils::ExceptionInvalidParams);

  ScAddr resultAddr;
  EXPECT_FALSE(result.Get(0, resultAddr));
  EXPECT_EQ(resultAddr, ScAddr::Empty);

  EXPECT_THROW(result[0], utils::ExceptionInvalidParams);
}

TEST_F(ScTemplateGenApiTest, GenTemplateWithReplacedTargetHavingUnextendableType)
{
  ScTemplate templ;
  templ.Triple(ScType::NodeVar >> "_addr1", ScType::EdgeDCommonVar >> "_arc", ScType::NodeVarTuple >> "_addr2");
  templ.Triple("_addr2", ScType::EdgeAccessVarPosTemp, "_arc");

  ScAddr const & nodeAddr2 = m_ctx->GenerateNode(ScType::NodeConst);

  ScTemplateParams params;
  params.Add("_addr2", nodeAddr2);

  ScTemplateGenResult result;
  EXPECT_THROW(m_ctx->GenerateByTemplate(templ, result, params), utils::ExceptionInvalidParams);
}

TEST_F(ScTemplateGenApiTest, GenTemplateWithNodeReplacedByArc)
{
  ScTemplate templ;
  templ.Triple(ScType::NodeVar, ScType::EdgeDCommonVar >> "_arc", ScType::NodeVar);
  templ.Triple(ScType::NodeVar >> "_addr1", ScType::EdgeAccessVarPosTemp, "_arc");
  templ.Triple("_arc", ScType::EdgeAccessVarPosTemp, ScType::NodeVar >> "_addr2");

  ScAddr const & nodeAddr1 = m_ctx->GenerateNode(ScType::NodeConst);
  ScAddr const & nodeAddr2 = m_ctx->GenerateNode(ScType::NodeConst);
  ScAddr const & edgeAddr = m_ctx->GenerateConnector(ScType::EdgeDCommonConst, nodeAddr1, nodeAddr2);

  ScTemplateParams params;
  params.Add("_addr2", edgeAddr);

  ScTemplateGenResult result;
  m_ctx->GenerateByTemplate(templ, result, params);

  EXPECT_EQ(result["_addr2"], edgeAddr);
}
