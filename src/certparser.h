#include <cstdint>

#include <span>
#include <unordered_map>
#include <string>

void parse(std::span<uint8_t>, size_t indent = 0);


static std::unordered_map<std::string, std::string> oid_name = {
    {"1.2.840.113549.1.1.11", "sha256WithRSAEncryption"},
    {"1.2.840.113549.1.1.1", "rsaEncryption"},
    {"1.3.6.1.4.1.11129.2.4.2", "CT Precertificate SignedCertificateTimestamps"},
    {"1.3.6.1.5.5.7.1.1", "authorityInfoAccess"},
    {"2.5.4.3", "commonName"},
    {"2.5.4.6", "countryName"},
    {"2.5.4.7", "localityName"},
    {"2.5.4.8", "stateOrProvinceName"},
    {"2.5.4.10", "organizationName"},
    {"2.5.4.11", "organizationalUnitName"},
    {"2.5.29.14", "subjectKeyIdentifier"},
    {"2.5.29.15", "keyUsage"},
    {"2.5.29.17", "subjectAltName"},
    {"2.5.29.19", "basicConstraints"},
    {"2.5.29.32", "certificatePolicies"},
    {"2.5.29.35", "authorityKeyIdentifier"},
    {"2.5.29.37", "extKeyUsage"},
    {"2.5.29.31", "cRLDistributionPoints"}
};
