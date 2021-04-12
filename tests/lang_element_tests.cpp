#define CATCH_CONFIG_ENABLE_BENCHMARKING
#include "samal_lib/AST.hpp"
#include "samal_lib/Compiler.hpp"
#include "samal_lib/ExternalVMValue.hpp"
#include "samal_lib/Parser.hpp"
#include "samal_lib/VM.hpp"
#include <catch2/catch.hpp>
#include <charconv>
#include <iostream>

using namespace samal;

TEST_CASE("Ensure that template type inference works 1", "[samal_element_tests]") {
    Datatype incompleteType = Datatype::createTupleType({ Datatype::createUndeterminedIdentifierType("T"), Datatype::createUndeterminedIdentifierType("S") });
    Datatype fullType = Datatype::createTupleType({ Datatype::createSimple(samal::DatatypeCategory::i32), Datatype::createSimple(samal::DatatypeCategory::i64)});
    UndeterminedIdentifierReplacementMap map;
    incompleteType.inferTemplateTypes(fullType, map, {});
    REQUIRE(map.size() == 2);
    REQUIRE(map.at("T").type == Datatype::createSimple(samal::DatatypeCategory::i32));
    REQUIRE(map.at("S").type == Datatype::createSimple(samal::DatatypeCategory::i64));
}

TEST_CASE("Ensure that template type inference works 2", "[samal_element_tests]") {
    Datatype incompleteType = Datatype::createTupleType({ Datatype::createUndeterminedIdentifierType("T"), Datatype::createUndeterminedIdentifierType("S") });
    Datatype fullType = Datatype::createSimple(samal::DatatypeCategory::i64);
    UndeterminedIdentifierReplacementMap map;
    REQUIRE_THROWS(incompleteType.inferTemplateTypes(fullType, map, {}));
}