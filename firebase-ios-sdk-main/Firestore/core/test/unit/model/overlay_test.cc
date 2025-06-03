/*
 * Copyright 2022 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <sstream>
#include <type_traits>
#include <utility>

#include "Firestore/core/src/model/mutation.h"
#include "Firestore/core/src/model/overlay.h"
#include "Firestore/core/src/model/patch_mutation.h"
#include "Firestore/core/src/model/resource_path.h"
#include "Firestore/core/test/unit/testutil/equals_tester.h"
#include "Firestore/core/test/unit/testutil/testutil.h"
#include "absl/strings/string_view.h"
#include "gtest/gtest.h"

namespace firebase {
namespace firestore {
namespace model {
namespace {

using testing::EndsWith;
using testing::HasSubstr;
using testing::StartsWith;
using testutil::Map;
using testutil::PatchMutation;

constexpr int SAMPLE_BATCH_ID = 123;

Mutation SampleMutation(absl::string_view path = "doc/col") {
  return PatchMutation(path, Map("key", "value"));
}

TEST(OverlayTest, TypeTraits) {
  static_assert(std::is_constructible<Overlay>::value, "is_constructible");
  static_assert(std::is_destructible<Overlay>::value, "is_destructible");
  static_assert(std::is_default_constructible<Overlay>::value,
                "is_default_constructible");
  static_assert(std::is_copy_constructible<Overlay>::value,
                "is_copy_constructible");
  static_assert(std::is_move_constructible<Overlay>::value,
                "is_move_constructible");
  static_assert(std::is_copy_assignable<Overlay>::value, "is_copy_assignable");
  static_assert(std::is_move_assignable<Overlay>::value, "is_move_assignable");
}

TEST(OverlayTest, DefaultConstructor) {
  Overlay overlay;

  EXPECT_EQ(overlay.largest_batch_id(), -1);
  EXPECT_EQ(overlay.mutation(), Mutation());
}

TEST(OverlayTest, ConstructorWithValidMutation) {
  Overlay overlay(SAMPLE_BATCH_ID, SampleMutation());

  EXPECT_EQ(overlay.largest_batch_id(), SAMPLE_BATCH_ID);
  EXPECT_EQ(overlay.mutation(), SampleMutation());
  EXPECT_EQ(overlay.key(), SampleMutation().key());
}

TEST(OverlayTest, ConstructorWithInvalidMutation) {
  Overlay overlay(SAMPLE_BATCH_ID, Mutation());

  EXPECT_EQ(overlay.largest_batch_id(), SAMPLE_BATCH_ID);
  EXPECT_EQ(overlay.mutation(), Mutation());
}

TEST(OverlayTest, CopyConstructorWithValidInstance) {
  const Overlay overlay_copy_src(SAMPLE_BATCH_ID, SampleMutation());

  Overlay overlay_copy_dest(overlay_copy_src);

  EXPECT_EQ(overlay_copy_dest.largest_batch_id(), SAMPLE_BATCH_ID);
  EXPECT_EQ(overlay_copy_dest.mutation(), SampleMutation());
}

TEST(OverlayTest, CopyConstructorWithInvalidInstance) {
  const Overlay invalid_overlay;

  Overlay overlay_copy_dest(invalid_overlay);
}

TEST(OverlayTest, MoveConstructorWithValidInstance) {
  Overlay overlay_move_src(SAMPLE_BATCH_ID, SampleMutation());

  Overlay overlay_move_dest(std::move(overlay_move_src));

  EXPECT_FALSE(overlay_move_src.mutation().is_valid());
  EXPECT_EQ(overlay_move_dest.largest_batch_id(), SAMPLE_BATCH_ID);
  EXPECT_EQ(overlay_move_dest.mutation(), SampleMutation());
}

TEST(OverlayTest, MoveConstructorWithInvalidInstance) {
  Overlay invalid_overlay;

  Overlay overlay_move_dest(std::move(invalid_overlay));
}

TEST(OverlayTest, CopyAssignmentOperatorWithValidInstance) {
  const Overlay overlay_copy_src(123, SampleMutation("col1/doc1"));
  Overlay overlay_copy_dest(456, SampleMutation("col2/doc2"));

  overlay_copy_dest = overlay_copy_src;

  EXPECT_EQ(overlay_copy_dest.largest_batch_id(), 123);
  EXPECT_EQ(overlay_copy_dest.mutation(), SampleMutation("col1/doc1"));
}

TEST(OverlayTest, CopyAssignmentOperatorWithInvalidInstance) {
  const Overlay invalid_overlay;
  Overlay overlay_copy_dest(456, SampleMutation("col2/doc2"));

  overlay_copy_dest = invalid_overlay;
}

TEST(OverlayTest, MoveAssignmentOperatorWithValidInstance) {
  Overlay overlay_move_src(123, SampleMutation("col1/doc1"));
  Overlay overlay_move_dest(456, SampleMutation("col2/doc2"));

  overlay_move_dest = std::move(overlay_move_src);

  EXPECT_FALSE(overlay_move_src.mutation().is_valid());
  EXPECT_EQ(overlay_move_dest.largest_batch_id(), 123);
  EXPECT_EQ(overlay_move_dest.mutation(), SampleMutation("col1/doc1"));
}

TEST(OverlayTest, MoveAssignmentOperatorWithInvalidInstance) {
  Overlay invalid_overlay;
  Overlay overlay_move_dest(456, SampleMutation("col2/doc2"));

  overlay_move_dest = std::move(invalid_overlay);
}

TEST(OverlayTest, largest_batch_id) {
  const Overlay overlay123(123, SampleMutation());
  const Overlay overlay456(456, SampleMutation());

  EXPECT_EQ(overlay123.largest_batch_id(), 123);
  EXPECT_EQ(overlay456.largest_batch_id(), 456);
}

TEST(OverlayTest, mutation) {
  const Overlay overlay_abc(SAMPLE_BATCH_ID, SampleMutation("col/abc"));
  const Overlay overlay_xyz(SAMPLE_BATCH_ID, SampleMutation("col/xyz"));

  EXPECT_EQ(overlay_abc.mutation(), SampleMutation("col/abc"));
  EXPECT_EQ(overlay_xyz.mutation(), SampleMutation("col/xyz"));
}

TEST(OverlayTest, key) {
  const Overlay overlay(SAMPLE_BATCH_ID, SampleMutation());

  const DocumentKey& key = overlay.key();

  EXPECT_EQ(key, SampleMutation().key());
}

TEST(OverlayTest, EqualsAndHash) {
  testutil::EqualsTester<Overlay>()
      .AddEqualityGroup(Overlay(), Overlay())
      .AddEqualityGroup(Overlay(SAMPLE_BATCH_ID, Mutation()),
                        Overlay(SAMPLE_BATCH_ID, Mutation()))
      .AddEqualityGroup(Overlay(SAMPLE_BATCH_ID, SampleMutation("col/abc")),
                        Overlay(SAMPLE_BATCH_ID, SampleMutation("col/abc")))
      .AddEqualityGroup(Overlay(SAMPLE_BATCH_ID + 1, SampleMutation("col/abc")),
                        Overlay(SAMPLE_BATCH_ID + 1, SampleMutation("col/abc")))
      .AddEqualityGroup(Overlay(SAMPLE_BATCH_ID, SampleMutation("col/xyz")),
                        Overlay(SAMPLE_BATCH_ID, SampleMutation("col/xyz")))
      .TestEquals();
}

TEST(OverlayTest, ToStringOnInvalidInstance) {
  const Overlay invalid_overlay = Overlay();

  const std::string invalid_overlay_string = invalid_overlay.ToString();

  EXPECT_THAT(invalid_overlay_string, StartsWith("Overlay("));
  EXPECT_THAT(invalid_overlay_string, EndsWith(")"));
}

TEST(OverlayTest, ToStringOnInvalidInstanceWithABatchId) {
  const Overlay invalid_overlay = Overlay(1234, Mutation());

  const std::string invalid_overlay_string = invalid_overlay.ToString();

  EXPECT_THAT(invalid_overlay_string, StartsWith("Overlay("));
  EXPECT_THAT(invalid_overlay_string, HasSubstr("largest_batch_id=1234"));
  EXPECT_THAT(invalid_overlay_string, EndsWith(")"));
}

TEST(OverlayTest, ToStringOnValidInstance) {
  const Overlay overlay = Overlay(1234, SampleMutation("abc/xyz"));

  const std::string overlay_string = overlay.ToString();

  EXPECT_THAT(overlay_string, StartsWith("Overlay("));
  EXPECT_THAT(overlay_string, EndsWith(")"));
  EXPECT_THAT(overlay_string, HasSubstr("largest_batch_id=1234"));
  EXPECT_THAT(overlay_string, HasSubstr("mutation="));
  EXPECT_THAT(overlay_string, HasSubstr("abc/xyz"));
}

TEST(OverlayTest, OperatorLeftShiftIntoOstream) {
  const Overlay overlay = Overlay(1234, SampleMutation("abc/xyz"));
  std::ostringstream ss;

  ss << overlay;

  EXPECT_EQ(ss.str(), overlay.ToString());
}

TEST(OverlayHashTest, OverlayHashTest) {
  const OverlayHash overlay_hash;
  const Overlay overlay1 = Overlay(1234, SampleMutation("abc/xyz"));
  const Overlay overlay2 = Overlay(5678, SampleMutation("def/uvw"));

  EXPECT_EQ(overlay_hash(overlay1), overlay1.Hash());
  EXPECT_EQ(overlay_hash(overlay2), overlay2.Hash());
}

}  // namespace
}  // namespace model
}  // namespace firestore
}  // namespace firebase
