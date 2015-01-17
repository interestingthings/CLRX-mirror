/*
 *  CLRadeonExtender - Unofficial OpenCL Radeon Extensions Library
 *  Copyright (C) 2014-2015 Mateusz Szpakowski
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
/*! \file CLIParser.h
 * \brief command line parser
 */

#ifndef __CLRX_CLIPARSER_H__
#define __CLRX_CLIPARSER_H__

#include <CLRX/Config.h>
#include <exception>
#include <string>
#include <vector>
#include <ostream>
#include <iostream>
#include <cstring>
#include <CLRX/utils/Utilities.h>
#include <CLRX/utils/Containers.h>

/// main namespace
namespace CLRX
{

#define CLRX_CLI_AUTOHELP \
    { "help", '?', CLIArgType::NONE, false, "print help", nullptr }, \
    { "usage", 0, CLIArgType::NONE, false, "print usage", nullptr },
    
/*
 * CommandLine Interface
 */

/// type of argument of the option
enum class CLIArgType: cxuchar
{
    NONE = 0, ///< no argument
    BOOL, UINT, INT, UINT64, INT64, SIZE, FLOAT, DOUBLE, STRING,
    TRIMMED_STRING, ///< trimmed string (without spaces at begin and end)
    SINGLE_MAX = TRIMMED_STRING,
    BOOL_ARRAY = 32,
    UINT_ARRAY, INT_ARRAY, UINT64_ARRAY, INT64_ARRAY, SIZE_ARRAY, FLOAT_ARRAY,
    DOUBLE_ARRAY, STRING_ARRAY,
    TRIMMED_STRING_ARRAY, ///< trimmed string (without spaces at begin and end)
    ARRAY_MAX = TRIMMED_STRING_ARRAY
};

/// Command line option description
struct CLIOption
{
    const char* longName;   ///< long name of option
    char shortName;         ///< short name of option (single character)
    CLIArgType argType;     ///< type of argument of option (or none)
    bool argIsOptional;     ///< if true then option argument is optional
    const char* description;    ///< description of option
    const char* argName;    ///< name of argument of option
};

/// CLI exception class
class CLIException: public Exception
{
public:
    CLIException() = default;
    explicit CLIException(const std::string& message);
    CLIException(const std::string& message, char shortName);
    CLIException(const std::string& message, const std::string& longName);
    CLIException(const std::string& message, const CLIOption& option,
             bool chooseShortName);
    virtual ~CLIException() throw() = default;
};

/// The Command Line Parser (parses options and their arguments)
/** This class implements a command-line parser which provides short options
 * and long options with/without arguments. Argument can be a string, number, boolean,
 * and list of previous. Conventions of the option's (and their arguments) syntax
 * was adapted from popt library.
 *
 * Option argument can be attached in the next argument or in the rest of the argument
 * (after '=' or not for short options). If argument option is optional then can be
 * attached in a next argument only when argument does not have option
 * (otherwise argument will be treated likes next option).
 * 
 * Elements of option argument array are separated by comma. Commas and backslashes can be
 * entered by using backslash escapes in a string array element. An element of string array
 * can also be empty.
 * 
 * Option identified by optionId which is order number. First optionId is 0. Option can have
 * short name, long name or both. Option without argument must have argType set likes
 * CLIArgType::NONE (no type, no argument). Argument name (argName)
 * is optional (can be null).
 * 
 * IMPORTANT NOTICE: Option's list and argument's list must be available and unmodified
 * while whole lifecycle of this CLIParser. For whole lifecycle of any option argument or
 * left over argument, a CLIParser must be alive.
 * (because it keeps values of these arguments). */
class CLIParser: public NonCopyableAndNonMovable
{
private:
    template<typename T>
    struct OptTypeTrait { static const CLIArgType type; };
    
    typedef Array<std::pair<const char*, cxuint> > LongNameMap;
    
    struct OptionEntry
    {
        bool isSet;
        bool isArg;
        union {
            bool b;
            cxint i;
            cxuint u;
            cxlong li;
            cxulong lu;
            cxllong lli;
            cxullong llu;
            int32_t i32;
            uint32_t u32;
            int64_t i64;
            uint64_t u64;
            size_t size;
            float f;
            double d;
            const char* s;
            bool* bArr;
            cxint* iArr;
            cxuint* uArr;
            cxlong* liArr;
            cxulong* luArr;
            cxllong* lliArr;
            cxullong* lluArr;
            int32_t* i32Arr;
            uint32_t* u32Arr;
            int64_t* i64Arr;
            uint64_t* u64Arr;
            size_t* sizeArr;
            float* fArr;
            double* dArr;
            const char** sArr;
            
            operator bool() const { return b; }
            operator cxint() const { return i; }
            operator cxuint() const { return u; }
            operator cxlong() const { return li; }
            operator cxulong() const { return lu; }
            operator cxllong() const { return lli; }
            operator cxullong() const { return llu; }
            operator float() const { return f; }
            operator double() const { return d; }
            operator const char*() const { return s; }
            
            operator const bool*() const { return bArr; }
            operator const cxint*() const { return iArr; }
            operator const cxuint*() const { return uArr; }
            operator const cxlong*() const { return liArr; }
            operator const cxulong*() const { return luArr; }
            operator const cxllong*() const { return lliArr; }
            operator const cxullong*() const { return lluArr; }
            operator const float*() const { return fArr; }
            operator const double*() const { return dArr; }
            operator const char**() const { return sArr; }
        } v;
        size_t arrSize;
        OptionEntry() : isSet(false), isArg(false) { }
    };
    
    const CLIOption* options;
    const char* programName;
    cxuint argc;
    const char** argv;
    std::vector<const char*> leftOverArgs;
    
    Array<OptionEntry> optionEntries;
    LongNameMap longNameMap;
    cxuint* shortNameMap;
    
    void handleExceptionsForGetOptArg(cxuint optionId, CLIArgType argType) const;
    void parseOptionArg(cxuint optionId, const char* optArg, bool chooseShortName);
    
public:
    /// constructor
    /**
     * \param programName name of program
     * \param options null-terminated (shortName==0, longName==NULL) options list
     * \param argc argc
     * \param argv argv
     */
    CLIParser(const char* programName, const CLIOption* options,
            cxuint argc, const char** argv);
    ~CLIParser();
    
    /// parse options from arguments
    void parse();
    
    /// handle printing of help or usage. returns false when help or usage not enabled
    bool handleHelpOrUsage(std::ostream& os = std::cout) const;
    
    /// find option by shortName, returns optionId
    cxuint findOption(char shortName) const;
    
    /// find option by longName, returns optionId
    cxuint findOption(const char* longName) const;
    
    /// get option argument if it provided
    template<typename T>
    T getOptArg(cxuint optionId) const
    {
        handleExceptionsForGetOptArg(optionId, OptTypeTrait<T>::type);
        return static_cast<T>(optionEntries[optionId].v);
    }
    
    /// get option argument if it provided
    template<typename T>
    T getShortOptArg(char shortName) const
    { return getOptArg<T>(findOption(shortName)); }
    
    /// get option argument if it provided
    template<typename T>
    T getLongOptArg(const char* longName) const
    { return getOptArg<T>(findOption(longName)); }
    
    /// get option argument array  if it provided
    /**
     * \param optionId id of option
     * \param length length of array
     */
    template<typename T>
    const T* getOptArgArray(cxuint optionId, size_t& length) const
    {
        handleExceptionsForGetOptArg(optionId, OptTypeTrait<T*>::type);
        const OptionEntry& optEntry = optionEntries[optionId];
        length = optEntry.arrSize;
        return static_cast<const T*>(optEntry.v);
    }
    
    /// get option argument array  if it provided
    /**
     * \param shortName short name of option
     * \param length length of array
     */
    template<typename T>
    const T* getShortOptArgArray(char shortName, size_t& length) const
    { return getOptArgArray<T>(findOption(shortName), length); }
    
    /// get option argument array  if it provided
    /**
     * \param longName long name of option
     * \param length length of array
     */
    template<typename T>
    const T* getLongOptArgArray(const char* longName, size_t& length) const
    { return getOptArgArray<T>(findOption(longName), length); }
    
    /// returns true when argument provided for specified option
    bool hasOptArg(cxuint optionId) const
    {
        if (optionId >= optionEntries.size())
            throw CLIException("No such command line option!");
        return optionEntries[optionId].isArg;
    }
    
    /// returns true when argument provided for specified option
    bool hasShortOptArg(char shortName) const
    { return hasOptArg(findOption(shortName)); }
    
    /// returns true when argument provided for specified option
    bool hasLongOptArg(const char* longName) const
    { return hasOptArg(findOption(longName)); }
    
    /// returns true if option included in command line
    bool hasOption(cxuint optionId) const
    { 
        if (optionId >= optionEntries.size())
            throw CLIException("No such command line option!");
        return optionEntries[optionId].isSet;
    }
    
    /// returns true if option included in command line
    bool hasShortOption(char shortName) const
    { return hasOption(findOption(shortName)); }
    
    /// returns true if option included in command line
    bool hasLongOption(const char* longName) const
    { return hasOption(findOption(longName)); }
    
    /// get left over arguments number
    cxuint getArgsNum() const
    { return leftOverArgs.size()-1; }
    /// get left over arguments (null-terminated)
    const char* const* getArgs() const
    { return leftOverArgs.data(); }
    
    /// print help for program (lists options)
    void printHelp(std::ostream& os = std::cout) const;
    /// print usage
    void printUsage(std::ostream& os = std::cout) const;
};

template<>
struct CLIParser::OptTypeTrait<bool> {
static const CLIArgType type = CLIArgType::BOOL; };

template<>
struct CLIParser::OptTypeTrait<cxuint> {
static const CLIArgType type = CLIArgType::UINT; };

template<>
struct CLIParser::OptTypeTrait<cxint> {
static const CLIArgType type = CLIArgType::INT; };

template<>
struct CLIParser::OptTypeTrait<cxulong> {
static const CLIArgType type = (sizeof(cxulong)==8)?CLIArgType::UINT64:CLIArgType::UINT; };

template<>
struct CLIParser::OptTypeTrait<cxlong> {
static const CLIArgType type = (sizeof(cxlong)==8)?CLIArgType::INT64:CLIArgType::INT; };

template<>
struct CLIParser::OptTypeTrait<cxullong> {
static const CLIArgType type = CLIArgType::UINT64; };

template<>
struct CLIParser::OptTypeTrait<cxllong> {
static const CLIArgType type = CLIArgType::INT64; };

template<>
struct CLIParser::OptTypeTrait<float> {
static const CLIArgType type = CLIArgType::FLOAT; };

template<>
struct CLIParser::OptTypeTrait<double> {
static const CLIArgType type = CLIArgType::DOUBLE; };

template<>
struct CLIParser::OptTypeTrait<const char*> {
static const CLIArgType type = CLIArgType::STRING; };

template<>
struct CLIParser::OptTypeTrait<cxuint*> {
static const CLIArgType type = CLIArgType::UINT_ARRAY; };

template<>
struct CLIParser::OptTypeTrait<bool*> {
static const CLIArgType type = CLIArgType::BOOL_ARRAY; };

template<>
struct CLIParser::OptTypeTrait<cxint*> {
static const CLIArgType type = CLIArgType::INT_ARRAY; };

template<>
struct CLIParser::OptTypeTrait<cxulong*> {
static const CLIArgType type =
    (sizeof(cxulong)==8)?CLIArgType::UINT64_ARRAY:CLIArgType::UINT_ARRAY; };

template<>
struct CLIParser::OptTypeTrait<cxlong*> {
static const CLIArgType type =
    (sizeof(cxlong)==8)?CLIArgType::INT64_ARRAY:CLIArgType::INT_ARRAY; };

template<>
struct CLIParser::OptTypeTrait<cxullong*> {
static const CLIArgType type = CLIArgType::UINT64_ARRAY; };

template<>
struct CLIParser::OptTypeTrait<cxllong*> {
static const CLIArgType type = CLIArgType::INT64_ARRAY; };

template<>
struct CLIParser::OptTypeTrait<float*> {
static const CLIArgType type = CLIArgType::FLOAT_ARRAY; };

template<>
struct CLIParser::OptTypeTrait<double*> {
static const CLIArgType type = CLIArgType::DOUBLE_ARRAY; };

template<>
struct CLIParser::OptTypeTrait<const char**> {
static const CLIArgType type = CLIArgType::STRING_ARRAY; };

};

#endif
