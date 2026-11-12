#include "../SqlAutocomplete.h"

#include <gtest/gtest.h>

#include <algorithm>

#include <string>

TEST(SqlShellAutocompleteTest, CompletesUniqueKeywordPrefixIgnoringCase) {
  std::string line = "sel";
  std::size_t cursor = line.size();
  const auto matches = SqlAutocomplete::suggestionsFor("sel");

  ASSERT_EQ(matches.size(), 1U);
  EXPECT_EQ(matches.front(), "SELECT");
  ASSERT_TRUE(SqlAutocomplete::replaceToken(line, cursor, matches.front()));
  EXPECT_EQ(line, "SELECT");
  EXPECT_EQ(cursor, line.size());
}

TEST(SqlShellAutocompleteTest, ReturnsMultipleSuggestionsForAmbiguousPrefix) {
  const auto matches = SqlAutocomplete::suggestionsFor("s");

  EXPECT_GT(matches.size(), 1U);
  EXPECT_NE(std::find(matches.begin(), matches.end(), "SELECT"), matches.end());
  EXPECT_NE(std::find(matches.begin(), matches.end(), "SUM"), matches.end());
}

TEST(SqlShellAutocompleteTest, ReplacesOnlyTheTokenAtTheCursor) {
  std::string line = "SELECT * FROM pe WHERE id = 1";
  std::size_t cursor = std::string("SELECT * FROM pe").size();

  ASSERT_TRUE(SqlAutocomplete::replaceToken(line, cursor, "people"));
  EXPECT_EQ(line, "SELECT * FROM people WHERE id = 1");
  EXPECT_EQ(cursor, std::string("SELECT * FROM people").size());
}

TEST(SqlShellAutocompleteTest, DoesNotSuggestAnExactKeywordAgain) {
  EXPECT_TRUE(SqlAutocomplete::suggestionsFor("SELECT").empty());
  EXPECT_TRUE(SqlAutocomplete::suggestionsFor("").empty());
}

TEST(SqlShellAutocompleteTest, DoesNotCompleteInsideStringLiterals) {
  const std::string line = "SELECT 'sel";
  EXPECT_TRUE(SqlAutocomplete::isInsideStringLiteral(line, line.size()));
  EXPECT_FALSE(SqlAutocomplete::isInsideStringLiteral("SELECT sel", 10));
}

TEST(SqlShellAutocompleteTest, HandlesEscapedQuotesInStringLiterals) {
  const std::string line = "SELECT 'it''s sel";
  EXPECT_TRUE(SqlAutocomplete::isInsideStringLiteral(line, line.size()));
}

TEST(SqlShellAutocompleteTest, ShowsUniqueCompletionInlineWhileTyping) {
  EXPECT_EQ(SqlAutocomplete::inlineSuggestion("SELECT * fr", 11), "OM");
}

TEST(SqlShellAutocompleteTest, HidesInlineCompletionForAmbiguousOrQuotedInput) {
  EXPECT_TRUE(SqlAutocomplete::inlineSuggestion("SELECT * s", 10).empty());
  EXPECT_TRUE(SqlAutocomplete::inlineSuggestion("SELECT 'fr", 10).empty());
}

TEST(SqlShellAutocompleteTest, HidesInlineCompletionWhenCursorIsNotAtEnd) {
  EXPECT_TRUE(SqlAutocomplete::inlineSuggestion("SELECT * fr", 10).empty());
}
