#include "../../domain/types/SqlCharacterSemantics.h"
#include "../../domain/types/SqlTypeRegistry.h"

#include <gtest/gtest.h>

#include <stdexcept>
#include <string>

using QueryExecutionService::domain::countSqlCharacters;
using QueryExecutionService::domain::normalizeSqlCharacterValue;
using QueryExecutionService::domain::resolveSqlType;

TEST(SqlCharacterSemanticsTest, CountsUtf8CharactersRatherThanBytes) {
  EXPECT_EQ(countSqlCharacters("hello"), 5U);
  EXPECT_EQ(countSqlCharacters("café"), 4U);
  EXPECT_EQ(countSqlCharacters("東京"), 2U);
  EXPECT_EQ(countSqlCharacters("🙂"), 1U);
}

TEST(SqlCharacterSemanticsTest, RejectsMalformedUtf8) {
  EXPECT_THROW(countSqlCharacters(std::string("\xc0\xaf", 2)),
               std::invalid_argument);
  EXPECT_THROW(countSqlCharacters(std::string("\xe2\x82", 2)),
               std::invalid_argument);
  EXPECT_THROW(countSqlCharacters(std::string("\xed\xa0\x80", 3)),
               std::invalid_argument);
}

TEST(SqlCharacterSemanticsTest, BlankPadsCharToDeclaredCharacterLength) {
  EXPECT_EQ(normalizeSqlCharacterValue(resolveSqlType("CHAR", {.length = 5}),
                                       "cat"),
            "cat  ");
  EXPECT_EQ(normalizeSqlCharacterValue(resolveSqlType("CHAR(3)"), "cat"),
            "cat");
}

TEST(SqlCharacterSemanticsTest,
     AllowsOnlyExcessTrailingSpacesBeyondCharLength) {
  EXPECT_EQ(normalizeSqlCharacterValue(resolveSqlType("CHAR(3)"), "cat  "),
            "cat");
  EXPECT_THROW(normalizeSqlCharacterValue(resolveSqlType("CHAR(3)"), "cats"),
               std::invalid_argument);
}

TEST(SqlCharacterSemanticsTest, EnforcesVarcharLengthInCharacters) {
  EXPECT_EQ(normalizeSqlCharacterValue(resolveSqlType("VARCHAR(2)"), "東京"),
            "東京");
  EXPECT_THROW(
      normalizeSqlCharacterValue(resolveSqlType("VARCHAR(2)"), "東京市"),
      std::invalid_argument);
}

TEST(SqlCharacterSemanticsTest, VarcharAllowsExcessTrailingSpaces) {
  EXPECT_EQ(normalizeSqlCharacterValue(resolveSqlType("VARCHAR(3)"), "cat  "),
            "cat");
}

TEST(SqlCharacterSemanticsTest, UnboundedVarcharAndTextPreserveInput) {
  EXPECT_EQ(normalizeSqlCharacterValue(resolveSqlType("VARCHAR"), "a  "),
            "a  ");
  EXPECT_EQ(normalizeSqlCharacterValue(resolveSqlType("TEXT"), "a  "), "a  ");
}

TEST(SqlCharacterSemanticsTest, RejectsNonCharacterSqlTypes) {
  EXPECT_THROW(normalizeSqlCharacterValue(resolveSqlType("INTEGER"), "1"),
               std::invalid_argument);
}
