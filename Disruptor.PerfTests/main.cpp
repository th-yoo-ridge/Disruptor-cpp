#include "stdafx.h"

//#include <boost/algorithm/string.hpp>
#include <string>
#include <algorithm>
#include <cctype>

#include "LatencyTestSession.h"
#include "TestRepository.h"
#include "ThroughputTestSession.h"

using namespace Disruptor::PerfTests;

void runAllTests(const TestRepository& testRepository);
void runOneTest(const TestRepository& testRepository, const std::string& testName);

static void trim(std::string& s)
{
    // Left trim
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
    // Right trim
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

// Case-insensitive equality
static bool iequals(const std::string& a, const std::string& b)
{
    return std::equal(a.begin(), a.end(),
                      b.begin(), b.end(),
                      [](unsigned char c1, unsigned char c2) {
                          return std::tolower(c1) == std::tolower(c2);
                      });
}

int main(int, char**)
{
    auto& testRepository = TestRepository::instance();

    std::string testName;

    std::cout << "Test name (ALL by default):  " << testName << " ?" << std::endl;

    std::getline(std::cin, testName);

    trim(testName);  // replaces boost::algorithm::trim

    if (iequals(testName, "ALL") || testName.empty())  // replaces boost::algorithm::iequals
    {
        runAllTests(testRepository);
    }
    else
    {
        runOneTest(testRepository, testName);
    }

    return 0;
}

// int main(int, char**)
// {
//     auto& testRepository = TestRepository::instance();
//     std::string testName;
//     std::cout << "Test name (ALL by default):  " << testName << " ?" << std::endl;
//     std::getline(std::cin, testName);
//     boost::algorithm::trim(testName);
//     if (boost::algorithm::iequals(testName, "ALL") || testName.empty())
//     {
//         runAllTests(testRepository);
//     }
//     else
//     {
//         runOneTest(testRepository, testName);
//     }
//     return 0;
// }

void runAllTests(const TestRepository& testRepository)
{
    for (auto&& info : testRepository.allLatencyTests())
    {
        LatencyTestSession session(info);
        session.run();
    }

    for (auto&& info : testRepository.allThrougputTests())
    {
        ThroughputTestSession session(info);
        session.run();
    }
}

void runOneTest(const TestRepository& testRepository, const std::string& testName)
{
    LatencyTestInfo latencyTestInfo;
    if (testRepository.tryGetLatencyTest(testName, latencyTestInfo))
    {
        LatencyTestSession session(latencyTestInfo);
        session.run();
    }
    else
    {
        ThroughputTestInfo throughputTestInfo;
        if (testRepository.tryGetThroughputTest(testName, throughputTestInfo))
        {
            ThroughputTestSession session(throughputTestInfo);
            session.run();
        }
    }
}
