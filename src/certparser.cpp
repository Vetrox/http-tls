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
        << "TAG: " << tagname[id_tag(octet)] << " (" << id_tag(octet) << ")" << std::endl;
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

void parse(std::span<uint8_t> data, size_t indent) {
    for (int i = 0; i < data.size();) {
        print_octet(data[i], indent);
        IDToken id = parse_id_octet(data[i]);
        i++;
        for (int j = 0; j < indent; j++) {
            std::cout << " ";
        }

        uint64_t length = parse_length(std::span(data.begin() + i, data.end()), &i);
        std::cout << "LENGTH: " << length << " BYTES. ";
        std::span<uint8_t> span = std::span<uint8_t>(data.begin() + i, data.begin() + i + length);
       
        if (id.id_encoding == Encoding::Primitive){
            print_hex(span);
        } else {
             std::cout << std::endl;
            parse(span, indent + 1);
        }
        i += length;
    }
}



