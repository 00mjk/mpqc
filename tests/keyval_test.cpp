#include <vector>
#include <sstream>

#include <boost/serialization/export.hpp>

#include <mpqc/util/keyval/keyval.hpp>
#include <catch.hpp>

using std::cout;
using std::endl;
using std::vector;
using std::string;
using std::stringstream;
using mpqc::KeyVal;
using mpqc::DescribedClass;

struct Base : public DescribedClass {
    Base(const KeyVal& kv) : DescribedClass(), value_(kv.value<int>("value")) {}
    virtual ~Base() {}

    int value_;
    int value() const { return value_; }

  private:
    static DescribedClass::registrar<Base> reg_;
};
DescribedClass::registrar<Base> Base::reg_("Base");

template <size_t tag>
struct Derived : public Base {
    Derived(const KeyVal& kv) : Base(kv), value_(kv.value<double>("dvalue")) {}
    ~Derived() {
      // force initialization of reg_
      bool x = (&reg_ == 0);
    }

    double value_;
    double value() const { return value_; }

  private:
    static DescribedClass::registrar<Derived> reg_;
};
template <size_t tag>
DescribedClass::registrar<Derived<tag>> Derived<tag>::reg_(std::string("Derived<") + std::to_string(tag) + std::string(">"));

// this forces instantiation of Derived<0>, only needed because Derived<0> does not appear
// anywhere else
BOOST_CLASS_EXPORT_IMPLEMENT(Derived<0>)

TEST_CASE("KeyVal", "[keyval]"){

  // setup tests programmatic construction

  KeyVal kv;

  kv.assign ("x", 0);
  REQUIRE(kv.value<int>("x") == 0);

  kv.assign (":x", "0");
  REQUIRE(kv.value<string>(":x") == "0");
  REQUIRE(kv.value<int>(":x") == 0);

  kv.assign (":z:0", true).assign (":z:1", -1.75);

  REQUIRE(kv.value<string>(":z:0") == "true");
  REQUIRE_THROWS(kv.value<int>(":z:0")); // cannot obtain bool as int
  REQUIRE(kv.value<bool>(":z:0") == true);
  REQUIRE(kv.value<float>(":z:1") == -1.75);
  REQUIRE(kv.value<double>(":z:1") == -1.75);

  kv.assign (":z:a:0", vector<int> ( {0, 1, 2}));
  //REQUIRE_THROWS(kv.value<vector<int>>(":z:a:0")); // not yet implemented

  SECTION("JSON read/write"){
    stringstream oss;
    REQUIRE_NOTHROW(kv.write_json(oss));
    //cout << oss.str();
  }

  SECTION("JSON read/write"){
    stringstream oss;
    REQUIRE_NOTHROW(kv.write_xml(oss));
    //cout << oss.str();
  }

  SECTION("making subtree KeyVal"){
    auto kv_z = kv.keyval (":z");
    REQUIRE(kv_z.value<bool> ("0") == true);
    REQUIRE(kv_z.value<double> ("1") == -1.75);
  }

  SECTION("make classes"){

    {
      KeyVal kv;
      kv.assign("value", 1).assign("type", "Base");
      auto x1 = kv.class_ptr<Base>();
      REQUIRE(x1->value() == 1);
      auto x2 = kv.class_ptr<Base>(); // this returns the cached ptr
      REQUIRE(x1 == x2);
    }
    {
      KeyVal kv;
      kv.assign("value", 2).assign("dvalue", 2.0).assign("type", "Derived<0>");
      auto x1 = kv.class_ptr<Derived<0>>();
      REQUIRE(x1->value() == 2.0);
      auto x2 = kv.class_ptr<Derived<0>>(); // this returns the cached ptr
      REQUIRE(x1 == x2);
      auto x3 = kv.class_ptr<Base>(); // this returns the cached ptr, cast to Base
      REQUIRE(std::dynamic_pointer_cast<Base>(x1) == x3);
    }
  }

  SECTION("check references"){
    KeyVal kv;
    kv.assign ("i1", 1);
    kv.assign ("i2:a", true);
    kv.assign ("i2:b", 2);
    kv.assign ("i3", "$:i2:a");
    kv.assign ("i4", "$:i2:..:i1");
    kv.assign ("i5", "$i2:b");
    kv.assign ("i2:c", "$..:i3");

    kv.assign("c1:type", "Base").assign("c1:value", 1);
    kv.assign ("i2:c2", "$:c1");

//    stringstream oss;
//    REQUIRE_NOTHROW(kv.write_json(oss));
//    cout << oss.str();

    REQUIRE(kv.value<int>("i4") == 1);
    REQUIRE(kv.value<int>("i5") == 2);
    REQUIRE(kv.value<bool>("i2:c") == true);

    auto kv_c1 = kv.keyval("c1");
    auto c1 = kv_c1.class_ptr<Base>();
    REQUIRE(c1->value() == 1);
    auto kv_c2 = kv.keyval("i2:c2");
    auto c2 = kv_c2.class_ptr<Base>(); // returns cached ptr
    REQUIRE(c1 == c2);
  }

  SECTION("read complex JSON"){

    KeyVal kv;

    const char input[] =       \
"{                             \
  \"a\":\"0\",                 \
  \"b\":\"1.25\",              \
  \"base\": {                  \
     \"type\":\"Base\",        \
     \"value\":\"$:a\"         \
  },                           \
  \"deriv0\": {                \
     \"type\":\"Derived<0>\",  \
     \"value\":\"$:a\",        \
     \"dvalue\":\"$:b\"        \
  },                           \
  \"mpqc\": {                  \
     \"base\":\"$..:base\",    \
     \"deriv\":\"$:deriv0\"    \
  }                            \
}";

    stringstream iss(input);
    REQUIRE_NOTHROW(kv.read_json(iss));

//    stringstream oss;
//    REQUIRE_NOTHROW(kv.write_json(oss));
//    cout << oss.str();

    auto b1 = kv.keyval("mpqc:base").class_ptr<Base>();
    auto b2 = kv.keyval("base").class_ptr<Base>();
    REQUIRE(b1 == b2);
    Derived<0> x(kv.keyval("mpqc:deriv"));
    auto d1 = kv.keyval("mpqc:deriv").class_ptr<Derived<0>>();
    auto d2 = kv.keyval("deriv0").class_ptr<Derived<0>>();
    REQUIRE(d1 == d2);
    REQUIRE(d1->value() == kv.value<double>("b"));

  }

} // end of test case

