#include "generator/feature_generator.hpp"

#include "generator/feature_builder.hpp"
#include "generator/intermediate_data.hpp"
#include "generator/intermediate_elements.hpp"

#include "indexer/data_header.hpp"


#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/stl_helpers.hpp"

#include <functional>
#include <utility>


///////////////////////////////////////////////////////////////////////////////////////////////////
// FeaturesCollector implementation
///////////////////////////////////////////////////////////////////////////////////////////////////
namespace feature
{

FeaturesCollector::FeaturesCollector(std::string const & fName, FileWriter::Op op)
  : m_datFile(fName, op), m_writeBuffer(kBufferSize) {}

FeaturesCollector::~FeaturesCollector()
{
  FlushBuffer();
}

template <typename ValueT, size_t ValueSizeT = sizeof(ValueT) + 1>
std::pair<char[ValueSizeT], uint8_t> PackValue(ValueT v)
{
  static_assert(std::is_integral<ValueT>::value, "Non integral value");
  static_assert(std::is_unsigned<ValueT>::value, "Non unsigned value");

  std::pair<char[ValueSizeT], uint8_t> res;
  res.second = 0;
  while (v > 127)
  {
    res.first[res.second++] = static_cast<uint8_t>((v & 127) | 128);
    v >>= 7;
  }
  res.first[res.second++] = static_cast<uint8_t>(v);
  return res;
}

void FeaturesCollector::FlushBuffer()
{
  m_datFile.Write(m_writeBuffer.data(), m_writePosition);
  m_writePosition = 0;
}

void FeaturesCollector::Flush()
{
  FlushBuffer();
  m_datFile.Flush();
}

void FeaturesCollector::Write(char const * src, size_t size)
{
  do
  {
    if (m_writePosition == kBufferSize)
      FlushBuffer();

    size_t const part_size = std::min(size, kBufferSize - m_writePosition);
    memcpy(&m_writeBuffer[m_writePosition], src, part_size);
    m_writePosition += part_size;
    size -= part_size;
    src += part_size;
  } while (size > 0);
}

uint32_t FeaturesCollector::WriteFeatureBase(std::vector<char> const & bytes, FeatureBuilder const & fb)
{
  size_t const sz = bytes.size();
  CHECK(sz != 0, ("Empty feature not allowed here!"));

  auto const & packedSize = PackValue(sz);
  Write(packedSize.first, packedSize.second);
  Write(&bytes[0], sz);

  m_bounds.Add(fb.GetLimitRect());
  return m_featureID++;
}

uint32_t FeaturesCollector::Collect(FeatureBuilder const & fb)
{
  FeatureBuilder::Buffer bytes;
  fb.SerializeForIntermediate(bytes);
  uint32_t const featureId = WriteFeatureBase(bytes, fb);
  CHECK_LESS(0, m_featureID, ());
  return featureId;
}

uint32_t CheckedFilePosCast(FileWriter const & f)
{
  uint64_t pos = f.Pos();
  CHECK_LESS_OR_EQUAL(pos, static_cast<uint64_t>(std::numeric_limits<uint32_t>::max()),
                      ("Feature offset is out of 32bit boundary!"));
  return static_cast<uint32_t>(pos);
}
}  // namespace feature
