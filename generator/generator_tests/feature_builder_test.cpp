#include "testing/testing.hpp"

#include "types_helper.hpp"

#include "generator/feature_builder.hpp"
#include "generator/generator_tests_support/test_with_classificator.hpp"
#include "generator/geometry_holder.hpp"

#include "indexer/classificator_loader.hpp"
#include "indexer/covered_object.hpp"
#include "indexer/data_header.cpp"
#include "indexer/feature_visibility.hpp"

#include "base/geo_object_id.hpp"

#include <limits>

using namespace feature;

using namespace generator::tests_support;
using namespace tests;

UNIT_CLASS_TEST(TestWithClassificator, FBuilder_ManyTypes)
{
  FeatureBuilder fb1;
  FeatureParams params;

  char const * arr1[][1] = {
    { "building" },
  };
  AddTypes(params, arr1);

  char const * arr2[][2] = {
    { "place", "country" },
    { "place", "state" },
    /// @todo Can't realize is it deprecated or we forgot to add clear styles for it.
    //{ "place", "county" },
    { "place", "region" },
    { "place", "city" },
    { "place", "town" },
  };
  AddTypes(params, arr2);

  params.FinishAddingTypes();
  params.AddHouseNumber("75");
  params.AddHouseName("Best House");
  params.AddName("default", "Name");

  fb1.SetParams(params);
  fb1.SetCenter(m2::PointD(0, 0));

  TEST(fb1.RemoveInvalidTypes(), ());
  Check(fb1);

  FeatureBuilder::Buffer buffer;
  TEST(fb1.PreSerializeAndRemoveUselessNamesForIntermediate(), ());
  fb1.SerializeForIntermediate(buffer);

  FeatureBuilder fb2;
  fb2.DeserializeFromIntermediate(buffer);

  Check(fb2);
  TEST_EQUAL(fb1, fb2, ());
  TEST_EQUAL(fb2.GetTypesCount(), 6, ());
}

UNIT_CLASS_TEST(TestWithClassificator, FBuilder_LineTypes)
{
  FeatureBuilder fb1;
  FeatureParams params;

  char const * arr2[][2] = {
    { "railway", "rail" },
    { "highway", "motorway" },
    { "hwtag", "oneway" },
    { "psurface", "paved_good" },
    { "junction", "roundabout" },
  };

  AddTypes(params, arr2);
  params.FinishAddingTypes();
  fb1.SetParams(params);

  fb1.AddPoint(m2::PointD(0, 0));
  fb1.AddPoint(m2::PointD(1, 1));
  fb1.SetLinear();

  TEST(fb1.RemoveInvalidTypes(), ());
  Check(fb1);

  FeatureBuilder::Buffer buffer;
  TEST(fb1.PreSerializeAndRemoveUselessNamesForIntermediate(), ());
  fb1.SerializeForIntermediate(buffer);

  FeatureBuilder fb2;
  fb2.DeserializeFromIntermediate(buffer);

  Check(fb2);
  TEST_EQUAL(fb1, fb2, ());
  TEST_EQUAL(fb2.GetTypesCount(), 5, ());
}

UNIT_CLASS_TEST(TestWithClassificator, FBuilder_Waterfall)
{
  FeatureBuilder fb1;
  FeatureParams params;

  char const * arr[][2] = {{"waterway", "waterfall"}};
  AddTypes(params, arr);
  TEST(params.FinishAddingTypes(), ());

  fb1.SetParams(params);
  fb1.SetCenter(m2::PointD(1, 1));

  TEST(fb1.RemoveInvalidTypes(), ());
  Check(fb1);

  FeatureBuilder::Buffer buffer;
  TEST(fb1.PreSerializeAndRemoveUselessNamesForIntermediate(), ());
  fb1.SerializeForIntermediate(buffer);

  FeatureBuilder fb2;
  fb2.DeserializeFromIntermediate(buffer);

  Check(fb2);
  TEST_EQUAL(fb1, fb2, ());
  TEST_EQUAL(fb2.GetTypesCount(), 1, ());;
}

UNIT_CLASS_TEST(TestWithClassificator, FBbuilder_GetMostGeneralOsmId)
{
  FeatureBuilder fb;

  fb.AddOsmId(base::MakeOsmNode(1));
  TEST_EQUAL(fb.GetMostGenericOsmId(), base::MakeOsmNode(1), ());

  fb.AddOsmId(base::MakeOsmNode(2));
  fb.AddOsmId(base::MakeOsmWay(1));
  TEST_EQUAL(fb.GetMostGenericOsmId(), base::MakeOsmWay(1), ());

  fb.AddOsmId(base::MakeOsmNode(3));
  fb.AddOsmId(base::MakeOsmWay(2));
  fb.AddOsmId(base::MakeOsmRelation(1));
  TEST_EQUAL(fb.GetMostGenericOsmId(), base::MakeOsmRelation(1), ());
}

UNIT_CLASS_TEST(TestWithClassificator, FVisibility_RemoveUselessTypes)
{
  Classificator const & c = classif();

  {
    vector<uint32_t> types;
    types.push_back(c.GetTypeByPath({ "building" }));
    types.push_back(c.GetTypeByPath({ "amenity", "theatre" }));

    TEST(RemoveUselessTypes(types, GeomType::Area), ());
    TEST_EQUAL(types.size(), 2, ());
  }

  {
    vector<uint32_t> types;
    types.push_back(c.GetTypeByPath({ "highway", "primary" }));
    types.push_back(c.GetTypeByPath({ "building" }));

    TEST(RemoveUselessTypes(types, GeomType::Area, true /* emptyName */), ());
    TEST_EQUAL(types.size(), 1, ());
    TEST_EQUAL(types[0], c.GetTypeByPath({ "building" }), ());
  }
}

UNIT_CLASS_TEST(TestWithClassificator, FBuilder_RemoveUselessNames)
{
  FeatureParams params;

  char const * arr3[][3] = { { "boundary", "administrative", "2" } };
  AddTypes(params, arr3);
  char const * arr2[][2] = { { "barrier", "fence" } };
  AddTypes(params, arr2);
  params.FinishAddingTypes();

  params.AddName("default", "Name");
  params.AddName("ru", "Имя");

  FeatureBuilder fb1;
  fb1.SetParams(params);

  fb1.AddPoint(m2::PointD(0, 0));
  fb1.AddPoint(m2::PointD(1, 1));
  fb1.SetLinear();

  TEST(!fb1.GetName(0).empty(), ());
  TEST(!fb1.GetName(8).empty(), ());

  fb1.RemoveUselessNames();

  TEST(fb1.GetName(0).empty(), ());
  TEST(fb1.GetName(8).empty(), ());

  Check(fb1);
}

UNIT_CLASS_TEST(TestWithClassificator, FeatureParams_Parsing)
{
  {
    FeatureParams params;
    params.AddStreet("Embarcadero\nstreet");
    TEST_EQUAL(params.GetStreet(), "Embarcadero street", ());
  }

  {
    FeatureParams params;
    params.AddAddress("165 \t\t Dolliver Street");
    TEST_EQUAL(params.GetStreet(), "Dolliver Street", ());
  }

  {
    FeatureParams params;

    params.MakeZero();
    TEST(params.AddHouseNumber("123"), ());
    TEST_EQUAL(params.house.Get(), "123", ());

    params.MakeZero();
    TEST(params.AddHouseNumber("0000123"), ());
    TEST_EQUAL(params.house.Get(), "123", ());

    params.MakeZero();
    TEST(params.AddHouseNumber("000000"), ());
    TEST_EQUAL(params.house.Get(), "0", ());
  }
}

UNIT_CLASS_TEST(TestWithClassificator, FeatureBuilder_SerializeCoveredObjectForBuildingPoint)
{
  FeatureBuilder fb;
  FeatureParams params;

  char const * arr1[][1] = {
    { "building" },
  };
  AddTypes(params, arr1);

  params.FinishAddingTypes();
  params.AddHouseNumber("75");
  params.AddHouseName("Best House");
  params.AddName("default", "Name");

  fb.AddOsmId(base::MakeOsmNode(1));
  fb.SetParams(params);
  fb.SetCenter(m2::PointD(10.1, 15.8));

  TEST(fb.RemoveInvalidTypes(), ());
  Check(fb);

  feature::DataHeader header;
  header.SetGeometryCodingParams(serial::GeometryCodingParams());
  header.SetScales({scales::GetUpperScale()});
  feature::GeometryHolder holder(fb, header, std::numeric_limits<uint32_t>::max() /* maxTrianglesNumber */);

  auto & buffer = holder.GetBuffer();
  TEST(fb.PreSerializeAndRemoveUselessNamesForMwm(buffer), ());
  fb.SerializeCoveredObject(serial::GeometryCodingParams(), buffer);

  using indexer::CoveredObject;
  CoveredObject object;
  object.Deserialize(buffer.m_buffer.data());

  TEST_EQUAL(CoveredObject::FromStoredId(object.GetStoredId()), base::MakeOsmNode(1), ());
  object.ForEachPoint([] (auto && point) {
    TEST(base::AlmostEqualAbs(point, m2::PointD(10.1, 15.8), 1e-7), ());
  });
}

UNIT_CLASS_TEST(TestWithClassificator, FeatureBuilder_SerializeCoveredObjectForLine)
{
  FeatureBuilder fb;
  FeatureParams params;

  char const * arr1[][2] = {
    { "highway", "residential"},
  };
  AddTypes(params, arr1);
  params.FinishAddingTypes();
  params.AddName("default", "Arbat Street");

  fb.AddOsmId(base::MakeOsmNode(1));
  fb.SetParams(params);
  fb.SetLinear();
  fb.AddPoint(m2::PointD(10.1, 15.8));
  fb.AddPoint(m2::PointD(10.2, 15.9));
  fb.AddPoint(m2::PointD(10.4, 15.9));

  TEST(fb.RemoveInvalidTypes(), ());
  Check(fb);

  feature::DataHeader header;
  header.SetGeometryCodingParams(serial::GeometryCodingParams());
  header.SetScales({scales::GetUpperScale()});
  feature::GeometryHolder holder(fb, header,
                                 std::numeric_limits<uint32_t>::max() /* maxTrianglesNumber */);
  holder.AddPoints(holder.GetSourcePoints(), 0 /* scaleIndex */);

  auto & buffer = holder.GetBuffer();
  auto preserialize = fb.PreSerializeAndRemoveUselessNamesForMwm(buffer);
  CHECK(preserialize, ());
  fb.SerializeCoveredObject(serial::GeometryCodingParams(), buffer);

  using indexer::CoveredObject;
  CoveredObject object;
  object.Deserialize(buffer.m_buffer.data());

  TEST_EQUAL(CoveredObject::FromStoredId(object.GetStoredId()), base::MakeOsmNode(1), ());
  auto localityObjectsPoints = std::vector<m2::PointD>{};
  object.ForEachPoint([&] (auto && point) {
    localityObjectsPoints.push_back(point);
  });
  TEST_EQUAL(localityObjectsPoints.size(), 3, ());
  TEST(base::AlmostEqualAbs(localityObjectsPoints[0], {10.1, 15.8}, 1e-6), ());
  TEST(base::AlmostEqualAbs(localityObjectsPoints[1], {10.2, 15.9}, 1e-6), ());
  TEST(base::AlmostEqualAbs(localityObjectsPoints[2], {10.4, 15.9}, 1e-6), ());
}

UNIT_TEST(FeatureBuilder_SerializeAccuratelyForIntermediate)
{
  FeatureBuilder fb1;
  FeatureParams params;
  classificator::Load();

  char const * arr2[][2] = {
    { "railway", "rail" },
    { "highway", "motorway" },
    { "hwtag", "oneway" },
    { "psurface", "paved_good" },
    { "junction", "roundabout" },
  };

  AddTypes(params, arr2);
  params.FinishAddingTypes();
  fb1.SetParams(params);

  auto const diff = 0.33333333334567;
  for (size_t i = 0; i < 100; ++i)
      fb1.AddPoint(m2::PointD(i + diff, i + 1 + diff));

  fb1.SetLinear();

  TEST(fb1.RemoveInvalidTypes(), ());
  Check(fb1);

  FeatureBuilder::Buffer buffer;
  TEST(fb1.PreSerializeAndRemoveUselessNamesForIntermediate(), ());
  fb1.SerializeAccuratelyForIntermediate(buffer);

  FeatureBuilder fb2;
  fb2.DeserializeAccuratelyFromIntermediate(buffer);

  Check(fb2);
  TEST(fb1.IsExactEq(fb2), ());
}
