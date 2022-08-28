#include "certparser.h"
#include <iostream>
#include <iomanip>
#include <sstream>


IDClass id_class(uint8_t octet) {
    octet &= 0b1100'0000;
    return (IDClass) octet;
}

std::string classname(uint8_t octet) {
    switch (id_class(octet)) {
        case IDClass::Universal:
            return "Universal";
        case IDClass::Application:
            return "Application";
        case IDClass::Context_specific:
            return "Context specific";
        case IDClass::Private:
            return "Private";
        default:
            return "UNKNOWN CLASS TYPE";
    }
}


Encoding id_encoding(uint8_t octet) {
    octet &= 0b0010'0000;
    return (Encoding) octet;
}

std::string encoding_name(uint8_t octet) {
    switch (id_encoding(octet)) {
        case Encoding::Primitive:
            return "Primitive";
        case Encoding::Constructed:
            return "Constructed";
        default:
            return "UNKNOWN CLASS TYPE";
    }
}



Tag id_tag(uint8_t octet) {
    octet &= 0b0001'1111;
    return (Tag) octet;
}



enum LengthType {
    Short = 0b0000'0000,
    Long = 0b1000'0000
};

LengthType length_type(uint8_t octet) {
    octet &= 0b1000'0000;
    return (LengthType) octet;
}

uint8_t short_length(uint8_t octet) {
    if (length_type(octet) != LengthType::Short) {
        std::cout << "ERROR: Trying to decode a long type length octet as a short one" << std::endl;
        abort();
    }
    return octet;
}

uint64_t long_length(std::span<uint8_t> octets, int* index) {
    if ((octets[0] & 0b1000'0000) == 0) {
        std::cout << "ERROR: Malformed Long-Length octet" << std::endl;
        abort();
    }
    uint8_t subsequent_length_octets = octets[0] & 0b0111'1111;
    *index += 1;
    if (subsequent_length_octets == 0b0111'1111) {
        std::cout << "ERROR: Use of reserved Long-Length octet length specifier value";
        abort();
    }
    if (subsequent_length_octets > 8) {
        std::cout << "ERROR: System is not capable of handling " << (int) subsequent_length_octets << " subsequent length octets" << std::endl; 
        abort();
    }
    *index += subsequent_length_octets;
    uint64_t length = 0;
    for (int i = 0; i < subsequent_length_octets; i++) {
        length |= octets[i + 1] << ((subsequent_length_octets - i - 1) * 8);
    }

    return length;
}

uint64_t parse_length(std::span<uint8_t> octets, int* index) {
    if (length_type(octets[0]) == LengthType::Short) {
        *index += 1;
        return short_length(octets[0]);
    }
    
    // NOTE: Distinguished Encoding Rules (DER) asserts this to have the "definite"-Form
    
    return long_length(octets, index);
}

/*
 * FIXME: possibly incomplete. see section 11 "Restrictions on BER employed by both CER and DER"
 * Distinguished Encoding Rules:
 * - Use "definite"-Length form 
 * - Bitstring, octetstring, and restricted characterstring shall use primitive encoding. 
 * - The order of Set members shall be in ascending tag-id 
 */


/*
 * Integer
Sequence
Object
Null
Set
Pritnablestring
utf8string
utctime
bitstring
octetstring
 *
 */

bool parse_boolean(uint8_t octet) { // just as c
    if (octet != 0) {
        return true;
    } else {
        return false;
    }
}

std::span<uint8_t> parse_integer(std::span<uint8_t> octets) {
    return octets;
}

std::span<uint8_t> parse_enum(std::span<uint8_t> octets) {
    return parse_integer(octets);
}

std::span<uint8_t> parse_object(std::span<uint8_t> octets) {
    return octets; // FIXME: unclear what section 8.19 means
}

std::string parse_string(std::span<uint8_t> octets) {
    return std::string(octets.begin(), octets.end());
}

// parse_real unsupported


std::string as_hex(std::vector<uint8_t> octets) {
    std::stringstream s;
    s << std::hex << std::setfill('0');
    bool first = true;
    for (auto& octet : octets) {
        if (!first) s << ":";
        first = false;
        s << std::setw(2) << (int) octet;
    }
    s << std::dec;
    return s.str();
}

void print_octet(uint8_t octet, size_t indent = 0) {
    for (int i = 0; i < indent; i++) {
        std::cout << " ";
    }
    std::cout << "CLASS: " << classname(octet) << " (" << id_class(octet) << "), "
        << "ENCOD: " << encoding_name(octet) << " (" << id_encoding(octet) << "), "
        << "TAG: " << tagname[id_tag(octet)] << " (" << id_tag(octet) << ")";
}


IDToken parse_id_octet(uint8_t octet) {
    return {
        .id_class = id_class(octet),
        .id_encoding = id_encoding(octet),
        .id_tag = id_tag(octet)
    };
}


void print_indent(size_t num) {
    for (int i; i < num; i++) {
        std::cout << " ";
    }
}

std::string parse_oid(std::span<uint8_t> octets) {
    // take first byte
    
    std::string oid = "";
    oid += std::to_string(octets[0] / 40) + "." + std::to_string(octets[0] % 40);

    int i_start = 1;
    int i_cur = 1;
    while (true) {
        while (true) {
            if (i_cur >= octets.size()) {
                return oid;
            }
            if ((octets[i_cur++] & 0b1000'0000) == 0) break;
        }

        if ((i_cur - i_start) <= 0) {
            return oid;
        }

        if ((i_cur - i_start) > 8) {
            std::cout << "ERROR: Oid with more than 64 bit number" << std::endl;
            abort();
        }

        uint64_t num = 0;
        for (int i = 0; i < (i_cur - i_start); i++) {
            num <<= 7;
            num |= ((uint64_t) (octets[i_start+i] & 0b0111'1111));
        }
        oid += "." + std::to_string(num);
        i_start = i_cur;
    }
    // unreachable
}


std::vector<ASNObj>* parse_impl(std::span<uint8_t> data) {
    auto* objs = new std::vector<ASNObj>();
    for (int i = 0; i < data.size();) {
        IDToken id = parse_id_octet(data[i]);
        i++;
        
        uint64_t length = parse_length(std::span(data.begin() + i, data.end()), &i);
        std::span<uint8_t> span = std::span<uint8_t>(data.begin() + i, data.begin() + i + length);
      
        void* content;
        if (id.id_encoding == Encoding::Primitive) {
            if (id.id_tag == Tag::OBJECT_IDENTIFIER) {
                std::string oid = parse_oid(span);
                if (oid_name.find(oid) != oid_name.end()) {
                    oid = oid_name.at(oid) + " (" + oid + ")";
                }
                auto* v = new std::vector<uint8_t>(oid.begin(), oid.end());
                content = v;
            } else {
                content = new std::vector<uint8_t>(span.begin(), span.end());
            }
        } else {
            content = parse_impl(span);
        }
        i += length;

        objs->emplace_back(ASNObj(id, length, content));
    }

    return objs;
}

std::vector<ASNObj> parse_raw(std::span<uint8_t> data) {
    auto* parsed = parse_impl(data);
    
    if (!parsed) {
        std::cout << "ERROR: PARSE_IMPL returned null";
    }

    return *parsed;
}

X509v3 parse(std::span<uint8_t> data) {
    auto asnobjs = parse_raw(data);
    if (asnobjs.size() != 1) {
        std::cout << "ERROR: Unexpected number of ASN Objects to be interpretet as one certficate. Amount: " 
            << std::to_string(asnobjs.size()) << std::endl;
        abort();
    }

    ASNObj cert_and_certSigAlg_and_certSig = asnobjs[0];
    auto sequence = cert_and_certSigAlg_and_certSig.as_ASNObjs()[0].as_ASNObjs();
    if (sequence.size() != 8) {
        std::cout << "ERROR: Certificate Contents include not exactly 8 Members." << std::endl;
        abort();
    }
    
    X509v3 v3;
    
    v3.version = sequence[0].as_ASNObjs()[0].as_integer();
    if (v3.version != UnsignedBigInt(2)) {
       std::cout << "ERROR: Version mismatch. expected version number 2 (v3), got version number " << v3.version.as_decimal() << std::endl;
        abort();
    }

    v3.serial_number = sequence[1].as_integer();
   
    v3.signature_algorithm = sequence[2].as_ASNObjs()[0].as_string();

    if (v3.signature_algorithm != "sha256WithRSAEncryption (1.2.840.113549.1.1.11)") {
        std::cout << "ERROR: Signature algorithm type mismatch got: " << v3.signature_algorithm << std::endl;
        abort();
    }
    // FIXME: parse rest of signature algorithm info depending on algorithm type
    
    IssuerInfo issuer_info;
    auto issuerInfo = sequence[3].as_ASNObjs();
    auto in = issuerInfo[0].as_ASNObjs()[0].as_ASNObjs();
    if (in[0].as_string() != "countryName (2.5.4.6)") {
        std::cout << "ERROR: countryName" << std::endl;
        abort();
    }
    issuer_info.country_name = in[1].as_string();
    
    in = issuerInfo[1].as_ASNObjs()[0].as_ASNObjs();
    if (in[0].as_string() != "organizationName (2.5.4.10)") {
        std::cout << "ERROR: organizationName" << std::endl;
        abort();
    }
    issuer_info.organization_name = in[1].as_string();
    
    in = issuerInfo[2].as_ASNObjs()[0].as_ASNObjs();
    if (in[0].as_string() != "organizationalUnitName (2.5.4.11)") {
        std::cout << "ERROR: organizationalUnitName" << std::endl;
        abort();
    }
    issuer_info.organizational_unit_name = in[1].as_string();
    
    in = issuerInfo[3].as_ASNObjs()[0].as_ASNObjs();
    if (in[0].as_string() != "commonName (2.5.4.3)") {
        std::cout << "ERROR: commonName" << std::endl;
        abort();
    }
    issuer_info.common_name = in[1].as_string();

    v3.issuer_info = std::move(issuer_info);


    auto validity = sequence[4].as_ASNObjs();
    v3.valid_not_before = validity[0].as_utc();
    v3.valid_not_after =  validity[1].as_utc();

    SubjectInfo subject_info;
    for (auto const& info : sequence[5].as_ASNObjs()) {
        auto const& pair = info.as_ASNObjs()[0].as_ASNObjs();
        auto const& oid = pair[0].as_string();
        auto const& value = pair[1].as_string();
        if (oid == "countryName (2.5.4.6)") {
            subject_info.country_name = value;
        } else if(oid == "stateOrProvinceName (2.5.4.8)") {
            subject_info.state_name = value;
        } else if(oid == "localityName (2.5.4.7)") {
            subject_info.locality_name = value;
        } else if(oid == "organizationName (2.5.4.10)") {
            subject_info.organization_name = value;
        } else if(oid == "organizationalUnitName (2.5.4.11)") {
            subject_info.organizational_unit_name = value;
        } else if(oid == "commonName (2.5.4.3)") {
            subject_info.common_name = value;
        } else {
            std::cout << "unrecognized Identifier: " << oid << std::endl;
            abort(); // maybe ignore
        }
    }
    v3.subject_info = std::move(subject_info);

    PublicKey pubkey_info;
    auto subjectPublicKeyInfo = sequence[6].as_ASNObjs();
    auto subjectAlorithmIdentier = subjectPublicKeyInfo[0].as_ASNObjs()[0].as_string();
    if (subjectAlorithmIdentier != "rsaEncryption (1.2.840.113549.1.1.1)") {
        std::cout << "ERROR: publicKeyInfo: algorithm type mismatch" << std::endl;
        abort();
    }
    pubkey_info.algorithm_type = subjectAlorithmIdentier;
        // FIXME: parse rest of algorithm parameters depending on type
    auto pubKeyOctets = subjectPublicKeyInfo[1].as_octets(); // FIXME: this is the publicKEY. IT HAS AN OWN RSAPUBKEY ASN1 STRUCTURE.
    if (pubKeyOctets.at(0) != 0) {
        std::cout << "ERROR: empirically discovered padding 00 octet is not in public key." << std::endl;
        abort();
    }
    auto parsedPubKey = parse_raw(std::span<uint8_t>(pubKeyOctets.begin()+1, pubKeyOctets.end())).at(0).as_ASNObjs();
    pubkey_info.modulus = parsedPubKey.at(0).as_integer();
    pubkey_info.exponent = parsedPubKey.at(1).as_integer();

    // FIXME: handle extensions. sequence[7].to_string()
    v3.public_key = std::move(pubkey_info);

    Signature sig_info;
    auto sigAlg = cert_and_certSigAlg_and_certSig.as_ASNObjs()[1].as_ASNObjs()[0].as_string();
    if (sigAlg != "sha256WithRSAEncryption (1.2.840.113549.1.1.11)") {
        std::cout << "ERROR: signature algorithm mismatch" << std::endl;
        abort();
    }
    sig_info.algorithm_type = sigAlg;
    sig_info.data = cert_and_certSigAlg_and_certSig.as_ASNObjs()[2].as_integer();

    v3.signature = std::move(sig_info);

    return std::move(v3);
}
    // TODO: verify the sha256 hash of the cerifificate with the (publicModulus, publicExponent) of the CA one above this certificate
