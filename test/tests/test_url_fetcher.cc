// test_url_fetcher.cc -- the server-side request forgery guard
//
// This is the only place the server fetches an address a user chose, so it is
// the place where "a malicious link" means something. The danger is not the
// content of the page: it is that a server which fetches anything asked of it
// becomes a proxy into its own network. This machine can reach the local model
// endpoint on 192.168.86.x, its own loopback, and every device on the LAN --
// and the app is reachable from the internet through a tunnel.
//
// So these tests are mostly a list of addresses that must never be fetched.
// They are worth more than the usual test: a regression here is not a broken
// feature, it is an open door, and it would not show up as anything failing.

#include <gtest/gtest.h>

#include <string>

#include "utils/UrlFetcher.h"

TEST(UrlFetcherGuardTest, BlocksLoopback) {
    EXPECT_TRUE(UrlFetcher::IsBlockedAddress("127.0.0.1")) << "the server itself";
    EXPECT_TRUE(UrlFetcher::IsBlockedAddress("127.1.2.3")) << "the whole 127/8 range is loopback";
    EXPECT_TRUE(UrlFetcher::IsBlockedAddress("::1"));
}

// The LAN this machine sits on, which is where the model endpoint lives.
TEST(UrlFetcherGuardTest, BlocksPrivateRanges) {
    EXPECT_TRUE(UrlFetcher::IsBlockedAddress("192.168.86.124")) << "the local model endpoint";
    EXPECT_TRUE(UrlFetcher::IsBlockedAddress("192.168.0.1")) << "a home router";
    EXPECT_TRUE(UrlFetcher::IsBlockedAddress("10.0.0.5"));
    EXPECT_TRUE(UrlFetcher::IsBlockedAddress("172.16.0.1"));
    EXPECT_TRUE(UrlFetcher::IsBlockedAddress("172.31.255.254"));
    EXPECT_TRUE(UrlFetcher::IsBlockedAddress("fd00::1")) << "IPv6 unique local";
}

// 169.254.169.254 is the cloud metadata address, which serves credentials on
// several providers. Blocked here so this cannot become a credential leak if
// the app is ever run somewhere other than a desk.
TEST(UrlFetcherGuardTest, BlocksLinkLocalAndCloudMetadata) {
    EXPECT_TRUE(UrlFetcher::IsBlockedAddress("169.254.169.254"));
    EXPECT_TRUE(UrlFetcher::IsBlockedAddress("169.254.0.1"));
    EXPECT_TRUE(UrlFetcher::IsBlockedAddress("fe80::1"));
}

TEST(UrlFetcherGuardTest, BlocksOddRangesThatAreNotPublicEither) {
    EXPECT_TRUE(UrlFetcher::IsBlockedAddress("0.0.0.0"));
    EXPECT_TRUE(UrlFetcher::IsBlockedAddress("100.64.0.1")) << "carrier-grade NAT";
    EXPECT_TRUE(UrlFetcher::IsBlockedAddress("224.0.0.1")) << "multicast";
    EXPECT_TRUE(UrlFetcher::IsBlockedAddress("255.255.255.255"));
}

// An IPv4-mapped IPv6 address is a way of writing the same private address
// differently, and judging it by its shape rather than its value would wave it
// straight through.
TEST(UrlFetcherGuardTest, BlocksPrivateAddressesWrittenAsIpv4Mapped) {
    EXPECT_TRUE(UrlFetcher::IsBlockedAddress("::ffff:127.0.0.1"));
    EXPECT_TRUE(UrlFetcher::IsBlockedAddress("::ffff:192.168.86.124"));
}

TEST(UrlFetcherGuardTest, AllowsOrdinaryPublicAddresses) {
    EXPECT_FALSE(UrlFetcher::IsBlockedAddress("93.184.216.34")) << "example.com";
    EXPECT_FALSE(UrlFetcher::IsBlockedAddress("8.8.8.8"));
    EXPECT_FALSE(UrlFetcher::IsBlockedAddress("2606:2800:220:1:248:1893:25c8:1946"));
}

// Anything unparseable is refused. Failing closed is the only safe direction
// for a check whose job is to say no.
TEST(UrlFetcherGuardTest, RefusesWhatItCannotClassify) {
    EXPECT_TRUE(UrlFetcher::IsBlockedAddress(""));
    EXPECT_TRUE(UrlFetcher::IsBlockedAddress("not-an-address"));
    EXPECT_TRUE(UrlFetcher::IsBlockedAddress("999.999.999.999"));
}

// Schemes other than http(s) are refused before any lookup happens: file://
// would read the disk, and there is no reason a debate needs either.
TEST(UrlFetcherGuardTest, RefusesNonHttpSchemes) {
    for (const std::string& url :
         {"file:///C:/Windows/win.ini", "ftp://example.com/x", "gopher://example.com/",
          "javascript:alert(1)", "data:text/html,hi", "example.com/no-scheme"}) {
        const auto result = UrlFetcher::Fetch(url);
        EXPECT_FALSE(result.ok) << url << " should not be fetchable";
        EXPECT_NE(result.error.find("http"), std::string::npos)
            << "the error should say what is accepted, for " << url;
    }
}

// A link that reads as a public host but resolves to loopback is the whole
// point of resolving before judging.
TEST(UrlFetcherGuardTest, RefusesAHostnameThatResolvesToLoopback) {
    const auto result = UrlFetcher::Fetch("http://localhost:8080/");
    EXPECT_FALSE(result.ok);
    EXPECT_NE(result.error.find("private network"), std::string::npos);
}

TEST(UrlFetcherTest, StripsTagsScriptsAndStyleFromHtml) {
    const std::string html =
        "<html><head><style>body{color:red}</style>"
        "<script>var x = 1; document.write('junk');</script></head>"
        "<body><h1>Headline</h1><p>First sentence.</p><p>Second &amp; third.</p></body></html>";

    const std::string text = UrlFetcher::HtmlToText(html);

    EXPECT_NE(text.find("Headline"), std::string::npos);
    EXPECT_NE(text.find("First sentence."), std::string::npos);
    EXPECT_NE(text.find("Second & third."), std::string::npos) << "entities become characters";
    EXPECT_EQ(text.find("document.write"), std::string::npos) << "script contents must not survive";
    EXPECT_EQ(text.find("color:red"), std::string::npos) << "style contents must not survive";
    EXPECT_EQ(text.find('<'), std::string::npos) << "no tags should remain";
}

TEST(UrlFetcherTest, KeepsParagraphBreaksSoSentencesDoNotRunTogether) {
    const std::string text = UrlFetcher::HtmlToText("<p>One.</p><p>Two.</p>");
    EXPECT_NE(text.find("One."), std::string::npos);
    EXPECT_NE(text.find("Two."), std::string::npos);
    EXPECT_EQ(text.find("One.Two."), std::string::npos) << "a break must separate the paragraphs";
}
