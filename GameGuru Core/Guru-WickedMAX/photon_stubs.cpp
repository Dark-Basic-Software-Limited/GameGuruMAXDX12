// photon_stubs.cpp
// Minimal no-op stub implementations for Photon SDK symbols.
// This satisfies the linker when the Photon static libraries are not available.
// Multiplayer features will NOT work at runtime with these stubs.

#include <cstdlib>
#include <cstdarg>
#include <cstring>

// Include the Photon SDK headers so we define the actual class members
#include "Common-cpp/inc/JString.h"
#include "Common-cpp/inc/UTF8String.h"
#include "Common-cpp/inc/ANSIString.h"
#include "Common-cpp/inc/BaseCharString.h"
#include "Common-cpp/inc/Object.h"
#include "Common-cpp/inc/Hashtable.h"
#include "Common-cpp/inc/Base.h"
#include "Common-cpp/inc/ToString.h"
#include "Common-cpp/inc/Logger.h"
#include "Common-cpp/inc/LogFormatOptions.h"
#include "Common-cpp/inc/MemoryManagement/Internal/Interface.h"
#include "Common-cpp/inc/MemoryManagement/AllocatorInterface.h"
#include "Common-cpp/inc/Helpers/KeyToObject.h"
// Provide minimal definitions for forward-declared internal types that are
// needed by UniquePointer/SharedPointer member destructors in PhotonPeer
namespace ExitGames { namespace Photon { namespace Internal {
	class PeerData { public: virtual ~PeerData() {} };
	class PeerBase { public: virtual ~PeerBase() {} };
}}}

#include "Photon-cpp/inc/PhotonPeer.h"
#include "Photon-cpp/inc/TrafficStats.h"
#include "Photon-cpp/inc/TrafficStatsGameLevel.h"
#include "LoadBalancing-cpp/inc/Client.h"
#include "LoadBalancing-cpp/inc/AuthenticationValues.h"
#include "LoadBalancing-cpp/inc/Player.h"
#include "LoadBalancing-cpp/inc/MutablePlayer.h"
#include "LoadBalancing-cpp/inc/MutableRoom.h"
#include "LoadBalancing-cpp/inc/Room.h"
#include "LoadBalancing-cpp/inc/RaiseEventOptions.h"
#include "LoadBalancing-cpp/inc/RoomOptions.h"
#include "LoadBalancing-cpp/inc/WebFlags.h"

// Helper subclasses to access protected constructors for static instances
namespace {
	struct TrafficStatsStub : public ExitGames::Photon::TrafficStats {
		TrafficStatsStub() : TrafficStats(0) {}
	};
	struct TrafficStatsGameLevelStub : public ExitGames::Photon::TrafficStatsGameLevel {
		TrafficStatsGameLevelStub() : TrafficStatsGameLevel() {}
	};
	struct MutableRoomStub : public ExitGames::LoadBalancing::MutableRoom {
		MutableRoomStub()
			: MutableRoom(ExitGames::Common::JString(), ExitGames::Common::Hashtable(), nullptr, ExitGames::Common::JVector<ExitGames::Common::JString>(), 0, 0, false, nullptr, false, ExitGames::Common::JVector<ExitGames::Common::JString>())
		{}
	};
	struct MutablePlayerStub : public ExitGames::LoadBalancing::MutablePlayer {
		MutablePlayerStub()
			: MutablePlayer(0, ExitGames::Common::Hashtable(), nullptr, nullptr)
		{}
	};
}

// ============================================================================
// ExitGames::Common::ToString
// ============================================================================

namespace ExitGames {
namespace Common {

ToString::~ToString(void) {}

JString ToString::typeToString(void) const
{
	static JString s;
	return s;
}

JString ToString::toString(bool withTypes) const
{
	JString s;
	toString(s, withTypes);
	return s;
}

// Static member definitions for ToString
const EG_CHAR* ToString::EG_STR_CHAR = L"";
const EG_CHAR* ToString::EG_STR_SCHAR = L"";
const EG_CHAR* ToString::EG_STR_UCHAR = L"";
const EG_CHAR* ToString::EG_STR_SHORT = L"";
const EG_CHAR* ToString::EG_STR_USHORT = L"";
const EG_CHAR* ToString::EG_STR_INT = L"";
const EG_CHAR* ToString::EG_STR_UINT = L"";
const EG_CHAR* ToString::EG_STR_LONG = L"";
const EG_CHAR* ToString::EG_STR_ULONG = L"";
const EG_CHAR* ToString::EG_STR_LONGLONG = L"";
const EG_CHAR* ToString::EG_STR_ULONGLONG = L"";
const EG_CHAR* ToString::EG_STR_FLOAT = L"";
const EG_CHAR* ToString::EG_STR_DOUBLE = L"";
const EG_CHAR* ToString::EG_STR_LONGDOUBLE = L"";
const EG_CHAR* ToString::EG_STR_BOOL = L"";

// ============================================================================
// ExitGames::Common::Base
// ============================================================================

Base::~Base(void) {}

void Base::setListener(const BaseListener* /*baseListener*/) {}

int Base::getDebugOutputLevel(void) { return 0; }

bool Base::setDebugOutputLevel(int /*debugLevel*/) { return false; }

const LogFormatOptions& Base::getLogFormatOptions(void)
{
	static LogFormatOptions opts;
	return opts;
}

void Base::setLogFormatOptions(const LogFormatOptions& /*options*/) {}

// Static member definition
Logger Base::mLogger;

// ============================================================================
// ExitGames::Common::Logger
// ============================================================================

Logger::Logger(int debugLevel)
	: mDebugLevel(debugLevel)
#ifdef EG_LOGGING
	, mpListener(nullptr)
#ifdef _EG_SWITCH_WINDOWS_PLATFORM
	, mInitialized(false)
#endif
#endif
{
}

Logger::~Logger(void) {}

void Logger::log(int /*debugLevel*/, const EG_CHAR* /*file*/, const EG_CHAR* /*function*/, bool /*printBrackets*/, unsigned int /*line*/, const EG_CHAR* /*dbgMsg*/, ...) const
{
	// no-op
}

void Logger::vlog(int /*debugLevel*/, const EG_CHAR* /*file*/, const EG_CHAR* /*function*/, bool /*printBrackets*/, unsigned int /*line*/, const EG_CHAR* /*dbgMsg*/, va_list /*args*/) const
{
	// no-op
}

int Logger::getDebugOutputLevel(void) const { return mDebugLevel; }

bool Logger::setDebugOutputLevel(int debugLevel) { mDebugLevel = debugLevel; return true; }

void Logger::setListener(const BaseListener& /*listener*/) {}

const LogFormatOptions& Logger::getFormatOptions(void) const
{
	static LogFormatOptions opts;
	return opts;
}

void Logger::setFormatOptions(const LogFormatOptions& /*formatOptions*/) {}

JString& Logger::toString(JString& retStr, bool /*withTypes*/) const
{
	return retStr;
}

JString Logger::padString(JString str, unsigned int /*padding*/) const
{
	return str;
}

// ============================================================================
// ExitGames::Common::LogFormatOptions
// ============================================================================

LogFormatOptions::LogFormatOptions(void)
	: mAddDateTime(false)
	, mAddLevel(false)
	, mAddFile(false)
	, mAddFunction(false)
	, mMaxNumberOfNamespaces(0)
	, mAddLine(false)
{
}

bool LogFormatOptions::getAddDateTime(void) const { return mAddDateTime; }
LogFormatOptions& LogFormatOptions::setAddDateTime(bool addTime) { mAddDateTime = addTime; return *this; }
bool LogFormatOptions::getAddLevel(void) const { return mAddLevel; }
LogFormatOptions& LogFormatOptions::setAddLevel(bool addLevel) { mAddLevel = addLevel; return *this; }
bool LogFormatOptions::getAddFile(void) const { return mAddFile; }
LogFormatOptions& LogFormatOptions::setAddFile(bool addFile) { mAddFile = addFile; return *this; }
bool LogFormatOptions::getAddFunction(void) const { return mAddFunction; }
LogFormatOptions& LogFormatOptions::setAddFunction(bool addFunction) { mAddFunction = addFunction; return *this; }
unsigned int LogFormatOptions::getMaxNumberOfNamespaces(void) const { return mMaxNumberOfNamespaces; }
LogFormatOptions& LogFormatOptions::setMaxNumberOfNamespaces(unsigned int maxNumberOfNamespaces) { mMaxNumberOfNamespaces = maxNumberOfNamespaces; return *this; }
bool LogFormatOptions::getAddLine(void) const { return mAddLine; }
LogFormatOptions& LogFormatOptions::setAddLine(bool addLine) { mAddLine = addLine; return *this; }
JString& LogFormatOptions::toString(JString& retStr, bool /*withTypes*/) const { return retStr; }

// ============================================================================
// ExitGames::Common::JString
// ============================================================================

JString::JString(unsigned int /*bufferlen*/)
	: mBuffer(nullptr), mBufferLen(0), mLength(0) {}

JString::JString(const char* /*Value*/)
	: mBuffer(nullptr), mBufferLen(0), mLength(0) {}

JString::JString(const EG_CHAR* /*Value*/)
	: mBuffer(nullptr), mBufferLen(0), mLength(0) {}

JString::JString(const JString& /*Value*/)
	: ToString(), mBuffer(nullptr), mBufferLen(0), mLength(0) {}

JString::JString(const UTF8String& /*Value*/)
	: mBuffer(nullptr), mBufferLen(0), mLength(0) {}

JString::JString(const ANSIString& /*Value*/)
	: mBuffer(nullptr), mBufferLen(0), mLength(0) {}

JString::~JString(void)
{
	if(mBuffer) EG_FREE(mBuffer);
}

JString& JString::operator=(const JString& /*Rhs*/) { return *this; }
JString& JString::operator=(const char* /*Rhs*/) { return *this; }
JString& JString::operator=(const EG_CHAR* /*Rhs*/) { return *this; }
JString& JString::operator=(const UTF8String& /*Rhs*/) { return *this; }
JString& JString::operator=(const ANSIString& /*Rhs*/) { return *this; }
JString& JString::operator=(char /*Rhs*/) { return *this; }
JString& JString::operator=(signed char /*Rhs*/) { return *this; }
JString& JString::operator=(unsigned char /*Rhs*/) { return *this; }
JString& JString::operator=(EG_CHAR /*Rhs*/) { return *this; }
JString& JString::operator=(short /*aNum*/) { return *this; }
JString& JString::operator=(unsigned short /*aNum*/) { return *this; }
JString& JString::operator=(int /*aNum*/) { return *this; }
JString& JString::operator=(unsigned int /*aNum*/) { return *this; }
JString& JString::operator=(long /*aNum*/) { return *this; }
JString& JString::operator=(unsigned long /*aNum*/) { return *this; }
JString& JString::operator=(long long /*aNum*/) { return *this; }
JString& JString::operator=(unsigned long long /*aNum*/) { return *this; }
JString& JString::operator=(float /*aNum*/) { return *this; }
JString& JString::operator=(double /*aNum*/) { return *this; }
JString& JString::operator=(long double /*aNum*/) { return *this; }
JString& JString::operator=(bool /*aBool*/) { return *this; }

JString::operator const EG_CHAR* (void) const
{
	static const EG_CHAR empty[] = { 0 };
	return mBuffer ? mBuffer : empty;
}

JString& JString::operator+=(const JString& /*Rhs*/) { return *this; }

bool JString::operator==(const JString& /*Rhs*/) const { return false; }
bool JString::operator!=(const JString& /*Rhs*/) const { return true; }
bool JString::operator<(const JString& /*Rhs*/) const { return false; }
bool JString::operator>(const JString& /*Rhs*/) const { return false; }
bool JString::operator<=(const JString& /*Rhs*/) const { return true; }
bool JString::operator>=(const JString& /*Rhs*/) const { return true; }

EG_CHAR JString::operator[](unsigned int /*Index*/) const { return 0; }
EG_CHAR& JString::operator[](unsigned int /*Index*/)
{
	static EG_CHAR dummy = 0;
	return dummy;
}

unsigned int JString::capacity(void) const { return 0; }
EG_CHAR JString::charAt(unsigned int /*index*/) const { return 0; }
int JString::compareTo(const JString& /*anotherString*/) const { return 0; }
const JString& JString::concat(const JString& /*str*/) { return *this; }
const EG_CHAR* JString::cstr(void) const
{
	static const EG_CHAR empty[] = { 0 };
	return mBuffer ? mBuffer : empty;
}
JString JString::deleteChars(unsigned int /*start*/, unsigned int /*length*/) const { return JString(); }
bool JString::endsWith(const JString& /*suffix*/) const { return false; }
void JString::ensureCapacity(unsigned int /*minCapacity*/) {}
bool JString::equals(const JString& /*anotherString*/) const { return false; }
bool JString::equalsIgnoreCase(const JString& /*anotherString*/) const { return false; }
int JString::indexOf(char /*ch*/) const { return -1; }
int JString::indexOf(char /*ch*/, unsigned int /*fromIndex*/) const { return -1; }
int JString::indexOf(EG_CHAR /*ch*/) const { return -1; }
int JString::indexOf(EG_CHAR /*ch*/, unsigned int /*fromIndex*/) const { return -1; }
int JString::indexOf(const JString& /*str*/) const { return -1; }
int JString::indexOf(const JString& /*str*/, unsigned int /*fromIndex*/) const { return -1; }
int JString::lastIndexOf(char /*ch*/) const { return -1; }
int JString::lastIndexOf(char /*ch*/, unsigned int /*fromIndex*/) const { return -1; }
int JString::lastIndexOf(EG_CHAR /*ch*/) const { return -1; }
int JString::lastIndexOf(EG_CHAR /*ch*/, unsigned int /*fromIndex*/) const { return -1; }
int JString::lastIndexOf(const JString& /*str*/) const { return -1; }
int JString::lastIndexOf(const JString& /*str*/, unsigned int /*fromIndex*/) const { return -1; }
unsigned int JString::length(void) const { return 0; }
JString JString::replace(char /*oldChar*/, char /*newChar*/) const { return JString(); }
JString JString::replace(EG_CHAR /*oldChar*/, EG_CHAR /*newChar*/) const { return JString(); }
JString JString::replace(const JString& /*match*/, const JString& /*replacement*/) const { return JString(); }
bool JString::startsWith(const JString& /*prefix*/) const { return false; }
bool JString::startsWith(const JString& /*prefix*/, unsigned int /*offset*/) const { return false; }
JString JString::substring(unsigned int /*beginIndex*/) const { return JString(); }
JString JString::substring(unsigned int /*beginIndex*/, unsigned int /*endIndex*/) const { return JString(); }
JString JString::toLowerCase(void) const { return JString(); }
JString JString::toUpperCase(void) const { return JString(); }
int JString::toInt(void) const { return 0; }
JString JString::trim(void) { return JString(); }

UTF8String JString::UTF8Representation(void) const
{
	return UTF8String();
}

ANSIString JString::ANSIRepresentation(void) const
{
	return ANSIString();
}

JString& JString::toString(JString& retStr, bool /*withTypes*/) const
{
	return retStr;
}

void JString::GetBuffer(unsigned int /*MaxStrLen*/) {}
void JString::verifyIndex(unsigned int /*number*/) const {}

JString operator+(const JString& /*Lsh*/, const JString& /*Rsh*/)
{
	return JString();
}

// ============================================================================
// ExitGames::Common::BaseCharString
// ============================================================================

BaseCharString::BaseCharString()
	: mBuffer(nullptr), mLength(0) {}

BaseCharString::~BaseCharString(void) {}

const char* BaseCharString::cstr(void) const
{
	static const char empty[] = "";
	return mBuffer ? mBuffer : empty;
}

unsigned int BaseCharString::length(void) const { return 0; }

JString& BaseCharString::toString(JString& retStr, bool /*withTypes*/) const
{
	return retStr;
}

// ============================================================================
// ExitGames::Common::ANSIString
// ============================================================================

ANSIString::ANSIString(void)
	: BaseCharString() {}

ANSIString::ANSIString(const ANSIString& /*str*/)
	: BaseCharString() {}

ANSIString::ANSIString(const JString& /*str*/)
	: BaseCharString() {}

ANSIString::ANSIString(const char* /*str*/)
	: BaseCharString() {}

ANSIString::ANSIString(const EG_CHAR* /*str*/)
	: BaseCharString() {}

ANSIString::~ANSIString(void) {}

ANSIString& ANSIString::operator=(const ANSIString& /*Rhs*/) { return *this; }
ANSIString& ANSIString::operator=(const JString& /*Rhs*/) { return *this; }
ANSIString& ANSIString::operator=(const char* /*Rhs*/) { return *this; }
ANSIString& ANSIString::operator=(const EG_CHAR* /*Rhs*/) { return *this; }

ANSIString::operator const char* (void) const
{
	static const char empty[] = "";
	return mBuffer ? mBuffer : empty;
}

ANSIString::operator JString (void) const
{
	return JString();
}

JString ANSIString::JStringRepresentation(void) const
{
	return JString();
}

unsigned int ANSIString::size(void) const { return 0; }

// ============================================================================
// ExitGames::Common::UTF8String
// ============================================================================

UTF8String::UTF8String(void)
	: BaseCharString() {}

UTF8String::UTF8String(const UTF8String& /*str*/)
	: BaseCharString() {}

UTF8String::UTF8String(const JString& /*str*/)
	: BaseCharString() {}

UTF8String::UTF8String(const char* /*str*/)
	: BaseCharString() {}

UTF8String::UTF8String(const EG_CHAR* /*str*/)
	: BaseCharString() {}

UTF8String::~UTF8String(void) {}

UTF8String& UTF8String::operator=(const UTF8String& /*Rhs*/) { return *this; }
UTF8String& UTF8String::operator=(const JString& /*Rhs*/) { return *this; }
UTF8String& UTF8String::operator=(const char* /*Rhs*/) { return *this; }
UTF8String& UTF8String::operator=(const EG_CHAR* /*Rhs*/) { return *this; }

UTF8String::operator const char* (void) const
{
	static const char empty[] = "";
	return mBuffer ? mBuffer : empty;
}

UTF8String::operator JString (void) const
{
	return JString();
}

JString UTF8String::JStringRepresentation(void) const
{
	return JString();
}

unsigned int UTF8String::size(void) const { return 0; }
unsigned int UTF8String::size(const JString& /*str*/) { return 0; }

// ============================================================================
// ExitGames::Common::Object
// ============================================================================

Object::Object(void)
	: mData{}, mpData(nullptr), mSize(0), mpSizes(nullptr), mType(0), mCustomType(0), mDimensions(0) {}

Object::~Object(void) {}

Object::Object(const Object& /*toCopy*/)
	: Base(), mData{}, mpData(nullptr), mSize(0), mpSizes(nullptr), mType(0), mCustomType(0), mDimensions(0) {}

Object& Object::operator=(const Object& /*toCopy*/) { return *this; }

bool Object::operator==(const Object& /*toCompare*/) const { return false; }
bool Object::operator!=(const Object& /*toCompare*/) const { return true; }

nByte Object::getType(void) const { return mType; }
nByte Object::getCustomType(void) const { return mCustomType; }
const short* Object::getSizes(void) const { return mpSizes; }
unsigned int Object::getDimensions(void) const { return mDimensions; }

JString& Object::toString(JString& retStr, bool /*withTypes*/) const
{
	return retStr;
}

Object::Object(const void* /*data*/, nByte /*type*/, nByte /*customType*/, bool /*makeCopy*/)
	: mData{}, mpData(nullptr), mSize(0), mpSizes(nullptr), mType(0), mCustomType(0), mDimensions(0) {}

Object::Object(const void* /*data*/, nByte /*type*/, nByte /*customType*/, int /*size*/, bool /*makeCopy*/)
	: mData{}, mpData(nullptr), mSize(0), mpSizes(nullptr), mType(0), mCustomType(0), mDimensions(0) {}

Object::Object(const nByte* /*data*/, int /*size*/, bool /*makeCopy*/)
	: mData{}, mpData(nullptr), mSize(0), mpSizes(nullptr), mType(0), mCustomType(0), mDimensions(0) {}

Object::Object(const void* /*data*/, nByte /*type*/, nByte /*customType*/, unsigned int /*dimensions*/, const short* /*sizes*/, bool /*makeCopy*/)
	: mData{}, mpData(nullptr), mSize(0), mpSizes(nullptr), mType(0), mCustomType(0), mDimensions(0) {}

const void* Object::getData(void) const { return mpData; }
const void* const* Object::getDataPointer(void) const { return nullptr; }
Object& Object::assign(const Object& /*toCopy*/) { return *this; }

// Private helpers
void Object::setSizes(const short* /*sizes*/, unsigned int /*dimensions*/) {}
void Object::setDimensions(unsigned int /*dimensions*/) {}
void Object::setData(const void* /*data*/) {}
void Object::setDataNoCopy(void* /*data*/) {}
void Object::setType(nByte /*type*/) {}
void Object::setCustomType(nByte /*customType*/) {}
void Object::set(const void* /*data*/, nByte /*type*/, nByte /*customType*/, bool /*makeCopy*/) {}
void Object::set(const void* /*data*/, nByte /*type*/, nByte /*customType*/, int /*size*/, bool /*makeCopy*/) {}
void Object::set(const void* /*data*/, nByte /*type*/, nByte /*customType*/, unsigned int /*dimensions*/, const short* /*sizes*/, bool /*makeCopy*/) {}
void Object::setWithoutCleanup(const void* /*data*/, nByte /*type*/, nByte /*customType*/, bool /*makeCopy*/) {}
void Object::setWithoutCleanup(const void* /*data*/, nByte /*type*/, nByte /*customType*/, int /*size*/, bool /*makeCopy*/) {}
void Object::setWithoutCleanup(const void* /*data*/, nByte /*type*/, nByte /*customType*/, unsigned int /*dimensions*/, const short* /*sizes*/, bool /*makeCopy*/) {}
void Object::setToNULL(void) {}
void Object::cleanup(const void* /*pData*/, unsigned int /*recursionDepth*/) {}
void Object::copyArray(const void* /*pDataIn*/, void** /*pDataOut*/, unsigned int /*recursionDepth*/) const {}
bool Object::equalsArray(const void* /*pData1*/, const void* /*pData2*/, unsigned int /*recursionDepth*/) const { return false; }
JString& Object::toStringHelper(const Object& /*object*/, JString& retStr, bool /*withTypes*/, bool /*butWithOutTopLevelTypes*/) { return retStr; }
JString Object::payloadTypeToString(void) const { return JString(); }
bool Object::compareHelper(const void* /*pData1*/, const void* /*pData2*/, nByte /*type*/, nByte /*customType*/, unsigned int /*dimensions*/, const short* /*arraySizes*/, unsigned int /*recursionDepth*/) const { return false; }
Object::Data Object::constructDataInstance(nByte /*payload*/) { Data d{}; return d; }
Object::Data Object::constructDataInstance(short /*payload*/) { Data d{}; return d; }
Object::Data Object::constructDataInstance(int /*payload*/) { Data d{}; return d; }
Object::Data Object::constructDataInstance(int64 /*payload*/) { Data d{}; return d; }
Object::Data Object::constructDataInstance(float /*payload*/) { Data d{}; return d; }
Object::Data Object::constructDataInstance(double /*payload*/) { Data d{}; return d; }
Object::Data Object::constructDataInstance(bool /*payload*/) { Data d{}; return d; }

// ============================================================================
// ExitGames::Common::Hashtable
// ============================================================================

Hashtable::Hashtable(void) {}
Hashtable::~Hashtable(void) {}
Hashtable::Hashtable(const Hashtable& /*toCopy*/) : Base() {}
Hashtable& Hashtable::operator=(const Hashtable& /*toCopy*/) { return *this; }
bool Hashtable::operator==(const Hashtable& /*toCompare*/) const { return false; }
bool Hashtable::operator!=(const Hashtable& /*toCompare*/) const { return true; }

const Object& Hashtable::operator[](unsigned int /*index*/) const
{
	static Object dummy;
	return dummy;
}

Object& Hashtable::operator[](unsigned int /*index*/)
{
	static Object dummy;
	return dummy;
}

void Hashtable::put(const Hashtable& /*src*/) {}
unsigned int Hashtable::getSize(void) const { return 0; }

const JVector<Object>& Hashtable::getKeys(void) const
{
	static JVector<Object> emptyKeys;
	return emptyKeys;
}

void Hashtable::removeAllElements(void) {}
JString& Hashtable::toString(JString& retStr, bool /*withTypes*/) const { return retStr; }

// Private implementations
void Hashtable::putImplementation(const Object& /*key*/, const Object& /*val*/) {}
const Object* Hashtable::getValueImplementation(const Object& /*key*/) const { return nullptr; }
void Hashtable::removeImplementation(const Object& /*key*/) {}
bool Hashtable::containsImplementation(const Object& /*key*/) const { return false; }
bool Hashtable::haveSameKey(const Object& /*one*/, const Object& /*two*/) { return false; }

// ============================================================================
// ExitGames::Common::MemoryManagement::Internal::Interface
// ============================================================================

namespace MemoryManagement {
namespace Internal {

size_t Interface::mMaxSizeForAllocatorUsage = 0;
AllocatorInterface* Interface::mpAllocator = nullptr;

void Interface::setMaxAllocSize(size_t /*maxAllocSize*/) {}
void Interface::setMaxSizeForAllocatorUsage(size_t maxSizeForAllocatorUsage) { mMaxSizeForAllocatorUsage = maxSizeForAllocatorUsage; }
void Interface::setAllocator(AllocatorInterface& allocator) { mpAllocator = &allocator; }
void Interface::setAllocatorToDefault(void) { mpAllocator = nullptr; }

void* Interface::malloc(size_t size)
{
	return std::malloc(size ? size : 1);
}

void Interface::free(void* p)
{
	std::free(p);
}

void* Interface::realloc(void* p, size_t size)
{
	return std::realloc(p, size ? size : 1);
}

void* Interface::calloc(size_t num, size_t size)
{
	return std::calloc(num ? num : 1, size ? size : 1);
}

AllocatorInterface* Interface::getAllocator(size_t /*size*/) { return mpAllocator; }

} // namespace Internal

AllocatorInterface* AllocatorInterface::get(void)
{
	return nullptr;
}

} // namespace MemoryManagement

// ============================================================================
// ExitGames::Common::Helpers::KeyToObject
// ============================================================================

namespace Helpers {

const Object& KeyToObject::get(const Object& key)
{
	return key;
}

Object KeyToObject::get(const char* /*key*/)
{
	return Object();
}

Object KeyToObject::get(const wchar_t* /*key*/)
{
	return Object();
}

} // namespace Helpers

// ============================================================================
// ExitGames::Common::Helpers::SmartPointerBase
// ============================================================================

namespace Helpers {

SmartPointerBase::~SmartPointerBase(void) {}

} // namespace Helpers

// ============================================================================
// ExitGames::Common::DictionaryBase
// ============================================================================

DictionaryBase::DictionaryBase(void) : mpTypeInfo(nullptr) {}
DictionaryBase::~DictionaryBase(void) {}
DictionaryBase::DictionaryBase(const DictionaryBase& /*toCopy*/) : Base(), mpTypeInfo(nullptr) {}
DictionaryBase& DictionaryBase::operator=(const DictionaryBase& /*toCopy*/) { return *this; }
bool DictionaryBase::operator==(const DictionaryBase& /*toCompare*/) const { return false; }
bool DictionaryBase::operator!=(const DictionaryBase& /*toCompare*/) const { return true; }
void DictionaryBase::removeAllElements(void) {}
JString DictionaryBase::typeToString(void) const { return JString(); }
JString& DictionaryBase::toString(JString& retStr, bool /*withTypes*/) const { return retStr; }
const Hashtable& DictionaryBase::getHashtable(void) const { return mHashtable; }
unsigned int DictionaryBase::getSize(void) const { return 0; }
const nByte* DictionaryBase::getKeyTypes(void) const { return nullptr; }
const nByte* DictionaryBase::getValueTypes(void) const { return nullptr; }
const unsigned int* DictionaryBase::getValueDimensions(void) const { return nullptr; }
DictionaryBase& DictionaryBase::assign(const DictionaryBase& /*toCopy*/) { return *this; }
void DictionaryBase::put(const DictionaryBase& /*src*/) {}
const Object& DictionaryBase::getElementAt(unsigned int /*index*/, const Object* /*dummyDeducer*/) const
{
	static Object dummy;
	return dummy;
}
Object& DictionaryBase::getElementAt(unsigned int /*index*/, const Object* /*dummyDeducer*/)
{
	static Object dummy;
	return dummy;
}
JVector<Object> DictionaryBase::getKeys(const Object* /*dummyDeducer*/) const { return JVector<Object>(); }
DictionaryBase::DictionaryBase(const nByte* /*pKeyTypes*/, const nByte* /*pValueTypes*/, const unsigned int* /*pValueDimensions*/) : mpTypeInfo(nullptr) {}
bool DictionaryBase::compare(const DictionaryBase& /*toCompare*/) const { return false; }
DictionaryBase* DictionaryBase::copy(short /*amount*/) const { return nullptr; }

} // namespace Common
} // namespace ExitGames

// ============================================================================
// ExitGames::Photon::TrafficStats
// ============================================================================

namespace ExitGames {
namespace Photon {

TrafficStats::TrafficStats(int packageHeaderSize)
	: mPackageHeaderSize(packageHeaderSize)
	, mReliableCommandCount(0)
	, mUnreliableCommandCount(0)
	, mFragmentCommandCount(0)
	, mControlCommandCount(0)
	, mTotalPacketCount(0)
	, mTotalCommandsInPackets(0)
	, mReliableCommandBytes(0)
	, mUnreliableCommandBytes(0)
	, mFragmentCommandBytes(0)
	, mControlCommandBytes(0)
	, mTimestampOfLastAck(0)
	, mTimestampOfLastReliableCommand(0)
{
}

TrafficStats::~TrafficStats(void) {}

int TrafficStats::getPackageHeaderSize(void) const { return mPackageHeaderSize; }
int TrafficStats::getReliableCommandCount(void) const { return mReliableCommandCount; }
int TrafficStats::getUnreliableCommandCount(void) const { return mUnreliableCommandCount; }
int TrafficStats::getFragmentCommandCount(void) const { return mFragmentCommandCount; }
int TrafficStats::getControlCommandCount(void) const { return mControlCommandCount; }
int TrafficStats::getTotalPacketCount(void) const { return mTotalPacketCount; }
int TrafficStats::getTotalCommandsInPackets(void) const { return mTotalCommandsInPackets; }
int TrafficStats::getReliableCommandBytes(void) const { return mReliableCommandBytes; }
int TrafficStats::getUnreliableCommandBytes(void) const { return mUnreliableCommandBytes; }
int TrafficStats::getFragmentCommandBytes(void) const { return mFragmentCommandBytes; }
int TrafficStats::getControlCommandBytes(void) const { return mControlCommandBytes; }
int TrafficStats::getTotalCommandCount(void) const { return 0; }
int TrafficStats::getTotalCommandBytes(void) const { return 0; }
int TrafficStats::getTotalPacketBytes(void) const { return 0; }
int TrafficStats::getTimestampOfLastAck(void) const { return mTimestampOfLastAck; }
int TrafficStats::getTimestampOfLastReliableCommand(void) const { return mTimestampOfLastReliableCommand; }
Common::JString& TrafficStats::toString(Common::JString& retStr, bool /*withTypes*/) const { return retStr; }

// ============================================================================
// ExitGames::Photon::TrafficStatsGameLevel
// ============================================================================

TrafficStatsGameLevel::TrafficStatsGameLevel(void)
	: mTimeOfLastDispatchCall(0)
	, mTimeOfLastSendCall(0)
	, mOperationByteCount(0)
	, mOperationCount(0)
	, mResultByteCount(0)
	, mResultCount(0)
	, mEventByteCount(0)
	, mEventCount(0)
	, mLongestOpResponseCallback(0)
	, mLongestOpResponseCallbackOpCode(0)
	, mLongestEventCallback(0)
	, mLongestEventCallbackCode(0)
	, mLongestDeltaBetweenDispatching(0)
	, mLongestDeltaBetweenSending(0)
	, mDispatchIncomingCommandsCalls(0)
	, mSendOutgoingCommandsCalls(0)
{
}

TrafficStatsGameLevel::~TrafficStatsGameLevel(void) {}

int TrafficStatsGameLevel::getOperationByteCount(void) const { return mOperationByteCount; }
int TrafficStatsGameLevel::getOperationCount(void) const { return mOperationCount; }
int TrafficStatsGameLevel::getResultByteCount(void) const { return mResultByteCount; }
int TrafficStatsGameLevel::getResultCount(void) const { return mResultCount; }
int TrafficStatsGameLevel::getEventByteCount(void) const { return mEventByteCount; }
int TrafficStatsGameLevel::getEventCount(void) const { return mEventCount; }
int TrafficStatsGameLevel::getLongestOpResponseCallback(void) const { return mLongestOpResponseCallback; }
nByte TrafficStatsGameLevel::getLongestOpResponseCallbackOpCode(void) const { return mLongestOpResponseCallbackOpCode; }
int TrafficStatsGameLevel::getLongestEventCallback(void) const { return mLongestEventCallback; }
nByte TrafficStatsGameLevel::getLongestEventCallbackCode(void) const { return mLongestEventCallbackCode; }
int TrafficStatsGameLevel::getLongestDeltaBetweenDispatching(void) const { return mLongestDeltaBetweenDispatching; }
int TrafficStatsGameLevel::getLongestDeltaBetweenSending(void) const { return mLongestDeltaBetweenSending; }
int TrafficStatsGameLevel::getDispatchIncomingCommandsCalls(void) const { return mDispatchIncomingCommandsCalls; }
int TrafficStatsGameLevel::getSendOutgoingCommandsCalls(void) const { return mSendOutgoingCommandsCalls; }
int TrafficStatsGameLevel::getTotalByteCount(void) const { return 0; }
int TrafficStatsGameLevel::getTotalMessageCount(void) const { return 0; }
int TrafficStatsGameLevel::getTotalIncomingByteCount(void) const { return 0; }
int TrafficStatsGameLevel::getTotalIncomingMessageCount(void) const { return 0; }
int TrafficStatsGameLevel::getTotalOutgoingByteCount(void) const { return 0; }
int TrafficStatsGameLevel::getTotalOutgoingMessageCount(void) const { return 0; }
void TrafficStatsGameLevel::resetMaximumCounters(void) {}
Common::JString& TrafficStatsGameLevel::toString(Common::JString& retStr, bool /*withTypes*/) const { return retStr; }
Common::JString TrafficStatsGameLevel::toStringVitalStats(void) const { return Common::JString(); }

// ============================================================================
// ExitGames::Photon::OperationResponse
// ============================================================================

OperationResponse::OperationResponse(nByte operationCode, short returnCode)
	: mOperationCode(operationCode), mReturnCode(returnCode) {}

OperationResponse::~OperationResponse(void) {}

OperationResponse::OperationResponse(const OperationResponse& toCopy)
	: mOperationCode(toCopy.mOperationCode), mReturnCode(toCopy.mReturnCode), mDebugMessage(toCopy.mDebugMessage), mParameters(toCopy.mParameters) {}

OperationResponse& OperationResponse::operator=(const OperationResponse& toCopy)
{
	mOperationCode = toCopy.mOperationCode;
	mReturnCode = toCopy.mReturnCode;
	mDebugMessage = toCopy.mDebugMessage;
	mParameters = toCopy.mParameters;
	return *this;
}

const Common::Object& OperationResponse::operator[](unsigned int index) const
{
	static Common::Object dummy;
	return dummy;
}

Common::JString OperationResponse::toString(bool /*withDebugMessage*/, bool /*withParameters*/, bool /*withParameterTypes*/) const { return Common::JString(); }
Common::Object OperationResponse::getParameterForCode(nByte /*parameterCode*/) const { return Common::Object(); }
nByte OperationResponse::getOperationCode(void) const { return mOperationCode; }
short OperationResponse::getReturnCode(void) const { return mReturnCode; }
const Common::JString& OperationResponse::getDebugMessage(void) const { return mDebugMessage; }
const Common::Dictionary<nByte, Common::Object>& OperationResponse::getParameters(void) const { return mParameters; }
void OperationResponse::setDebugMessage(const EG_CHAR* /*msg*/) {}
void OperationResponse::addParameter(nByte /*parameterCode*/, const Common::Object& /*par*/) {}

// ============================================================================
// ExitGames::Photon::EventData
// ============================================================================

EventData::EventData(nByte code, const Common::Dictionary<nByte, Common::Object>& params)
	: mCode(code), mParameters(params) {}

EventData::~EventData(void) {}

EventData::EventData(const EventData& toCopy)
	: mCode(toCopy.mCode), mParameters(toCopy.mParameters) {}

EventData& EventData::operator=(const EventData& toCopy)
{
	mCode = toCopy.mCode;
	mParameters = toCopy.mParameters;
	return *this;
}

const Common::Object& EventData::operator[](unsigned int /*index*/) const
{
	static Common::Object dummy;
	return dummy;
}

Common::JString EventData::toString(bool /*withParameters*/, bool /*withParameterTypes*/) const { return Common::JString(); }
Common::Object EventData::getParameterForCode(nByte /*parameterCode*/) const { return Common::Object(); }
nByte EventData::getCode(void) const { return mCode; }
const Common::Dictionary<nByte, Common::Object>& EventData::getParameters(void) const { return mParameters; }

// ============================================================================
// ExitGames::Photon::OperationRequest
// ============================================================================

OperationRequest::OperationRequest(nByte operationCode, const OperationRequestParameters& parameters)
	: mOperationCode(operationCode), mParameters(parameters) {}

OperationRequest::~OperationRequest(void) {}

OperationRequest::OperationRequest(const OperationRequest& toCopy)
	: mOperationCode(toCopy.mOperationCode), mParameters(toCopy.mParameters) {}

OperationRequest& OperationRequest::operator=(const OperationRequest& toCopy)
{
	mOperationCode = toCopy.mOperationCode;
	mParameters = toCopy.mParameters;
	return *this;
}

const Common::Object& OperationRequest::operator[](unsigned int /*index*/) const
{
	static Common::Object dummy;
	return dummy;
}

Common::JString OperationRequest::toString(bool /*withParameters*/, bool /*withParameterTypes*/) const { return Common::JString(); }
Common::Object OperationRequest::getParameterForCode(nByte /*parameterCode*/) const { return Common::Object(); }
nByte OperationRequest::getOperationCode(void) const { return mOperationCode; }
const OperationRequestParameters& OperationRequest::getParameters(void) const { return mParameters; }
OperationRequestParameters& OperationRequest::getParameters(void) { return mParameters; }
void OperationRequest::setParameters(const OperationRequestParameters& parameters) { mParameters = parameters; }

// ============================================================================
// ExitGames::Photon::PhotonPeer
// ============================================================================

PhotonPeer::PhotonPeer(PhotonListener& listener, nByte connectionProtocol)
	: mConnectionProtocol(connectionProtocol)
{
}

PhotonPeer::PhotonPeer(PhotonListener& listener, bool /*usingObjC*/, nByte connectionProtocol)
	: mConnectionProtocol(connectionProtocol)
{
}

PhotonPeer::~PhotonPeer(void) {}

bool PhotonPeer::connect(const Common::JString& /*ipAddr*/, const Common::JString& /*appID*/) { return false; }
bool PhotonPeer::connect(const Common::JString& /*ipAddr*/, const Common::JString& /*appID*/, const Common::Object& /*customData*/) { return false; }
void PhotonPeer::disconnect(void) {}
void PhotonPeer::service(bool /*dispatchIncomingCommands*/) {}
void PhotonPeer::serviceBasic(void) {}
bool PhotonPeer::opCustom(const OperationRequest& /*operationRequest*/, bool /*sendReliable*/, nByte /*channelID*/, bool /*encrypt*/) { return false; }
bool PhotonPeer::sendOutgoingCommands(void) { return false; }
bool PhotonPeer::sendAcksOnly(void) { return false; }
bool PhotonPeer::dispatchIncomingCommands(void) { return false; }
bool PhotonPeer::establishEncryption(void) { return false; }
void PhotonPeer::fetchServerTimestamp(void) {}
void PhotonPeer::resetTrafficStats(void) {}
void PhotonPeer::resetTrafficStatsMaximumCounters(void) {}
Common::JString PhotonPeer::vitalStatsToString(bool /*all*/) const { return Common::JString(); }
#if defined EG_PLATFORM_SUPPORTS_CPP11 && defined EG_PLATFORM_SUPPORTS_MULTITHREADING
void PhotonPeer::pingServer(const Common::JString& /*address*/, unsigned int /*pingAttempts*/) {}
#endif
void PhotonPeer::initUserDataEncryption(const Common::JVector<nByte>& /*secret*/) {}
#if defined _EG_ENCRYPTOR_AVAILABLE
void PhotonPeer::initUDPEncryption(const Common::JVector<nByte>& /*encryptSecret*/, const Common::JVector<nByte>& /*HMACSecret*/) {}
#endif

PhotonListener* PhotonPeer::getListener(void) { return nullptr; }
int PhotonPeer::getServerTimeOffset(void) const { return 0; }
int PhotonPeer::getServerTime(void) const { return 0; }
int PhotonPeer::getBytesOut(void) const { return 0; }
int PhotonPeer::getBytesIn(void) const { return 0; }
int PhotonPeer::getByteCountCurrentDispatch(void) const { return 0; }
int PhotonPeer::getByteCountLastOperation(void) const { return 0; }
int PhotonPeer::getPeerState(void) const { return 0; }
int PhotonPeer::getSentCountAllowance(void) const { return 0; }
void PhotonPeer::setSentCountAllowance(int /*sentCountAllowance*/) {}
int PhotonPeer::getTimePingInterval(void) const { return 0; }
void PhotonPeer::setTimePingInterval(int /*timePingInterval*/) {}
int PhotonPeer::getRoundTripTime(void) const { return 0; }
int PhotonPeer::getRoundTripTimeVariance(void) const { return 0; }
int PhotonPeer::getTimestampOfLastSocketReceive(void) const { return 0; }
int PhotonPeer::getDebugOutputLevel(void) const { return 0; }
bool PhotonPeer::setDebugOutputLevel(int /*debugLevel*/) { return false; }
const Common::LogFormatOptions& PhotonPeer::getLogFormatOptions(void) const
{
	static Common::LogFormatOptions opts;
	return opts;
}
void PhotonPeer::setLogFormatOptions(const Common::LogFormatOptions& /*formatOptions*/) {}
int PhotonPeer::getIncomingReliableCommandsCount(void) const { return 0; }
short PhotonPeer::getPeerID(void) const { return 0; }
int PhotonPeer::getDisconnectTimeout(void) const { return 0; }
void PhotonPeer::setDisconnectTimeout(int /*disconnectTimeout*/) {}
int PhotonPeer::getQueuedIncomingCommands(void) const { return 0; }
int PhotonPeer::getQueuedOutgoingCommands(void) const { return 0; }
Common::JString PhotonPeer::getServerAddress(void) const { return Common::JString(); }
bool PhotonPeer::getIsPayloadEncryptionAvailable(void) const { return false; }
bool PhotonPeer::getIsEncryptionAvailable(void) const { return false; }
int PhotonPeer::getResentReliableCommands(void) const { return 0; }
int PhotonPeer::getLimitOfUnreliableCommands(void) const { return 0; }
void PhotonPeer::setLimitOfUnreliableCommands(int /*value*/) {}
bool PhotonPeer::getCRCEnabled(void) const { return false; }
void PhotonPeer::setCRCEnabled(bool /*crcEnabled*/) {}
int PhotonPeer::getPacketLossByCRC(void) const { return 0; }
bool PhotonPeer::getTrafficStatsEnabled(void) const { return false; }
void PhotonPeer::setTrafficStatsEnabled(bool /*trafficStatsEnabled*/) {}
int PhotonPeer::getTrafficStatsElapsedMs(void) const { return 0; }

const TrafficStats& PhotonPeer::getTrafficStatsIncoming(void) const
{
	static TrafficStatsStub ts;
	return ts;
}

const TrafficStats& PhotonPeer::getTrafficStatsOutgoing(void) const
{
	static TrafficStatsStub ts;
	return ts;
}

const TrafficStatsGameLevel& PhotonPeer::getTrafficStatsGameLevel(void) const
{
	static TrafficStatsGameLevelStub ts;
	return ts;
}

nByte PhotonPeer::getQuickResendAttempts(void) const { return 0; }
void PhotonPeer::setQuickResendAttempts(nByte /*quickResendAttempts*/) {}
nByte PhotonPeer::getConnectionProtocol(void) const { return mConnectionProtocol; }
void PhotonPeer::setConnectionProtocol(nByte connectionProtocol) { mConnectionProtocol = connectionProtocol; }
nByte PhotonPeer::getChannelCountUserChannels(void) const { return 0; }

short PhotonPeer::getPeerCount(void) { return 0; }
unsigned int PhotonPeer::getMaxAppIDLength(void) { return MAX_APP_ID_LENGTH; }

void PhotonPeer::init(PhotonListener& /*listener*/, nByte /*connectionProtocol*/) {}
void PhotonPeer::createPeerBase(void) {}

} // namespace Photon
} // namespace ExitGames

// ============================================================================
// ExitGames::LoadBalancing
// ============================================================================

namespace ExitGames {
namespace LoadBalancing {

// ============================================================================
// ExitGames::LoadBalancing::WebFlags
// ============================================================================

WebFlags::WebFlags(nByte webFlags)
	: mWebFlags(webFlags) {}

nByte WebFlags::getFlags(void) const { return mWebFlags; }
WebFlags& WebFlags::setFlags(nByte webFlags) { mWebFlags = webFlags; return *this; }
bool WebFlags::getHttpForward(void) const { return false; }
WebFlags& WebFlags::setHttpForward(bool /*httpWebForward*/) { return *this; }
bool WebFlags::getSendAuthCookie(void) const { return false; }
WebFlags& WebFlags::setSendAuthCookie(bool /*sendAuthCookie*/) { return *this; }
bool WebFlags::getSendSync(void) const { return false; }
WebFlags& WebFlags::setSendSync(bool /*sendSync*/) { return *this; }
bool WebFlags::getSendState(void) const { return false; }
WebFlags& WebFlags::setSendState(bool /*sendState*/) { return *this; }
Common::JString& WebFlags::toString(Common::JString& retStr, bool /*withTypes*/) const { return retStr; }

// ============================================================================
// ExitGames::LoadBalancing::AuthenticationValues
// ============================================================================

AuthenticationValues::AuthenticationValues(void)
	: mType(0) {}

nByte AuthenticationValues::getType(void) const { return mType; }
AuthenticationValues& AuthenticationValues::setType(nByte type) { mType = type; return *this; }

const Common::JString& AuthenticationValues::getParameters(void) const { return mParameters; }
AuthenticationValues& AuthenticationValues::setParameters(const Common::JString& parameters) { mParameters = parameters; return *this; }
AuthenticationValues& AuthenticationValues::setParametersWithUsernameAndToken(const Common::JString& /*username*/, const Common::JString& /*token*/) { return *this; }

const Common::JVector<nByte>& AuthenticationValues::getData(void) const { return mData; }
AuthenticationValues& AuthenticationValues::setData(const Common::JVector<nByte>& data) { mData = data; return *this; }

const Common::JString& AuthenticationValues::getSecret(void) const { return mSecret; }

const Common::JString& AuthenticationValues::getUserID(void) const { return mUserID; }
AuthenticationValues& AuthenticationValues::setUserID(const Common::JString& userID) { mUserID = userID; return *this; }
AuthenticationValues& AuthenticationValues::setSecret(const Common::JString& secret) { mSecret = secret; return *this; }

Common::JString& AuthenticationValues::toString(Common::JString& retStr, bool /*withTypes*/) const { return retStr; }

// ============================================================================
// ExitGames::LoadBalancing::LobbyStatsRequest
// ============================================================================

LobbyStatsRequest::LobbyStatsRequest(const Common::JString& name, nByte type)
	: mName(name), mType(type) {}

const Common::JString& LobbyStatsRequest::getName(void) const { return mName; }
nByte LobbyStatsRequest::getType(void) const { return mType; }
Common::JString& LobbyStatsRequest::toString(Common::JString& retStr, bool /*withTypes*/) const { return retStr; }

// ============================================================================
// ExitGames::LoadBalancing::FriendInfo
// ============================================================================

FriendInfo::FriendInfo(const Common::JString& userID, bool isOnline, const Common::JString& room)
	: mUserID(userID), mIsOnline(isOnline), mRoom(room) {}

Common::JString FriendInfo::getUserID(void) const { return mUserID; }
bool FriendInfo::getIsOnline(void) const { return mIsOnline; }
Common::JString FriendInfo::getRoom(void) const { return mRoom; }
bool FriendInfo::getIsInRoom(void) const { return false; }
Common::JString& FriendInfo::toString(Common::JString& retStr, bool /*withTypes*/) const { return retStr; }

// ============================================================================
// ExitGames::LoadBalancing::Room
// ============================================================================

Room::Room(void)
	: mPlayerCount(0), mMaxPlayers(0), mIsOpen(false), mDirectMode(0) {}

Room::Room(const Common::JString& name, const Common::Hashtable& /*properties*/)
	: mName(name), mPlayerCount(0), mMaxPlayers(0), mIsOpen(false), mDirectMode(0) {}

Room::~Room(void) {}

Room::Room(const Room& toCopy)
	: Common::Base()
	, mName(toCopy.mName)
	, mPlayerCount(toCopy.mPlayerCount)
	, mMaxPlayers(toCopy.mMaxPlayers)
	, mIsOpen(toCopy.mIsOpen)
	, mDirectMode(toCopy.mDirectMode)
	, mCustomProperties(toCopy.mCustomProperties)
{
}

Room& Room::operator=(const Room& /*toCopy*/) { return *this; }

const Common::JString& Room::getName(void) const { return mName; }
nByte Room::getPlayerCount(void) const { return mPlayerCount; }
nByte Room::getMaxPlayers(void) const { return mMaxPlayers; }
bool Room::getIsOpen(void) const { return mIsOpen; }
nByte Room::getDirectMode(void) const { return mDirectMode; }
const Common::Hashtable& Room::getCustomProperties(void) const { return mCustomProperties; }

bool Room::operator==(const Room& /*room*/) const { return false; }

Common::JString& Room::toString(Common::JString& retStr, bool /*withTypes*/) const { return retStr; }
Common::JString Room::toString(bool /*withTypes*/, bool /*withCustomProperties*/) const { return Common::JString(); }

bool Room::getIsMutable(void) const { return false; }
Room& Room::assign(const Room& /*toCopy*/) { return *this; }
void Room::cacheProperties(const Common::Hashtable& /*properties*/) {}
Common::JString Room::payloadToString(bool /*withTypes*/, bool /*withCustomProperties*/) const { return Common::JString(); }

// ============================================================================
// ExitGames::LoadBalancing::Player
// ============================================================================

Player::Player(void)
	: mNumber(0), mpRoom(nullptr), mIsInactive(false) {}

Player::Player(int number, const Common::Hashtable& /*properties*/, const MutableRoom* pRoom)
	: mNumber(number), mpRoom(pRoom), mIsInactive(false) {}

Player::~Player(void) {}

Player::Player(const Player& toCopy)
	: Common::Base()
	, mNumber(toCopy.mNumber)
	, mName(toCopy.mName)
	, mUserID(toCopy.mUserID)
	, mCustomProperties(toCopy.mCustomProperties)
	, mpRoom(toCopy.mpRoom)
	, mIsInactive(toCopy.mIsInactive)
{
}

Player& Player::operator=(const Player& /*toCopy*/) { return *this; }

int Player::getNumber(void) const { return mNumber; }
const Common::JString& Player::getName() const { return mName; }
const Common::JString& Player::getUserID() const { return mUserID; }
const Common::Hashtable& Player::getCustomProperties() const { return mCustomProperties; }
bool Player::getIsInactive(void) const { return mIsInactive; }
bool Player::getIsMasterClient(void) const { return false; }

bool Player::operator==(const Player& /*player*/) const { return false; }

Common::JString& Player::toString(Common::JString& retStr, bool /*withTypes*/) const { return retStr; }
Common::JString Player::toString(bool /*withTypes*/, bool /*withCustomProperties*/) const { return Common::JString(); }

bool Player::getIsMutable(void) const { return false; }
Player& Player::assign(const Player& /*toCopy*/) { return *this; }
void Player::setMutableRoomPointer(const MutableRoom* /*pRoom*/) {}
void Player::setIsInactive(bool /*isInactive*/) {}
void Player::cacheProperties(const Common::Hashtable& /*properties*/) {}
Common::JString Player::payloadToString(bool /*withTypes*/, bool /*withCustomProperties*/) const { return Common::JString(); }

// ============================================================================
// ExitGames::LoadBalancing::MutablePlayer
// ============================================================================

MutablePlayer::MutablePlayer(int number, const Common::Hashtable& properties, const MutableRoom* pRoom, PhotonClient* pClient)
	: Player(number, properties, pRoom), mpLoadBalancingClient(pClient) {}

MutablePlayer::~MutablePlayer(void) {}

MutablePlayer::MutablePlayer(const MutablePlayer& toCopy)
	: Player(toCopy), mpLoadBalancingClient(toCopy.mpLoadBalancingClient) {}

MutablePlayer& MutablePlayer::operator=(const Player& /*toCopy*/) { return *this; }
MutablePlayer& MutablePlayer::operator=(const MutablePlayer& /*toCopy*/) { return *this; }

void MutablePlayer::setName(const Common::JString& /*name*/, const WebFlags& /*webflags*/) {}
void MutablePlayer::mergeCustomProperties(const Common::Hashtable& /*customProperties*/, const WebFlags& /*webflags*/) {}
void MutablePlayer::addCustomProperties(const Common::Hashtable& /*customProperties*/, const WebFlags& /*webflags*/) {}

bool MutablePlayer::getIsMutable(void) const { return true; }
MutablePlayer& MutablePlayer::assign(const Player& /*toCopy*/) { return *this; }

// ============================================================================
// ExitGames::LoadBalancing::MutableRoom
// ============================================================================

MutableRoom::MutableRoom(const Common::JString& name, const Common::Hashtable& properties, PhotonClient* pClient, const Common::JVector<Common::JString>& propsListedInLobby, int playerTtl, int emptyRoomTtl, bool suppressRoomEvents, const Common::JVector<Common::JString>* /*pPlugins*/, bool publishUserID, const Common::JVector<Common::JString>& expectedUsers)
	: Room(name, properties)
	, mpLoadBalancingClient(pClient)
	, mIsVisible(true)
	, mMasterClientID(0)
	, mPropsListedInLobby(propsListedInLobby)
	, mLocalPlayerNumber(0)
	, mPlayerTtl(playerTtl)
	, mEmptyRoomTtl(emptyRoomTtl)
	, mSuppressRoomEvents(suppressRoomEvents)
	, mPublishUserID(publishUserID)
	, mExpectedUsers(expectedUsers)
{
}

MutableRoom::~MutableRoom(void) {}

MutableRoom::MutableRoom(const MutableRoom& toCopy)
	: Room(toCopy)
	, mpLoadBalancingClient(toCopy.mpLoadBalancingClient)
	, mIsVisible(toCopy.mIsVisible)
	, mMasterClientID(toCopy.mMasterClientID)
	, mPropsListedInLobby(toCopy.mPropsListedInLobby)
	, mLocalPlayerNumber(toCopy.mLocalPlayerNumber)
	, mPlayerTtl(toCopy.mPlayerTtl)
	, mEmptyRoomTtl(toCopy.mEmptyRoomTtl)
	, mSuppressRoomEvents(toCopy.mSuppressRoomEvents)
	, mPublishUserID(toCopy.mPublishUserID)
	, mExpectedUsers(toCopy.mExpectedUsers)
{
}

MutableRoom& MutableRoom::operator=(const Room& /*toCopy*/) { return *this; }
MutableRoom& MutableRoom::operator=(const MutableRoom& /*toCopy*/) { return *this; }

nByte MutableRoom::getPlayerCount(void) const { return 0; }
void MutableRoom::setMaxPlayers(nByte /*maxPlayers*/, const WebFlags& /*webflags*/) {}
void MutableRoom::setIsOpen(bool /*isOpen*/, const WebFlags& /*webflags*/) {}
bool MutableRoom::getIsVisible(void) const { return mIsVisible; }
void MutableRoom::setIsVisible(bool /*isVisible*/, const WebFlags& /*webflags*/) {}

const Common::JVector<Player*>& MutableRoom::getPlayers(void) const { return mPlayers; }
const Player* MutableRoom::getPlayerForNumber(int /*playerNumber*/) const { return nullptr; }
int MutableRoom::getMasterClientID(void) const { return mMasterClientID; }
const Common::JVector<Common::JString>& MutableRoom::getPropsListedInLobby(void) const { return mPropsListedInLobby; }
void MutableRoom::setPropsListedInLobby(const Common::JVector<Common::JString>& /*propsListedInLobby*/, const Common::JVector<Common::JString>& /*expectedList*/, const WebFlags& /*webflags*/) {}
int MutableRoom::getPlayerTtl(void) const { return mPlayerTtl; }
int MutableRoom::getEmptyRoomTtl(void) const { return mEmptyRoomTtl; }
bool MutableRoom::getSuppressRoomEvents(void) const { return mSuppressRoomEvents; }
const Common::JVector<Common::JString>* MutableRoom::getPlugins(void) const { return nullptr; }
bool MutableRoom::getPublishUserID(void) const { return mPublishUserID; }
const Common::JVector<Common::JString>& MutableRoom::getExpectedUsers(void) const { return mExpectedUsers; }
void MutableRoom::setExpectedUsers(const Common::JVector<Common::JString>& /*expectedUsers*/, const WebFlags& /*webflags*/) {}

void MutableRoom::mergeCustomProperties(const Common::Hashtable& /*customProperties*/, const Common::Hashtable& /*expectedCustomProperties*/, const WebFlags& /*webflags*/) {}
void MutableRoom::addCustomProperties(const Common::Hashtable& /*customProperties*/, const Common::Hashtable& /*expectedCustomProperties*/, const WebFlags& /*webflags*/) {}

Common::JString MutableRoom::toString(bool /*withTypes*/, bool /*withCustomProperties*/, bool /*withPlayers*/) const { return Common::JString(); }

bool MutableRoom::getIsMutable(void) const { return true; }
MutableRoom& MutableRoom::assign(const Room& /*toCopy*/) { return *this; }
Player* MutableRoom::createPlayer(int /*number*/, const Common::Hashtable& /*properties*/) const { return nullptr; }
void MutableRoom::destroyPlayer(const Player* /*pPlayer*/) const {}
Common::JString MutableRoom::payloadToString(bool /*withCustomProperties*/, bool /*withTypes*/, bool /*withPlayers*/) const { return Common::JString(); }
void MutableRoom::cacheProperties(const Common::Hashtable& /*properties*/) {}
void MutableRoom::setPlayers(const Common::JVector<Player*>& /*players*/) {}
void MutableRoom::removeAllPlayers(void) {}
void MutableRoom::destroyAllPlayers(void) {}
void MutableRoom::addPlayer(Player& /*player*/) {}
void MutableRoom::addLocalPlayer(Player& /*player*/) {}
void MutableRoom::addPlayer(int /*number*/, const Common::Hashtable& /*properties*/) {}
bool MutableRoom::removePlayer(int /*number*/) { return false; }
Common::JVector<Player*>& MutableRoom::getNonConstPlayers(void) { return mPlayers; }
bool MutableRoom::setMasterClientID(int /*id*/) { return false; }

// ============================================================================
// ExitGames::LoadBalancing::RaiseEventOptions
// ============================================================================

RaiseEventOptions::RaiseEventOptions(nByte channelID, nByte eventCaching, const int* /*targetPlayers*/, short /*numTargetPlayers*/, nByte receiverGroup, nByte interestGroup, const WebFlags& webFlags, int cacheSliceIndex)
	: mChannelID(channelID)
	, mEventCaching(eventCaching)
	, mReceiverGroup(receiverGroup)
	, mInterestGroup(interestGroup)
	, mWebFlags(webFlags)
	, mCacheSliceIndex(cacheSliceIndex)
{
}

RaiseEventOptions::~RaiseEventOptions(void) {}

RaiseEventOptions::RaiseEventOptions(const RaiseEventOptions& toCopy)
	: Common::Base()
	, mChannelID(toCopy.mChannelID)
	, mEventCaching(toCopy.mEventCaching)
	, mTargetPlayers(toCopy.mTargetPlayers)
	, mReceiverGroup(toCopy.mReceiverGroup)
	, mInterestGroup(toCopy.mInterestGroup)
	, mWebFlags(toCopy.mWebFlags)
	, mCacheSliceIndex(toCopy.mCacheSliceIndex)
{
}

RaiseEventOptions& RaiseEventOptions::operator=(const RaiseEventOptions& /*toCopy*/) { return *this; }

nByte RaiseEventOptions::getChannelID(void) const { return mChannelID; }
RaiseEventOptions& RaiseEventOptions::setChannelID(nByte channelID) { mChannelID = channelID; return *this; }
nByte RaiseEventOptions::getEventCaching(void) const { return mEventCaching; }
RaiseEventOptions& RaiseEventOptions::setEventCaching(nByte eventCaching) { mEventCaching = eventCaching; return *this; }
const int* RaiseEventOptions::getTargetPlayers(void) const { return nullptr; }
short RaiseEventOptions::getNumTargetPlayers(void) const { return 0; }
RaiseEventOptions& RaiseEventOptions::setTargetPlayers(const int* /*targetPlayers*/, short /*numTargetPlayers*/) { return *this; }
nByte RaiseEventOptions::getReceiverGroup(void) const { return mReceiverGroup; }
RaiseEventOptions& RaiseEventOptions::setReceiverGroup(nByte receiverGroup) { mReceiverGroup = receiverGroup; return *this; }
nByte RaiseEventOptions::getInterestGroup(void) const { return mInterestGroup; }
RaiseEventOptions& RaiseEventOptions::setInterestGroup(nByte interestGroup) { mInterestGroup = interestGroup; return *this; }
const WebFlags& RaiseEventOptions::getWebFlags(void) const { return mWebFlags; }
RaiseEventOptions& RaiseEventOptions::setWebFlags(const WebFlags& webFlags) { mWebFlags = webFlags; return *this; }
int RaiseEventOptions::getCacheSliceIndex(void) const { return mCacheSliceIndex; }
RaiseEventOptions& RaiseEventOptions::setCacheSliceIndex(int cacheSliceIndex) { mCacheSliceIndex = cacheSliceIndex; return *this; }
Common::JString& RaiseEventOptions::toString(Common::JString& retStr, bool /*withTypes*/) const { return retStr; }

// ============================================================================
// ExitGames::LoadBalancing::RoomOptions
// ============================================================================

RoomOptions::RoomOptions(bool isVisible, bool isOpen, nByte maxPlayers, const Common::Hashtable& customRoomProperties, const Common::JVector<Common::JString>& propsListedInLobby, const Common::JString& lobbyName, nByte lobbyType, int playerTtl, int emptyRoomTtl, bool suppressRoomEvents, const Common::JVector<Common::JString>* /*pPlugins*/, bool publishUserID, nByte directMode)
	: mIsVisible(isVisible)
	, mIsOpen(isOpen)
	, mMaxPlayers(maxPlayers)
	, mCustomRoomProperties(customRoomProperties)
	, mPropsListedInLobby(propsListedInLobby)
	, mLobbyName(lobbyName)
	, mLobbyType(lobbyType)
	, mPlayerTtl(playerTtl)
	, mEmptyRoomTtl(emptyRoomTtl)
	, mSuppressRoomEvents(suppressRoomEvents)
	, mPublishUserID(publishUserID)
	, mDirectMode(directMode)
{
}

RoomOptions::~RoomOptions(void) {}

RoomOptions::RoomOptions(const RoomOptions& toCopy)
	: Common::Base()
	, mIsVisible(toCopy.mIsVisible)
	, mIsOpen(toCopy.mIsOpen)
	, mMaxPlayers(toCopy.mMaxPlayers)
	, mCustomRoomProperties(toCopy.mCustomRoomProperties)
	, mPropsListedInLobby(toCopy.mPropsListedInLobby)
	, mLobbyName(toCopy.mLobbyName)
	, mLobbyType(toCopy.mLobbyType)
	, mPlayerTtl(toCopy.mPlayerTtl)
	, mEmptyRoomTtl(toCopy.mEmptyRoomTtl)
	, mSuppressRoomEvents(toCopy.mSuppressRoomEvents)
	, mPublishUserID(toCopy.mPublishUserID)
	, mDirectMode(toCopy.mDirectMode)
{
}

RoomOptions& RoomOptions::operator=(const RoomOptions& /*toCopy*/) { return *this; }

bool RoomOptions::getIsVisible(void) const { return mIsVisible; }
RoomOptions& RoomOptions::setIsVisible(bool isVisible) { mIsVisible = isVisible; return *this; }
bool RoomOptions::getIsOpen(void) const { return mIsOpen; }
RoomOptions& RoomOptions::setIsOpen(bool isOpen) { mIsOpen = isOpen; return *this; }
nByte RoomOptions::getMaxPlayers(void) const { return mMaxPlayers; }
RoomOptions& RoomOptions::setMaxPlayers(nByte maxPlayers) { mMaxPlayers = maxPlayers; return *this; }
const Common::Hashtable& RoomOptions::getCustomRoomProperties(void) const { return mCustomRoomProperties; }
RoomOptions& RoomOptions::setCustomRoomProperties(const Common::Hashtable& customRoomProperties) { mCustomRoomProperties = customRoomProperties; return *this; }
const Common::JVector<Common::JString>& RoomOptions::getPropsListedInLobby(void) const { return mPropsListedInLobby; }
RoomOptions& RoomOptions::setPropsListedInLobby(const Common::JVector<Common::JString>& propsListedInLobby) { mPropsListedInLobby = propsListedInLobby; return *this; }
const Common::JString& RoomOptions::getLobbyName(void) const { return mLobbyName; }
RoomOptions& RoomOptions::setLobbyName(const Common::JString& lobbyName) { mLobbyName = lobbyName; return *this; }
nByte RoomOptions::getLobbyType(void) const { return mLobbyType; }
RoomOptions& RoomOptions::setLobbyType(nByte lobbyType) { mLobbyType = lobbyType; return *this; }
int RoomOptions::getPlayerTtl(void) const { return mPlayerTtl; }
RoomOptions& RoomOptions::setPlayerTtl(int playerTtl) { mPlayerTtl = playerTtl; return *this; }
int RoomOptions::getEmptyRoomTtl(void) const { return mEmptyRoomTtl; }
RoomOptions& RoomOptions::setEmptyRoomTtl(int emptyRoomTtl) { mEmptyRoomTtl = emptyRoomTtl; return *this; }
bool RoomOptions::getSuppressRoomEvents(void) const { return mSuppressRoomEvents; }
RoomOptions& RoomOptions::setSuppressRoomEvents(bool suppressRoomEvents) { mSuppressRoomEvents = suppressRoomEvents; return *this; }
const Common::JVector<Common::JString>* RoomOptions::getPlugins(void) const { return nullptr; }
RoomOptions& RoomOptions::setPlugins(const Common::JVector<Common::JString>* /*pPlugins*/) { return *this; }
bool RoomOptions::getPublishUserID(void) const { return mPublishUserID; }
RoomOptions& RoomOptions::setPublishUserID(bool publishUserID) { mPublishUserID = publishUserID; return *this; }
nByte RoomOptions::getDirectMode(void) const { return mDirectMode; }
RoomOptions& RoomOptions::setDirectMode(nByte directMode) { mDirectMode = directMode; return *this; }
Common::JString& RoomOptions::toString(Common::JString& retStr, bool /*withTypes*/) const { return retStr; }

// ============================================================================
// ExitGames::LoadBalancing::Peer
// ============================================================================

Peer::Peer(Photon::PhotonListener& listener, nByte connectionProtocol)
	: Photon::PhotonPeer(listener, connectionProtocol)
{
}

Peer::~Peer(void) {}

bool Peer::opJoinLobby(const Common::JString& /*lobbyName*/, nByte /*lobbyType*/) { return false; }
bool Peer::opLeaveLobby(void) { return false; }
bool Peer::opCreateRoom(const Common::JString& /*gameID*/, const RoomOptions& /*options*/, const Common::Hashtable& /*customLocalPlayerProperties*/, const Common::JVector<Common::JString>& /*expectedUsers*/) { return false; }
bool Peer::opJoinRoom(const Common::JString& /*gameID*/, const RoomOptions& /*options*/, const Common::Hashtable& /*customLocalPlayerProperties*/, bool /*createIfNotExists*/, bool /*rejoin*/, int /*cacheSliceIndex*/, const Common::JVector<Common::JString>& /*expectedUsers*/) { return false; }
bool Peer::opJoinRandomRoom(const Common::Hashtable& /*customRoomProperties*/, nByte /*maxPlayers*/, nByte /*matchmakingMode*/, const Common::JString& /*lobbyName*/, nByte /*lobbyType*/, const Common::JString& /*sqlLobbyFilter*/, const Common::JVector<Common::JString>& /*expectedUsers*/) { return false; }
bool Peer::opLeaveRoom(bool /*willComeBack*/, bool /*sendAuthCookie*/) { return false; }
bool Peer::opRaiseEvent(bool /*reliable*/, const Common::Object& /*parameters*/, nByte /*eventCode*/, const RaiseEventOptions& /*options*/) { return false; }
bool Peer::opAuthenticate(const Common::JString& /*appID*/, const Common::JString& /*appVersion*/, bool /*encrypted*/, const AuthenticationValues& /*authenticationValues*/, bool /*lobbyStats*/, const Common::JString& /*regionCode*/) { return false; }
bool Peer::opAuthenticateOnce(const Common::JString& /*appID*/, const Common::JString& /*appVersion*/, nByte /*connectionProtocol*/, nByte /*encryptionMode*/, const AuthenticationValues& /*authenticationValues*/, bool /*lobbyStats*/, const Common::JString& /*regionCode*/) { return false; }
bool Peer::opFindFriends(const Common::JString* /*friendsToFind*/, short /*numFriendsToFind*/) { return false; }
bool Peer::opLobbyStats(const Common::JVector<LobbyStatsRequest>& /*lobbiesToQuery*/) { return false; }
bool Peer::opChangeGroups(const Common::JVector<nByte>* /*pGroupsToRemove*/, const Common::JVector<nByte>* /*pGroupsToAdd*/) { return false; }
bool Peer::opWebRpc(const Common::JString& /*uriPath*/) { return false; }
bool Peer::opWebRpc(const Common::JString& /*uriPath*/, const Common::Object& /*parameters*/, bool /*sendAuthCookie*/) { return false; }
bool Peer::opGetRegions(bool /*encrypted*/, const Common::JString& /*appID*/) { return false; }
bool Peer::opSetPropertiesOfPlayer(int /*playerNr*/, const Common::Hashtable& /*properties*/, const Common::Hashtable& /*expectedProperties*/, WebFlags /*webFlags*/) { return false; }
bool Peer::opSetPropertiesOfRoom(const Common::Hashtable& /*properties*/, const Common::Hashtable& /*expectedProperties*/, WebFlags /*webFlags*/) { return false; }

Photon::OperationRequestParameters Peer::opCreateRoomImplementation(const Common::JString& /*gameID*/, const RoomOptions& /*options*/, const Common::Hashtable& /*customLocalPlayerProperties*/, const Common::JVector<Common::JString>& /*expectedUsers*/) { return Photon::OperationRequestParameters(); }
Photon::OperationRequestParameters Peer::opJoinRoomImplementation(const Common::JString& /*gameID*/, const RoomOptions& /*options*/, const Common::Hashtable& /*customLocalPlayerProperties*/, bool /*createIfNotExists*/, bool /*rejoin*/, int /*cacheSliceIndex*/, const Common::JVector<Common::JString>& /*expectedUsers*/) { return Photon::OperationRequestParameters(); }
Photon::OperationRequestParameters Peer::enterRoomImplementation(const RoomOptions* /*pOptions*/, const Common::Hashtable& /*customLocalPlayerProperties*/, const Common::JVector<Common::JString>& /*expectedUsers*/) { return Photon::OperationRequestParameters(); }

// ============================================================================
// ExitGames::LoadBalancing::Client
// ============================================================================

// Static member definition
const EG_CHAR* Client::M_NAMESERVER = L"ns.photonengine.io";

Client::Client(Listener& listener, const Common::JString& applicationID, const Common::JString& appVersion, nByte connectionProtocol, bool autoLobbyStats, nByte regionSelectionMode, bool useAlternativePorts)
	: mpPeer(nullptr)
	, mListener(listener)
	, mAppVersion(appVersion)
	, mAppID(applicationID)
	, mPeerCount(0)
	, mRoomCount(0)
	, mMasterPeerCount(0)
	, mLastJoinType(0)
	, mLastLobbyJoinType(0)
	, mLastJoinWasRejoin(false)
	, mLastCacheSliceIndex(0)
	, mpCurrentlyJoinedRoom(nullptr)
	, mCachedErrorCodeFromGameServer(0)
	, mAutoJoinLobby(true)
	, mpLocalPlayer(nullptr)
	, mFriendListTimestamp(0)
	, mIsFetchingFriendList(false)
	, mState(0)
	, mAutoLobbyStats(autoLobbyStats)
	, mpMutablePlayerFactory(nullptr)
	, mpMutableRoomFactory(nullptr)
	, mDisconnectedCause(0)
	, M_REGION_SELECTION_MODE(regionSelectionMode)
	, M_CONNECTION_PROTOCOL(connectionProtocol)
	, mPingsPerRegion(5)
	, mUseAuthOnce(false)
	, mUseUDPEncryption(false)
	, mUseAlternativePorts(useAlternativePorts)
	, mpPuncherClient(nullptr)
{
}

Client::~Client(void) {}

bool Client::connect(const AuthenticationValues& /*authenticationValues*/, const Common::JString& /*username*/, const Common::JString& /*serverAddress*/, nByte /*serverType*/) { return false; }
void Client::disconnect(void) {}
void Client::service(bool /*dispatchIncomingCommands*/) {}
void Client::serviceBasic(void) {}
bool Client::opCustom(const Photon::OperationRequest& /*operationRequest*/, bool /*sendReliable*/, nByte /*channelID*/, bool /*encrypt*/) { return false; }
bool Client::sendOutgoingCommands(void) { return false; }
bool Client::sendAcksOnly(void) { return false; }
bool Client::dispatchIncomingCommands(void) { return false; }
void Client::fetchServerTimestamp(void) {}
void Client::resetTrafficStats(void) {}
void Client::resetTrafficStatsMaximumCounters(void) {}
Common::JString Client::vitalStatsToString(bool /*all*/) const { return Common::JString(); }

bool Client::opJoinLobby(const Common::JString& /*lobbyName*/, nByte /*lobbyType*/) { return false; }
bool Client::opLeaveLobby(void) { return false; }
bool Client::opCreateRoom(const Common::JString& /*gameID*/, const RoomOptions& /*options*/, const Common::JVector<Common::JString>& /*expectedUsers*/) { return false; }
bool Client::opJoinOrCreateRoom(const Common::JString& /*gameID*/, const RoomOptions& /*options*/, int /*cacheSliceIndex*/, const Common::JVector<Common::JString>& /*expectedUsers*/) { return false; }
bool Client::opJoinRoom(const Common::JString& /*gameID*/, bool /*rejoin*/, int /*cacheSliceIndex*/, const Common::JVector<Common::JString>& /*expectedUsers*/) { return false; }
bool Client::opJoinRandomRoom(const Common::Hashtable& /*customRoomProperties*/, nByte /*maxPlayers*/, nByte /*matchmakingMode*/, const Common::JString& /*lobbyName*/, nByte /*lobbyType*/, const Common::JString& /*sqlLobbyFilter*/, const Common::JVector<Common::JString>& /*expectedUsers*/) { return false; }
bool Client::opLeaveRoom(bool /*willComeBack*/, bool /*sendAuthCookie*/) { return false; }
bool Client::opFindFriends(const Common::JString* /*friendsToFind*/, short /*numFriendsToFind*/) { return false; }
bool Client::opLobbyStats(const Common::JVector<LobbyStatsRequest>& /*lobbiesToQuery*/) { return false; }
bool Client::opChangeGroups(const Common::JVector<nByte>* /*pGroupsToRemove*/, const Common::JVector<nByte>* /*pGroupsToAdd*/) { return false; }
bool Client::opCustomAuthenticationSendNextStepData(const AuthenticationValues& /*authenticationValues*/) { return false; }
bool Client::opWebRpc(const Common::JString& /*uriPath*/) { return false; }

bool Client::selectRegion(const Common::JString& /*selectedRegion*/) { return false; }
bool Client::reconnectAndRejoin(void) { return false; }

int Client::getServerTimeOffset(void) const { return 0; }
int Client::getServerTime(void) const { return 0; }
int Client::getBytesOut(void) const { return 0; }
int Client::getBytesIn(void) const { return 0; }
int Client::getByteCountCurrentDispatch(void) const { return 0; }
int Client::getByteCountLastOperation(void) const { return 0; }
int Client::getSentCountAllowance(void) const { return 0; }
void Client::setSentCountAllowance(int /*sentCountAllowance*/) {}
int Client::getTimePingInterval(void) const { return 0; }
void Client::setTimePingInterval(int /*timePingInterval*/) {}
int Client::getRoundTripTime(void) const { return 0; }
int Client::getRoundTripTimeVariance(void) const { return 0; }
int Client::getTimestampOfLastSocketReceive(void) const { return 0; }
int Client::getDebugOutputLevel(void) const { return 0; }
bool Client::setDebugOutputLevel(int /*debugLevel*/) { return false; }
const Common::LogFormatOptions& Client::getLogFormatOptions(void) const
{
	static Common::LogFormatOptions opts;
	return opts;
}
void Client::setLogFormatOptions(const Common::LogFormatOptions& /*formatOptions*/) {}
int Client::getIncomingReliableCommandsCount(void) const { return 0; }
short Client::getPeerID(void) const { return 0; }
int Client::getDisconnectTimeout(void) const { return 0; }
void Client::setDisconnectTimeout(int /*disconnectTimeout*/) {}
int Client::getQueuedIncomingCommands(void) const { return 0; }
int Client::getQueuedOutgoingCommands(void) const { return 0; }
bool Client::getIsPayloadEncryptionAvailable(void) const { return false; }
bool Client::getIsEncryptionAvailable(void) const { return false; }
int Client::getResentReliableCommands(void) const { return 0; }
int Client::getLimitOfUnreliableCommands(void) const { return 0; }
void Client::setLimitOfUnreliableCommands(int /*value*/) {}
bool Client::getCRCEnabled(void) const { return false; }
void Client::setCRCEnabled(bool /*crcEnabled*/) {}
int Client::getPacketLossByCRC(void) const { return 0; }
bool Client::getTrafficStatsEnabled(void) const { return false; }
void Client::setTrafficStatsEnabled(bool /*trafficStatsEnabled*/) {}
int Client::getTrafficStatsElapsedMs(void) const { return 0; }

const Photon::TrafficStats& Client::getTrafficStatsIncoming(void) const
{
	static TrafficStatsStub ts;
	return ts;
}

const Photon::TrafficStats& Client::getTrafficStatsOutgoing(void) const
{
	static TrafficStatsStub ts;
	return ts;
}

const Photon::TrafficStatsGameLevel& Client::getTrafficStatsGameLevel(void) const
{
	static TrafficStatsGameLevelStub ts;
	return ts;
}

nByte Client::getQuickResendAttempts(void) const { return 0; }
void Client::setQuickResendAttempts(nByte /*quickResendAttempts*/) {}
nByte Client::getChannelCountUserChannels(void) const { return 0; }
short Client::getPeerCount(void) { return 0; }

int Client::getState(void) const { return mState; }

const Common::JString& Client::getMasterserverAddress(void) const { return mMasterserver; }
int Client::getCountPlayersIngame(void) const { return 0; }
int Client::getCountGamesRunning(void) const { return 0; }
int Client::getCountPlayersOnline(void) const { return 0; }

MutableRoom& Client::getCurrentlyJoinedRoom(void)
{
	static MutableRoomStub dummyRoom;
	return mpCurrentlyJoinedRoom ? *mpCurrentlyJoinedRoom : dummyRoom;
}

const Common::JVector<Room*>& Client::getRoomList(void) const { return mRoomList; }
const Common::JVector<Common::JString>& Client::getRoomNameList(void) const { return mRoomNameList; }
bool Client::getIsInRoom(void) const { return false; }
bool Client::getIsInGameRoom(void) const { return false; }
bool Client::getIsInLobby(void) const { return false; }
bool Client::getAutoJoinLobby(void) const { return mAutoJoinLobby; }
void Client::setAutoJoinLobby(bool autoJoinLobby) { mAutoJoinLobby = autoJoinLobby; }

MutablePlayer& Client::getLocalPlayer(void)
{
	static MutablePlayerStub dummyPlayer;
	return mpLocalPlayer ? *mpLocalPlayer : dummyPlayer;
}

const Common::JVector<FriendInfo>& Client::getFriendList(void) const { return mFriendList; }
int Client::getFriendListAge(void) const { return mFriendListTimestamp; }
int Client::getDisconnectedCause(void) const { return mDisconnectedCause; }
const Common::JString& Client::getUserID(void) const
{
	static Common::JString empty;
	return empty;
}

#if defined EG_PLATFORM_SUPPORTS_CPP11 && defined EG_PLATFORM_SUPPORTS_MULTITHREADING
const Common::JString& Client::getRegionWithBestPing(void) const { return mRegionWithBestPing; }
#endif

// Protected virtual methods
bool Client::opSetPropertiesOfPlayer(int /*playerNr*/, const Common::Hashtable& /*properties*/, const Common::Hashtable& /*expectedProperties*/, WebFlags /*webFlags*/) { return false; }
bool Client::opSetPropertiesOfRoom(const Common::Hashtable& /*properties*/, const Common::Hashtable& /*expectedProperties*/, WebFlags /*webFlags*/) { return false; }
Room* Client::createRoom(const Common::JString& /*name*/, const Common::Hashtable& /*properties*/) { return nullptr; }
void Client::destroyRoom(const Room* /*pRoom*/) const {}
Internal::MutablePlayerFactory* Client::getMutablePlayerFactory(void) const { return nullptr; }
Internal::MutableRoomFactory* Client::getMutableRoomFactory(void) const { return nullptr; }

// From Photon::PhotonListener
void Client::onOperationResponse(const Photon::OperationResponse& /*operationResponse*/) {}
void Client::onStatusChanged(int /*statusCode*/) {}
void Client::onEvent(const Photon::EventData& /*eventData*/) {}
void Client::onPingResponse(const Common::JString& /*address*/, unsigned int /*result*/) {}
void Client::debugReturn(int /*debugLevel*/, const Common::JString& /*string*/) {}

// Private methods
void Client::readoutProperties(Common::Hashtable& /*roomProperties*/, Common::Hashtable& /*playerProperties*/, bool /*multiplePlayers*/, int /*targetPlayerNr*/) {}
void Client::handleConnectionFlowError(int /*oldState*/, int /*errorCode*/, const Common::JString& /*errorString*/) {}
void Client::onConnectToMasterFinished(bool /*comingFromGameserver*/) {}
void Client::onArrivalAndAuthentication(void) {}
MutablePlayer* Client::createMutablePlayer(int /*number*/, const Common::Hashtable& /*properties*/) { return nullptr; }
void Client::destroyMutablePlayer(const MutablePlayer* /*pPlayer*/) const {}
MutableRoom* Client::createMutableRoom(const Common::JString& /*name*/, const Common::Hashtable& /*properties*/, const Common::JVector<Common::JString>& /*propsListedInLobby*/, int /*playerTtl*/, int /*emptyRoomTtl*/, bool /*suppressRoomEvents*/, const Common::JVector<Common::JString>* /*pPlugins*/, bool /*publishUserID*/, const Common::JVector<Common::JString>& /*expectedUsers*/) { return nullptr; }
void Client::destroyMutableRoom(const MutableRoom* /*pRoom*/) const {}

#if defined EG_PLATFORM_SUPPORTS_CPP11 && defined EG_PLATFORM_SUPPORTS_MULTITHREADING
void Client::pingBestRegion(unsigned int /*pingsPerRegion*/) {}
#endif

bool Client::callPeerConnect(const Common::JString& /*address*/) { return false; }
bool Client::authenticate(void) { return false; }
Common::JString Client::addPortToAddress(const Common::JString& address, nByte /*serverType*/) { return address; }
unsigned short Client::getDefaultPort(nByte /*serverType*/, bool /*useAlternativePorts*/) { return 0; }
int Client::sendDirect(const Common::JVector<nByte>& /*buffer*/, const Common::JVector<int>& /*targetPlayers*/, bool /*fallbackRelay*/) { return 0; }
bool Client::initPuncher(void) { return false; }
bool Client::startPunch(int /*playerNr*/) { return false; }
void Client::onMasterClientChanged(int /*id*/, int /*oldID*/) {}
bool Client::getIsOnGameServer(void) const { return false; }

// ============================================================================
// ExitGames::LoadBalancing::Internal factory stubs
// ============================================================================

namespace Internal {

MutablePlayerFactory::~MutablePlayerFactory(void) {}
MutablePlayer* MutablePlayerFactory::create(int /*number*/, const Common::Hashtable& /*properties*/, const MutableRoom* /*pRoom*/, PhotonClient* /*pClient*/)
{
	return nullptr;
}
void MutablePlayerFactory::destroy(const MutablePlayer* /*pPlayer*/) {}

MutableRoomFactory::~MutableRoomFactory(void) {}
MutableRoom* MutableRoomFactory::create(const Common::JString& /*name*/, const Common::Hashtable& /*properties*/, PhotonClient* /*pClient*/, const Common::JVector<Common::JString>& /*propsListedInLobby*/, int /*playerTtl*/, int /*emptyRoomTtl*/, bool /*suppressRoomEvents*/, const Common::JVector<Common::JString>* /*pPlugins*/, bool /*publishUserID*/, const Common::JVector<Common::JString>& /*expectedUsers*/)
{
	return nullptr;
}
void MutableRoomFactory::destroy(const MutableRoom* /*pRoom*/) {}

// RoomFactory has static (non-virtual) methods
Room* RoomFactory::create(const Common::JString& /*name*/, const Common::Hashtable& /*properties*/)
{
	return nullptr;
}
void RoomFactory::destroy(const Room* /*pRoom*/) {}

// PlayerFactory has static (non-virtual) methods
Player* PlayerFactory::create(int /*number*/, const Common::Hashtable& /*properties*/, const MutableRoom* /*pRoom*/)
{
	return nullptr;
}
void PlayerFactory::destroy(const Player* /*pPlayer*/) {}

} // namespace Internal

// ============================================================================
// ExitGames::LoadBalancing LobbyStatsResponse (if needed by JVector template)
// ============================================================================

LobbyStatsResponse::LobbyStatsResponse(const Common::JString& name, nByte type, int peerCount, int roomCount)
	: mName(name), mType(type), mPeerCount(peerCount), mRoomCount(roomCount) {}

const Common::JString& LobbyStatsResponse::getName(void) const { return mName; }
nByte LobbyStatsResponse::getType(void) const { return mType; }
int LobbyStatsResponse::getPeerCount(void) const { return mPeerCount; }
int LobbyStatsResponse::getRoomCount(void) const { return mRoomCount; }
Common::JString& LobbyStatsResponse::toString(Common::JString& retStr, bool /*withTypes*/) const { return retStr; }

} // namespace LoadBalancing
} // namespace ExitGames
