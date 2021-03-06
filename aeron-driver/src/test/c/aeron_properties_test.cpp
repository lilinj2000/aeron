/*
 * Copyright 2014-2019 Real Logic Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <functional>

#include <gtest/gtest.h>

extern "C"
{
#include "util/aeron_properties_util.h"
#include "util/aeron_error.h"
}

class PropertiesTest : public testing::Test
{
public:
    PropertiesTest()
    {
        aeron_properties_parse_init(&m_state);
    }

    static int propertyHandler(void *clientd, const char *name, const char *value)
    {
        auto test = reinterpret_cast<PropertiesTest*>(clientd);

        test->m_name = std::string(name);
        test->m_value = std::string(value);
        return 1;
    }

    int parseLine(const char *line)
    {
        std::string lineStr(line);
        m_name = "";
        m_value = "";

        return aeron_properties_parse_line(&m_state, lineStr.c_str(), lineStr.length(), propertyHandler, this);
    }


protected:
    aeron_properties_parser_state_t m_state;
    std::string m_name;
    std::string m_value;
};

TEST_F(PropertiesTest, shouldNotParseMalformedPropertyLine)
{
    EXPECT_EQ(parseLine(" airon"), -1);
    EXPECT_EQ(parseLine("="), -1);
    EXPECT_EQ(parseLine("=val"), -1);
}

TEST_F(PropertiesTest, shouldNotParseTooLongALine)
{
    EXPECT_EQ(aeron_properties_parse_line(&m_state, "line", sizeof(m_state.property_str), propertyHandler, this), -1);
}

TEST_F(PropertiesTest, shouldIgnoreComments)
{
    EXPECT_EQ(parseLine(" #"), 0);
    EXPECT_EQ(parseLine("# comment"), 0);
    EXPECT_EQ(parseLine("! bang"), 0);
    EXPECT_EQ(parseLine("        ! bang"), 0);
}

TEST_F(PropertiesTest, shouldIgnoreBlankLines)
{
    EXPECT_EQ(parseLine(""), 0);
    EXPECT_EQ(parseLine(" "), 0);
}

TEST_F(PropertiesTest, shouldParseSimpleLine)
{
    EXPECT_EQ(parseLine("propertyName=propertyValue"), 1);
    EXPECT_EQ(m_name, "propertyName");
    EXPECT_EQ(m_value, "propertyValue");

    EXPECT_EQ(parseLine("propertyName:propertyValue"), 1);
    EXPECT_EQ(m_name, "propertyName");
    EXPECT_EQ(m_value, "propertyValue");
}

TEST_F(PropertiesTest, shouldParseSimpleLineWithNameWhiteSpace)
{
    EXPECT_EQ(parseLine("   propertyName=propertyValue"), 1);
    EXPECT_EQ(m_name, "propertyName");
    EXPECT_EQ(m_value, "propertyValue");

    EXPECT_EQ(parseLine("propertyName :propertyValue"), 1);
    EXPECT_EQ(m_name, "propertyName");
    EXPECT_EQ(m_value, "propertyValue");

    EXPECT_EQ(parseLine("\tpropertyName  =propertyValue"), 1);
    EXPECT_EQ(m_name, "propertyName");
    EXPECT_EQ(m_value, "propertyValue");
}

TEST_F(PropertiesTest, shouldParseSimpleLineWithLeadingValueWhiteSpace)
{
    EXPECT_EQ(parseLine("propertyName=  propertyValue"), 1);
    EXPECT_EQ(m_name, "propertyName");
    EXPECT_EQ(m_value, "propertyValue");

    EXPECT_EQ(parseLine("propertyName:\tpropertyValue"), 1);
    EXPECT_EQ(m_name, "propertyName");
    EXPECT_EQ(m_value, "propertyValue");
}

TEST_F(PropertiesTest, shouldParseSimpleLineWithNoValue)
{
    EXPECT_EQ(parseLine("propertyName="), 1);
    EXPECT_EQ(m_name, "propertyName");
    EXPECT_EQ(m_value, "");

    EXPECT_EQ(parseLine("   propertyName="), 1);
    EXPECT_EQ(m_name, "propertyName");
    EXPECT_EQ(m_value, "");

    EXPECT_EQ(parseLine("propertyName :"), 1);
    EXPECT_EQ(m_name, "propertyName");
    EXPECT_EQ(m_value, "");
}

TEST_F(PropertiesTest, shouldParseSimpleContinuation)
{
    EXPECT_EQ(parseLine("propertyName=\\"), 0);
    EXPECT_EQ(parseLine("propertyValue"), 1);
    EXPECT_EQ(m_name, "propertyName");
    EXPECT_EQ(m_value, "propertyValue");
}

TEST_F(PropertiesTest, shouldParseSimpleContinuationWithWhitespace)
{
    EXPECT_EQ(parseLine("propertyName= property\\"), 0);
    EXPECT_EQ(parseLine("   Value"), 1);
    EXPECT_EQ(m_name, "propertyName");
    EXPECT_EQ(m_value, "propertyValue");
}

TEST_F(PropertiesTest, shouldParseContinuationWithComment)
{
    EXPECT_EQ(parseLine("propertyName= property\\"), 0);
    EXPECT_EQ(parseLine("#"), 0);
    EXPECT_EQ(parseLine("   Value"), 1);
    EXPECT_EQ(m_name, "propertyName");
    EXPECT_EQ(m_value, "propertyValue");
}

TEST_F(PropertiesTest, shouldParseContinuationWithBlankLine)
{
    EXPECT_EQ(parseLine("propertyName= property\\"), 0);
    EXPECT_EQ(parseLine("\\"), 0);
    EXPECT_EQ(parseLine("   Value"), 1);
    EXPECT_EQ(m_name, "propertyName");
    EXPECT_EQ(m_value, "propertyValue");
}
