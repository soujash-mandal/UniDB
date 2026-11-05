
TEST(SqlParserTest, ParsesInAndNotInLiteralLists) {
  SqlParserImpl parser;
  const auto inResult =
      parser.parse("SELECT id FROM users WHERE id IN (1, 2, -3)");
  ASSERT_TRUE(std::holds_alternative<SqlStatement>(inResult));
  const auto &inSelect =
      std::get<SelectStatement>(std::get<SqlStatement>(inResult));
  const auto &inPredicate =
      std::get<WhereExpression>(inSelect.where).predicate;
  EXPECT_EQ(inPredicate.comparison, ComparisonOperator::In);
  ASSERT_EQ(inPredicate.values.size(), 3U);
  EXPECT_EQ(std::get<std::int64_t>(inPredicate.values[0]), 1);
  EXPECT_EQ(std::get<std::int64_t>(inPredicate.values[1]), 2);
  EXPECT_EQ(std::get<std::int64_t>(inPredicate.values[2]), -3);

  const auto notInResult =
      parser.parse("SELECT name FROM users WHERE name NOT IN ('Ada', 'Grace')");
  ASSERT_TRUE(std::holds_alternative<SqlStatement>(notInResult));
  const auto &notInSelect =
      std::get<SelectStatement>(std::get<SqlStatement>(notInResult));
  const auto &notInPredicate =
      std::get<WhereExpression>(notInSelect.where).predicate;
  EXPECT_EQ(notInPredicate.comparison, ComparisonOperator::NotIn);
  ASSERT_EQ(notInPredicate.values.size(), 2U);
  EXPECT_EQ(std::get<std::string>(notInPredicate.values[0]), "Ada");
  EXPECT_EQ(std::get<std::string>(notInPredicate.values[1]), "Grace");
}

TEST(SqlParserTest, RejectsMalformedInLists) {
  SqlParserImpl parser;
  for (const auto *sql :
       {"SELECT * FROM users WHERE id IN ()",
        "SELECT * FROM users WHERE id IN (1,)",
        "SELECT * FROM users WHERE id IN (1, 2",
        "SELECT * FROM users WHERE id IN 1",
        "SELECT * FROM users WHERE id NOT 1",
        "SELECT * FROM users WHERE id NOT IN (1, 2))",
        "SELECT * FROM users WHERE id IN (1) AND"}) {
    const auto result = parser.parse(sql);
    ASSERT_TRUE(std::holds_alternative<SqlParseError>(result)) << sql;
    EXPECT_EQ(std::get<SqlParseError>(result).code,
              SqlParseErrorCode::SyntaxError)
        << sql;
  }
}
