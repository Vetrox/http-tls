#include "certparser.h"
#include <iostream>
#include <iomanip>

// from: https://www.itu.int/rec/T-REC-X.690-202102-I/en
enum IDClass {
    Universal = 0b0000'0000,
    Application = 0b0100'0000,
    Context_specific = 0b1000'0000,
    Private = 0b1100'0000
};


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

enum Encoding {
    Primitive = 0b0000'0000, // no eol
    Constructed = 0b0010'0000 // with end-of-line octets
};

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

Tag id_tag(uint8_t octet) {
    octet &= 0b0001'1111;
    return (Tag) octet;
}


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


void print_hex(std::span<uint8_t> octets) {
    std::cout << std::hex << std::setfill('0');
    bool first = true;
    for (auto& octet : octets) {
        if (!first) std::cout << ":";
        first = false;
        std::cout << std::setw(2) << (int) octet;
    }
    std::cout << std::dec << std::endl;
}

void print_octet(uint8_t octet, size_t indent = 0) {
    for (int i = 0; i < indent; i++) {
        std::cout << " ";
    }
    std::cout << "CLASS: " << classname(octet) << " (" << id_class(octet) << "), "
        << "ENCOD: " << encoding_name(octet) << " (" << id_encoding(octet) << "), "
        << "TAG: " << tagname[id_tag(octet)] << " (" << id_tag(octet) << ")";
}

struct IDToken {
    IDClass id_class;
    Encoding id_encoding;
    Tag id_tag;
};

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

void print_oid(std::span<uint8_t> octets) {
    // take first byte
    
    std::string oid = "";
    oid += std::to_string(octets[0] / 40) + "." + std::to_string(octets[0] % 40);

    int i_start = 1;
    int i_cur = 1;
    while (true) {
        while (true) {
            if (i_cur >= octets.size()) {
                if (oid_name.find(oid) != oid_name.end()) {
                    std::cout << oid_name[oid] << " (" << oid << ")" << std::endl;
                } else {
                    std::cout << "WARNING: Could not decode oid " << oid << std::endl;
                }
                return;
            }
            if ((octets[i_cur++] & 0b1000'0000) == 0) break;
        }

        if ((i_cur - i_start) <= 0) {
            return;
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

}


void parse(std::span<uint8_t> data, size_t indent) {
    for (int i = 0; i < data.size();) {
        print_octet(data[i], indent);
        IDToken id = parse_id_octet(data[i]);
        i++;
    
        print_indent(indent);

        uint64_t length = parse_length(std::span(data.begin() + i, data.end()), &i);
        std::cout << ", LENGTH: " << length << " BYTES." << std::endl;
        std::span<uint8_t> span = std::span<uint8_t>(data.begin() + i, data.begin() + i + length);
       
        if (id.id_encoding == Encoding::Primitive) {
            print_indent(indent + 1);
            if (id.id_tag == Tag::UTF8String || id.id_tag == Tag::PrintableString) {
                std::cout << parse_string(span) << std::endl;
            } else if(id.id_tag == Tag::OBJECT_IDENTIFIER) {
                print_oid(span);
            } else {
                print_hex(span); 
            }
        } else {
            parse(span, indent + 1);
        }
        i += length;
    }
}



