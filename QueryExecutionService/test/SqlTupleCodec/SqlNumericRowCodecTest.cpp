#include "../../domain/codec/SqlNumericRowCodec.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

using QueryExecutionService::domain::SqlNumericRowCodec;

TEST(SqlNumericRowCodecTest, RoundTripsExactValuesWithoutPrecisionLoss) {
  const unidb::sql::SqlNumericValue value{
      true, "123456789012345678901234567890123456789", 17};

  EXPECT_EQ(SqlNumericRowCodec::decode(SqlNumericRowCodec::encode(value)),
            value);
}

TEST(SqlNumericRowCodecTest, RoundTripsNegativeScaleAndZero) {
  const unidb::sql::SqlNumericValue negativeScale{false, "12345", -4};
  const unidb::sql::SqlNumericValue zero{false, "0", 0};

  EXPECT_EQ(
      SqlNumericRowCodec::decode(SqlNumericRowCodec::encode(negativeScale)),
      negativeScale);
  EXPECT_EQ(SqlNumericRowCodec::decode(SqlNumericRowCodec::encode(zero)), zero);
}

TEST(SqlNumericRowCodecTest, RejectsUnknownFormatVersion) {
  const std::vector<char> bytes{2, 0, 0, 0, 0, 1, 0, 0, 0, '1'};

  EXPECT_THROW(SqlNumericRowCodec::decode(bytes), std::runtime_error);
}

TEST(SqlNumericRowCodecTest, RejectsInvalidSign) {
  const std::vector<char> bytes{1, 2, 0, 0, 0, 1, 0, 0, 0, '1'};

  EXPECT_THROW(SqlNumericRowCodec::decode(bytes), std::runtime_error);
}

TEST(SqlNumericRowCodecTest, RejectsTruncatedAndTrailingCoefficientBytes) {
  const std::vector<char> truncated{1, 0, 0, 0, 0, 2, 0, 0, 0, '1'};
  const std::vector<char> trailing{1, 0, 0, 0, 0, 1, 0, 0, 0, '1', '2'};

  EXPECT_THROW(SqlNumericRowCodec::decode(truncated), std::runtime_error);
  EXPECT_THROW(SqlNumericRowCodec::decode(trailing), std::runtime_error);
}

TEST(SqlNumericRowCodecTest, RejectsNonCanonicalCoefficient) {
  const std::vector<char> bytes{1, 0, 0, 0, 0, 2, 0, 0, 0, '0', '1'};

  EXPECT_THROW(SqlNumericRowCodec::decode(bytes), std::runtime_error);
}

TEST(SqlNumericRowCodecTest, RejectsNonCanonicalValuesDuringEncoding) {
  EXPECT_THROW(SqlNumericRowCodec::encode({false, "001", 0}),
               std::invalid_argument);
}
