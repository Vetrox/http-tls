#pragma once

#include "numutils.h"

struct IssuerInfo {
    std::string country_name {};
    std::string organization_name {};
    std::string organizational_unit_name {};
    std::string common_name {};
};

struct SubjectInfo {
    std::string country_name {};
    std::string state_name {}; // or province name
    std::string locality_name {};
    std::string organization_name {};
    std::string organizational_unit_name {};
    std::string common_name {};
};

struct PublicKey {
    std::string algorithm_type;
    UnsignedBigInt modulus;
    UnsignedBigInt exponent;
};

struct Signature {
    std::string algorithm_type;
    UnsignedBigInt data;
};

struct X509v3 {
    UnsignedBigInt version;
    UnsignedBigInt serial_number;
    std::string signature_algorithm;
    IssuerInfo issuer_info;
    std::string valid_not_before;
    std::string valid_not_after;
    SubjectInfo subject_info;
    PublicKey public_key;
    // TODO: consider adding extensions
    Signature signature;
    std::vector<uint8_t> raw_bytes;
};
