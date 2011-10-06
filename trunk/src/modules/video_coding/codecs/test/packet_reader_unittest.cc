/*
 *  Copyright (c) 2011 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#include "gtest/gtest.h"
#include "packet_reader.h"
#include "typedefs.h"
#include "unittest_utils.h"

namespace webrtc {
namespace test {

class PacketReaderTest: public PacketRelatedTest {
 protected:
  PacketReader* reader_;

  PacketReaderTest() {
    // To avoid warnings when using ASSERT_DEATH
    ::testing::FLAGS_gtest_death_test_style = "threadsafe";
  }

  virtual ~PacketReaderTest() {
  }

  void SetUp() {
    reader_ = new PacketReader();
  }

  void TearDown() {
    delete reader_;
  }

  void VerifyPacketData(int expected_length,
                        int actual_length,
                        WebRtc_UWord8* original_data_pointer,
                        WebRtc_UWord8* new_data_pointer) {
    EXPECT_EQ(expected_length, actual_length);
    EXPECT_EQ(*original_data_pointer, *new_data_pointer);
    EXPECT_EQ(0, memcmp(original_data_pointer, new_data_pointer,
                        actual_length));
  }
};

// Test lack of initialization
TEST_F(PacketReaderTest, Uninitialized) {
  WebRtc_UWord8* data_pointer = NULL;
  EXPECT_EQ(-1, reader_->NextPacket(&data_pointer));
  EXPECT_EQ(NULL, data_pointer);
}

TEST_F(PacketReaderTest, InitializeNullDataArgument) {
  ASSERT_DEATH(reader_->InitializeReading(NULL, kPacketDataLength,
                                          kPacketSizeInBytes), "");
}

TEST_F(PacketReaderTest, InitializeInvalidLengthArgument) {
  ASSERT_DEATH(reader_->InitializeReading(packet_data_, -1, kPacketSizeInBytes),
               "");
}

TEST_F(PacketReaderTest, InitializeZeroLengthArgument) {
  reader_->InitializeReading(packet_data_, 0, kPacketSizeInBytes);
  ASSERT_EQ(0, reader_->NextPacket(&packet_data_pointer_));
}

TEST_F(PacketReaderTest, InitializeInvalidPacketSizeArgument) {
  ASSERT_DEATH(reader_->InitializeReading(packet_data_, kPacketDataLength,
                                          0), "");
}

// Test with something smaller than one packet
TEST_F(PacketReaderTest, NormalSmallData) {
  const int kDataLengthInBytes = 1499;
  WebRtc_UWord8 data[kDataLengthInBytes];
  WebRtc_UWord8* data_pointer = data;
  memset(data, 1, kDataLengthInBytes);

  reader_->InitializeReading(data, kDataLengthInBytes, kPacketSizeInBytes);
  int length_to_read = reader_->NextPacket(&data_pointer);
  VerifyPacketData(kDataLengthInBytes, length_to_read, data, data_pointer);
  EXPECT_EQ(0, data_pointer - data);  // pointer hasn't moved

  // Reading another one shall result in 0 bytes:
  length_to_read = reader_->NextPacket(&data_pointer);
  EXPECT_EQ(0, length_to_read);
  EXPECT_EQ(kDataLengthInBytes, data_pointer - data);
}

// Test with data length that exactly matches one packet
TEST_F(PacketReaderTest, NormalOnePacketData) {
  WebRtc_UWord8 data[kPacketSizeInBytes];
  WebRtc_UWord8* data_pointer = data;
  memset(data, 1, kPacketSizeInBytes);

  reader_->InitializeReading(data, kPacketSizeInBytes, kPacketSizeInBytes);
  int length_to_read = reader_->NextPacket(&data_pointer);
  VerifyPacketData(kPacketSizeInBytes, length_to_read, data, data_pointer);
  EXPECT_EQ(0, data_pointer - data);  // pointer hasn't moved

  // Reading another one shall result in 0 bytes:
  length_to_read = reader_->NextPacket(&data_pointer);
  EXPECT_EQ(0, length_to_read);
  EXPECT_EQ(kPacketSizeInBytes, data_pointer - data);
}

// Test with data length that will result in 3 packets
TEST_F(PacketReaderTest, NormalLargeData) {
  reader_->InitializeReading(packet_data_, kPacketDataLength,
                             kPacketSizeInBytes);

  int length_to_read = reader_->NextPacket(&packet_data_pointer_);
  VerifyPacketData(kPacketSizeInBytes, length_to_read,
                   packet1_, packet_data_pointer_);

  length_to_read = reader_->NextPacket(&packet_data_pointer_);
  VerifyPacketData(kPacketSizeInBytes, length_to_read,
                   packet2_, packet_data_pointer_);

  length_to_read = reader_->NextPacket(&packet_data_pointer_);
  VerifyPacketData(1u, length_to_read,
                   packet3_, packet_data_pointer_);

  // Reading another one shall result in 0 bytes:
  length_to_read = reader_->NextPacket(&packet_data_pointer_);
  EXPECT_EQ(0, length_to_read);
  EXPECT_EQ(kPacketDataLength, packet_data_pointer_ - packet_data_);
}

// Test with empty data.
TEST_F(PacketReaderTest, EmptyData) {
  const int kDataLengthInBytes = 0;
  WebRtc_UWord8 data[kDataLengthInBytes];
  WebRtc_UWord8* data_pointer = data;
  reader_->InitializeReading(data, kDataLengthInBytes, kPacketSizeInBytes);
  EXPECT_EQ(kDataLengthInBytes, reader_->NextPacket(&data_pointer));
  EXPECT_EQ(*data, *data_pointer);
  // Do it again to make sure nothing changes
  EXPECT_EQ(kDataLengthInBytes, reader_->NextPacket(&data_pointer));
  EXPECT_EQ(*data, *data_pointer);
}

}  // namespace test
}  // namespace webrtc
