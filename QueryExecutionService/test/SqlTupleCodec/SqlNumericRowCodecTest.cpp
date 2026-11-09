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
  const auto encoded = SqlNumericRowCodec::encode(value);
  const auto decoded = SqlNumericRowCodec::decode(encoded);

  EXPECT_EQ(decoded, value);
}

TEST(SqlNumericRowCodecTest, RoundTripsNegativeScaleAndZero) {
  const unidb::sql::SqlNumericValue negativeScale{false, "12345", -4};
  const unidb::sql::SqlNumericValue zero{false, "0", 0};
  const auto encodedNegativeScale = SqlNumericRowCodec::encode(negativeScale);
  const auto encodedZero = SqlNumericRowCodec::encode(zero);

  EXPECT_EQ(SqlNumericRowCodec::decode(encodedNegativeScale), negativeScale);
  EXPECT_EQ(SqlNumericRowCodec::decode(encodedZero), zero);
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
  const unidb::sql::SqlNumericValue nonCanonical{false, "001", 0};

  EXPECT_THROW(SqlNumericRowCodec::encode(nonCanonical),
               std::invalid_argument);
}
