#include <cstdint>

#include <span>
#include <unordered_map>
#include <string>
#include <stdexcept>
#include <vector>

#include "numutils.h"
#include "cert.h"

class ASNObj;
std::vector<uint8_t> chop_decrypted_signature(std::vector<uint8_t>);
std::vector<ASNObj> parse_raw(std::span<uint8_t>);
X509v3 parse(std::span<uint8_t>);
std::string as_hex(std::vector<uint8_t>);

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


// from: https://www.itu.int/rec/T-REC-X.690-202102-I/en
enum IDClass {
    Universal = 0b0000'0000,
    Application = 0b0100'0000,
    Context_specific = 0b1000'0000,
    Private = 0b1100'0000
};

enum Encoding {
    Primitive = 0b0000'0000, // no eol
    Constructed = 0b0010'0000 // with end-of-line octets
};

// FIXME: this is not complete: use Table 1: Universal class tag assignments of ITU-T X.680 | ISO/IEC 8824-1
// 31 = comprise leading octet followed by one or more subsequent octets (curr. unsupportet)
enum Tag {
    Reserved = 0,
    BOOLEAN,
    INTEGER,
    BIT_STRING,
    OCTET_STRING,
    NIL, // NULL
    OBJECT_IDENTIFIER,
    ObjectDescriptor,
    INSTANCE_OF, // EXTERNAL
    REAL,
    ENUMERATED,
    EMBEDDED_PDV,
   	UTF8String,
   	RELATIVE_OID,
   	SEQUENCE = 16, // SEQUENCE OF
   	SET, // SET OF
   	NumericString,
   	PrintableString,
   	TeletexString, // T61String
   	VideotexString,
   	IA5String,
   	UTCTime,
   	GeneralizedTime,
   	GraphicString,
   	VisibleString, // ISO646
   	GeneralString,
    UniversalString,
    CHARACTER_STRING,
 	BMPString,
    ComposeUnsupported = 0b1'1111
};

static std::string tagname[32] {
    "Reserved",
    "BOOLEAN",
    "INTEGER",
    "BIT_STRING",
    "OCTET_STRING",
    "NIL", 
    "OBJECT_IDENTIFIER",
    "ObjectDescriptor",
    "INSTANCE_OF", 
    "REAL",
    "ENUMERATED",
    "EMBEDDED_PDV",
   	"UTF8String",
   	"RELATIVE_OID",
    "NotATag",
    "NotATag",
    "SEQUENCE",
   	"SET", 
   	"NumericString",
   	"PrintableString",
   	"TeletexString",
   	"VideotexString",
   	"IA5String",
   	"UTCTime",
   	"GeneralizedTime",
   	"GraphicString",
   	"VisibleString",
   	"GeneralString",
    "UniversalString",
    "CHARACTER_STRING",
 	"BMPString",
    "ComposeUnsupported"
};

struct IDToken {
    IDClass id_class;
    Encoding id_encoding;
    Tag id_tag;
};

class ASNObj {
public:
    ASNObj(std::vector<uint8_t> asn_raw_bytes, IDToken id, uint64_t length, void* content) : 
        m_asn_raw_bytes(std::move(asn_raw_bytes)),
        m_id(std::move(id)),
        m_length(length),
        m_content(content)
    {
    }

    bool is_primitive() const {
        return m_id.id_encoding == Encoding::Primitive;
    }

    bool is_string() const {
        return m_id.id_tag == Tag::UTF8String || m_id.id_tag == Tag::PrintableString;
    }

    bool is_oid() const {
        return m_id.id_tag == Tag::OBJECT_IDENTIFIER;
    }

    bool is_utc() const {
        return m_id.id_tag == Tag::UTCTime;
    }

    bool is_integer() const {
        return m_id.id_tag == Tag::INTEGER;
    }

    bool is_null() const {
        return m_id.id_tag == Tag::NIL;
    }

    std::string as_utc() {
        if (!is_primitive()) throw 444;
        if (!is_utc()) throw 445;

        // YYMMDD000000Z. len = 13
        // FIXME: other sources say YYMMDDhhmm[ss]Z or YYMMDDhhmm[ss](+|-)hhmm
        auto octets = as_octets();
        if (octets.size() != 13) throw 446;
        if (octets[12] != (uint8_t) 'Z') throw 447;
        
        std::string out = "";
        out += octets[4];
        out += octets[5];
        out += ".";
        out += octets[2];
        out += octets[3];
        out += ".";
        
        // RFC5280 section 4.1.2.5.1.
        out += (octets[0] < '5') ? "20" : "19";
        out += octets[0];
        out += octets[1];

        return out;
    }

    std::string as_string() const {
        if (!is_primitive()) throw 444;
        if (!is_string() && !is_oid()) throw 445;

        auto octets = as_octets();
        return std::string(octets.begin(), octets.end());
    }

    UnsignedBigInt as_integer() const { // NOTE: Big endian (network endianess)
        if (!is_primitive()) throw 444;
        // if (!is_integer()) throw 445;

        auto data = *(std::vector<uint8_t>*)m_content;
        return UnsignedBigInt(std::vector<uint8_t>(data.rbegin(), data.rend())); // FIXME: allow signed numbers
    }

    std::vector<uint8_t> as_octets() const {
        if (!is_primitive()) throw 444;

        return *(std::vector<uint8_t>*) m_content;
    }

    std::vector<ASNObj> as_ASNObjs() const {
        if (is_primitive()) throw 444;
        
        return *(std::vector<ASNObj>*) m_content;
    }

    std::vector<uint8_t> raw_bytes() const {
        return m_asn_raw_bytes;
    }

    std::string to_string() {
        std::string s = "";
        // s += is_primitive() ? "pri" : "con";
        // s += " ";
        if (is_primitive()) {
            if (is_string() || is_oid()) {
                s += (is_oid() ? "oid" : "str");
                s += ": " + as_string();
            } else if (is_integer()){
                s += (tagname[m_id.id_tag]);
                s += ":";
                s += as_integer().as_decimal();
            } else if (is_utc()) {
                s += "time: ";
                s += as_utc();
            } else if (is_null()) {
                s += "NULL";
            } else {
                s += "raw: ";
                s += as_hex(as_octets());
            }
        } else {
            s += "list( ";
            for (ASNObj obj : as_ASNObjs()) {
                s += obj.to_string();
                s += " ";
            }
            s += ")";
        }
        return s;
    }

    ~ASNObj() { /*
        if (!is_primitive()) {
            delete (std::vector<ASNObj>*) m_content;
        }
        delete (std::vector<uint8_t>*) m_content; */
    }
private:
    IDToken m_id;
    uint64_t m_length;
    void* m_content;
    std::vector<uint8_t> m_asn_raw_bytes;
};
