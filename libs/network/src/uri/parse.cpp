// Copyright 2009, 2010, 2011 Dean Michael Berris, Jeroen Habraken, Glyn Matthews.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)


#include <boost/network/uri/detail/uri_parts.hpp>
#include <boost/config/warning_disable.hpp>
#include <boost/spirit/home/qi.hpp>
#include <boost/fusion/adapted/struct/adapt_struct.hpp>

BOOST_FUSION_ADAPT_TPL_STRUCT
(
    (String),
    (boost::network::uri::detail::hierarchical_part)(String),
    (String, user_info)
    (String, host)
    (String, port)
    (String, path)
    );

BOOST_FUSION_ADAPT_TPL_STRUCT
(
    (String),
    (boost::network::uri::detail::uri_parts)(String),
    (String, scheme)
    (boost::network::uri::detail::hierarchical_part<String>, hier_part)
    (String, query)
    (String, fragment)
    );

namespace boost {
namespace network {
namespace uri {
namespace detail {
namespace qi = boost::spirit::qi;

template <
    class String
    >
struct uri_grammar : qi::grammar<
    typename String::const_iterator
  , detail::uri_parts<String>()> {

    typedef String string_type;
    typedef typename String::const_iterator const_iterator;

    uri_grammar() : uri_grammar::base_type(start, "uri") {
        // gen-delims = ":" / "/" / "?" / "#" / "[" / "]" / "@"
        gen_delims %= qi::char_(":/?#[]@");
        // sub-delims = "!" / "$" / "&" / "'" / "(" / ")" / "*" / "+" / "," / ";" / "="
        sub_delims %= qi::char_("!$&'()*+,;=");
        // reserved = gen-delims / sub-delims
        reserved %= gen_delims | sub_delims;
        // unreserved = ALPHA / DIGIT / "-" / "." / "_" / "~"
        unreserved %= qi::alnum | qi::char_("-._~");
        // pct-encoded = "%" HEXDIG HEXDIG
        pct_encoded %= qi::char_("%") >> qi::repeat(2)[qi::xdigit];

        // pchar = unreserved / pct-encoded / sub-delims / ":" / "@"
        pchar %= qi::raw[
            unreserved | pct_encoded | sub_delims | qi::char_(":@")
            ];

        // segment = *pchar
        segment %= qi::raw[*pchar];
        // segment-nz = 1*pchar
        segment_nz %= qi::raw[+pchar];
        // segment-nz-nc = 1*( unreserved / pct-encoded / sub-delims / "@" )
        segment_nz_nc %= qi::raw[
            +(unreserved | pct_encoded | sub_delims | qi::char_("@"))
            ];
        // path-abempty  = *( "/" segment )
        path_abempty %=
            qi::raw[*(qi::char_("/") >> segment)]
            ;
        // path-absolute = "/" [ segment-nz *( "/" segment ) ]
        path_absolute %=
            qi::raw[
                    qi::char_("/")
                    >>  -(segment_nz >> *(qi::char_("/") >> segment))
                    ]
            ;
        // path-rootless = segment-nz *( "/" segment )
        path_rootless %=
            qi::raw[segment_nz >> *(qi::char_("/") >> segment)]
            ;
        // path-empty = 0<pchar>
        path_empty %=
            qi::raw[qi::eps]
            ;

        // scheme = ALPHA *( ALPHA / DIGIT / "+" / "-" / "." )
        scheme %=
            qi::raw[qi::alpha >> *(qi::alnum | qi::char_("+.-"))]
            ;

        // user_info = *( unreserved / pct-encoded / sub-delims / ":" )
        user_info %=
            qi::raw[*(unreserved | pct_encoded | sub_delims | qi::char_(":"))]
            ;

        ip_literal %=
            qi::lit('[') >> (ipv6address | ipvfuture) >> ']'
            ;

        ipvfuture %=
            qi::lit('v') >> +qi::xdigit >> '.' >> +( unreserved | sub_delims | ':')
            ;

        ipv6address %= qi::raw[
                                                                      qi::repeat(6)[h16 >> ':'] >> ls32
            |                                                 "::" >> qi::repeat(5)[h16 >> ':'] >> ls32
            | qi::raw[                                h16] >> "::" >> qi::repeat(4)[h16 >> ':'] >> ls32
            | qi::raw[            +(*(h16 >> ':')) >> h16] >> "::" >> qi::repeat(3)[h16 >> ':'] >> ls32
            | qi::raw[qi::repeat(2)[*(h16 >> ':')] >> h16] >> "::" >> qi::repeat(2)[h16 >> ':'] >> ls32
            | qi::raw[qi::repeat(3)[*(h16 >> ':')] >> h16] >> "::" >>               h16 >> ':'  >> ls32
            | qi::raw[qi::repeat(4)[*(h16 >> ':')] >> h16] >> "::"                              >> ls32
            | qi::raw[qi::repeat(5)[*(h16 >> ':')] >> h16] >> "::"                              >> h16
            | qi::raw[qi::repeat(6)[*(h16 >> ':')] >> h16] >> "::"
            ];

        // ls32 = ( h16 ":" h16 ) / IPv4address
        ls32 %= (h16 >> ':' >> h16) | ipv4address
            ;

        // h16 = 1*4HEXDIG
        h16 %= qi::repeat(1, 4)[qi::xdigit]
            ;

        // dec-octet = DIGIT / %x31-39 DIGIT / "1" 2DIGIT / "2" %x30-34 DIGIT / "25" %x30-35
        dec_octet %=
            !(qi::lit('0') >> qi::digit)
            >>  qi::raw[
                qi::uint_parser<boost::uint8_t, 10, 1, 3>()
                ];

        // IPv4address = dec-octet "." dec-octet "." dec-octet "." dec-octet
        ipv4address %= qi::raw[
            dec_octet >> qi::repeat(3)[qi::lit('.') >> dec_octet]
            ];

        // reg-name = *( unreserved / pct-encoded / sub-delims )
        reg_name %= qi::raw[
            *(unreserved | pct_encoded | sub_delims)
            ];

        // TODO, host = IP-literal / IPv4address / reg-name
        host %=
            qi::raw[ip_literal | ipv4address | reg_name]
            ;

        // port %= qi::ushort_;
        port %=
            qi::raw[*qi::digit]
            ;

        // query = *( pchar / "/" / "?" )
        query %=
            qi::raw[*(pchar | qi::char_("/?"))]
            ;

        // fragment = *( pchar / "/" / "?" )
        fragment %=
            qi::raw[*(pchar | qi::char_("/?"))]
            ;

        // hier-part = "//" authority path-abempty / path-absolute / path-rootless / path-empty
        // authority = [ userinfo "@" ] host [ ":" port ]
        hier_part %=
            (
                "//"
                >>  -(user_info >> '@')
                >>  host
                >> -(':' >> port)
                >>  path_abempty
                )
            |
            (
                    qi::attr(string_type())
                >>  qi::attr(string_type())
                >>  qi::attr(string_type())
                >>  (
                    path_absolute
                    |   path_rootless
                    |   path_empty
                    )
                )
            ;

        start %=
            scheme >> ':'
            >> hier_part
            >>  -('?' >> query)
            >>  -('#' >> fragment)
            ;
    }

    qi::rule<const_iterator, typename string_type::value_type()>
    gen_delims, sub_delims, reserved, unreserved;
    qi::rule<const_iterator, string_type()>
    pct_encoded, pchar;

    qi::rule<const_iterator, string_type()>
    segment, segment_nz, segment_nz_nc;
    qi::rule<const_iterator, string_type()>
    path_abempty, path_absolute, path_rootless, path_empty;

    qi::rule<const_iterator, string_type()>
    dec_octet, ipv4address, reg_name, ipv6address, ipvfuture, ip_literal;

    qi::rule<const_iterator, string_type()>
    h16, ls32;

    qi::rule<const_iterator, string_type()>
    host, port;

    qi::rule<const_iterator, string_type()>
    scheme, user_info, query, fragment;

    qi::rule<const_iterator, hierarchical_part<string_type>()>
    hier_part;

    // actual uri parser
    qi::rule<const_iterator, uri_parts<string_type>()> start;

};

bool parse(std::string::const_iterator first,
           std::string::const_iterator last,
           uri_parts<std::string> &parts) {
    namespace qi = boost::spirit::qi;
    static detail::uri_grammar<std::string> grammar;
    bool is_valid = qi::parse(first, last, grammar, parts);
    return is_valid && (first == last);
}
} // namespace detail
} // namespace uri
} // namespace network
} // namespace boost
